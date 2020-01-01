
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
 * vl -- Verilog Simulator and Verilog support library.                   *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

/*========================================================================*
  Copyright (c) 1992, 1993
        Regents of the University of California
  All rights reserved.

  Use and copying of this software and preparation of derivative works
  based upon this software are permitted.  However, any distribution of
  this software or derivative works must include the above copyright 
  notice.

  This software is made available AS IS, and neither the Electronics
  Research Laboratory or the Universify of California make any
  warranty about the software, its performance or its conformity to
  any specification.

  Author: Szu-Tsung Cheng, stcheng@ic.Berkeley.EDU
          10/92
          10/93
 *========================================================================*/

#include "vl_st.h"
#include "vl_list.h"
#include "vl_defs.h"
#include "vl_types.h"


//---------------------------------------------------------------------------
//  Verilog description objects
//---------------------------------------------------------------------------

namespace {
    vl_context *var_context()
    {
        return (vl_var::simulator->context);
    }

    void set_var_context(vl_context *cx)
    {
        vl_var::simulator->context = cx;
    }
}


vl_module::vl_module()
{
    type = 0;
    name = 0;
    ports = 0;
    sig_st = 0;
    inst_count = 0;
    instance = 0;
    mod_items = 0;
    inst_st = 0;
    func_st = 0;
    task_st = 0;
    blk_st = 0;
    tunit = 1.0;
    tprec = 1.0;
}


vl_module::vl_module(vl_desc *desc, char *mn, lsList<vl_port*> *pts,
    lsList<vl_stmt*> *mitems)
{
    type = ModDecl;
    name = mn;
    ports = pts;
    sig_st = 0;
    inst_count = 0;
    instance = 0;
    mod_items = mitems;
    inst_st = 0;
    func_st = 0;
    task_st = 0;
    blk_st = 0;
    tunit = 1.0;
    tprec = 1.0;
    if (desc) {
        desc->modules->newEnd(this);
        desc->mp_st->insert(mn, this);
    }
}


vl_module::~vl_module()
{
    delete [] name;
    delete_list(ports);
    delete_list(mod_items);
    delete_table(sig_st);
    delete inst_st;
    delete func_st;
    delete task_st;
    delete blk_st;
}


vl_module *
vl_module::copy()
{
    sort_moditems();

    vl_module *retval = new vl_module;
    retval->type = type;
    retval->name = vl_strdup(name);

    vl_context *cx = var_context();
    while (cx) {
        if (cx->module && !strcmp(cx->module->name, name)) {
            vl_error("recursive instantiation of module %s", name);
            vl_var::simulator->abort();
            return (retval);
        }
        cx = cx->parent;
    }

    set_var_context(vl_context::push(var_context(), retval));
    retval->ports = copy_list(ports);
    retval->mod_items = copy_list(mod_items);
    set_var_context(vl_context::pop(var_context()));
    retval->tunit = tunit;
    retval->tprec = tprec;

    return (retval);
}


void
vl_module::init()
{
    set_var_context(vl_context::push(var_context(), this));
    init_list(mod_items);
    set_var_context(vl_context::pop(var_context()));
}
// End vl_module functions.


vl_primitive::vl_primitive()
{
    type = 0;
    name = 0;
    ports = 0;
    sig_st = 0;
    inst_count = 0;
    instance = 0;
    decls = 0;
    initial = 0;
    ptable = 0;
    rows = 0;
    seq_init = false;
    for (int i = 0; i < MAXPRIMLEN; iodata[i++] = 0) ;
    for (int i = 0; i < MAXPRIMLEN; lastvals[i++] = BitDC) ;
}


vl_primitive::vl_primitive(vl_desc *desc, char *prim_name)
{
    type = CombPrimDecl;
    name = prim_name;
    ports = 0;
    sig_st = 0;
    inst_count = 0;
    instance = 0;
    decls = 0;
    initial = 0;
    ptable = 0;
    rows = 0;
    seq_init = false;
    for (int i = 0; i < MAXPRIMLEN; iodata[i++] = 0) ;
    for (int i = 0; i < MAXPRIMLEN; lastvals[i++] = BitDC) ;
    if (desc) {
        desc->primitives->newEnd(this);
        desc->mp_st->insert(prim_name, this);
    }
}


vl_primitive::~vl_primitive()
{
    delete [] name;
    delete_list(ports);
    delete_list(decls);
    delete initial;
    delete_table(sig_st);
    delete [] ptable;
}


void
vl_primitive::init_table(lsList<vl_port*> *prim_ports,
    lsList<vl_decl*> *prim_decls, vl_bassign_stmt *initial_stmt,
    lsList<vl_prim_entry*> *prim_entries)
{
    ports = prim_ports;
    decls = prim_decls;
    initial = initial_stmt;
    if (prim_entries) {
        rows = prim_entries->length();
        if (rows) {
            ptable = new unsigned char[rows*MAXPRIMLEN];
            unsigned char *row = ptable;
            lsGen<vl_prim_entry*> gen(prim_entries);
            int os = type == SeqPrimDecl ? 2 : 1;
            vl_prim_entry *e;
            while (gen.next(&e)) {
                unsigned char *col = row;
                *col++ = e->next_state;
                if (type == SeqPrimDecl)
                    *col++ = e->state;
                for (int i = 0; i < MAXPRIMLEN - os; i++)
                    *col++ = e->inputs[i];
                row += MAXPRIMLEN;
            }
        }
    }
    delete_list(prim_entries);
}


vl_primitive *
vl_primitive::copy()
{
    vl_primitive *retval = new vl_primitive();
    retval->type = type;
    retval->name = vl_strdup(name);

    set_var_context(vl_context::push(var_context(), retval));
    retval->ports = copy_list(ports);
    retval->decls = copy_list(decls);
    if (initial)
        retval->initial = initial->copy();
    retval->rows = rows;
    if (ptable) {
        retval->ptable = new unsigned char[rows*MAXPRIMLEN];
        memcpy(retval->ptable, ptable, rows*MAXPRIMLEN);
    }
    set_var_context(vl_context::pop(var_context()));
    return (retval);
}


void
vl_primitive::init()
{
    set_var_context(vl_context::push(var_context(), this));
    init_list(decls);
    set_var_context(vl_context::pop(var_context()));
}
// End vl_primitive functions.


vl_prim_entry::vl_prim_entry(lsList<int> *ip, unsigned char st,
    unsigned char output)
{
    state = st;
    next_state = output;
    int i = 0;
    int insym;
    lsGen<int> gen(ip);
    while (i < MAXPRIMLEN && gen.next(&insym))
        inputs[i++] = insym;
    while (i < MAXPRIMLEN)
        inputs[i++] = PrimNone;
}
// End vl_prim_entry functions.


