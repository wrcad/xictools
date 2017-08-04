
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

#ifndef UNDOLIST_H
#define UNDOLIST_H


namespace ed_undolist {
    struct Hlist;
}

// In toplevel cell editing, limit the undo list length.
// In subedits, the undo length is unlimited, so that the
// unedited state can be recovered.
#define TOPLEVEL_UNDO_LEN 25

// Keep track of changes of object descs.
//
struct Ochg : public op_change_t
{
    Ochg(CDo *d, CDo *a, Ochg *n) : op_change_t(d, a)
        {
            oc_next = n;
        }

    static void destroy(Ochg *oc, bool freedel)
        {
            while (oc) {
                Ochg *ox = oc;
                oc = oc->oc_next;
                if (freedel)
                    delete ox->oc_delobj;
                delete ox;
            }
        }

    static Ochg *reverse_list(Ochg *thiso)
        {
            Ochg *oc = thiso, *o0 = 0;
            while (oc) {
                Ochg *ox = oc;
                oc = oc->oc_next;
                ox->oc_next = o0;
                o0 = ox;
            }
            return (o0);
        }

    static Ochg *purge(Ochg *thiso, const CDo *obj)
        {
            Ochg *o0 = thiso, *op = 0, *on;
            for (Ochg *o = o0; o; o = on) {
                on = o->oc_next;
                if (obj) {
                    if (o->oc_delobj == obj)
                        o->oc_delobj = 0;
                    if (o->oc_addobj == obj)
                        o->oc_addobj = 0;
                }
                if (!o->oc_delobj && !o->oc_addobj) {
                    if (op)
                        op->oc_next = on;
                    else
                        o0 = on;
                    delete o;
                    continue;
                }
                op = o;
            }
            return (o0);
        }

    static Ochg *purge_layer(Ochg *thiso, const CDl *ld)
        {
            Ochg *o0 = thiso, *op = 0, *on;
            for (Ochg *o = o0; o; o = on) {
                on = o->oc_next;
                if (ld) {
                    if (o->oc_delobj && o->oc_delobj->ldesc() == ld)
                        o->oc_delobj = 0;
                    if (o->oc_addobj && o->oc_addobj->ldesc() == ld)
                        o->oc_addobj = 0;
                }
                if (!o->oc_delobj && !o->oc_addobj) {
                    if (op)
                        op->oc_next = on;
                    else
                        o0 = on;
                    delete o;
                    continue;
                }
                op = o;
            }
            return (o0);
        }

    static Ochg *copy(const Ochg *thiso)
        {
            Ochg *l0 = 0, *le = 0;
            for (const Ochg *oc = thiso; oc; oc = oc->oc_next) {
                if (!l0)
                    l0 = le = new Ochg(oc->oc_delobj, oc->oc_addobj, 0);
                else {
                    le->oc_next = new Ochg(oc->oc_delobj, oc->oc_addobj, 0);
                    le = le->oc_next;
                }
            }
            return (l0);
        }

    Ochg *next_chg()        const { return (oc_next); }
    void set_next_chg(Ochg *n)    { oc_next = n; };

private:
    Ochg *oc_next;
};


// Keep track of property changes of object descs.
//
struct Pchg
{
    Pchg()
        {
            pc_next = 0;
            pc_delprp = 0;
            pc_addprp = 0;
            pc_odesc = 0;
        }

    Pchg(CDo *o, CDp *dp, CDp *ap, Pchg* n)
        {
            pc_next = n;
            pc_delprp = dp;
            pc_addprp = ap;
            pc_odesc = o;
        }

    static void destroy(Pchg *pc, bool freedel)
        {
            while (pc) {
                Pchg *px = pc;
                pc = pc->pc_next;
                if (freedel)
                    delete px->pc_delprp;
                delete px;
            }
        }

    static Pchg *reverse_list(Pchg *thisp)
        {
            Pchg *pc = thisp, *p0 = 0;
            while (pc) {
                Pchg *px = pc;
                pc = pc->pc_next;
                px->pc_next = p0;
                p0 = px;
            }
            return (p0);
        }

