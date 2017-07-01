
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
 $Id: undolist.cc,v 1.128 2016/02/21 18:49:42 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "edit.h"
#include "undolist.h"
#include "pcell.h"
#include "drcif.h"
#include "extif.h"
#include "scedif.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "cd_propnum.h"
#include "cd_celldb.h"
#include "si_parsenode.h"
#include "si_parser.h"
#include "si_interp.h"
#include "menu.h"
#include "promptline.h"
#include "errorlog.h"
#include "select.h"
#include "events.h"


/**************************************************************************
 *
 * Functions to record the insertion and deletion of objects, and to
 * maintain the undo/redo lists.
 *
 **************************************************************************/

// Use these functions in commands that modify the database.
// (1) Call ListCheck() or ListCheckPush() to initialize and check
//     consistency.
// (2) Call RecordObjectChange() on any objects created/deleted.  This
//     is automatic if the CDs::newXXX() functions are used to create
//     the object.  Similarly, the analogous property list command
//     RecordPrptyChange() should be called when properties are changed,
//     but this is not necessary if the object is also being changed.
// (3) Call CommitChanges() or ListPop() when the modifications are
//     complete.  This completes the add/delete and links the changes
//     into the undo list.


namespace {
    // Trash bin class, saves objects for deletion.
    //
    struct sTrash
    {
        sTrash()
            {
                objects = new SymTab(false, false);
            }

        void add_deferred(CDs *sd, CDo *obj)
            {
                if (!sd || !obj)
                    return;
                objects->add((unsigned long)obj, sd, true);
            }

        void empty_trash();

    private:
        SymTab *objects;    // objects to be deleted
    };

    sTrash Trash;


    // Empty the trash.  Here is where deletions are actually performed.
    //
    void
    sTrash::empty_trash()
    {
        SymTabEnt *h;
        SymTabGen stgen(objects, true);
        while ((h = stgen.next()) != 0) {
            CDo *obj = (CDo*)h->stTag;
            CDs *sd = (CDs*)h->stData;
            sd->unlink(obj, false);
            delete h;
        }
    }
    // End of sTrash functions.
}


namespace ed_undolist {
    // Struct to save hypertext entries that change.
    struct Hlist : public hyEntData
    {
        Hlist(hyEnt*, Hlist*);
        ~Hlist()
            {
                hyParent::destroy(hyPrnt);
                hyParent::destroy(hyPrxy);
            }

        void free();
        void rotate();

        hyEnt *from;
        Hlist *next;
    };


    Hlist::Hlist(hyEnt *ent, Hlist *nx) : hyEntData(*ent)
    {
        from = ent;
        next = nx;
        hyPrnt = hyParent::dup(hyPrnt);
        hyPrxy = hyParent::dup(hyPrxy);
    }


    void
    Hlist::free()
    {
        Hlist *h = this;
        while (h) {
            Hlist *hn = h->next;
            delete h;
            h = hn;
        }
    }


    void
    Hlist::rotate()
    {
        if (!from)
            return;
        hyEntData *d1 = this;
        hyEntData *d2 = from;
        hyEntData t(*d1);
        *d1 = *d2;
        *d2 = t;
    }
    // End of Hlist functions.
}


Oper::Oper()
{
    o_next = 0;
    o_next_in_group = 0;
    memset(o_cmd, 0, sizeof(o_cmd));
    o_cell_desc = 0;
    o_cprop_list = 0;
    o_obj_list = 0;
    o_prp_list = 0;
    o_ent_list = 0;
    o_selected = false;
    o_holdover = false;
    o_no_inc_mod = false;
    o_flags = 0;
}


Oper::~Oper()
{
    Oper *onext;
    for (Oper *o = o_next_in_group; o; o = onext) {
        onext = o->o_next_in_group;
        o->o_next_in_group = 0;
        delete o;
    }
    clearLists(true);
}


// Return true if there has been a change recorded.  This is easy for
// the objects and object properties.  Cell properties, however, have
// to be compared which is a bit laborious.
//
// The argument is optionally used to pass a flag that indicates that
// we can avoid making state changes in CommitChanges to would
// invalidate extraction, if there were no other changes.
//
bool
Oper::changed(bool *p_noupd) const
{
    if (p_noupd)
        *p_noupd = false;
    if (o_prp_list || o_ent_list)
        return (true);
    if (o_obj_list) {
        if (!p_noupd || o_cell_desc->isElectrical())
            return (true);

        // In physical mode, check and see if a change was made to a
        // "real" layer, not just internal/derived.  If no such
        // changes were make, set the flag.

        for (Ochg *oc = obj_list(); oc; oc = oc->next_chg()) {
            if (oc->odel()) {
                CDLtype type = oc->odel()->ldesc()->layerType();
                if (type == CDLnormal || type == CDLcellInstance)
                    return (true);
            }
            if (oc->oadd()) {
                CDLtype type = oc->oadd()->ldesc()->layerType();
                if (type == CDLnormal || type == CDLcellInstance)
                    return (true);
            }
        }
        *p_noupd = true;
    }
    if (!o_cell_desc)
        return (false);

    CDp *pold = o_cprop_list;
    CDp *pcur = o_cell_desc->prptyList();
    if (!pold && !pcur)
        return (false);
    int nold = 0;
    for (CDp *p = pold; p; p = p->next_prp())
        nold++;
    if (!nold)
        return (true);
    int ncur = 0;
    for (CDp *p = pcur; p; p = p->next_prp())
        ncur++;
    if (ncur != nold)
        return (true);
    CDp **ary = new CDp*[nold];
    nold = 0;
    for (CDp *p = pold; p; p = p->next_prp())
        ary[nold++] = p;
    bool ptest = p_noupd && o_cell_desc->isElectrical() &&
        CurCell() && CurCell()->cellname() == o_cell_desc->cellname();
    for (CDp *p = pcur; p; p = p->next_prp()) {
        sLstr curlstr;
        curlstr.add(" ");
        p->print(&curlstr, 0, 0);

        bool found = false;
        for (int i = 0; i < nold; i++) {
            if (!ary[i])
                continue;
            if (ary[i]->value() != p->value())
                continue;
            sLstr oldlstr;
            oldlstr.add(" ");
            ary[i]->print(&oldlstr, 0, 0);
            if (!strcmp(curlstr.string(), oldlstr.string())) {
                ary[i] = 0;
                found = true;
                break;
            }
            if (ptest && p->value() == P_SYMBLC &&
                    !strcmp(curlstr.string() + 2, oldlstr.string() + 2)) {

                // The change was switching to/from symbolic viewing
                // mode.  If this was the only change, we can avoid
                // state changes that would require recomputing
                // extraction and similar.

                *p_noupd = true;
                ary[i] = 0;
                found = true;
                break;
            }
        }
        if (!found) {
            delete [] ary;
            return (true);
        }
    }
    delete [] ary;
    return (false);
}


// For non-symbolic electrical cells, see if a cell terminal has been
// moved, created, or deleted.  If so, check the cells containing
// instances of the current cell, and update connection points and
// dots.
//
void
Oper::fixParentConnections()
{
    if (!celldesc())
        return;
    if (!celldesc()->isElectrical())
        return;
    if (celldesc()->isSymbolic())
        return;

    SymTab tab(false, false);
    CDp_snode *pn = (CDp_snode*)celldesc()->prpty(P_NODE);
    for ( ; pn; pn = (CDp_snode*)pn->next())
        tab.add(pn->index(), pn, true);

    bool chg = false;
    for (CDp *px = cprop_list(); px; px = px->next_prp()) {
        if (px->value() != P_NODE)
             continue;
        CDp_snode *pxn = (CDp_snode*)px;
        pn = (CDp_snode*)tab.get(pxn->index());
        if (pn == (CDp_snode*)ST_NIL) {
            // Terminal has been deleted.
            cTfmStack stk;
            CDm_gen mgen(celldesc(), GEN_MASTER_REFS);
            for (CDm *mdesc = mgen.m_first(); mdesc; mdesc = mgen.m_next()) {
                CDs *sd = mdesc->parent();
                if (sd)
                    ScedIf()->dotsSetDirty(sd);
            }
            chg = true;
        }
        else {
            int x1, y1, x2, y2;
            pxn->get_schem_pos(&x1, &y1);
            pn->get_schem_pos(&x2, &y2);
            if (x1 != x2 || y1 != y2) {
                // Terminal has moved.
                ScedIf()->addParentConnection(celldesc(), x2, y2);
                chg = true;
            }
            tab.remove(pxn->index());
        }
    }

    // Any remaining terminals were added.
    SymTabGen gen(&tab);
    SymTabEnt *h;
    while ((h = gen.next()) != 0) {
        pn = (CDp_snode*)h->stData;
        int x, y;
        pn->get_schem_pos(&x, &y);
        ScedIf()->addParentConnection(celldesc(), x, y);
        chg = true;
    }
    if (chg) {
        // Make sure that the instances have the correct BBs or there
        // will be trouble when updating dots.
        celldesc()->reflect();
        ScedIf()->dotsUpdateDirty();
    }
}


void
Oper::clearLists(bool del_content)
{
    freePrpList();
    o_obj_list->free(del_content);
    o_obj_list = 0;
    o_prp_list->free(del_content);
    o_prp_list = 0;
    o_ent_list->free();
    o_ent_list = 0;
}