vl_port::vl_port(short t, char *n, lsList<vl_var*> *exprs)
{
    type = t;
    name = n;
    port_exp = exprs;
}


vl_port::~vl_port()
{
    delete [] name;
    lsGen<vl_var*> gen(port_exp);
    vl_var *v;
    while (gen.next(&v)) {
        if (!(v->flags() & VAR_IN_TABLE))
            delete v;
    }
    delete(port_exp);
}


vl_port *
vl_port::copy()
{
    const vl_port *prt = this;
    if (!prt)
        return (0);
    return (new vl_port(type, vl_strdup(name), copy_list(port_exp)));
}
// End vl_port functions.


vl_port_connect::vl_port_connect(short t, char *n, vl_expr *e)
{
    type = t;
    name = n;
    expr = e;
    i_assign = 0;
    o_assign = 0;
}


vl_port_connect::~vl_port_connect()
{
    delete [] name;
    delete expr;
    delete i_assign;
    delete o_assign;
}


vl_port_connect *
vl_port_connect::copy()
{
    const vl_port_connect *pc = this;
    if (!pc)
        return (0);
    return (new vl_port_connect(type, vl_strdup(name),
        expr ? expr->copy() : 0));
}
// End vl_port_connect functions.


//---------------------------------------------------------------------------
//  Module items
//---------------------------------------------------------------------------

vl_decl::vl_decl(short t, vl_strength stren, vl_range *r, vl_delay *del,
    lsList<vl_bassign_stmt*> *l, lsList<vl_var*> *i)
{
    type = t;
    range = r;
    ids = i;
    list = l;
    delay = del;
    strength = stren;
}


vl_decl::vl_decl(short t, vl_range *r, lsList<vl_var*> *i)
{
    type = t;
    range = r;
    ids = i;
    list = 0;
    delay = 0;
}


vl_decl::vl_decl(short t, lsList<char*> *l)
{
    type = t;
    range = 0;
    ids = new lsList<vl_var*>;
    list = 0;
    delay = 0;

    lsGen<char*> gen(l);
    char *ev;
    while (gen.next(&ev)) {
        vl_var *v = new vl_var(ev, 0);
        ids->newEnd(v);
    }
    delete l;
}


vl_decl::vl_decl(short t, vl_range *r, lsList<vl_bassign_stmt*> *assigns)
{
    type = t;
    range = r;
    ids = 0;
    list = assigns;
    delay = 0;
}


vl_decl::~vl_decl()
{
    // the variables are deleted with the sig_st
    delete range;
    lsGen<vl_var*> gen(ids);
    vl_var *v;
    while (gen.next(&v)) {
        if (!(v->flags() & VAR_IN_TABLE))
            delete v;
    }
    delete(ids);
    delete_list(list);
    delete delay;
}


vl_decl *
vl_decl::copy()
{
    return (new vl_decl(type, strength, range->copy(), delay->copy(),
        copy_list(list), copy_list(ids)));
}


// Put the variables in the symbol table, initialize types, evaluate
// parameters.
//
void
vl_decl::init()
{
    if (ids) {
        // declared without assignment
        lsGen<vl_var*> gen(ids);
        vl_var *v;
        while (gen.next(&v)) {
            table<vl_var*> *st = symtab(v);
            if (!st)
                break;
            vl_var *nvar;
            if (!st->lookup(v->name(), &nvar)) {
                st->insert(v->name(), v);
                v->or_flags(VAR_IN_TABLE);
            }
            else if (v != nvar) {
                ids->replace(v, nvar);
                delete v;
                v = nvar;
            }
            var_setup(v, type);
            if (v->net_type() >= REGwire) {
                if (!v->delay())
                    v->set_delay(delay->copy());
            }
        }
    }
    if (list && type != DefparamDecl) {
        // declared with assignment
        lsGen<vl_bassign_stmt*> gen(list);
        vl_bassign_stmt *bs;
        while (gen.next(&bs)) {
            table<vl_var*> *st = symtab(bs->lhs);
            if (!st)
                break;
            vl_var *nvar;
            if (!st->lookup(bs->lhs->name(), &nvar)) {
                st->insert(bs->lhs->name(), bs->lhs);
                bs->lhs->or_flags(VAR_IN_TABLE);
            }
            else if (bs->lhs != nvar) {
                if (bs->lhs->delay() && !nvar->delay()) {
                    nvar->set_delay(bs->lhs->delay());
                    bs->lhs->set_delay(0);
                }
                delete bs->lhs;
                bs->lhs = nvar;
            }
            bs->flags |= BAS_SAVE_LHS;
            var_setup(bs->lhs, type);
            if (bs->lhs->net_type() >= REGwire) {
                if (!bs->lhs->delay())
                    bs->lhs->set_delay(delay->copy());
            }
            if (type == ParamDecl || type == RegDecl || type == IntDecl ||
                    type == TimeDecl || type == RealDecl)
                *bs->lhs = bs->rhs->eval();
        }
    }
}


// Return the symbol table for the declaration.
//
table<vl_var*> *
vl_decl::symtab(vl_var *var)
{
    if (!var->name()) {
        vl_error("unnamed variable in %s declaration", decl_type());
        errout(this);
        var->simulator->abort();
        return (0);
    }
    vl_context *cx = var->simulator->context;
    table<vl_var*> *st = 0;
    if (cx->block) {
        if (!cx->block->sig_st)
            cx->block->sig_st = new table<vl_var*>;
        st = cx->block->sig_st;
    }
    else if (cx->fjblk) {
        if (!cx->fjblk->sig_st)
            cx->fjblk->sig_st = new table<vl_var*>;
        st = cx->fjblk->sig_st;
    }
    else if (cx->function) {
        if (!cx->function->sig_st)
            cx->function->sig_st = new table<vl_var*>;
        st = cx->function->sig_st;
    }
    else if (cx->task) {
        if (!cx->task->sig_st)
            cx->task->sig_st = new table<vl_var*>;
        st = cx->task->sig_st;
    }
    else if (cx->primitive) {
        if (!cx->primitive->sig_st)
            cx->primitive->sig_st = new table<vl_var*>;
        st = cx->primitive->sig_st;
    }
    else if (cx->module) {
        if (!cx->module->sig_st)
            cx->module->sig_st = new table<vl_var*>;
        st = cx->module->sig_st;
    }
    if (!st) {
        vl_error("no symbol table for %s declaration", var->name());
        var->simulator->abort();
    }
    return (st);
}