    static Pchg *purge(Pchg *thisp, const CDo *obj)
        {
            Pchg *p0 = thisp, *pp = 0, *pn;
            for (Pchg *p = p0; p; p = pn) {
                pn = p->pc_next;
                if (p->pc_odesc == obj) {
                    if (pp)
                        pp->pc_next = pn;
                    else
                        p0 = pn;
                    delete p;
                    continue;
                }
                pp = p;
            }
            return (p0);
        }

    static Pchg *purge_layer(Pchg *thisp, const CDl *ld)
        {
            Pchg *p0 = thisp, *pp = 0, *pn;
            for (Pchg *p = p0; p; p = pn) {
                pn = p->pc_next;
                if (!p->pc_odesc || p->pc_odesc->ldesc() == ld) {
                    if (pp)
                        pp->pc_next = pn;
                    else
                        p0 = pn;
                    delete p;
                    continue;
                }
                pp = p;
            }
            return (p0);
        }

    void swap()
        {
            CDp *tmp = pc_delprp;
            pc_delprp = pc_addprp;
            pc_addprp = tmp;
        }

    Pchg *next_chg()        const { return (pc_next); }
    CDp *pdel()             const { return (pc_delprp); }
    CDp *padd()             const { return (pc_addprp); }
    CDo *odesc()            const { return (pc_odesc); }

private:
    Pchg *pc_next;
    CDp *pc_delprp;         // deleted property desc
    CDp *pc_addprp;         // added property desc
    CDo *pc_odesc;          // object to which the properties belong
};


enum { CBBall, CBBadd, CBBdel };

// Linked list of operations for Undo/Redo.
//
struct Oper
{
    Oper();
    ~Oper();

    static void destroy(Oper *o)
        {
            while (o) {
                Oper *ox = o;
                o = o->o_next;
                delete ox;
            }
        }

    bool changed(bool* = 0) const;
    void fixParentConnections();
    void clearLists(bool);
    static void purge(Oper*, const CDs*, const CDl*);
    static void purge(Oper*, const CDs*, const CDo*);
    void freePrpList();
    void copyPrpList(CDp*);
    void changeBB(BBox*, BBox* = 0);
    void check_objects();
    void queue_check();
    bool is_pcprms_change();

    Oper *next_op()                     const { return (o_next); }
    Oper *next_in_group()               const { return (o_next_in_group); }
    char *cmd()                         { return (o_cmd); }
    CDs *celldesc()                     const { return (o_cell_desc); }
    CDp *cprop_list()                   const { return (o_cprop_list); }
    Ochg *obj_list()                    const { return (o_obj_list); }
    Pchg *prp_list()                    const { return (o_prp_list); }
    ed_undolist::Hlist *ent_list()      const { return (o_ent_list); }
    bool selected()                     const { return (o_selected); }
    bool holdover()                     const { return (o_holdover); }
    bool no_inc_mod()                   const { return (o_no_inc_mod); }
    unsigned int flags()                const { return (o_flags); }

    Ochg **obj_list_addr()              { return (&o_obj_list); }

    void set_next_op(Oper *o)           { o_next = o; }
    void set_next_in_group(Oper *o)     { o_next_in_group = o; }
    void set_celldesc(CDs *s)           { o_cell_desc = s; }
    void set_cprop_list(CDp *p)         { o_cprop_list = p; }
    void set_obj_list(Ochg *o)          { o_obj_list = o; }
    void set_prp_list(Pchg *p)          { o_prp_list = p; }
    void set_ent_list(ed_undolist::Hlist *h) { o_ent_list = h; }
    void set_selected(bool b)           { o_selected = b; }
    void set_holdover(bool b)           { o_holdover = b; }
    void set_no_inc_mod(bool b)         { o_no_inc_mod = b; }
    void set_flags(unsigned int f)      { o_flags = f; }

private:
    Oper *o_next;                       // next undo/redo group
    Oper *o_next_in_group;              // operations for different cells
    char o_cmd[8];                      // command producing operation
    CDs *o_cell_desc;                   // cell desc of operations
    CDp *o_cprop_list;                  // cell's previous properties
    Ochg *o_obj_list;                   // objects added/deleted
    Pchg *o_prp_list;                   // object properties added/deleted
    ed_undolist::Hlist *o_ent_list;     // hypertext references
    bool o_selected;                    // object was selected
    bool o_holdover;                    // operation after cell update
    bool o_no_inc_mod;                  // don't inc modified flag
    unsigned int o_flags;               // cell's flags (symbolic mode?)
};