// Eliminate anything on sd/ld from the list.  If sd is null, eliminate
// anything on ld.  Otherwise, eliminate objects on ld in sdesc.
//
void
Oper::purge(const CDs *sd, const CDl *ld)
{
    // Since the list goes backward in time, have to save deletes until
    // we're through, otherwise an object might be deleted that we later
    // find in an add.
    for (Oper *cur = this; cur; cur = cur->o_next) {
        cur->o_next_in_group->purge(sd, ld);
        if (!sd || sd == cur->o_cell_desc) {
            cur->o_obj_list = cur->o_obj_list->purge_layer(ld);
            cur->o_prp_list = cur->o_prp_list->purge_layer(ld);
        }
    }
}


// Eliminate sd/od from the list.  The sd is the container for od and must
// be given.  If od is null, eliminate all sd objects/properties.
//
void
Oper::purge(const CDs *sd, const CDo *od)
{
    if (!sd)
        return;
    for (Oper *cur = this; cur; cur = cur->o_next) {
        cur->o_next_in_group->purge(sd, od);
        if (cur->o_cell_desc == sd) {
            if (!od)
                cur->clearLists(true);
            else {
                cur->o_obj_list = cur->o_obj_list->purge(od);
                cur->o_prp_list = cur->o_prp_list->purge(od);
            }
        }
    }
}


// Free the property list, but don't clobber the symbolic representation.
//
void
Oper::freePrpList()
{
    for (CDp *pd = o_cprop_list; pd; pd = o_cprop_list) {
        o_cprop_list = pd->next_prp();
        if (pd->value() == P_SYMBLC && pd->is_elec())
            // don't free the rep!
            ((CDp_sym*)pd)->set_symrep(0);
        delete pd;
    }
    o_cprop_list = 0;
}


// Copy the property list, but don't copy the symbolic rep.
//
void
Oper::copyPrpList(CDp *pd)
{
    o_cprop_list = 0;
    CDp *pp=0;
    for ( ; pd; pd = pd->next_prp()) {
        CDp *pcopy;
        if (pd->value() == P_SYMBLC && pd->is_elec()) {
            // For efficiency, don't copy the rep.
            CDp_sym *ps = new CDp_sym;
            ps->set_string(pd->string());
            ps->set_symrep(((CDp_sym*)pd)->symrep());
            ps->set_active(((CDp_sym*)pd)->active());
            pcopy = ps;
        }
        else
            pcopy = pd->dup();
        if (!pcopy)
            continue;
        if (!pp)
            pp = o_cprop_list = pcopy;
        else {
            pp->set_next_prp(pcopy);
            pp = pp->next_prp();
        }
    }
}


// If the second argument is null, compute the BB of the add and
// delete lists.  If the second argument is not null, return the add
// and delete list BBs separately.
//
void
Oper::changeBB(BBox *BB, BBox *dBB)
{
    *BB = CDnullBB;
    if (dBB)
        *dBB = CDnullBB;
    for (const Ochg *oc = o_obj_list; oc; oc = oc->next_chg()) {
        if (oc->oadd())
            BB->add(&oc->oadd()->oBB());
        if (oc->odel()) {
            if (dBB)
                dBB->add(&oc->odel()->oBB());
            else
                BB->add(&oc->odel()->oBB());
        }
    }

    // Bloat a bit to account for such things as vertex indicators and
    // terminal location marks, which may extend out of the subcell
    // bounding box.

    int delta = (int)(4.0/DSP()->MainWdesc()->Ratio());
    if (delta == 0)
        delta = 1;
    BB->bloat(delta);
    if (dBB)
        dBB->bloat(delta);
}


namespace {
    void
    pobject(CDo *od, FILE *fp)
    {
        switch (od->type()) {
        case CDBOX:
            fprintf(fp, "B %d,%d %d,%d\n", od->oBB().left, od->oBB().bottom,
                od->oBB().right, od->oBB().top);
            break;
        case CDPOLYGON:
            fprintf(fp, "P %d,%d %d,%d\n", od->oBB().left, od->oBB().bottom,
                od->oBB().right, od->oBB().top);
            break;
        case CDWIRE:
            fprintf(fp, "W %d,%d %d,%d\n", od->oBB().left, od->oBB().bottom,
                od->oBB().right, od->oBB().top);
            break;
        case CDLABEL:
            fprintf(fp, "L %d,%d %d,%d\n", od->oBB().left, od->oBB().bottom,
                od->oBB().right, od->oBB().top);
            break;
        case CDINSTANCE:
            fprintf(fp, "C %s %d,%d %d,%d\n",
                OCALL(od)->cellname()->string(),
                od->oBB().left, od->oBB().bottom,
                od->oBB().right, od->oBB().top);
            break;
        }
    }
}


// Perform consistency testing, remove duplicate and unneeded objects, and
// remove unused entries from the object list.  This must be called with the
// object_list in first-to-last order (i.e., after reversal).
//
void
Oper::check_objects()
{
    if (XM()->DebugFlags() & DBG_UNDOLIST) {
        for (Ochg *oc = o_obj_list; oc; oc = oc->next_chg()) {
            if (oc->oadd()) {
                fprintf(DBG_FP, "A %p %d %d  ", oc->oadd(),
                    oc->oadd()->state(), oc->oadd()->flags());
                pobject(oc->oadd(), DBG_FP);
            }
            if (oc->odel()) {
                fprintf (DBG_FP, "D %p %d %d  ", oc->odel(),
                    oc->odel()->state(), oc->odel()->flags());
                pobject(oc->odel(), DBG_FP);
            }
            fprintf(DBG_FP, "---\n");
        }
    }

    // Check for adding or deleting same object twice
    int adup = 0;
    for (Ochg *oc = o_obj_list; oc; oc = oc->next_chg()) {
        if (oc->oadd())
            oc->oadd()->set_flag(CDinqueue);
    }
    for (Ochg *oc = o_obj_list; oc; oc = oc->next_chg()) {
        if (oc->oadd()) {
            if (!oc->oadd()->has_flag(CDinqueue)) {
                oc->set_oadd(0);
                adup++;
            }
            else
                oc->oadd()->unset_flag(CDinqueue);
        }
    }
    int ddup = 0;
    int dflag = 0;
    for (Ochg *oc = o_obj_list; oc; oc = oc->next_chg()) {
        if (oc->odel()) {
            oc->odel()->set_flag(CDinqueue);
            // Since it is in the queue, better have CDDeleted.
            if (oc->odel()->state() != CDDeleted) {
                oc->odel()->set_state(CDDeleted);
                dflag++;
            }
        }
    }
    for (Ochg *oc = o_obj_list; oc; oc = oc->next_chg()) {
        if (oc->odel()) {
            if (!oc->odel()->has_flag(CDinqueue)) {
                oc->set_odel(0);
                ddup++;
            }
            else
                oc->odel()->unset_flag(CDinqueue);
        }
    }

    if (XM()->DebugFlags()) {
        if (adup || ddup || dflag) {
            Log()->ErrorLogV("undo list",
                "Info: undo list inconsistency detected, fixed.\n"
                "add_dups=%d del_dups=%d unset=%d", adup, ddup, dflag);
        }
    }

    // Delete from add and delete lists any object which appears in both.
    // An object which is created and then merged will appear in both
    // lists.  These objects never have DRC error indicators, nor are
    // they in the selection queue.
    //
    SymTab *tab = new SymTab(false, false);
    for (Ochg *oc1 = o_obj_list; oc1; oc1 = oc1->next_chg()) {
        if (oc1->oadd() && oc1->oadd()->state() == CDDeleted) {
            // Since the delete flag was set, the object must have
            // been deleted later.  Entries after the present occurred
            // later since list was reversed.

            tab->add((unsigned long)oc1->oadd(), oc1, false);
        }
        if (oc1->odel()) {
            Ochg *oc2 = (Ochg*)tab->get((unsigned long)oc1->odel());
            if (oc2 != (Ochg*)ST_NIL) {
                // The object is deleted from memory here.
                if (XM()->DebugFlags() & DBG_UNDOLIST) {
                    fprintf(DBG_FP, "DELETE %p ", oc1->odel());
                    pobject(oc1->odel(), DBG_FP);
                }
                tab->remove((unsigned long)oc1->odel());
                o_cell_desc->unlink(oc1->odel(), false);
                oc1->set_odel(0);
                oc2->set_oadd(0);
            }
        }
    }
    if (tab->allocated() != 0) {
        Log()->ErrorLogV("undo list",
            "Info: undo list inconsistency detected, ignored.\n"
            "Additions and deletions don't match.");
    }
    delete tab;

    // Remove empty entries.
    o_obj_list = o_obj_list->purge(0);
}


void
Oper::queue_check()
{
    char tbuf[64];
    tbuf[0] = 0;
    if (o_obj_list)
        strcat(tbuf, " objects");
    if (o_prp_list)
        strcat(tbuf, " properties");
    if (o_ent_list)
        strcat(tbuf, " hypertext");
    if (*tbuf)
        Log()->WarningLogV("undo list", "Queues not clear:%s.\n", tbuf);
}