// Take care of initialization of the declared variables.
//
void
vl_decl::var_setup(vl_var *var, int vtype)
{
    if (vtype == ParamDecl) {
        var->configure(range, vtype);
        var->set_net_type(REGparam);
        return;
    }
    var->configure(range, vtype, var->range());

    switch (vtype) {
    case RealDecl:
        var->data().r = 0.0;
        return;
    case IntDecl:
        var->data().i = 0;
        return;
    case TimeDecl:
        var->data().t = 0;
        return;
    case EventDecl:
        if (var->net_type() == REGnone) {
            var->set_net_type(REGevent);
            return;
        }
        break;
    case InputDecl:
        if (var->io_type() == IOnone) {
            var->set_io_type(IOinput);
            if (var->net_type() == REGnone) {
                var->set_net_type(REGwire);
                var->setbits(BitZ);
            }
            return;
        }
        break;
    case OutputDecl:
        if (var->io_type() == IOnone) {
            var->set_io_type(IOoutput);
            if (var->net_type() == REGnone) {
                var->set_net_type(REGwire);
                var->setbits(BitZ);
            }
            return;
        }
        break;
    case InoutDecl:
        if (var->io_type() == IOnone) {
            var->set_io_type(IOinout);
            if (var->net_type() == REGnone) {
                var->set_net_type(REGwire);
                var->setbits(BitZ);
            }
            return;
        }
        break;
    case RegDecl:
        if (var->net_type() == REGnone ||
                (var->net_type() == REGwire && var->io_type() != IOnone)) {
            var->set_net_type(REGreg);
            var->setbits(BitDC);
            return;
        }
        break;
    case WireDecl:
        if (var->net_type() == REGnone ||
                (var->net_type() == REGwire && var->io_type() != IOnone)) {
            var->set_net_type(REGwire);
            var->setbits(BitZ);
            var->set_strength(strength);
            return;
        }
        break;
    case TriDecl:
        if (var->net_type() == REGnone ||
                (var->net_type() == REGwire && var->io_type() != IOnone)) {
            var->set_net_type(REGtri);
            var->setbits(BitZ);
            var->set_strength(strength);
            return;
        }
        break;
    case Tri0Decl:
        if (var->net_type() == REGnone ||
                (var->net_type() == REGwire && var->io_type() != IOnone)) {
            var->set_net_type(REGtri0);
            var->setbits(BitL);
            var->set_strength(STRpull, STRpull);
            return;
        }
        break;
    case Tri1Decl:
        if (var->net_type() == REGnone ||
                (var->net_type() == REGwire && var->io_type() != IOnone)) {
            var->set_net_type(REGtri1);
            var->setbits(BitH);
            var->set_strength(STRpull, STRpull);
            return;
        }
        break;
    case Supply0Decl:
        if (var->net_type() == REGnone ||
                (var->net_type() == REGwire && var->io_type() != IOnone)) {
            var->set_net_type(REGsupply0);
            var->setbits(BitL);
            var->set_strength(STRsupply, STRsupply);
            return;
        }
        break;
    case Supply1Decl:
        if (var->net_type() == REGnone ||
                (var->net_type() == REGwire && var->io_type() != IOnone)) {
            var->set_net_type(REGsupply1);
            var->setbits(BitH);
            var->set_strength(STRsupply, STRsupply);
            return;
        }
        break;
    case WandDecl:
        if (var->net_type() == REGnone ||
                (var->net_type() == REGwire && var->io_type() != IOnone)) {
            var->set_net_type(REGwand);
            var->setbits(BitZ);
            var->set_strength(strength);
            return;
        }
        break;
    case TriandDecl:
        if (var->net_type() == REGnone ||
                (var->net_type() == REGwire && var->io_type() != IOnone)) {
            var->set_net_type(REGtriand);
            var->setbits(BitZ);
            var->set_strength(strength);
            return;
        }
        break;
    case WorDecl:
        if (var->net_type() == REGnone ||
                (var->net_type() == REGwire && var->io_type() != IOnone)) {
            var->set_net_type(REGtriand);
            var->setbits(BitZ);
            var->set_strength(strength);
            return;
        }
        break;
    case TriorDecl:
        if (var->net_type() == REGnone ||
                (var->net_type() == REGwire && var->io_type() != IOnone)) {
            var->set_net_type(REGwand);
            var->setbits(BitZ);
            var->set_strength(strength);
            return;
        }
        break;
    case TriregDecl:
        if (var->net_type() == REGnone ||
                (var->net_type() == REGwire && var->io_type() != IOnone)) {
            var->set_net_type(REGtrireg);
            var->setbits(BitDC);
            var->set_strength(strength);
            return;
        }
        break;
    default:
        var->set_net_type(REGnone);
        return;
    }
    vl_error("symbol %s redeclared as %s", var->name(), decl_type());
    errout(this);
    var->simulator->abort();
}
// End vl_decl functions.


vl_procstmt::vl_procstmt(short t, vl_stmt *s)
{
    type = t;
    stmt = s;
    lasttime = (vl_time_t)-1;
}


vl_procstmt::~vl_procstmt()
{
    delete stmt;
}


vl_procstmt *
vl_procstmt::copy()
{
    return (new vl_procstmt(type, stmt ? stmt->copy() : 0));
}


void
vl_procstmt::init()
{
    if (stmt)
        stmt->init();
}
// End vl_procstmt functions.



vl_cont_assign::vl_cont_assign(vl_strength stren, vl_delay *del,
    lsList<vl_bassign_stmt*> *as)
{
    type = ContAssign;
    strength = stren;
    delay = del;
    assigns = as;
}


vl_cont_assign::~vl_cont_assign()
{
    delete delay;
    delete_list(assigns);
}


vl_cont_assign *
vl_cont_assign::copy()
{
    return (new vl_cont_assign(strength, delay->copy(), copy_list(assigns)));
}
// End vl_cont_assign functions.


vl_specify_block::vl_specify_block(lsList<vl_specify_item*> *list)
{
    type = SpecBlock;
    items = list;
    vl_warn("specify blocks are ignored");
}


vl_specify_block::~vl_specify_block()
{
    delete_list(items);
}


vl_specify_block *
vl_specify_block::copy()
{
    return (new vl_specify_block(copy_list(items)));
}
// End of vl_specify_block functions.


vl_specify_item::vl_specify_item(short t)
{
    type = t;
    params = 0;
    lhs = 0;
    rhs = 0;
    expr = 0;
    pol = 0;
    list1 = 0;
    list2 = 0;
    edge_id = 0;
    ifex = 0;
}


vl_specify_item::vl_specify_item(short t, lsList<vl_bassign_stmt*> *p)
{
    type = t;
    params = p;
    lhs = 0;
    rhs = 0;
    expr = 0;
    pol = 0;
    list1 = 0;
    list2 = 0;
    edge_id = 0;
    ifex = 0;
}