// State storage for mode switch.
//
struct ULstate
{
    ULstate(CDcellName n, Oper *o, Oper *r)
        { cellname = n; operations = o; redo_list = r; }
    ~ULstate() { Oper::destroy(operations); Oper::destroy(redo_list); }

    CDcellName cellname;
    Oper *operations;
    Oper *redo_list;
};


inline class cUndoList *Ulist();

// Main list interface class.
//
class cUndoList
{
    static cUndoList *ptr()
        {
            if (!instancePtr)
                on_null_ptr();
            return (instancePtr);
        }

    static void on_null_ptr();

public:
    friend inline cUndoList *Ulist() { return (cUndoList::ptr()); }

    cUndoList();
    void ListBegin(bool, bool);
    void ListCheck(const char*, CDs*, bool);
    void ListCheckPush(const char*, CDs*, bool, bool);
    bool ListPop(bool = false);
    void ListChangeCell(CDs*);
    void RestoreObjects();
    void RecordObjectChange(CDs*, CDo*, CDo*);
    bool RecordPrptyChange(CDs*, CDo*, CDp*, CDp*);
    bool CommitChanges(bool = false, bool = false);
    void RecordHYent(hyEnt*, bool);
    void UndoOperation();
    void RedoOperation();
    void ListFinalize(bool);
    void InvalidateLayer(CDs*, CDl*);
    void InvalidateObject(CDs*, CDo*, bool);
    ULstate *PopState();
    void PushState(ULstate*);
    void ConnectIncr(CDs*, Oper*);
    int SelectLast(const char*);
    void *GetPcPrmChanges();
    void ResetPcPrmChanges(void*);

public:
    bool HasChanged()
        {
            for (Oper *cur = &ul_curop; cur; cur = cur->next_in_group()) {
                if (!cur->celldesc())
                    continue;
                if (cur->changed())
                    return (true);
            }
            return (false);
        }

    bool HasUndo() { return (ul_operations != 0); }
    bool HasRedo() { return (ul_redo_list != 0); }
    bool HasOneRedo() { return (ul_redo_list && !ul_redo_list->next_op()); }

    int CountRedo()
        {
            int cnt = 0;
            for (Oper *o = ul_redo_list; o; o = o->next_op())
                cnt++;
            return (cnt);
        }

    Oper *RotateUndo(Oper *op)
        { Oper *tmp = ul_operations; ul_operations = op; return (tmp); }
    Oper *RotateRedo(Oper *op)
        { Oper *tmp = ul_redo_list; ul_redo_list = op; return (tmp); }

    const Oper &CurOp() { return (ul_curop); }

    void SetUndoLength(int l) { if (l >= 0) ul_undo_length = l; }
    int UndoLength() { return (ul_undo_length); }

    Oper *UndoList() { return (ul_operations); }
    Oper *RedoList() { return (ul_redo_list); }

private:
    // undolist.cc
    void trim_undo_list();
    void do_operation(Oper*, bool);
    void qcheck();

    // undolist_setif.cc
    void setupInterface();

    Oper ul_curop;          // Queues for provisionally added and
                            // deleted objects.
    Oper *ul_operations;    // Queues for Undo/Redo.
    Oper *ul_redo_list;
    Oper *ul_push_list;     // List head for ListPush/Pop
    int ul_undo_length;     // Max length of undo list.

    static cUndoList *instancePtr;
};

#endif