// Return true if this represents a reparameterization via
// XICP_PC_PARAMS property change of the current cell, which is a
// pcell sub-master.
//
bool
Oper::is_pcprms_change()
{
    if (!celldesc()->isPCellSubMaster())
        return (false);
    if (celldesc() != CurCell())
        return (false);
    CDp *p1 = celldesc()->prpty(XICP_PC_PARAMS);
    if (!p1 || !p1->string())
        return (false);
    CDp *p2 = cprop_list();
    while (p2 && p2->value() != XICP_PC_PARAMS)
        p2 = p2->next_prp();
    if (!p2 || !p2->string())
        return (false);
    // If the strings differ, assume that the property has changed.
    // This will be the only change for this Oper if so.
    return (strcmp(p1->string(), p2->string()));
}
// End of Oper functions


cUndoList *cUndoList::instancePtr = 0;

cUndoList::cUndoList()
{
    if (instancePtr) {
        fprintf(stderr, "Singleton class cUndoList already instantiated.\n");
        exit(1);
    }
    instancePtr = this;

    ul_operations = ul_redo_list = 0;
    ul_push_list = 0;
    ul_undo_length = TOPLEVEL_UNDO_LEN;

    setupInterface();
}


// Private static error exit.
//
void
cUndoList::on_null_ptr()
{
    fprintf(stderr, "Singleton class cUndoList used before instantiated.\n");
    exit(1);
}


// Begin an editing session, usually called when a new cell is opened. 
// If noreinit is true, don't reinit to the current cell.
//
void
cUndoList::ListBegin(bool noreinit, bool nocheck)
{
    if (!nocheck && XM()->DebugFlags())
        ul_curop.queue_check();
    ul_curop.clearLists(false);
    strcpy(ul_curop.cmd(), "no_cmd");
    if (DSP()->CurCellName() && !noreinit)
        ul_curop.set_celldesc(CurCell());
    else
        ul_curop.set_celldesc(0);
    if (ul_curop.celldesc())
        ul_curop.copyPrpList(ul_curop.celldesc()->prptyList());
    ul_curop.set_selected(false);
    ul_curop.set_flags(ul_curop.celldesc() ?
        ul_curop.celldesc()->getFlags() : 0);
    ul_curop.set_next_op(0);
    ul_curop.set_next_in_group(0);
}


// This function is called before each command which modifies the
// database.  It initializes the lists and saves copies of things
// for later use.  If selected is true, objects that are selected
// when added to the delete list will be selected after an undo.
//
void
cUndoList::ListCheck(const char *cmd, CDs *sdesc, bool selected)
{
    // Empty the trash, should be safe now
    Trash.empty_trash();

    if (XM()->DebugFlags())
        ul_curop.queue_check();
    CommitChanges();

    if (!cmd || !*cmd)
        cmd = "no_cmd";
    strncpy(ul_curop.cmd(), cmd, 7);
    ul_curop.cmd()[7] = '\0';
    ul_curop.freePrpList();
    ul_curop.set_celldesc(sdesc);
    ul_curop.set_selected(false);
    ul_curop.set_flags(0);
    if (sdesc) {
        ul_curop.copyPrpList(sdesc->prptyList());
        ul_curop.set_selected((sdesc->cellname() == DSP()->CurCellName() ?
            selected : false));
        ul_curop.set_flags(sdesc->getFlags());
    }
    ul_curop.set_next_op(0);
    ul_curop.set_next_in_group(0);

    // The 'selected' entry is set if objects were selected before an
    // operation was performed.  It causes the original objects to
    // become selected after undo.  We turn this flag off for the
    // previous command here as we are entering a new command.
    //
    if (ul_operations)
        ul_operations->set_selected(false);

    Oper *cur = ul_redo_list;
    ul_redo_list = 0;
    cur->clear();
}


// This variation is for use in commands that can be called within
// other commands, where ListCheck() has already been called and its
// state is expected to be valid after the second command returns.
// A call to ListPop() should follow each call to ListCheckPush().
//
// Basic rule:  If a command calls InitCallback(), which includes all
// NOTSAFE menu commands, then any open command will be exited, and
// ListCheck() should be used.  Otherwise, ListCheckPush()/ListPop()
// should be used.
//
// The override_symbolic flag should be set if changes are to a
// symbolic cell's main data and never to the symbolic data.  Used for
// operations that can change the cell's content independent of
// symbolic status.
//
void
cUndoList::ListCheckPush(const char *cmd, CDs *sdesc, bool selected,
    bool override_symbolic)
{
    // Symbolic override used in electrical mode only.
    if (!sdesc->isElectrical())
        override_symbolic = false;
    if (override_symbolic && sdesc->isSymbolic())
        sdesc = sdesc->owner();

    Oper *op = new Oper;
    *op = ul_curop;
    op->set_next_op(ul_push_list);
    ul_push_list = op;

    if (!cmd || !*cmd)
        cmd = "???";
    strncpy(ul_curop.cmd(), cmd, 7);
    ul_curop.cmd()[7] = '\0';
    ul_curop.set_cprop_list(0);
    ul_curop.set_celldesc(sdesc);
    ul_curop.copyPrpList(sdesc->prptyList());
    ul_curop.set_selected((sdesc->cellname() == DSP()->CurCellName() ?
        selected : false));
    ul_curop.set_flags(sdesc->getFlags());
    ul_curop.set_next_op(0);
    ul_curop.set_next_in_group(0);

    ul_curop.set_obj_list(0);
    ul_curop.set_prp_list(0);
    ul_curop.set_ent_list(0);
}


// This calls CommitChanges() and restores the state pushed in
// ListCheckPush()
//
bool
cUndoList::ListPop(bool redisplay)
{
    bool ret = false;
    if (ul_push_list) {
        ret = CommitChanges(redisplay);

        ul_curop.freePrpList();
        ul_curop = *ul_push_list;
        Oper *p = ul_push_list;
        ul_push_list = ul_push_list->next_op();

        p->set_next_in_group(0);
        p->set_cprop_list(0);
        p->set_obj_list(0);
        p->set_prp_list(0);
        p->set_ent_list(0);
        delete p;
    }
    return (ret);
}


// If cells other than the current cell are modified, this should be
// called before the modifications if these modifications are to be
// part of the same undo as the current cell modifications.
//
void
cUndoList::ListChangeCell(CDs *sdesc)
{
    if (HasChanged()) {

        Oper *op = new Oper;
        *op = ul_curop;

        // initialize
        ul_curop.set_next_in_group(op);
        ul_curop.set_cprop_list(0);
        ul_curop.set_celldesc(sdesc);
        ul_curop.copyPrpList(sdesc->prptyList());
        ul_curop.set_obj_list(0);
        ul_curop.set_prp_list(0);
        ul_curop.set_ent_list(0);
        ul_curop.set_selected(false);
        ul_curop.set_flags(sdesc->getFlags());
    }
    else {
        ul_curop.freePrpList();
        ul_curop.set_celldesc(sdesc);
        ul_curop.copyPrpList(sdesc->prptyList());
        ul_curop.set_selected(false);
    }
    // This is not compatible with the override feature.
}


// Bail out of an operation.
//
void
cUndoList::RestoreObjects()
{
    for (Ochg *oc = ul_curop.obj_list(); oc; oc = oc->next_chg()) {
        if (oc->oadd())
            oc->oadd()->set_state(CDDeleted);
        if (oc->odel())
            oc->odel()->set_state(CDVanilla);
    }
    Selections.purgeDeleted(CurCell());

    Ochg *ocn;
    for (Ochg *oc = ul_curop.obj_list(); oc; oc = ocn) {
        ocn = oc->next_chg();
        if (oc->oadd())
            ul_curop.celldesc()->unlink(oc->oadd(), false);
        delete oc;
    }
    ul_curop.set_obj_list(0);
    ul_curop.celldesc()->computeBB();

    Pchg *pcn;
    for (Pchg *pc = ul_curop.prp_list(); pc; pc = pcn) {
        pcn = pc->next_chg();
        if (pc->padd())
            delete pc->padd();
        pc->odesc()->link_prpty_list(pc->pdel());
        delete pc;
    }
    ul_curop.set_prp_list(0);
}