vl_specify_item::vl_specify_item(short t, vl_path_desc *l,
    lsList<vl_expr*> *r)
{
    type = t;
    params = 0;
    lhs = l;
    rhs = r;
    expr = 0;
    pol = 0;
    list1 = 0;
    list2 = 0;
    edge_id = 0;
    ifex = 0;
}


vl_specify_item::vl_specify_item(short t, vl_expr *e,
    lsList<vl_spec_term_desc*> *l1, int p, lsList<vl_spec_term_desc*> *l2,
    lsList<vl_expr*> *r)
{
    type = t;  // SpecLSPathDecl
    params = 0;
    lhs = 0;
    rhs = r;
    expr = e;
    pol = p;
    list1 = l1;
    list2 = l2;
    edge_id = 0;
    ifex = 0;
}


vl_specify_item::vl_specify_item(short t, vl_expr *ifx, int es,
    lsList<vl_spec_term_desc*> *l1, lsList<vl_spec_term_desc*> *l2, int p,
    vl_expr *e, lsList<vl_expr*> *r)
{
    type = t;  // SpecESPathDecl
    params = 0;
    lhs = 0;
    rhs = r;
    expr = e;
    pol = p;
    list1 = l1;
    list2 = l2;
    edge_id = es;
    ifex = ifx;
}


vl_specify_item::~vl_specify_item()
{
    delete_list(params);
    delete lhs;
    delete_list(rhs);
    delete expr;
    delete_list(list1);
    delete_list(list2);
    delete ifex;
}


vl_specify_item *
vl_specify_item::copy()
{
    const vl_specify_item *sitm = this;
    if (!sitm)
        return (0);
    vl_specify_item *retval = new vl_specify_item(type);
    retval->params = copy_list(params);
    retval->lhs = lhs->copy();
    retval->rhs = copy_list(rhs);
    retval->expr = expr ? expr->copy() : 0;
    retval->pol = pol;
    retval->list1 = copy_list(list1);
    retval->list2 = copy_list(list2);
    retval->edge_id = edge_id;
    retval->ifex = ifex ? ifex->copy() : 0;
    return (retval);
}
// End or vl_specify_item functions.


vl_spec_term_desc::vl_spec_term_desc(char *n, vl_expr *x1, vl_expr *x2)
{
    name = n;
    exp1 = x1;
    exp2 = x2;
    pol = 0;
}


vl_spec_term_desc::vl_spec_term_desc(int p, vl_expr *x1)
{
    name = 0;
    exp1 = x1;
    exp2 = 0;
    pol = p;
}


vl_spec_term_desc::~vl_spec_term_desc()
{
    delete [] name;
    delete exp1;
    delete exp2;
}


vl_spec_term_desc *
vl_spec_term_desc::copy()
{
    vl_spec_term_desc *stdsc = this;
    if (!stdsc)
        return (0);
    vl_spec_term_desc *retval = new vl_spec_term_desc(vl_strdup(name),
        exp1 ? exp1->copy() : 0, exp2 ? exp2->copy() : 0);
    retval->pol = pol;
    return (retval);
}
// End of vl_spec_term_desc functions.


vl_path_desc::vl_path_desc(vl_spec_term_desc *t1, vl_spec_term_desc *t2)
{
    type = PathLeadTo;
    list1 = new lsList<vl_spec_term_desc*>;
    list1->newEnd(t1);
    list2 = new lsList<vl_spec_term_desc*>;
    list2->newEnd(t2);
}


vl_path_desc::vl_path_desc(lsList<vl_spec_term_desc*> *l1,
    lsList<vl_spec_term_desc*> *l2)
{
    type = PathAll;
    list1 = l1;
    list2 = l2;
}


vl_path_desc::~vl_path_desc()
{
    delete_list(list1);
    delete_list(list2);
}


vl_path_desc *
vl_path_desc::copy()
{
    const vl_path_desc *pdsc = this;
    if (!pdsc)
        return (0);
    vl_path_desc *retval = new vl_path_desc(copy_list(list1),
        copy_list(list2));
    retval->type = type;
    return (retval);
}
// End of vl_path_desc functions.


vl_task::vl_task(char *n, lsList<vl_decl*> *d, lsList<vl_stmt*> *s)
{
    type = TaskDecl;
    name = n;
    decls = d;
    stmts = s;
    sig_st = 0;
    blk_st = 0;
}


vl_task::~vl_task()
{
    delete [] name;
    delete_list(decls);
    delete_list(stmts);
    delete_table(sig_st);
    delete blk_st;
}


vl_task *
vl_task::copy()
{
    vl_task *task = new vl_task(vl_strdup(name), 0, 0);
    set_var_context(vl_context::push(var_context(), task));
    task->decls = copy_list(decls);
    task->stmts = copy_list(stmts);
    set_var_context(vl_context::pop(var_context()));

    vl_module *current_mod = var_context()->currentModule();
    if (current_mod) {
        if (!current_mod->task_st)
            current_mod->task_st = new table<vl_task*>;
        current_mod->task_st->insert(task->name, task);
    }
    return (task);
}


void
vl_task::init()
{
    set_var_context(vl_context::push(var_context(), this));
    init_list(decls);
    init_list(stmts);
    set_var_context(vl_context::pop(var_context()));
}
// End vl_task functions.


vl_function::vl_function(short t, vl_range *r, char *n, lsList<vl_decl*> *d,
    lsList<vl_stmt*> *s)
{
    type = t;
    name = n;
    range = r;
    decls = d;
    stmts = s;
    sig_st = 0;
    blk_st = 0;
}


vl_function::~vl_function()
{
    delete [] name;
    delete range;
    delete_list(decls);
    delete_list(stmts);
    delete_table(sig_st);
    delete_table(blk_st);
}


vl_function *
vl_function::copy()
{
    vl_function *fcn = new vl_function(type, range->copy(), vl_strdup(name),
        0, 0);
    set_var_context(vl_context::push(var_context(), fcn));
    fcn->decls = copy_list(decls);
    fcn->stmts = copy_list(stmts);
    set_var_context(vl_context::pop(var_context()));

    vl_module *current_mod = var_context()->currentModule();
    if (current_mod) {
        if (!current_mod->func_st)
            current_mod->func_st = new table<vl_function*>;
        current_mod->func_st->insert(fcn->name, fcn);
    }
    return (fcn);
}


void
vl_function::init()
{
    set_var_context(vl_context::push(var_context(), this));
    init_list(decls);
    init_list(stmts);
    set_var_context(vl_context::pop(var_context()));
}
// End vl_function functions.