// Make the olddesc "invisible" in the database, and add its pointer
// to the delete list.  Turn off display of the reference point
// and terminals.
//
// Record newdesc in the add list.  Object has already been created
// in the database.
//
void
cUndoList::RecordObjectChange(CDs *sdesc, CDo *olddesc, CDo *newdesc)
{
    if (!sdesc)
        return;

    if (XM()->DebugFlags() & DBG_UNDOLIST) {
        if (olddesc)
            fprintf(DBG_FP, "ROC %c %s delete %c %s\n",
                sdesc->isElectrical() ? 'E' : 'P',
                sdesc->cellname()->string(),
                olddesc->type(), olddesc->type() == CDINSTANCE ?
                    OCALL(olddesc)->cellname()->string() : "");
        if (newdesc)
            fprintf(DBG_FP, "ROC %c %s add %c %s\n",
                sdesc->isElectrical() ? 'E' : 'P',
                sdesc->cellname()->string(),
                newdesc->type(), newdesc->type() == CDINSTANCE ?
                    OCALL(newdesc)->cellname()->string() : "");
        fprintf(DBG_FP, "---\n");
    }

    if (sdesc != ul_curop.celldesc()) {
        // This shouldn't happen
        if (!ul_curop.celldesc())
            ul_curop.set_celldesc(sdesc);
        else
            Log()->ErrorLogV("undo list",
                "Objects changed in non-target cell, this can not be undone.");
    }

    ED()->prptyUpdateList(olddesc, newdesc);
        // remove from list in prpty command
    SI()->UpdateObject(olddesc, newdesc);
        // update script handles

    // Make the list insertion atomic.  It seems that a mutex can be
    // avoided.
    // if (sdesc == ul_curop.celldesc())
    //   ul_curop.set_obj_list(new Ochg(olddesc, newdesc, ul_curop.obj_list()));
    if (sdesc == ul_curop.celldesc()) {
        Ochg *ox,  *oc = new Ochg(olddesc, newdesc, 0);
        do {
            ox = ul_curop.obj_list();
            oc->set_next_chg(ox);
        } while (! __sync_bool_compare_and_swap(
            (unsigned long*)ul_curop.obj_list_addr(), (unsigned long)ox,
            (unsigned long)oc));
    }

    if (newdesc)
        // object is already added to database
        newdesc->set_state(CDVanilla);

    if (olddesc) {
        olddesc->set_state(CDDeleted);
        if (sdesc->cellname() == DSP()->CurCellName()) {
            if (olddesc->type() == CDINSTANCE) {
                WindowDesc *wdesc;
                WDgen wgen(WDgen::MAIN, WDgen::CDDB);
                while ((wdesc = wgen.next()) != 0) {
                    if (wdesc->IsSimilar(DSP()->MainWdesc()))
                        wdesc->ShowInstanceOriginMark(ERASE, (CDc*)olddesc);
                }
            }
            if (!sdesc->isElectrical())
                DSP()->ShowOdescPhysProperties(olddesc, ERASE);
        }
        if (sdesc != ul_curop.celldesc()) {
            // Should never get here, but if we do the deletion should
            // be deferred until the caller is through with the
            // selection queue, etc.
            Trash.add_deferred(sdesc, olddesc);
        }
    }
    if (newdesc && newdesc->type() == CDINSTANCE)
        ED()->checkAbutment((CDc*)newdesc);
}


// If odesc is not null, unlink oldp from the property list of odesc,
// and add it to the the deleted property list.  Record newp in the
// property add list.  The property has not yet been linked.  If odesc
// is null, perform similar actions on the cell properties.  In either
// case, pseudo-properties are handled.  True is returned if the
// property undo lists change.
//
bool
cUndoList::RecordPrptyChange(CDs *sdesc, CDo *odesc, CDp *oldp, CDp *newp)
{
    if (!sdesc)
        return (false);
    if (odesc) {
        // object properties
        if (sdesc != ul_curop.celldesc()) {
            // This shouldn't happen
            if (!ul_curop.celldesc())
                ul_curop.set_celldesc(sdesc);
            else {
                Log()->ErrorLogV(mh::EditOperation,
                    "Attempted object property change in non-target cell,\n"
                    "operation not performed.");
                return (false);
            }
        }

        if (newp) {
            if (newp->value() == XICP_PC_PARAMS &&
                    odesc->type() == CDINSTANCE) {
                Errs()->init_error();
                if (!ED()->reparamInstance(sdesc, (CDc*)odesc, newp)) {
                    Errs()->add_error("reparamInstance failed");
                    Log()->ErrorLog(mh::EditOperation, Errs()->get_error());
                }
                delete newp;
                return (false);
            }
            if (newp->value() >= XprpType && newp->value() < XprpEnd &&
                    !sdesc->isElectrical()) {
                // Intercept set property request for physical
                // geometry change, strip "long text".
                char *s = hyList::hy_strip(newp->string());
                newp->set_string(s);
                delete [] s;
                Errs()->init_error();
                if (!ED()->acceptPseudoProp(odesc, sdesc, newp->value(),
                        newp->string())) {
                    Errs()->add_error("accept_pseudo_prop failed");
                    Log()->ErrorLog(mh::EditOperation, Errs()->get_error());
                }
                delete newp;
                return (false);
            }
        }
        if (oldp) {
            // Note that this is not done if the newp is a pseudo-property.
            odesc->prptyUnlink(oldp);
            oldp->set_next_prp(0);
        }
        if (sdesc->isElectrical() && odesc->type() == CDINSTANCE) {
            if ((newp && newp->value() == P_NAME) ||
                    (oldp && oldp->value() == P_NAME))
                sdesc->unsetConnected();
            else if ((newp && newp->value() == P_RANGE) ||
                    (oldp && oldp->value() == P_RANGE))
                sdesc->unsetConnected();
        }
        ul_curop.set_prp_list(new Pchg(odesc, oldp, newp, ul_curop.prp_list()));
        SI()->UpdatePrpty(odesc, oldp, newp);
    }
    else {
        // cell properties
        if (newp) {
            const char *p_cell = "PCell";
            if (newp->value() == XICP_PC_PARAMS) {
                if (sdesc->isPCellSubMaster()) {
                    Errs()->init_error();
                    if (!ED()->reparamSubMaster(sdesc, newp->string())) {
                        Errs()->add_error("reparamSubmaster failed");
                        Log()->ErrorLog(p_cell, Errs()->get_error());
                    }
                    else {
                        if (DSP()->CurMode() == Physical) {
                            // Any DRC errors from the old instances
                            // will still be around.  Being lazy,
                            // clear all DRC errors.
                            DrcIf()->clearCurError();
                        }
                        DSP()->RedisplayAll();
                    }
                    delete newp;
                    return (false);  // no need to call CommitChanges()
                }
                else {
                    // PCell super-master, adding or replacing params.
                    char *s;
                    if (!PC()->formatParams(newp->string(), &s, 0)) {
                        Errs()->add_error(
                            "formatParams failed: parameter syntax error");
                        Log()->ErrorLog(p_cell, Errs()->get_error());
                        delete [] newp;
                        return (false);
                    }
                    newp->set_string(s);
                    delete [] s;
                    if (!oldp) {
                        // make sure old property is gone
                        oldp = sdesc->prpty(XICP_PC_PARAMS);
                    }
                    if (sdesc->prpty(XICP_PC_SCRIPT)) {
                        // All needed properties exist, so set flags.
                        sdesc->setPCell(true, true, false);
                    }
                }
            }
            else if (newp->value() == XICP_PC_SCRIPT) {
                if (!oldp) {
                    // make sure old property is gone
                    oldp = sdesc->prpty(XICP_PC_SCRIPT);
                }
                if (sdesc->prpty(XICP_PC_PARAMS)) {
                    // All needed properties exist, so set flags.
                    sdesc->setPCell(true, true, false);
                }
            }
            else if (newp->value() == XICP_FLAGS) {
                sdesc->prptyUpdateFlags(newp->string());
                if (!oldp)
                    // make sure old property is gone
                    oldp = sdesc->prpty(XICP_FLAGS);
            }
        }
        if (oldp) {
            if (oldp->value() == XICP_FLAGS && !newp)
                sdesc->prptyUpdateFlags("0");
            sdesc->prptyUnlink(oldp);
            delete oldp;
        }
        if (newp) {
            newp->set_next_prp(sdesc->prptyList());
            sdesc->setPrptyList(newp);
        }
    }
    return (true);
}


namespace {
    void setup_label_undo(CDc *pcdold, CDc *pcdnew, bool undo)
    {
        CDap ap(pcdnew);
        if (!undo) {
            // Add an internal property to store the transform to be
            // used to keep instance labels in the correct position
            // after undo/redo.  This is the relative transform from
            // old to new.
            //
            cTfmStack stk;
            stk.TPush();
            stk.TApplyTransform(pcdnew);
            stk.TPush();
            stk.TApplyTransform(pcdold);
            stk.TInverse();
            stk.TLoadInverse();
            stk.TPremultiply();
            CDtx tx;
            stk.TCurrent(&tx);
            sLstr lstr;
            tx.print_string(lstr);
            pcdold->prptyAdd(XICP_CLABEL, lstr.string(), Physical);

            // Now move the instance labels.  We have to iterate
            // through array elements, and find duals by present label
            // placement (we can't use the extraction structs since
            // the new instance has not been associated.

            for (unsigned int iy = 0; iy < ap.ny; iy++) {
                for (unsigned int ix = 0; ix < ap.nx; ix++) {
                    int vecix;
                    CDc *ed = pcdold->findElecDualOfPhys(&vecix, ix, iy);
                    if (!ed)
                        continue;

                    // Do this before changing position!
                    DSP()->ShowInstTerminalMarks(ERASE, ed, vecix);
                    DSP()->ShowInstanceLabel(ERASE, ed, vecix);

                    CDp_range *pr = (CDp_range*)ed->prpty(P_RANGE);
                    CDp_name *pn;
                    if (vecix > 0 && pr)
                        pn = pr->name_prp(0, vecix);
                    else
                        pn = (CDp_name*)ed->prpty(P_NAME);
                    if (!pn)
                        continue;
                    int px = pn->pos_x();
                    int py = pn->pos_y();
                    stk.TPoint(&px, &py);
                    pn->set_pos_x(px);
                    pn->set_pos_y(py);
                }
            }
            return;
        }

        // Find the transform from the original move.  Remove the
        // property.

        CDp *px = pcdold->prpty(XICP_CLABEL);
        if (!px)
            return;
        pcdold->prptyUnlink(px);

        CDtx tx;
        if (!tx.parse(px->string())) {
            Log()->ErrorLogV(mh::Internal,
                "Internal error, failed to parse transform string: \"%s\".",
                px->string());
            delete px;
            return;
        }
        delete px;

        // Invert the transform, and save it as a new property.
        cTfmStack stk;
        stk.TPush();
        stk.TLoadCurrent(&tx);
        stk.TInverse();
        stk.TLoadInverse();
        stk.TCurrent(&tx);
        sLstr lstr;
        tx.print_string(lstr);
        pcdnew->prptyAdd(XICP_CLABEL, lstr.string(), Physical);

        for (unsigned int iy = 0; iy < ap.ny; iy++) {
            for (unsigned int ix = 0; ix < ap.nx; ix++) {
                int vecix;
                CDc *ed = pcdnew->findElecDualOfPhys(&vecix, ix, iy);
                if (!ed)
                    continue;

                // Do this before changing position!
                DSP()->ShowInstTerminalMarks(ERASE, ed, vecix);
                DSP()->ShowInstanceLabel(ERASE, ed, vecix);

                CDp_range *pr = (CDp_range*)ed->prpty(P_RANGE);
                CDp_name *pn;
                if (vecix > 0 && pr)
                    pn = pr->name_prp(0, vecix);
                else
                    pn = (CDp_name*)ed->prpty(P_NAME);
                if (!pn)
                    continue;
                int x = pn->pos_x();
                int y = pn->pos_y();
                stk.TPoint(&x, &y);
                pn->set_pos_x(x);
                pn->set_pos_y(y);
            }
        }
        stk.TPop();
    }


    // This block is for putting the physical subcell name label in the
    // right place in a move operation.  The location is saved in a
    // property.
    //
    void move_hack(Oper *cur, bool undo)
    {
        if (!ExtIf()->hasExtract())
            return;
        if (DSP()->ShowTerminals()) {
            for (Ochg *oc = cur->obj_list(); oc; oc = oc->next_chg()) {
                if (!oc->oadd() || oc->oadd()->type() != CDINSTANCE ||
                        !oc->odel() || oc->odel()->type() != CDINSTANCE)
                    continue;

                CDc *cd = OCALL(oc->oadd());
                CDs *msdesc = cd->masterCell();
                if (!msdesc)
                    continue;
                if (msdesc->isElectrical())
                    continue;
                CDc *oldcd = OCALL(oc->odel());
                if (oldcd->masterCell() != msdesc)
                    continue;
                CDs *esdesc = CDcdb()->findCell(msdesc->cellname(),
                    Electrical);
                if (!esdesc || esdesc->elecCellType() != CDelecSubc)
                    continue;

                // The old and new instances are physical subcircuits
                // with the same master.

                setup_label_undo(oldcd, cd, undo);
            }
        }
    }


    // Remove all references to ent from the given list.
    //
    void purge_hylist(Oper *list, hyEnt *ent)
    {
        for (Oper *cur = list; cur; cur = cur->next_op()) {
            ed_undolist::Hlist *hp = 0;
            ed_undolist::Hlist *hn, *h;
            for (h = cur->ent_list(); h; h = hn) {
                hn = h->next;
                if (h->from == ent) {
                    if (!hp)
                        cur->set_ent_list(hn);
                    else
                        hp->next = hn;
                    delete h;
                    continue;
                }
                hp = h;
            }
        }
    }
}


// If create is true, record the original content of a hypertext entry
// structure, and link it in a list, along with the address of the
// original structure.  Void pointers are used to avoid having to
// include hypertext definitions globally.
//
// If create is false, Remove all references to ent in the undo/redo
// storage.  Call when ent is about to be freed.
//
void
cUndoList::RecordHYent(hyEnt *ent, bool create)
{
    if (create) {
        if (ent)
            ul_curop.set_ent_list(
                new ed_undolist::Hlist(ent, ul_curop.ent_list()));
    }
    else {
        purge_hylist(ul_operations, ent);
        purge_hylist(ul_redo_list, ent);
        ul_curop.set_next_op(0);  // should never be otherwise
        purge_hylist(&ul_curop, ent);
    }
}


// The CommitChanges function is called at the end of each operation
// to perform cleanup and add the operation state to the history list. 
// Note that the cell properties are copied and saved for each
// operation, since the entries may change as objects change.  If
// redisplay is true, the updated areas will be redisplayed.  The
// function returns true if a change was detected and processed.
//
bool
cUndoList::CommitChanges(bool redisplay, bool nodrc)
{
    ED()->handleAbutment();
    DSP()->SetIncompleteObject(0);
    bool change_made = false;
    bool no_inc_modified = false;
    bool bb_set = false;
    BBox addBB, delBB;
    for (Oper *cur = &ul_curop; cur; cur = cur->next_in_group()) {
        if (!cur->celldesc())
            continue;
        bool no_inc;
        if (!cur->changed(&no_inc)) {
            if (!no_inc)
                continue;
            if (!change_made)
                no_inc_modified = true;
        }
        else
            no_inc_modified = false;
        change_made = true;

        // Reverse order of the lists, so oldest change is first.
        cur->set_obj_list(cur->obj_list()->reverse_list());
        cur->set_prp_list(cur->prp_list()->reverse_list());

        // Save name label position.
        move_hack(cur, false);

        // Erase terminals if they might change.
        bool update_all_terminals = false;
        if (DSP()->ShowTerminals() &&
                cur->celldesc()->cellname() == DSP()->CurCellName()) {
            for (Ochg *oc = cur->obj_list(); oc; oc = oc->next_chg()) {
                if ((oc->oadd() && oc->oadd()->type() == CDINSTANCE) ||
                        (oc->odel() && oc->odel()->type() == CDINSTANCE)) {
                    update_all_terminals = true;
                    break;
                }
            }
            if (!update_all_terminals && cur->celldesc()->isElectrical()) {
                for (Pchg *pc = cur->prp_list(); pc; pc = pc->next_chg()) {
                    if ((pc->padd() && pc->padd()->value() == P_NAME) ||
                            (pc->pdel() && pc->pdel()->value() == P_NAME)) {
                        update_all_terminals = true;
                        break;
                    }
                    if ((pc->padd() && pc->padd()->value() == P_SYMBLC) ||
                            (pc->pdel() && pc->pdel()->value() == P_SYMBLC)) {
                        update_all_terminals = true;
                        break;
                    }
                }
            }
        }
        if (update_all_terminals)
            DSP()->ShowTerminals(ERASE);

        // Link new properties.
        for (Pchg *pc = cur->prp_list(); pc; pc = pc->next_chg())
            pc->odesc()->link_prpty_list(pc->padd());

        // Compute BB of change, do this before removing objects that
        // are added and deleted.
        if (redisplay && cur->celldesc()->cellname() == DSP()->CurCellName()) {
            cur->changeBB(&addBB, &delBB);
            bb_set = true;
        }

        // Look for SYMBLC instance property changes, and update the
        // instance BB.
        if (cur->celldesc()->isElectrical()) {
            for (Pchg *pc = cur->prp_list(); pc; pc = pc->next_chg()) {
                if (pc->odesc() && pc->odesc()->type() == CDINSTANCE) {
                    if ((pc->pdel() && pc->pdel()->value() == P_SYMBLC) ||
                            (pc->padd() && pc->padd()->value() == P_SYMBLC)) {
                        CDc *pcd = (CDc*)pc->odesc();
                        pcd->updateBB();
                        pcd->updateTerminals(0);
                        if (redisplay) {
                            pcd->addSymbChangeBB(&addBB);
                            bb_set = true;
                        }
                    }
                }
            }
        }

        // Check object list consistency, and remove duplicates.
        // Objects that are added and deleted are hard-deleted here,
        // and removed from list.
        cur->check_objects();

        // Perform the deletions.
        bool has_deletes = false;
        for (Ochg *oc = cur->obj_list(); oc; oc = oc->next_chg()) {
            if (oc->odel()) {
                if (!has_deletes) {
                    Selections.purgeDeleted(CurCell());
                    has_deletes = true;
                }
                cur->celldesc()->unlink(oc->odel(), true);
            }
        }
        cur->celldesc()->computeBB();

        // Erase DRC error indicators, and maybe perform interactive DRC
        // on new objects.
        //
        BBox *eBBp = 0;
        if (!cur->celldesc()->isElectrical()) {
            if (has_deletes)
                DrcIf()->eraseListError(cur->obj_list(), false);
            if (!nodrc && cur->celldesc() == CurCell(Physical)) {
                if (DrcIf()->intrListTest(cur->obj_list(), eBBp) && eBBp &&
                        redisplay)
                    addBB.add(eBBp);
            }
        }

        // Check associated vire vertices.
        if (DSP()->CurMode() == Electrical) {
            for (Ochg *oc = cur->obj_list(); oc; oc = oc->next_chg()) {
                if (oc->oadd())
                    ScedIf()->install(oc->oadd(), cur->celldesc(), false);
                if (oc->odel())
                    ScedIf()->uninstall(oc->odel(), cur->celldesc());
            }
        }

        // Update display of dots.
        if (cur->celldesc() == CurCell(Electrical) &&
                ScedIf()->showingDots() != DotsNone) {
            for (Ochg *oc = cur->obj_list(); oc; oc = oc->next_chg()) {
                ScedIf()->updateDots(CurCell(Electrical), oc->oadd());
                ScedIf()->updateDots(CurCell(Electrical), oc->odel());
            }
        }

        if (!no_inc_modified) {
            // Don't do this if only symbolic mode changed.  We allow
            // the user to change back when done editing the cell, at
            // which point, if the user decides not to revert, the
            // modification is recorded.
            //
            // Similarly, in physical mode, don't increment the
            // modified count if changes were made only to internal or
            // derived layers.

            cur->celldesc()->incModified();

            if (cur->celldesc()->isElectrical()) {
                CDcbin cbin(cur->celldesc());
                if (cbin.phys())
                    cbin.phys()->setAssociated(false);

                // Update the connectivity incrementally.
                ConnectIncr(cur->celldesc(), cur);
            }
        }

        // Display terminals.
        if (update_all_terminals) {
            for (Ochg *oc = cur->obj_list(); oc; oc = oc->next_chg()) {
                if (oc->oadd() && oc->oadd()->type() == CDINSTANCE) {
                    CDc *cd = OCALL(oc->oadd());
                    CDs *msdesc = cd->masterCell();
                    if (msdesc && !msdesc->isElectrical()) {
                        // We've added a physical subcell, if it has
                        // an electrical dual, update terminal
                        // locations.

                        CDap ap(cd);
                        for (unsigned int iy = 0; iy < ap.ny; iy++) {
                            for (unsigned int ix = 0; ix < ap.nx; ix++) {
                                int vecix;
                                CDc *ed = cd->findElecDualOfPhys(&vecix,
                                    ix, iy);
                                if (!ed)
                                    continue;
                                ExtIf()->placePhysSubcTerminals(ed, vecix,
                                    cd, ix, iy);
                                DSP()->ShowInstanceLabel(DISPLAY, ed, vecix);
                            }
                        }
                    }
                }
            }
            DSP()->ShowTerminals(DISPLAY);
        }
    }

    if (change_made) {
        if (DSP()->CurMode() == Electrical) {
            for (int i = 1; i < DSP_NUMWINS; i++) {
                if (DSP()->Window(i))
                    DSP()->Window(i)->UpdateProxy();
            }
            DSP()->LabelHyUpdate();
        }
        ul_curop.fixParentConnections();

        // save for undo
        trim_undo_list();
        Oper *cur = new Oper(ul_curop);
        cur->set_no_inc_mod(no_inc_modified);

        // The 'selected' entry is set if objects were selected before
        // an operation was performed.  This is also cleared in
        // ListCheck.
        //
        if (ul_operations)
            ul_operations->set_selected(false);

        cur->set_next_op(ul_operations);
        ul_operations = cur;

        // initialize
        CDs *cursd = CurCell();  // this can be null!
        ul_curop.set_next_in_group(0);
        ul_curop.set_cprop_list(0);
        ul_curop.set_celldesc(cursd);
        ul_curop.copyPrpList(cursd ? cursd->prptyList() : 0);
        ul_curop.set_obj_list(0);
        ul_curop.set_prp_list(0);
        ul_curop.set_ent_list(0);
        ul_curop.set_holdover(false);   // never set
        ul_curop.set_no_inc_mod(false); // never set
        ul_curop.set_flags(cursd ? cursd->getFlags() : 0);

        // After an operation, the redo list may be bogus, so free it.
        cur = ul_redo_list;
        ul_redo_list = 0;
        cur->clear();
        ED()->prptyRelist();  // update the Properties pop-ups

        XM()->ShowParameters();
        XM()->InfoRefresh();
    }
    if (bb_set) {
        bool a_ok = (addBB.right > addBB.left && addBB.top > addBB.bottom);
        bool d_ok = (delBB.right > delBB.left && delBB.top > delBB.bottom);
        if (a_ok && d_ok) {
            if (addBB.left > delBB.right || delBB.left > addBB.right ||
                    addBB.bottom > delBB.top || delBB.bottom > addBB.top) {

                // No overlap, redisplay both areas.
                DSP()->RedisplayArea(&delBB);
                DSP()->RedisplayArea(&addBB);
            }
            else {
                // Some overlap, just combine and redisplay.
                addBB.add(&delBB);
                DSP()->RedisplayArea(&addBB);
            }
        }
        else if (a_ok)
            DSP()->RedisplayArea(&addBB);
        else if (d_ok)
            DSP()->RedisplayArea(&delBB);
    }
    return (change_made);
}