vl_gate_inst_list::vl_gate_inst_list(short t, vl_dlstr *dlstr,
    lsList<vl_gate_inst*> *g)
{
    type = t;
    delays = 0;
    if (dlstr) {
        strength = dlstr->strength;
        delays = dlstr->delay;
    }
    gates = g;
}


vl_gate_inst_list::~vl_gate_inst_list()
{
    delete delays;
    delete_list(gates);
}


vl_gate_inst_list *
vl_gate_inst_list::copy()
{
    lsList<vl_gate_inst*> *newgates = gates ? new lsList<vl_gate_inst*> : 0;
    vl_dlstr dlstr;
    dlstr.strength = strength;
    dlstr.delay = delays->copy();
    vl_gate_inst_list *retval = new vl_gate_inst_list(type, &dlstr, newgates);
    if (gates) {
        lsGen<vl_gate_inst*> gen(gates);
        vl_gate_inst *inst;
        while (gen.next(&inst)) {
            vl_gate_inst *newinst = inst->copy();
            newinst->inst_list = retval;
            newgates->newEnd(newinst);
            if (newinst->name) {
                vl_module *current_mod = var_context()->currentModule();
                if (current_mod) {
                    if (!current_mod->inst_st)
                        current_mod->inst_st = new table<vl_inst*>;
                    current_mod->inst_st->insert(newinst->name, newinst);
                }
            }
        }
    }
    return (retval);
}
// End vl_gate_inst_list functions.


vl_mp_inst_list::vl_mp_inst_list(MPtype mpt, char *n, vl_dlstr *dlstr,
    lsList<vl_mp_inst*> *m)
{
    type = ModPrimInst;
    mptype = mpt;
    name = n;
    params_or_delays = 0;
    if (dlstr) {
        strength = dlstr->strength;
        params_or_delays = dlstr->delay;
    }
    mps = m;
}


vl_mp_inst_list::~vl_mp_inst_list()
{
    delete [] name;
    delete_list(mps);
    delete params_or_delays;
}


vl_mp_inst_list *
vl_mp_inst_list::copy()
{
    lsList<vl_mp_inst*> *newmps = mps ? new lsList<vl_mp_inst*> : 0;
    vl_dlstr dlstr;
    dlstr.strength = strength;
    dlstr.delay = params_or_delays->copy();
    vl_mp_inst_list *retval = new vl_mp_inst_list(mptype, vl_strdup(name),
        &dlstr, newmps);
    if (mps) {
        lsGen<vl_mp_inst*> gen(mps);
        vl_mp_inst *inst;
        while (gen.next(&inst)) {
            vl_mp_inst *newinst = inst->copy();
            newinst->inst_list = retval;
            newmps->newEnd(newinst);
            vl_module *current_mod = var_context()->currentModule();
            if (current_mod && newinst->name) {
                if (!current_mod->inst_st)
                    current_mod->inst_st = new table<vl_inst*>;
                current_mod->inst_st->insert(newinst->name, newinst);
            }
            if (current_mod) {
                vl_mp *mp;
                if (!vl_var::simulator->description->mp_st->
                        lookup(name, &mp) || !mp) {
                    vl_error("instance of %s with no master", name);
                    vl_var::simulator->abort();
                }
                else {
                    mp = mp->copy();
                    mp->instance = newinst;
                    newinst->master = mp;
                    if (mp->type == ModDecl)
                        retval->mptype = MPmod;
                    else
                        retval->mptype = MPprim;
                }
            }
        }
    }
    return (retval);
}


void
vl_mp_inst_list::init()
{
    if (mps) {
        lsGen<vl_mp_inst*> gen(mps);
        vl_mp_inst *inst;
        while (gen.next(&inst)) {
            if (inst->master) {
                set_var_context(vl_context::push(var_context(), inst->master));
                inst->master->init();
                set_var_context(vl_context::pop(var_context()));
            }
        }
    }
}
// End vl_mp_inst_list functions.


//---------------------------------------------------------------------------
//  Statements
//---------------------------------------------------------------------------

vl_bassign_stmt::vl_bassign_stmt(short t, vl_var *v, vl_event_expr *c,
    vl_delay *d, vl_var *r)
{
    type = t;
    lhs = v;
    range = 0;
    rhs = r;
    wait = 0;
    delay = d;
    event = c;
    tmpvar = 0;
}


vl_bassign_stmt::~vl_bassign_stmt()
{
    // wait is a pointer to someone else's delay value, don't free
    if (!(flags & BAS_SAVE_LHS))
        delete lhs;
    if (!(flags & BAS_SAVE_RHS))
        delete rhs;
    delete delay;
    delete range;
    delete event;
    delete tmpvar;
}


vl_bassign_stmt *
vl_bassign_stmt::copy()
{
    vl_bassign_stmt *bs = new vl_bassign_stmt(type, lhs->copy(),
        event->copy(), delay->copy(), rhs->copy());
    return (bs);
}


void
vl_bassign_stmt::init()
{
    // set initial event expression value
    if (event)
        event->init();
}
// End vl_bassign_stmt functions.