// The main functions for undo/redo.
//
void
cUndoList::UndoOperation()
{
    Oper *cur = ul_operations;
    if (cur)
        ul_operations = cur->next_op();
    else {
        PL()->ShowPrompt("Nothing in list to undo.");
        return;
    }
    if (!cur->no_inc_mod()) {
        if (cur->holdover())
            cur->celldesc()->incModified();
        else
            cur->celldesc()->decModified();
        if (cur->celldesc()->isElectrical()) {
            CDcbin cbin(cur->celldesc());
            if (cbin.phys())
                cbin.phys()->setAssociated(false);
        }
    }
    if (!cur->celldesc()->isElectrical())
        DrcIf()->eraseListError(cur->obj_list(), true);
    bool tmp_exgp = false;
    if (strcmp(cur->cmd(), "flatten")) {
        // Allow undo/redo of flatten operation with an immutable
        // inverted ground plane layer from extraction.
        tmp_exgp = ExtIf()->setInvGroundPlaneImmutable(false);
    }
    do_operation(cur, true);
    if (strcmp(cur->cmd(), "flatten"))
        ExtIf()->setInvGroundPlaneImmutable(tmp_exgp);
    cur->fixParentConnections();
    PL()->ShowPromptV("%s undone in cell %s.",
        cur->cmd(), cur->celldesc()->cellname());
    cur->set_next_op(ul_redo_list);
    ul_redo_list = cur;
    XM()->InfoRefresh();
    if (DSP()->CurMode() == Electrical) {
        for (int i = 1; i < DSP_NUMWINS; i++) {
            if (DSP()->Window(i))
                DSP()->Window(i)->UpdateProxy();
        }
        DSP()->LabelHyUpdate();
    }
}


void
cUndoList::RedoOperation()
{
    Oper *cur = ul_redo_list;
    if (cur)
        ul_redo_list = cur->next_op();
    else {
        PL()->ShowPrompt("Nothing in list to redo.");
        return;
    }
    if (!cur->no_inc_mod()) {
        if (cur->holdover())
            cur->celldesc()->decModified();
        else
            cur->celldesc()->incModified();
        if (cur->celldesc()->isElectrical()) {
            CDcbin cbin(cur->celldesc());
            if (cbin.phys())
                cbin.phys()->setAssociated(false);
        }
    }
    bool tmp_exgp = false;
    if (strcmp(cur->cmd(), "flatten"))
        tmp_exgp = ExtIf()->setInvGroundPlaneImmutable(false);
    do_operation(cur, false);
    if (strcmp(cur->cmd(), "flatten"))
        ExtIf()->setInvGroundPlaneImmutable(tmp_exgp);
    cur->fixParentConnections();
    PL()->ShowPromptV("%s redone in cell %s.",
        cur->cmd(), cur->celldesc()->cellname());
    cur->set_next_op(ul_operations);
    ul_operations = cur;
    XM()->InfoRefresh();
    if (DSP()->CurMode() == Electrical) {
        for (int i = 1; i < DSP_NUMWINS; i++) {
            if (DSP()->Window(i))
                DSP()->Window(i)->UpdateProxy();
        }
        DSP()->LabelHyUpdate();
    }
}


// This is called at the end of an editing session.  If keep_undo, the
// cell is being saved (so the modification count is zeroed) but
// retained as the current cell.  In this case, a flag is set in the
// undo items to indicate a "holdover".  Otherwise, the lists are
// cleared.
//
void
cUndoList::ListFinalize(bool keep_undo)
{
    if (keep_undo) {
        for (Oper *cur = ul_operations; cur; cur = cur->next_op())
            cur->set_holdover(true);
    }
    else {
        Oper *cur = ul_operations;
        ul_operations = 0;
        cur->clear();
        cur = ul_redo_list;
        ul_redo_list = 0;
        cur->clear();
    }
}


// Eliminate anything on sd/ld from the operations lists.  If sd is
// null, eliminate anything on ld.  Otherwise, eliminate objects on ld
// in sdesc.  Called when deleting objects on a layer without undo.
//
void
cUndoList::InvalidateLayer(CDs *sd, CDl *ld)
{
    ul_curop.purge(sd, ld);
    ul_operations->purge(sd, ld);
    ul_redo_list->purge(sd, ld);
}


// Eliminate sd/od from the operations lists.  The sd is the container
// for od and must be given.  If od is null, eliminate all sd
// objects/properties.  Called when deleting an object without undo.
//
void
cUndoList::InvalidateObject(CDs *sd, CDo *od, bool save)
{
    if (!sd)
        return;
    if (!save) {
        ul_operations->purge(sd, od);
        ul_redo_list->purge(sd, od);
    }
}


ULstate *
cUndoList::PopState()
{
    ULstate *eul = new ULstate(DSP()->CurCellName(), ul_operations,
        ul_redo_list);
    ul_operations = 0;
    ul_redo_list = 0;
    return (eul);
}


void
cUndoList::PushState(ULstate *eul)
{
    ul_operations->clear();
    ul_operations = 0;
    ul_redo_list->clear();
    ul_redo_list = 0;

    if (eul) {
        if (eul->cellname == DSP()->CurCellName()) {
            ul_operations = eul->operations;
            eul->operations = 0;
            ul_redo_list = eul->redo_list;
            eul->redo_list = 0;
        }
        delete eul;
    }
}