vl_sys_task_stmt::vl_sys_task_stmt(char *n, lsList<vl_expr*> *a)
{
    type = SysTaskEnableStmt;
    name = n;
    args = a;
    dtype = DSPall;
    flags = 0;
    if (!strcmp(name, "$time") || !strcmp(name, "$stime"))
        action = &vl_simulator::sys_time;
    else if (!strcmp(name, "$printtimescale"))
        action = &vl_simulator::sys_printtimescale;
    else if (!strcmp(name, "$timeformat"))
        action = &vl_simulator::sys_timeformat;

    else if (!strcmp(name, "$display"))
        action = &vl_simulator::sys_display;
    else if (!strcmp(name, "$displayb")) {
        action = &vl_simulator::sys_display;
        dtype = DSPb;
    }
    else if (!strcmp(name, "$displayh")) {
        action = &vl_simulator::sys_display;
        dtype = DSPh;
    }
    else if (!strcmp(name, "$displayo")) {
        action = &vl_simulator::sys_display;
        dtype = DSPo;
    }
    else if (!strcmp(name, "$write")) {
        action = &vl_simulator::sys_display;
        flags |= SYSno_nl;
    }
    else if (!strcmp(name, "$writeb")) {
        action = &vl_simulator::sys_display;
        flags |= SYSno_nl;
        dtype = DSPb;
    }
    else if (!strcmp(name, "$writeh")) {
        action = &vl_simulator::sys_display;
        flags |= SYSno_nl;
        dtype = DSPh;
    }
    else if (!strcmp(name, "$writeo")) {
        action = &vl_simulator::sys_display;
        flags |= SYSno_nl;
        dtype = DSPo;
    }
    else if (!strcmp(name, "$strobe")) {
        action = &vl_simulator::sys_display;
        flags |= SYSafter;
    }
    else if (!strcmp(name, "$strobeb")) {
        action = &vl_simulator::sys_display;
        flags |= SYSafter;
        dtype = DSPb;
    }
    else if (!strcmp(name, "$strobeh")) {
        action = &vl_simulator::sys_display;
        flags |= SYSafter;
        dtype = DSPh;
    }
    else if (!strcmp(name, "$strobeo")) {
        action = &vl_simulator::sys_display;
        flags |= SYSafter;
        dtype = DSPo;
    }
    else if (!strcmp(name, "$monitor"))
        action = &vl_simulator::sys_monitor;
    else if (!strcmp(name, "$monitorb")) {
        action = &vl_simulator::sys_monitor;
        dtype = DSPb;
    }
    else if (!strcmp(name, "$monitorh")) {
        action = &vl_simulator::sys_monitor;
        dtype = DSPh;
    }
    else if (!strcmp(name, "$monitoro")) {
        action = &vl_simulator::sys_monitor;
        dtype = DSPo;
    }
    else if (!strcmp(name, "$monitoron"))
        action = &vl_simulator::sys_monitor_on;
    else if (!strcmp(name, "$monitoroff"))
        action = &vl_simulator::sys_monitor_off;
    else if (!strcmp(name, "$stop"))
        action = &vl_simulator::sys_stop;
    else if (!strcmp(name, "$finish"))
        action = &vl_simulator::sys_finish;
    else if (!strcmp(name, "$fopen"))
        action = &vl_simulator::sys_fopen;
    else if (!strcmp(name, "$fclose"))
        action = &vl_simulator::sys_fclose;
    else if (!strcmp(name, "$fdisplay"))
        action = &vl_simulator::sys_fdisplay;
    else if (!strcmp(name, "$fdisplayb")) {
        action = &vl_simulator::sys_fdisplay;
        dtype = DSPb;
    }
    else if (!strcmp(name, "$fdisplayh")) {
        action = &vl_simulator::sys_fdisplay;
        dtype = DSPh;
    }
    else if (!strcmp(name, "$fdisplayo")) {
        action = &vl_simulator::sys_fdisplay;
        dtype = DSPo;
    }
    else if (!strcmp(name, "$fwrite")) {
        action = &vl_simulator::sys_fdisplay;
        flags |= SYSno_nl;
    }
    else if (!strcmp(name, "$fwriteb")) {
        action = &vl_simulator::sys_fdisplay;
        flags |= SYSno_nl;
        dtype = DSPb;
    }
    else if (!strcmp(name, "$fwriteh")) {
        action = &vl_simulator::sys_fdisplay;
        flags |= SYSno_nl;
        dtype = DSPh;
    }
    else if (!strcmp(name, "$fwriteo")) {
        action = &vl_simulator::sys_fdisplay;
        flags |= SYSno_nl;
        dtype = DSPo;
    }
    else if (!strcmp(name, "$fstrobe")) {
        action = &vl_simulator::sys_fdisplay;
        flags |= SYSafter;
    }
    else if (!strcmp(name, "$fstrobeb")) {
        action = &vl_simulator::sys_fdisplay;
        flags |= SYSafter;
        dtype = DSPb;
    }
    else if (!strcmp(name, "$fstrobeh")) {
        action = &vl_simulator::sys_fdisplay;
        flags |= SYSafter;
        dtype = DSPh;
    }
    else if (!strcmp(name, "$fstrobeo")) {
        action = &vl_simulator::sys_fdisplay;
        flags |= SYSafter;
        dtype = DSPo;
    }
    else if (!strcmp(name, "$fmonitor"))
        action = &vl_simulator::sys_fmonitor;
    else if (!strcmp(name, "$fmonitorb")) {
        action = &vl_simulator::sys_fmonitor;
        dtype = DSPb;
    }
    else if (!strcmp(name, "$fmonitorh")) {
        action = &vl_simulator::sys_fmonitor;
        dtype = DSPh;
    }
    else if (!strcmp(name, "$fmonitoro")) {
        action = &vl_simulator::sys_fmonitor;
        dtype = DSPo;
    }
    else if (!strcmp(name, "$fmonitor_on"))
        action = &vl_simulator::sys_fmonitor_on;
    else if (!strcmp(name, "$fmonitor_off"))
        action = &vl_simulator::sys_fmonitor_off;
    else if (!strcmp(name, "$random"))
        action = &vl_simulator::sys_random;
    else if (!strcmp(name, "$dumpfile"))
        action = &vl_simulator::sys_dumpfile;
    else if (!strcmp(name, "$dumpvars"))
        action = &vl_simulator::sys_dumpvars;
    else if (!strcmp(name, "$dumpall"))
        action = &vl_simulator::sys_dumpall;
    else if (!strcmp(name, "$dumpon"))
        action = &vl_simulator::sys_dumpon;
    else if (!strcmp(name, "$dumpoff"))
        action = &vl_simulator::sys_dumpoff;
    else if (!strcmp(name, "$readmemb"))
        action = &vl_simulator::sys_readmemb;
    else if (!strcmp(name, "$readmemh"))
        action = &vl_simulator::sys_readmemh;
    else {
        vl_warn("unknown system command %s, ignored", name);
        action = &vl_simulator::sys_noop;
    }
}


vl_sys_task_stmt::~vl_sys_task_stmt()
{
    delete [] name;
    delete_list(args);
}


vl_sys_task_stmt *
vl_sys_task_stmt::copy()
{
    return (new vl_sys_task_stmt(vl_strdup(name), copy_list(args)));
}
// End vl_sys_task_stmt functions.


vl_begin_end_stmt::vl_begin_end_stmt(char *n, lsList<vl_decl*> *d,
    lsList<vl_stmt*> *s)
{
    type = BeginEndStmt;
    name = n;
    decls = d;
    stmts = s;
    sig_st = 0;
    blk_st = 0;
    flags = 0;
}


vl_begin_end_stmt::~vl_begin_end_stmt()
{
    if (flags & SIM_INTERNAL)
        delete stmts;
    else {
        delete [] name;
        delete_list(decls);
        delete_list(stmts);
        delete_table(sig_st);
        delete blk_st;
    }
}


vl_begin_end_stmt *
vl_begin_end_stmt::copy()
{
    vl_begin_end_stmt *stmt = new vl_begin_end_stmt(vl_strdup(name), 0, 0);
    set_var_context(vl_context::push(var_context(), stmt));
    stmt->decls = copy_list(decls);
    stmt->stmts = copy_list(stmts);
    set_var_context(vl_context::pop(var_context()));

    if (stmt->name) {
        vl_context *cx = var_context();
        if (cx->block) {
            if (!cx->block->blk_st)
                cx->block->blk_st = new table<vl_stmt*>;
            cx->block->blk_st->insert(stmt->name, stmt);
        }
        else if (cx->fjblk) {
            if (!cx->fjblk->blk_st)
                cx->fjblk->blk_st = new table<vl_stmt*>;
            cx->fjblk->blk_st->insert(stmt->name, stmt);
        }
        else if (cx->task) {
            if (!cx->task->blk_st)
                cx->task->blk_st = new table<vl_stmt*>;
            cx->task->blk_st->insert(stmt->name, stmt);
        }
        else if (cx->function) {
            if (!cx->function->blk_st)
                cx->function->blk_st = new table<vl_stmt*>;
            cx->function->blk_st->insert(stmt->name, stmt);
        }
        else if (cx->module) {
            if (!cx->module->blk_st)
                cx->module->blk_st = new table<vl_stmt*>;
            cx->module->blk_st->insert(stmt->name, stmt);
        }
        else {
            vl_error("if/else block %s has no parent", stmt->name);
            vl_var::simulator->abort();
        }
    }
    return (stmt);
}


void
vl_begin_end_stmt::init()
{
    set_var_context(vl_context::push(var_context(), this));
    init_list(decls);
    init_list(stmts);
    set_var_context(vl_context::pop(var_context()));
}
// End vl_begin_end_stmt functions.


vl_if_else_stmt::vl_if_else_stmt(vl_expr *c, vl_stmt *if_s, vl_stmt *else_s)
{
    type = IfElseStmt;
    cond = c;
    if_stmt = if_s;
    else_stmt = else_s;
}


vl_if_else_stmt *
vl_if_else_stmt::copy()
{
    return (new vl_if_else_stmt(cond->copy(),
        if_stmt ? if_stmt->copy() : 0,
        else_stmt ? else_stmt->copy() : 0));
}


void
vl_if_else_stmt::init()
{
    if (if_stmt)
        if_stmt->init();
    if (else_stmt)
        else_stmt->init();
}
// End vl_if_else_stmt functions.


vl_case_stmt::vl_case_stmt(short t, vl_expr *c, lsList<vl_case_item*> *case_it)
{
    type = t;
    cond = c;
    case_items = case_it;
}


vl_case_stmt::~vl_case_stmt()
{
    delete cond;
    delete_list(case_items);
}


vl_case_stmt *
vl_case_stmt::copy()
{
    return (new vl_case_stmt(type, cond->copy(), copy_list(case_items)));
}


void
vl_case_stmt::init()
{
    init_list(case_items);
}
// End vl_case_stmt functions.


vl_case_item::vl_case_item(short t, lsList<vl_expr*> *e, vl_stmt *s)
{
    type = t;
    exprs = e;
    stmt = s;
}


vl_case_item::~vl_case_item()
{
    delete_list(exprs);
    delete stmt;
}


vl_case_item *
vl_case_item::copy()
{
    vl_case_item *citm = this;
    if (!citm)
        return (0);
    return (new vl_case_item(type, copy_list(exprs), stmt ? stmt->copy() : 0));
}


void
vl_case_item::init()
{
    if (stmt)
        stmt->init();
}
// End vl_case_item functions.


vl_forever_stmt::vl_forever_stmt(vl_stmt *s)
{
    type = ForeverStmt;
    stmt = s;
}


vl_forever_stmt *
vl_forever_stmt::copy()
{
    return (new vl_forever_stmt(stmt ? stmt->copy() : 0));
}


void
vl_forever_stmt::init()
{
    if (stmt)
        stmt->init();
}
// End vl_forever_stmt functions.


vl_repeat_stmt::vl_repeat_stmt(vl_expr *c, vl_stmt *s)
{
    type = RepeatStmt;
    count = c;
    cur_count = 0;
    stmt = s;
}


vl_repeat_stmt *
vl_repeat_stmt::copy()
{
    return (new vl_repeat_stmt(count->copy(), stmt ? stmt->copy() : 0));
}


void
vl_repeat_stmt::init()
{
    if (stmt)
        stmt->init();
}
// End vl_repeat_stmt functions.


vl_while_stmt::vl_while_stmt(vl_expr *c, vl_stmt *s)
{
    type = WhileStmt;
    cond = c;
    stmt = s;
}


vl_while_stmt *
vl_while_stmt::copy()
{
    return (new vl_while_stmt(cond->copy(), stmt ? stmt->copy() : 0));
}


void
vl_while_stmt::init()
{
    if (stmt)
        stmt->init();
}
// End vl_while_stmt functions.


vl_for_stmt::vl_for_stmt(vl_bassign_stmt *i, vl_expr *c, vl_bassign_stmt *e,
    vl_stmt *s)
{
    type = ForStmt;
    initial = i;
    cond = c;
    end = e;
    stmt = s;
}


vl_for_stmt *
vl_for_stmt::copy()
{
    return (new vl_for_stmt(initial ? initial->copy() : 0, cond->copy(),
        end ? end->copy() : 0, stmt ? stmt->copy() : 0));
}


void
vl_for_stmt::init()
{
    if (stmt)
        stmt->init();
}
// End vl_for_stmt functions.


vl_delay_control_stmt::vl_delay_control_stmt(vl_delay *del, vl_stmt *s)
{
    type = DelayControlStmt;
    delay = del;
    stmt = s;
}


vl_delay_control_stmt *
vl_delay_control_stmt::copy()
{
    return (new vl_delay_control_stmt(delay->copy(), stmt ? stmt->copy() : 0));
}


void
vl_delay_control_stmt::init()
{
    if (stmt)
        stmt->init();
}
// End vl_delay_control_stmt functions.


vl_event_control_stmt::vl_event_control_stmt(vl_event_expr *e, vl_stmt *s)
{
    type = EventControlStmt;
    event = e;
    stmt = s;
}


vl_event_control_stmt *
vl_event_control_stmt::copy()
{
    return (new vl_event_control_stmt(event->copy(), stmt ? stmt->copy() : 0));
}


// Add a dummy event to the source variable(s) to indicate that somebody
// is listening for changes, and evaluate each expression to set the
// initial value
//
void
vl_event_control_stmt::init()
{
    if (stmt)
        stmt->init();
    if (event)
        event->init();
}
// End vl_event_control_stmt functions.


vl_wait_stmt::vl_wait_stmt(vl_expr *c, vl_stmt *s)
{
    type = WaitStmt;
    cond = c;
    stmt = s;                
    event = 0;
}


vl_wait_stmt *
vl_wait_stmt::copy()
{
    return (new vl_wait_stmt(cond->copy(), stmt ? stmt->copy() : 0));
}


void
vl_wait_stmt::init()
{
    if (stmt)
        stmt->init();
}
// End vl_wait_stmt functions.