// Incrementally update connectivity after an operation.  Presently,
// this doesn't do anything except update the names.  The connected
// status is always false after an operation.
//
void
cUndoList::ConnectIncr(CDs *sd, Oper *op)
{
    if (!sd || !sd->isElectrical())
        return;
    bool doupd = false;
    for (Ochg *oc = op->obj_list(); oc; oc = oc->next_chg()) {
        if (oc->odel() && (oc->odel()->type() == CDINSTANCE ||
                oc->odel()->type() == CDWIRE)) {
            doupd = true;
            break;
        }
        if (oc->oadd() && (oc->oadd()->type() == CDINSTANCE ||
                oc->oadd()->type() == CDWIRE)) {
            doupd = true;
            break;
        }
    }

    if (!doupd) {
        for (Pchg *pc = op->prp_list(); pc; pc = pc->next_chg()) {
            if (pc->pdel() && (pc->pdel()->value() == P_NAME ||
                    pc->pdel()->value() == P_RANGE)) {
                doupd = true;
                break;
            }
            if (pc->padd() && (pc->padd()->value() == P_NAME ||
                    pc->padd()->value() == P_RANGE)) {
                doupd = true;
                break;
            }
        }
    }

    // If a name changes, call updateNames() since there may be
    // other changes due to name conflicts.
    if (doupd) {
        if (sd == CurCell(Electrical, true)) {
            ScedIf()->PopUpNodeMap(0, MODE_UPD);
            // Above calls connect() if window is instantiated.
            ScedIf()->updateNames(sd);
        }
        return;
    }

    // If here, no device needed updating, but maybe a mutual?
    for (Ochg *oc = op->obj_list(); oc; oc = oc->next_chg()) {
        if (oc->odel() && oc->odel()->type() == CDLABEL) {
            CDp_lref *prf = (CDp_lref*)oc->odel()->prpty(P_LABRF);
            if (prf && prf->cellref() == sd) {
                doupd = true;
                break;
            }
        }
        if (oc->oadd() && oc->oadd()->type() == CDLABEL) {
            CDp_lref *prf = (CDp_lref*)oc->oadd()->prpty(P_LABRF);
            if (prf && prf->cellref() == sd) {
                doupd = true;
                break;
            }
        }
    }
    if (doupd)
        sd->prptyMutualUpdate();
}


// Select all objects recently added.  This is used in the SelectLast
// script function.
//
int
cUndoList::SelectLast(const char *types)
{
    CDs *cursd = CurCell();
    int cnt = 0;
    for (Ochg *oc = ul_curop.obj_list(); oc; oc = oc->next_chg()) {
        if (oc->oadd() && oc->oadd()->state() == CDVanilla &&
                (!types || strchr(types, oc->oadd()->type()))) {
            Selections.insertObject(cursd, oc->oadd(), true);
            cnt++;
        }
    }
    return (cnt);
}


namespace {
    struct sPcPch
    {
        sPcPch(Oper *u, Oper *r) { undos = u; redos = r; }

        Oper *undos;
        Oper *redos;
    };
}


// If the current cell is a pcell sub-master, this will return the
// undo/redo list elements that are XICP_PC_PARAMS property changes
// for this cell.  The rest of the lists are cleared.  This is some
// hackery to allow undo/redo of pcell reparameterization of the
// current cell.  The returned object is opaque, and should be passed
// to ResetPcPrmChanges().
//
void *
cUndoList::GetPcPrmChanges()
{
    Oper *uops = ul_operations;
    ul_operations = 0;
    Oper *oprv = 0, *onxt;
    for (Oper *cur = uops; cur; cur = onxt) {
        onxt = cur->next_op();
        if (!cur->is_pcprms_change()) {
            if (oprv)
                oprv->set_next_op(onxt);
            else
                uops = onxt;
            delete cur;
            continue;
        }
        oprv = cur;
    }
    Oper *rops = ul_redo_list;
    ul_redo_list = 0;
    oprv = 0;
    for (Oper *cur = rops; cur; cur = onxt) {
        onxt = cur->next_op();
        if (!cur->is_pcprms_change()) {
            if (oprv)
                oprv->set_next_op(onxt);
            else
                rops = onxt;
            delete cur;
            continue;
        }
        oprv = cur;
    }
    if (uops || rops)
        return (new sPcPch(uops, rops));
    return (0);
}


// Put the lists back, so that reparameterization can be
// undone/redone.
void
cUndoList::ResetPcPrmChanges(void *pcptr)
{
    // The lists should already be clear.
    Oper *ops = ul_operations;
    ul_operations = 0;
    ops->clear();
    ops = ul_redo_list;
    ul_redo_list = 0;
    ops->clear();
    sPcPch **p = (sPcPch**)pcptr;
    if (p && *p) {
        ul_operations = (*p)->undos;
        ul_redo_list = (*p)->redos;
        delete *p;
        *p = 0;
    }
}


// Free history list elements if the list is too long.
//
void
cUndoList::trim_undo_list()
{
    if (!ul_undo_length)
        // set to zero, no length limit
        return;
    Oper *o = ul_operations;
    if (!o)
        return;
    if (ul_undo_length == 1) {
        o->clear();
        ul_operations = 0;
        return;
    }
    for (int i = 2; o->next_op(); o = o->next_op(), i++) {
        if (i == ul_undo_length) {
            o->next_op()->clear();
            o->set_next_op(0);
            return;
        }
    }
}


// Actually perform undo/redo.
//
void
cUndoList::do_operation(Oper *curop, bool undo)
{
    enum { part_redisplay, full_redisplay };
    int redisplay_phys = part_redisplay;
    int redisplay_elec = part_redisplay;
    bool dodisplay_phys = false;
    bool dodisplay_elec = false;
    bool symb_mode_change = false;

    CDs *cursd = CurCell();
    if (!cursd)
        return;
    BBox BBphys, BBelec;
    for (Oper *cur = curop; cur; cur = cur->next_in_group()) {

        // Hypertext links.
        for (ed_undolist::Hlist *hl = cur->ent_list(); hl; hl = hl->next)
            hl->rotate();
        if (!cur->celldesc())
            continue;

        // Cell properties.
        if (cur->is_pcprms_change()) {
            CDp *p2 = cur->cprop_list();
            while (p2 && p2->value() != XICP_PC_PARAMS)
                p2 = p2->next_prp();
            cur->copyPrpList(cur->celldesc()->prptyList());
            CDp *pnew = new CDp(p2->string(), p2->value());
            // This reverts the pcell sub-master.
            RecordPrptyChange(cur->celldesc(), 0, 0, pnew);
        }
        else {
            CDp *pd = cur->celldesc()->prptyList();
            cur->celldesc()->setPrptyList(cur->cprop_list());
            cur->set_cprop_list(pd);
        }
        if (cur->celldesc() == ul_curop.celldesc()) {
            ul_curop.freePrpList();
            ul_curop.copyPrpList(cur->celldesc()->prptyList());
        }

        bool is_elec = cur->celldesc()->isElectrical();
        bool is_toplevel =
            (cur->celldesc()->cellname() == DSP()->CurCellName());
        if (!is_toplevel) {
            if (is_elec)
                redisplay_elec = full_redisplay;
            else
                redisplay_phys = full_redisplay;
        }

        // Check for symbolic mode change.
        //
        // A "symbolic mode change" occurs if a symbolic property is
        // added or deleted, and/or the active flag effectively changes. 
        // In symbolic mode (schematic cell has symbolic property with
        // active flag set), a given window may or may not be
        // displaying the symbolic cell, depending on such things as
        // the no_top_symbolic attribute.
        //
        // We simply redraw all electrical windows on symbolic mode
        // change.

        if (DSP()->CurMode() == Electrical &&
                cur->celldesc()->cellname() == cursd->cellname()) {

            bool new_symb_prpty_state = false;
            CDp_sym *psnew = (CDp_sym*)cur->celldesc()->prpty(P_SYMBLC);
            if (psnew && psnew->active())
                new_symb_prpty_state = true;

            bool old_symb_prpty_state = false;
            CDp_sym *psold = (CDp_sym*)cur->cprop_list();
            while (psold && psold->value() != P_SYMBLC)
                psold = (CDp_sym*)psold->next_prp();
            if (psold && psold->active())
                old_symb_prpty_state = true;

            if (new_symb_prpty_state != old_symb_prpty_state) {
                ScedIf()->assertSymbolic(new_symb_prpty_state);
                redisplay_elec = full_redisplay;
                dodisplay_elec = true;
                symb_mode_change = true;
            }
        }

        move_hack(cur, true);

        // Erase terminals if they might change.
        bool update_top_terminals = false;
        bool update_all_terminals = false;
        if (DSP()->ShowTerminals() &&
                cur->celldesc()->cellname() == DSP()->CurCellName()) {
            update_top_terminals = true;
            update_all_terminals = symb_mode_change;
            if (!update_all_terminals) {
                for (Ochg *oc = cur->obj_list(); oc; oc = oc->next_chg()) {
                    if ((oc->oadd() && oc->oadd()->type() == CDINSTANCE) ||
                            (oc->odel() &&
                            oc->odel()->type() == CDINSTANCE)) {
                        update_all_terminals = true;
                        break;
                    }
                }
            }
            if (!update_all_terminals && cur->celldesc()->isElectrical()) {
                for (Pchg *pc = cur->prp_list(); pc; pc = pc->next_chg()) {
                    if ((pc->padd() && pc->padd()->value() == P_NAME) ||
                            (pc->pdel() && pc->pdel()->value() == P_NAME)) {
                        update_all_terminals = true;
                        break;
                    }
                    if ((pc->padd() && pc->padd()->value() == P_SYMBLC) ||
                            (pc->pdel() && pc->pdel()->value() == P_SYMBLC)) {
                        update_all_terminals = true;
                        break;
                    }
                }
            }
        }
        if (update_all_terminals)
            DSP()->ShowTerminals(ERASE);
        else if (update_top_terminals)
            DSP()->ShowCellTerminalMarks(ERASE);

        // Objects.
        if (cur->obj_list()) {
            if (is_toplevel) {
                if (is_elec)
                    cur->changeBB(&BBelec);
                else
                    cur->changeBB(&BBphys);
            }
            if (is_elec)
                dodisplay_elec = true;
            else
                dodisplay_phys = true;

            // Take care of property string display.
            if (!is_elec && is_toplevel) {
                for (Ochg *oc = cur->obj_list(); oc; oc = oc->next_chg()) {
                    if (oc->oadd())
                        DSP()->ShowOdescPhysProperties(oc->oadd(), ERASE);
                }
            }

            // Add first, objects may be in both lists.
            bool has_add = false;
            for (Ochg *oc = cur->obj_list(); oc; oc = oc->next_chg()) {
                if (oc->oadd())
                    has_add = true;
                if (!oc->odel())
                    continue;
                cur->celldesc()->insert(oc->odel());
                if (has_add)
                    // fix property editor
                    ED()->prptyUpdateList(oc->oadd(), oc->odel());
                oc->odel()->set_state(CDVanilla);
                if (is_elec) {
                    ScedIf()->install(oc->odel(), cur->celldesc(), false);
                    if (!symb_mode_change && is_toplevel)
                        ScedIf()->updateDots(CurCell(Electrical), oc->odel());
                }
                if (undo && cur->selected() &&
                        !oc->odel()->has_flag(CDmergeDeleted)) {

                    // CDmergeDeleted is set if the object is
                    // unselected, and deleted as a result of a merge. 
                    // This flag is cleared if the object is ever
                    // selected.
                    //
                    // This is the only use made of this flag, i.e.,
                    // preventing spurious selection queue insertions.

                    Selections.insertObject(cursd, oc->odel(), true);
                }
            }

            if (has_add) {
                for (Ochg *oc = cur->obj_list(); oc; oc = oc->next_chg()) {
                    if (oc->oadd())
                        oc->oadd()->set_state(CDDeleted);
                }
                Selections.purgeDeleted(cursd);
                const BBox *sBB = cur->celldesc()->BB();
                bool fixbb = false;
                for (Ochg *oc = cur->obj_list(); oc; oc = oc->next_chg()) {
                    if (!oc->oadd())
                        continue;
                    if (!(*sBB > oc->oadd()->oBB()))
                        fixbb = true;
                    cur->celldesc()->unlink(oc->oadd(), true);
                    if (is_elec) {
                        ScedIf()->uninstall(oc->oadd(), cur->celldesc());
                        if (!symb_mode_change && is_toplevel)
                            ScedIf()->updateDots(CurCell(Electrical),
                                oc->oadd());
                    }
                }
                if (fixbb)
                    cur->celldesc()->computeBB();
            }
        }

        // Properties.
        if (!is_elec && is_toplevel) {
            for (Pchg *pc = cur->prp_list(); pc; pc = pc->next_chg()) {
                if (pc->pdel() || pc->padd())
                    DSP()->ShowOdescPhysProperties(pc->odesc(), ERASE);
            }
        }

        // Update, look for SYMBLC instance property changes, and
        // update the instance BB.
        for (Pchg *pc = cur->prp_list(); pc; pc = pc->next_chg()) {
            bool upd = false;
            if (is_elec && pc->odesc() &&
                    pc->odesc()->type() == CDINSTANCE &&
                    ((pc->pdel() && pc->pdel()->value() == P_SYMBLC) ||
                    (pc->padd() && pc->padd()->value() == P_SYMBLC))) {
                BBelec.add(&pc->odesc()->oBB());
                upd = true;
                dodisplay_elec = true;
            }
            pc->odesc()->link_prpty_list(pc->pdel());
            if (pc->padd())
                pc->odesc()->prptyUnlink(pc->padd());
            if (upd) {
                ((CDc*)pc->odesc())->updateBB();
                BBelec.add(&pc->odesc()->oBB());
                ((CDc*)pc->odesc())->updateTerminals(0);
            }
        }

        // Incrementally connect.
        if (cur->celldesc()->isElectrical()) {
            if (cur->celldesc()->isSymbolic()) {
                bool trd = DSP()->NoRedisplay();
                DSP()->SetNoRedisplay(true);
                ConnectIncr(cur->celldesc(), cur);
                DSP()->SetNoRedisplay(trd);
            }
            else
                ConnectIncr(cur->celldesc(), cur);
        }

        // Rotate lists.
        for (Ochg *oc = cur->obj_list(); oc; oc = oc->next_chg())
            oc->swap();
        for (Pchg *pc = cur->prp_list(); pc; pc = pc->next_chg())
            pc->swap();

        // Display terminals.
        if (update_all_terminals) {
            for (Ochg *oc = cur->obj_list(); oc; oc = oc->next_chg()) {
                if (oc->oadd() && oc->oadd()->type() == CDINSTANCE) {
                    CDc *cd = OCALL(oc->oadd());
                    CDs *msdesc = cd->masterCell();
                    if (msdesc && !msdesc->isElectrical()) {
                        // We've added a physical subcell, if it has
                        // an electrical dual, update terminal
                        // locations.

                        CDap ap(cd);
                        for (unsigned int iy = 0; iy < ap.ny; iy++) {
                            for (unsigned int ix = 0; ix < ap.nx; ix++) {
                                int vecix;
                                CDc *ed = cd->findElecDualOfPhys(&vecix,
                                    ix, iy);
                                if (!ed)
                                    continue;
                                ExtIf()->placePhysSubcTerminals(ed, vecix,
                                    cd, ix, iy);
                                DSP()->ShowInstanceLabel(DISPLAY, ed, vecix);
                            }
                        }
                    }
                }
            }
            if (!symb_mode_change)
                DSP()->ShowTerminals(DISPLAY);
        }
        else if (update_top_terminals)
            DSP()->ShowCellTerminalMarks(DISPLAY);

        if (!is_elec && is_toplevel) {
            for (Pchg *pc = cur->prp_list(); pc; pc = pc->next_chg()) {
                if (pc->pdel() || pc->padd())
                    DSP()->ShowOdescPhysProperties(pc->odesc(), DISPLAY);
            }
        }

        // Reflect node changes, should really test if this is necessary.
        cur->celldesc()->reflectTerminals();

        if (update_all_terminals && symb_mode_change)
            DSP()->ShowTerminals(DISPLAY);

        // Rotate cell flags.
        unsigned int tf = cur->flags();
        cur->set_flags(cur->celldesc()->getFlags());
        if (cur->celldesc()->isElectrical()) {
            unsigned int cf = cur->celldesc()->getFlags();
            // Don't set the connectivity flags if they aren't set now.
            // A connectivity update may be required.
            if (!(cf & CDs_CONNECT))
                tf &= ~CDs_CONNECT;
            if (!(cf & CDs_SPCONNECT))
                tf &= ~CDs_SPCONNECT;
        }
        else {
            unsigned int cf = cur->celldesc()->getFlags();
            if (!(cf & CDs_CONNECT))
                tf &= ~CDs_CONNECT;
            if (!(cf & CDs_DSEXT))
                tf &= ~CDs_DSEXT;
            if (!(cf & CDs_DUALS))
                tf &= ~CDs_DUALS;
        }
        cur->celldesc()->setFlags(tf);
    }

    XM()->ShowParameters();
    ScedIf()->PopUpNodeMap(0, MODE_UPD);
    ED()->prptyRelist();

    if (dodisplay_elec) {
        if (redisplay_elec == full_redisplay) {
            if (symb_mode_change) {
                WindowDesc *wdesc;
                WDgen wgen(WDgen::MAIN, WDgen::CDDB);
                while ((wdesc = wgen.next()) != 0) {
                    if (wdesc->IsSimilar(Electrical, DSP()->MainWdesc())) {
                        wdesc->CenterFullView();
                        wdesc->Redisplay(0);
                    }
                }
            }
            else
                DSP()->RedisplayAll(Electrical);
        }
        else {
            WindowDesc *wdesc;
            WDgen wgen(WDgen::MAIN, WDgen::CDDB);
            while ((wdesc = wgen.next()) != 0) {
                if (wdesc->IsSimilar(Electrical, DSP()->MainWdesc()))
                    wdesc->Redisplay(&BBelec);
            }
        }
    }
    if (dodisplay_phys) {
        if (redisplay_phys == full_redisplay)
            DSP()->RedisplayAll(Physical);
        else {
            WindowDesc *wdesc;
            WDgen wgen(WDgen::MAIN, WDgen::CDDB);
            while ((wdesc = wgen.next()) != 0) {
                if (wdesc->IsSimilar(Physical, DSP()->MainWdesc()))
                    wdesc->Redisplay(&BBphys);
            }
        }
    }
}