vl_send_event_stmt::vl_send_event_stmt(char *n)
{
    type = SendEventStmt;
    name = n;
}


vl_send_event_stmt *
vl_send_event_stmt::copy()
{
    return (new vl_send_event_stmt(vl_strdup(name)));
}
// End vl_send_event_stmt functions.


vl_fork_join_stmt::vl_fork_join_stmt(char *n, lsList<vl_decl*> *d,
    lsList<vl_stmt*> *s)
{
    type = ForkJoinStmt;
    name = n;
    decls = d;
    stmts = s;
    sig_st = 0;
    blk_st = 0;
    endcnt = 0;
    flags = 0;
}


vl_fork_join_stmt::~vl_fork_join_stmt()
{
    delete [] name;
    delete_list(decls);
    delete_list(stmts);
    delete_table(sig_st);
    delete blk_st;
}


vl_fork_join_stmt *
vl_fork_join_stmt::copy()
{
    vl_fork_join_stmt *stmt = new vl_fork_join_stmt(vl_strdup(name), 0, 0);
    set_var_context(vl_context::push(var_context(), stmt));
    stmt->decls = copy_list(decls);
    stmt->stmts = copy_list(stmts);
    set_var_context(vl_context::pop(var_context()));

    if (stmt->name) {
        vl_context *cx = var_context();
        if (cx->block) {
            if (!cx->block->blk_st)
                cx->block->blk_st = new table<vl_stmt*>;
            cx->block->blk_st->insert(stmt->name, stmt);
        }
        else if (cx->fjblk) {
            if (!cx->fjblk->blk_st)
                cx->fjblk->blk_st = new table<vl_stmt*>;
            cx->fjblk->blk_st->insert(stmt->name, stmt);
        }
        else if (cx->task) {
            if (!cx->task->blk_st)
                cx->task->blk_st = new table<vl_stmt*>;
            cx->task->blk_st->insert(stmt->name, stmt);
        }
        else if (cx->function) {
            if (!cx->function->blk_st)
                cx->function->blk_st = new table<vl_stmt*>;
            cx->function->blk_st->insert(stmt->name, stmt);
        }
        else if (cx->module) {
            if (!cx->module->blk_st)
                cx->module->blk_st = new table<vl_stmt*>;
            cx->module->blk_st->insert(stmt->name, stmt);
        }
        else {
            vl_error("fork/join block %s has no parent", stmt->name);
            vl_var::simulator->abort();
        }
    }
    return (stmt);
}


void
vl_fork_join_stmt::init()
{
    set_var_context(vl_context::push(var_context(), this));
    init_list(decls);
    init_list(stmts);
    set_var_context(vl_context::pop(var_context()));
}
// End vl_fork_join_stmt functions.


vl_task_enable_stmt::vl_task_enable_stmt(short t, char *n, lsList<vl_expr*> *a)
{
    type = t;
    name = n;
    args = a;
    task = 0;
}


vl_task_enable_stmt::~vl_task_enable_stmt()
{
    delete [] name;
    delete_list(args);
}


vl_task_enable_stmt *
vl_task_enable_stmt::copy()
{
    return (new vl_task_enable_stmt(type, vl_strdup(name), copy_list(args)));
}
// End vl_task_enable_stmt functions.


vl_disable_stmt::vl_disable_stmt(char *n)
{
    type = DisableStmt;
    name = n;
    target = 0;
}


vl_disable_stmt *
vl_disable_stmt::copy()
{
    return (new vl_disable_stmt(vl_strdup(name)));
}
// End vl_disable_stmt functions.


vl_deassign_stmt::vl_deassign_stmt(short t, vl_var *v)
{
    type = t;  // DeassignStmt or ReleaseStmt
    lhs = v;
    flags |= DAS_DEL_VAR;
}


vl_deassign_stmt::~vl_deassign_stmt()
{
    if (flags & DAS_DEL_VAR)
        delete lhs;
}


vl_deassign_stmt *
vl_deassign_stmt::copy()
{
    return (new vl_deassign_stmt(type, lhs->copy()));
}


void
vl_deassign_stmt::init()
{
    if (!lhs->name()) {
        if (lhs->data_type() != Dconcat) {
            vl_error("internal, unnamed variable in deassign");
            lhs->simulator->abort();
        }
        return;
    }
    vl_var *nvar = lhs->simulator->context->lookup_var(lhs->name(), false);
    if (!nvar) {
        vl_error("undeclared variable %s in deassign", lhs->name());
        lhs->simulator->abort();
    }
    if (nvar != lhs) {
        if (strcmp(nvar->name(), lhs->name()))
            // from another module, don't free it!
            flags &= ~DAS_DEL_VAR;
        delete lhs;
        lhs = nvar;
    }
    if (lhs->flags() & VAR_IN_TABLE)
        flags &= ~DAS_DEL_VAR;
}
// End vl_deassign_stmt functions.


//---------------------------------------------------------------------------
//  Instances
//---------------------------------------------------------------------------

vl_gate_inst::vl_gate_inst()
{
    name = 0;
    terms = 0;
    outputs = 0;
    gsetup = 0;
    geval = 0;
    gset = 0;
    string = 0;
    inst_list = 0;
    delay = 0;
    array = 0;
}


vl_gate_inst::vl_gate_inst(char *n, lsList<vl_expr*> *t, vl_range *r)
{
    type = 0;
    name = n;
    terms = t;
    outputs = 0;
    gsetup = 0;
    geval = 0;
    gset = 0;
    string = 0;
    inst_list = 0;
    delay = 0;
    array = r;
}


vl_gate_inst::~vl_gate_inst()
{
    delete [] name;
    delete_list(terms);
    delete_list(outputs);
    if (delay) {
        delay->delay1 = 0;
        delete delay;
    }
    delete array;
}


vl_gate_inst *
vl_gate_inst::copy()
{
    vl_gate_inst *g = new vl_gate_inst(vl_strdup(name), copy_list(terms),
        array->copy());
    g->type = type;
    g->gsetup = gsetup;
    g->geval = geval;
    g->gset = gset;
    g->string = string;
    return (g);
}
// End vl_gate_inst functions.


vl_mp_inst::vl_mp_inst(char *n, lsList<vl_port_connect*> *p)
{
    type = 0;
    name = n;
    ports = p;
    inst_list = 0;
    master = 0;
}


vl_mp_inst::~vl_mp_inst()
{
    delete [] name;
    delete_list(ports);
    delete master;
}


vl_mp_inst *
vl_mp_inst::copy()
{
    return (new vl_mp_inst(vl_strdup(name), copy_list(ports)));
}
// End vl_mp_inst functions.

