
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
        return (VS()->context());
    }

    void set_var_context(vl_context *cx)
    {
        VS()->set_context(cx);
    }
}


vl_module::vl_module()
{
    m_tunit = 1.0;
    m_tprec = 1.0;
    m_mod_items = 0;
    m_inst_st = 0;
    m_func_st = 0;
    m_task_st = 0;
    m_blk_st = 0;
}


vl_module::vl_module(vl_desc *desc, char *mn, lsList<vl_port*> *pts,
    lsList<vl_stmt*> *mitems)
{
    mp_type = ModDecl;
    mp_name = mn;
    mp_ports = pts;
    m_tunit = 1.0;
    m_tprec = 1.0;
    m_mod_items = mitems;
    m_inst_st = 0;
    m_func_st = 0;
    m_task_st = 0;
    m_blk_st = 0;
    if (desc) {
        desc->modules()->newEnd(this);
        desc->mp_st()->insert(mn, this);
    }
}


vl_module::~vl_module()
{
    delete_list(m_mod_items);
    delete m_inst_st;
    delete m_func_st;
    delete m_task_st;
    delete m_blk_st;
}


vl_module *
vl_module::copy()
{
    sort_moditems();

    vl_module *retval = new vl_module;
    retval->mp_type = mp_type;
    retval->mp_name = vl_strdup(mp_name);

    vl_context *cx = var_context();
    while (cx) {
        if (cx->module() && !strcmp(cx->module()->name(), name())) {
            vl_error("recursive instantiation of module %s", name());
            VS()->abort();
            return (retval);
        }
        cx = cx->parent();
    }

    set_var_context(vl_context::push(var_context(), retval));
    retval->mp_ports = copy_list(mp_ports);
    retval->m_mod_items = copy_list(m_mod_items);
    set_var_context(vl_context::pop(var_context()));
    retval->m_tunit = m_tunit;
    retval->m_tprec = m_tprec;

    return (retval);
}


void
vl_module::init()
{
    set_var_context(vl_context::push(var_context(), this));
    init_list(m_mod_items);
    set_var_context(vl_context::pop(var_context()));
}
// End vl_module functions.


vl_primitive::vl_primitive()
{
    p_decls = 0;
    p_initial = 0;
    p_ptable = 0;
    p_rows = 0;
    p_seq_init = false;
    memset(p_iodata, 0, MAXPRIMLEN*sizeof(vl_var*));
    memset(p_lastvals, BitDC, MAXPRIMLEN);
}


vl_primitive::vl_primitive(vl_desc *desc, char *prim_name)
{
    mp_type = CombPrimDecl;
    mp_name = prim_name;
    p_decls = 0;
    p_initial = 0;
    p_ptable = 0;
    p_rows = 0;
    p_seq_init = false;
    memset(p_iodata, 0, MAXPRIMLEN*sizeof(vl_var*));
    memset(p_lastvals, BitDC, MAXPRIMLEN);
    if (desc) {
        desc->primitives()->newEnd(this);
        desc->mp_st()->insert(prim_name, this);
    }
}


vl_primitive::~vl_primitive()
{
    delete_list(p_decls);
    delete p_initial;
    delete [] p_ptable;
}


void
vl_primitive::init_table(lsList<vl_port*> *prim_ports,
    lsList<vl_decl*> *prim_decls, vl_bassign_stmt *initial_stmt,
    lsList<vl_prim_entry*> *prim_entries)
{
    mp_ports = prim_ports;
    p_decls = prim_decls;
    p_initial = initial_stmt;
    if (prim_entries) {
        p_rows = prim_entries->length();
        if (p_rows) {
            p_ptable = new unsigned char[p_rows*MAXPRIMLEN];
            unsigned char *row = p_ptable;
            lsGen<vl_prim_entry*> gen(prim_entries);
            int os = (mp_type == SeqPrimDecl ? 2 : 1);
            vl_prim_entry *e;
            while (gen.next(&e)) {
                unsigned char *col = row;
                *col++ = e->next_state;
                if (mp_type == SeqPrimDecl)
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
    retval->mp_type = mp_type;
    retval->mp_name = vl_strdup(mp_name);

    set_var_context(vl_context::push(var_context(), retval));
    retval->mp_ports = copy_list(mp_ports);
    retval->p_decls = copy_list(p_decls);
    retval->p_initial = chk_copy(p_initial);
    retval->p_rows = p_rows;
    if (p_ptable) {
        retval->p_ptable = new unsigned char[p_rows*MAXPRIMLEN];
        memcpy(retval->p_ptable, p_ptable, p_rows*MAXPRIMLEN);
    }
    set_var_context(vl_context::pop(var_context()));
    return (retval);
}


void
vl_primitive::init()
{
    set_var_context(vl_context::push(var_context(), this));
    init_list(p_decls);
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


vl_port::vl_port(int t, char *n, lsList<vl_var*> *exprs)
{
    p_type = t;
    p_name = n;
    p_port_exp = exprs;
}


vl_port::~vl_port()
{
    delete [] p_name;
    lsGen<vl_var*> gen(p_port_exp);
    vl_var *v;
    while (gen.next(&v)) {
        if (!(v->flags() & VAR_IN_TABLE))
            delete v;
    }
    delete(p_port_exp);
}


vl_port *
vl_port::copy()
{
    return (new vl_port(p_type, vl_strdup(p_name), copy_list(p_port_exp)));
}
// End vl_port functions.


vl_port_connect::vl_port_connect(int t, char *n, vl_expr *e)
{
    pc_type = t;
    pc_name = n;
    pc_expr = e;
    pc_i_assign = 0;
    pc_o_assign = 0;
}


vl_port_connect::~vl_port_connect()
{
    delete [] pc_name;
    delete pc_expr;
    delete pc_i_assign;
    delete pc_o_assign;
}


vl_port_connect *
vl_port_connect::copy()
{
    return (new vl_port_connect(pc_type, vl_strdup(pc_name),
        chk_copy(pc_expr)));
}
// End vl_port_connect functions.


//---------------------------------------------------------------------------
//  Module items
//---------------------------------------------------------------------------

vl_decl::vl_decl(int t, vl_strength stren, vl_range *r, vl_delay *del,
    lsList<vl_bassign_stmt*> *l, lsList<vl_var*> *i)
{
    st_type = t;
    d_range = r;
    d_ids = i;
    d_list = l;
    d_delay = del;
    d_strength = stren;
}


vl_decl::vl_decl(int t, vl_range *r, lsList<vl_var*> *i)
{
    st_type = t;
    d_range = r;
    d_ids = i;
    d_list = 0;
    d_delay = 0;
}


vl_decl::vl_decl(int t, lsList<char*> *l)
{
    st_type = t;
    d_range = 0;
    d_ids = new lsList<vl_var*>;
    d_list = 0;
    d_delay = 0;

    lsGen<char*> gen(l);
    char *ev;
    while (gen.next(&ev)) {
        vl_var *v = new vl_var(ev, 0);
        d_ids->newEnd(v);
    }
    delete l;
}


vl_decl::vl_decl(int t, vl_range *r, lsList<vl_bassign_stmt*> *assigns)
{
    st_type = t;
    d_range = r;
    d_ids = 0;
    d_list = assigns;
    d_delay = 0;
}


vl_decl::~vl_decl()
{
    // the variables are deleted with the sig_st
    delete d_range;
    lsGen<vl_var*> gen(d_ids);
    vl_var *v;
    while (gen.next(&v)) {
        if (!(v->flags() & VAR_IN_TABLE))
            delete v;
    }
    delete(d_ids);
    delete_list(d_list);
    delete d_delay;
}


vl_decl *
vl_decl::copy()
{
    return (new vl_decl(st_type, d_strength, chk_copy(d_range),
        chk_copy(d_delay), copy_list(d_list), copy_list(d_ids)));
}


// Put the variables in the symbol table, initialize types, evaluate
// parameters.
//
void
vl_decl::init()
{
    if (d_ids) {
        // declared without assignment
        lsGen<vl_var*> gen(d_ids);
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
                d_ids->replace(v, nvar);
                delete v;
                v = nvar;
            }
            var_setup(v, st_type);
            if (v->net_type() >= REGwire) {
                if (!v->delay())
                    v->set_delay(chk_copy(d_delay));
            }
        }
    }
    if (d_list && st_type != DefparamDecl) {
        // declared with assignment
        lsGen<vl_bassign_stmt*> gen(d_list);
        vl_bassign_stmt *bs;
        while (gen.next(&bs)) {
            table<vl_var*> *st = symtab(bs->lhs());
            if (!st)
                break;
            vl_var *nvar;
            if (!st->lookup(bs->lhs()->name(), &nvar)) {
                st->insert(bs->lhs()->name(), bs->lhs());
                bs->lhs()->or_flags(VAR_IN_TABLE);
            }
            else if (bs->lhs() != nvar) {
                if (bs->lhs()->delay() && !nvar->delay()) {
                    nvar->set_delay(bs->lhs()->delay());
                    bs->lhs()->set_delay(0);
                }
                delete bs->lhs();
                bs->set_lhs(nvar);
            }
            bs->or_flags(BAS_SAVE_LHS);
            var_setup(bs->lhs(), st_type);
            if (bs->lhs()->net_type() >= REGwire) {
                if (!bs->lhs()->delay())
                    bs->lhs()->set_delay(chk_copy(d_delay));
            }
            if (st_type == ParamDecl || st_type == RegDecl ||
                    st_type == IntDecl || st_type == TimeDecl ||
                    st_type == RealDecl)
                *bs->lhs() = bs->rhs()->eval();
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
        VS()->abort();
        return (0);
    }
    vl_context *cx = VS()->context();
    table<vl_var*> *st = 0;
    if (cx->block()) {
        if (!cx->block()->sig_st())
            cx->block()->set_sig_st(new table<vl_var*>);
        st = cx->block()->sig_st();
    }
    else if (cx->fjblk()) {
        if (!cx->fjblk()->sig_st())
            cx->fjblk()->set_sig_st(new table<vl_var*>);
        st = cx->fjblk()->sig_st();
    }
    else if (cx->function()) {
        if (!cx->function()->sig_st())
            cx->function()->set_sig_st(new table<vl_var*>);
        st = cx->function()->sig_st();
    }
    else if (cx->task()) {
        if (!cx->task()->sig_st())
            cx->task()->set_sig_st(new table<vl_var*>);
        st = cx->task()->sig_st();
    }
    else if (cx->primitive()) {
        if (!cx->primitive()->sig_st())
            cx->primitive()->set_sig_st(new table<vl_var*>);
        st = cx->primitive()->sig_st();
    }
    else if (cx->module()) {
        if (!cx->module()->sig_st())
            cx->module()->set_sig_st(new table<vl_var*>);
        st = cx->module()->sig_st();
    }
    if (!st) {
        vl_error("no symbol table for %s declaration", var->name());
        VS()->abort();
    }
    return (st);
}


// Take care of initialization of the declared variables.
//
void
vl_decl::var_setup(vl_var *var, int vtype)
{
    if (vtype == ParamDecl) {
        var->configure(d_range, vtype);
        var->set_net_type(REGparam);
        return;
    }
    var->configure(d_range, vtype, var->range());

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
            var->set_strength(d_strength);
            return;
        }
        break;
    case TriDecl:
        if (var->net_type() == REGnone ||
                (var->net_type() == REGwire && var->io_type() != IOnone)) {
            var->set_net_type(REGtri);
            var->setbits(BitZ);
            var->set_strength(d_strength);
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
            var->set_strength(d_strength);
            return;
        }
        break;
    case TriandDecl:
        if (var->net_type() == REGnone ||
                (var->net_type() == REGwire && var->io_type() != IOnone)) {
            var->set_net_type(REGtriand);
            var->setbits(BitZ);
            var->set_strength(d_strength);
            return;
        }
        break;
    case WorDecl:
        if (var->net_type() == REGnone ||
                (var->net_type() == REGwire && var->io_type() != IOnone)) {
            var->set_net_type(REGtriand);
            var->setbits(BitZ);
            var->set_strength(d_strength);
            return;
        }
        break;
    case TriorDecl:
        if (var->net_type() == REGnone ||
                (var->net_type() == REGwire && var->io_type() != IOnone)) {
            var->set_net_type(REGwand);
            var->setbits(BitZ);
            var->set_strength(d_strength);
            return;
        }
        break;
    case TriregDecl:
        if (var->net_type() == REGnone ||
                (var->net_type() == REGwire && var->io_type() != IOnone)) {
            var->set_net_type(REGtrireg);
            var->setbits(BitDC);
            var->set_strength(d_strength);
            return;
        }
        break;
    default:
        var->set_net_type(REGnone);
        return;
    }
    vl_error("symbol %s redeclared as %s", var->name(), decl_type());
    errout(this);
    VS()->abort();
}
// End vl_decl functions.


vl_procstmt::vl_procstmt(int t, vl_stmt *s)
{
    st_type = t;
    pr_stmt = s;
    pr_lasttime = (vl_time_t)-1;
}


vl_procstmt::~vl_procstmt()
{
    delete pr_stmt;
}


vl_procstmt *
vl_procstmt::copy()
{
    return (new vl_procstmt(st_type, chk_copy(pr_stmt)));
}


void
vl_procstmt::init()
{
    if (pr_stmt)
        pr_stmt->init();
}
// End vl_procstmt functions.



vl_cont_assign::vl_cont_assign(vl_strength stren, vl_delay *del,
    lsList<vl_bassign_stmt*> *as)
{
    st_type = ContAssign;
    c_strength = stren;
    c_delay = del;
    c_assigns = as;
}


vl_cont_assign::~vl_cont_assign()
{
    delete c_delay;
    delete_list(c_assigns);
}


vl_cont_assign *
vl_cont_assign::copy()
{
    return (new vl_cont_assign(c_strength, chk_copy(c_delay),
        copy_list(c_assigns)));
}
// End vl_cont_assign functions.


vl_specify_block::vl_specify_block(lsList<vl_specify_item*> *list)
{
    st_type = SpecBlock;
    sb_items = list;
    vl_warn("specify blocks are ignored");
}


vl_specify_block::~vl_specify_block()
{
    delete_list(sb_items);
}


vl_specify_block *
vl_specify_block::copy()
{
    return (new vl_specify_block(copy_list(sb_items)));
}
// End of vl_specify_block functions.


vl_specify_item::vl_specify_item(int t)
{
    si_type = t;
    si_params = 0;
    si_lhs = 0;
    si_rhs = 0;
    si_expr = 0;
    si_pol = 0;
    si_edge_id = 0;
    si_list1 = 0;
    si_list2 = 0;
    si_ifex = 0;
}


vl_specify_item::vl_specify_item(int t, lsList<vl_bassign_stmt*> *p)
{
    si_type = t;
    si_params = p;
    si_lhs = 0;
    si_rhs = 0;
    si_expr = 0;
    si_pol = 0;
    si_edge_id = 0;
    si_list1 = 0;
    si_list2 = 0;
    si_ifex = 0;
}


vl_specify_item::vl_specify_item(int t, vl_path_desc *l,
    lsList<vl_expr*> *r)
{
    si_type = t;
    si_params = 0;
    si_lhs = l;
    si_rhs = r;
    si_expr = 0;
    si_pol = 0;
    si_edge_id = 0;
    si_list1 = 0;
    si_list2 = 0;
    si_ifex = 0;
}


vl_specify_item::vl_specify_item(int t, vl_expr *e,
    lsList<vl_spec_term_desc*> *l1, int p, lsList<vl_spec_term_desc*> *l2,
    lsList<vl_expr*> *r)
{
    si_type = t;  // SpecLSPathDecl
    si_params = 0;
    si_lhs = 0;
    si_rhs = r;
    si_expr = e;
    si_pol = p;
    si_edge_id = 0;
    si_list1 = l1;
    si_list2 = l2;
    si_ifex = 0;
}


vl_specify_item::vl_specify_item(int t, vl_expr *ifx, int es,
    lsList<vl_spec_term_desc*> *l1, lsList<vl_spec_term_desc*> *l2, int p,
    vl_expr *e, lsList<vl_expr*> *r)
{
    si_type = t;  // SpecESPathDecl
    si_params = 0;
    si_lhs = 0;
    si_rhs = r;
    si_expr = e;
    si_pol = p;
    si_edge_id = es;
    si_list1 = l1;
    si_list2 = l2;
    si_ifex = ifx;
}


vl_specify_item::~vl_specify_item()
{
    delete_list(si_params);
    delete si_lhs;
    delete_list(si_rhs);
    delete si_expr;
    delete_list(si_list1);
    delete_list(si_list2);
    delete si_ifex;
}


vl_specify_item *
vl_specify_item::copy()
{
    vl_specify_item *retval = new vl_specify_item(si_type);
    retval->si_params = copy_list(si_params);
    retval->si_lhs = chk_copy(si_lhs);
    retval->si_rhs = copy_list(si_rhs);
    retval->si_expr = chk_copy(si_expr);
    retval->si_pol = si_pol;
    retval->si_list1 = copy_list(si_list1);
    retval->si_list2 = copy_list(si_list2);
    retval->si_edge_id = si_edge_id;
    retval->si_ifex = chk_copy(si_ifex);
    return (retval);
}
// End or vl_specify_item functions.


vl_spec_term_desc::vl_spec_term_desc(char *n, vl_expr *x1, vl_expr *x2)
{
    st_name = n;
    st_exp1 = x1;
    st_exp2 = x2;
    st_pol = 0;
}


vl_spec_term_desc::vl_spec_term_desc(int p, vl_expr *x1)
{
    st_name = 0;
    st_exp1 = x1;
    st_exp2 = 0;
    st_pol = p;
}


vl_spec_term_desc::~vl_spec_term_desc()
{
    delete [] st_name;
    delete st_exp1;
    delete st_exp2;
}


vl_spec_term_desc *
vl_spec_term_desc::copy()
{
    vl_spec_term_desc *retval = new vl_spec_term_desc(vl_strdup(st_name),
        chk_copy(st_exp1), chk_copy(st_exp2));
    retval->st_pol = st_pol;
    return (retval);
}
// End of vl_spec_term_desc functions.


vl_path_desc::vl_path_desc(vl_spec_term_desc *t1, vl_spec_term_desc *t2)
{
    pa_type = PathLeadTo;
    pa_list1 = new lsList<vl_spec_term_desc*>;
    pa_list1->newEnd(t1);
    pa_list2 = new lsList<vl_spec_term_desc*>;
    pa_list2->newEnd(t2);
}


vl_path_desc::vl_path_desc(lsList<vl_spec_term_desc*> *l1,
    lsList<vl_spec_term_desc*> *l2)
{
    pa_type = PathAll;
    pa_list1 = l1;
    pa_list2 = l2;
}


vl_path_desc::~vl_path_desc()
{
    delete_list(pa_list1);
    delete_list(pa_list2);
}


vl_path_desc *
vl_path_desc::copy()
{
    vl_path_desc *retval =
        new vl_path_desc(copy_list(pa_list1), copy_list(pa_list2));
    retval->pa_type = pa_type;
    return (retval);
}
// End of vl_path_desc functions.


vl_task::vl_task(char *n, lsList<vl_decl*> *d, lsList<vl_stmt*> *s)
{
    st_type = TaskDecl;
    t_name = n;
    t_decls = d;
    t_stmts = s;
    t_sig_st = 0;
    t_blk_st = 0;
}


vl_task::~vl_task()
{
    delete [] t_name;
    delete_list(t_decls);
    delete_list(t_stmts);
    delete_table(t_sig_st);
    delete t_blk_st;
}


vl_task *
vl_task::copy()
{
    vl_task *task = new vl_task(vl_strdup(t_name), 0, 0);
    set_var_context(vl_context::push(var_context(), task));
    task->t_decls = copy_list(t_decls);
    task->t_stmts = copy_list(t_stmts);
    set_var_context(vl_context::pop(var_context()));

    vl_module *current_mod = var_context()->currentModule();
    if (current_mod) {
        if (!current_mod->task_st())
            current_mod->set_task_st(new table<vl_task*>);
        current_mod->task_st()->insert(task->t_name, task);
    }
    return (task);
}


void
vl_task::init()
{
    set_var_context(vl_context::push(var_context(), this));
    init_list(t_decls);
    init_list(t_stmts);
    set_var_context(vl_context::pop(var_context()));
}
// End vl_task functions.


vl_function::vl_function(int t, vl_range *r, char *n, lsList<vl_decl*> *d,
    lsList<vl_stmt*> *s)
{
    st_type = t;
    f_name = n;
    f_range = r;
    f_decls = d;
    f_stmts = s;
    f_sig_st = 0;
    f_blk_st = 0;
}


vl_function::~vl_function()
{
    delete [] f_name;
    delete f_range;
    delete_list(f_decls);
    delete_list(f_stmts);
    delete_table(f_sig_st);
    delete_table(f_blk_st);
}


vl_function *
vl_function::copy()
{
    vl_function *fcn =
        new vl_function(st_type, chk_copy(f_range), vl_strdup(f_name), 0, 0);
    set_var_context(vl_context::push(var_context(), fcn));
    fcn->f_decls = copy_list(f_decls);
    fcn->f_stmts = copy_list(f_stmts);
    set_var_context(vl_context::pop(var_context()));

    vl_module *current_mod = var_context()->currentModule();
    if (current_mod) {
        if (!current_mod->func_st())
            current_mod->set_func_st(new table<vl_function*>);
        current_mod->func_st()->insert(fcn->f_name, fcn);
    }
    return (fcn);
}


void
vl_function::init()
{
    set_var_context(vl_context::push(var_context(), this));
    init_list(f_decls);
    init_list(f_stmts);
    set_var_context(vl_context::pop(var_context()));
}
// End vl_function functions.


vl_gate_inst_list::vl_gate_inst_list(int t, vl_dlstr *dlstr,
    lsList<vl_gate_inst*> *g)
{
    st_type = t;
    g_delays = 0;
    if (dlstr) {
        g_strength = dlstr->strength();
        g_delays = dlstr->delay();
    }
    g_gates = g;
}


vl_gate_inst_list::~vl_gate_inst_list()
{
    delete g_delays;
    delete_list(g_gates);
}


vl_gate_inst_list *
vl_gate_inst_list::copy()
{
    lsList<vl_gate_inst*> *newgates = g_gates ? new lsList<vl_gate_inst*> : 0;
    vl_dlstr dlstr;
    dlstr.set_strength(g_strength);
    dlstr.set_delay(chk_copy(g_delays));
    vl_gate_inst_list *retval =
        new vl_gate_inst_list(st_type, &dlstr, newgates);
    if (g_gates) {
        lsGen<vl_gate_inst*> gen(g_gates);
        vl_gate_inst *inst;
        while (gen.next(&inst)) {
            vl_gate_inst *newinst = chk_copy(inst);
            newinst->set_inst_list(retval);
            newgates->newEnd(newinst);
            if (newinst->name()) {
                vl_module *current_mod = var_context()->currentModule();
                if (current_mod) {
                    if (!current_mod->inst_st())
                        current_mod->set_inst_st(new table<vl_inst*>);
                    current_mod->inst_st()->insert(newinst->name(), newinst);
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
    st_type = ModPrimInst;
    mp_mptype = mpt;
    mp_name = n;
    mp_prms_or_dlys = 0;
    if (dlstr) {
        mp_strength = dlstr->strength();
        mp_prms_or_dlys = dlstr->delay();
    }
    mp_mps = m;
}


vl_mp_inst_list::~vl_mp_inst_list()
{
    delete [] mp_name;
    delete_list(mp_mps);
    delete mp_prms_or_dlys;
}


vl_mp_inst_list *
vl_mp_inst_list::copy()
{
    lsList<vl_mp_inst*> *newmps = mp_mps ? new lsList<vl_mp_inst*> : 0;
    vl_dlstr dlstr;
    dlstr.set_strength(mp_strength);
    dlstr.set_delay(chk_copy(mp_prms_or_dlys));
    vl_mp_inst_list *retval = new vl_mp_inst_list(mp_mptype, vl_strdup(mp_name),
        &dlstr, newmps);
    if (mp_mps) {
        lsGen<vl_mp_inst*> gen(mp_mps);
        vl_mp_inst *inst;
        while (gen.next(&inst)) {
            vl_mp_inst *newinst = chk_copy(inst);
            newinst->set_inst_list(retval);
            newmps->newEnd(newinst);
            vl_module *current_mod = var_context()->currentModule();
            if (current_mod && newinst->name()) {
                if (!current_mod->inst_st())
                    current_mod->set_inst_st(new table<vl_inst*>);
                current_mod->inst_st()->insert(newinst->name(), newinst);
            }
            if (current_mod) {
                vl_mp *mp;
                if (!VS()->description()->mp_st()->lookup(mp_name, &mp) ||
                        !mp) {
                    vl_error("instance of %s with no master", mp_name);
                    VS()->abort();
                }
                else {
                    mp = mp->copy();
                    mp->set_instance(newinst);
                    newinst->set_master(mp);
                    if (mp->type() == ModDecl)
                        retval->set_mptype(MPmod);
                    else
                        retval->set_mptype(MPprim);
                }
            }
        }
    }
    return (retval);
}


void
vl_mp_inst_list::init()
{
    if (mp_mps) {
        lsGen<vl_mp_inst*> gen(mp_mps);
        vl_mp_inst *inst;
        while (gen.next(&inst)) {
            if (inst->master()) {
                set_var_context(vl_context::push(var_context(),
                    inst->master()));
                inst->master()->init();
                set_var_context(vl_context::pop(var_context()));
            }
        }
    }
}
// End vl_mp_inst_list functions.


//---------------------------------------------------------------------------
//  Statements
//---------------------------------------------------------------------------

vl_bassign_stmt::vl_bassign_stmt(int t, vl_var *v, vl_event_expr *c,
    vl_delay *d, vl_var *r)
{
    st_type = t;
    ba_lhs = v;
    ba_range = 0;
    ba_rhs = r;
    ba_wait = 0;
    ba_delay = d;
    ba_event = c;
    ba_tmpvar = 0;
}


vl_bassign_stmt::~vl_bassign_stmt()
{
    // wait is a pointer to someone else's delay value, don't free
    if (!(st_flags & BAS_SAVE_LHS))
        delete ba_lhs;
    if (!(st_flags & BAS_SAVE_RHS))
        delete ba_rhs;
    delete ba_delay;
    delete ba_range;
    delete ba_event;
    delete ba_tmpvar;
}


vl_bassign_stmt *
vl_bassign_stmt::copy()
{
    vl_bassign_stmt *bs = new vl_bassign_stmt(st_type, chk_copy(ba_lhs),
        chk_copy(ba_event), chk_copy(ba_delay), chk_copy(ba_rhs));
    return (bs);
}


void
vl_bassign_stmt::init()
{
    // set initial event expression value
    if (ba_event)
        ba_event->init();
}
// End vl_bassign_stmt functions.


vl_sys_task_stmt::vl_sys_task_stmt(char *n, lsList<vl_expr*> *a)
{
    st_type = SysTaskEnableStmt;
    sy_name = n;
    sy_args = a;
    sy_dtype = DSPall;
    sy_flags = 0;
    if (!strcmp(sy_name, "$time") || !strcmp(sy_name, "$stime"))
        action = &vl_simulator::sys_time;
    else if (!strcmp(sy_name, "$printtimescale"))
        action = &vl_simulator::sys_printtimescale;
    else if (!strcmp(sy_name, "$timeformat"))
        action = &vl_simulator::sys_timeformat;
    else if (!strcmp(sy_name, "$display"))
        action = &vl_simulator::sys_display;
    else if (!strcmp(sy_name, "$displayb")) {
        action = &vl_simulator::sys_display;
        sy_dtype = DSPb;
    }
    else if (!strcmp(sy_name, "$displayh")) {
        action = &vl_simulator::sys_display;
        sy_dtype = DSPh;
    }
    else if (!strcmp(sy_name, "$displayo")) {
        action = &vl_simulator::sys_display;
        sy_dtype = DSPo;
    }
    else if (!strcmp(sy_name, "$write")) {
        action = &vl_simulator::sys_display;
        sy_flags |= SYSno_nl;
    }
    else if (!strcmp(sy_name, "$writeb")) {
        action = &vl_simulator::sys_display;
        sy_flags |= SYSno_nl;
        sy_dtype = DSPb;
    }
    else if (!strcmp(sy_name, "$writeh")) {
        action = &vl_simulator::sys_display;
        sy_flags |= SYSno_nl;
        sy_dtype = DSPh;
    }
    else if (!strcmp(sy_name, "$writeo")) {
        action = &vl_simulator::sys_display;
        sy_flags |= SYSno_nl;
        sy_dtype = DSPo;
    }
    else if (!strcmp(sy_name, "$strobe")) {
        action = &vl_simulator::sys_display;
        sy_flags |= SYSafter;
    }
    else if (!strcmp(sy_name, "$strobeb")) {
        action = &vl_simulator::sys_display;
        sy_flags |= SYSafter;
        sy_dtype = DSPb;
    }
    else if (!strcmp(sy_name, "$strobeh")) {
        action = &vl_simulator::sys_display;
        sy_flags |= SYSafter;
        sy_dtype = DSPh;
    }
    else if (!strcmp(sy_name, "$strobeo")) {
        action = &vl_simulator::sys_display;
        sy_flags |= SYSafter;
        sy_dtype = DSPo;
    }
    else if (!strcmp(sy_name, "$monitor"))
        action = &vl_simulator::sys_monitor;
    else if (!strcmp(sy_name, "$monitorb")) {
        action = &vl_simulator::sys_monitor;
        sy_dtype = DSPb;
    }
    else if (!strcmp(sy_name, "$monitorh")) {
        action = &vl_simulator::sys_monitor;
        sy_dtype = DSPh;
    }
    else if (!strcmp(sy_name, "$monitoro")) {
        action = &vl_simulator::sys_monitor;
        sy_dtype = DSPo;
    }
    else if (!strcmp(sy_name, "$monitoron"))
        action = &vl_simulator::sys_monitor_on;
    else if (!strcmp(sy_name, "$monitoroff"))
        action = &vl_simulator::sys_monitor_off;
    else if (!strcmp(sy_name, "$stop"))
        action = &vl_simulator::sys_stop;
    else if (!strcmp(sy_name, "$finish"))
        action = &vl_simulator::sys_finish;
    else if (!strcmp(sy_name, "$fopen"))
        action = &vl_simulator::sys_fopen;
    else if (!strcmp(sy_name, "$fclose"))
        action = &vl_simulator::sys_fclose;
    else if (!strcmp(sy_name, "$fdisplay"))
        action = &vl_simulator::sys_fdisplay;
    else if (!strcmp(sy_name, "$fdisplayb")) {
        action = &vl_simulator::sys_fdisplay;
        sy_dtype = DSPb;
    }
    else if (!strcmp(sy_name, "$fdisplayh")) {
        action = &vl_simulator::sys_fdisplay;
        sy_dtype = DSPh;
    }
    else if (!strcmp(sy_name, "$fdisplayo")) {
        action = &vl_simulator::sys_fdisplay;
        sy_dtype = DSPo;
    }
    else if (!strcmp(sy_name, "$fwrite")) {
        action = &vl_simulator::sys_fdisplay;
        sy_flags |= SYSno_nl;
    }
    else if (!strcmp(sy_name, "$fwriteb")) {
        action = &vl_simulator::sys_fdisplay;
        sy_flags |= SYSno_nl;
        sy_dtype = DSPb;
    }
    else if (!strcmp(sy_name, "$fwriteh")) {
        action = &vl_simulator::sys_fdisplay;
        sy_flags |= SYSno_nl;
        sy_dtype = DSPh;
    }
    else if (!strcmp(sy_name, "$fwriteo")) {
        action = &vl_simulator::sys_fdisplay;
        sy_flags |= SYSno_nl;
        sy_dtype = DSPo;
    }
    else if (!strcmp(sy_name, "$fstrobe")) {
        action = &vl_simulator::sys_fdisplay;
        sy_flags |= SYSafter;
    }
    else if (!strcmp(sy_name, "$fstrobeb")) {
        action = &vl_simulator::sys_fdisplay;
        sy_flags |= SYSafter;
        sy_dtype = DSPb;
    }
    else if (!strcmp(sy_name, "$fstrobeh")) {
        action = &vl_simulator::sys_fdisplay;
        sy_flags |= SYSafter;
        sy_dtype = DSPh;
    }
    else if (!strcmp(sy_name, "$fstrobeo")) {
        action = &vl_simulator::sys_fdisplay;
        sy_flags |= SYSafter;
        sy_dtype = DSPo;
    }
    else if (!strcmp(sy_name, "$fmonitor"))
        action = &vl_simulator::sys_fmonitor;
    else if (!strcmp(sy_name, "$fmonitorb")) {
        action = &vl_simulator::sys_fmonitor;
        sy_dtype = DSPb;
    }
    else if (!strcmp(sy_name, "$fmonitorh")) {
        action = &vl_simulator::sys_fmonitor;
        sy_dtype = DSPh;
    }
    else if (!strcmp(sy_name, "$fmonitoro")) {
        action = &vl_simulator::sys_fmonitor;
        sy_dtype = DSPo;
    }
    else if (!strcmp(sy_name, "$fmonitor_on"))
        action = &vl_simulator::sys_fmonitor_on;
    else if (!strcmp(sy_name, "$fmonitor_off"))
        action = &vl_simulator::sys_fmonitor_off;
    else if (!strcmp(sy_name, "$random"))
        action = &vl_simulator::sys_random;
    else if (!strcmp(sy_name, "$dumpfile"))
        action = &vl_simulator::sys_dumpfile;
    else if (!strcmp(sy_name, "$dumpvars"))
        action = &vl_simulator::sys_dumpvars;
    else if (!strcmp(sy_name, "$dumpall"))
        action = &vl_simulator::sys_dumpall;
    else if (!strcmp(sy_name, "$dumpon"))
        action = &vl_simulator::sys_dumpon;
    else if (!strcmp(sy_name, "$dumpoff"))
        action = &vl_simulator::sys_dumpoff;
    else if (!strcmp(sy_name, "$readmemb"))
        action = &vl_simulator::sys_readmemb;
    else if (!strcmp(sy_name, "$readmemh"))
        action = &vl_simulator::sys_readmemh;
    else {
        vl_warn("unknown system command %s, ignored", sy_name);
        action = &vl_simulator::sys_noop;
    }
}


vl_sys_task_stmt::~vl_sys_task_stmt()
{
    delete [] sy_name;
    delete_list(sy_args);
}


vl_sys_task_stmt *
vl_sys_task_stmt::copy()
{
    return (new vl_sys_task_stmt(vl_strdup(sy_name), copy_list(sy_args)));
}
// End vl_sys_task_stmt functions.


vl_begin_end_stmt::vl_begin_end_stmt(char *n, lsList<vl_decl*> *d,
    lsList<vl_stmt*> *s)
{
    st_type = BeginEndStmt;
    be_name = n;
    be_decls = d;
    be_stmts = s;
    be_sig_st = 0;
    be_blk_st = 0;
}


vl_begin_end_stmt::~vl_begin_end_stmt()
{
    if (st_flags & SIM_INTERNAL)
        delete be_stmts;
    else {
        delete [] be_name;
        delete_list(be_decls);
        delete_list(be_stmts);
        delete_table(be_sig_st);
        delete be_blk_st;
    }
}


vl_begin_end_stmt *
vl_begin_end_stmt::copy()
{
    vl_begin_end_stmt *stmt = new vl_begin_end_stmt(vl_strdup(be_name), 0, 0);
    set_var_context(vl_context::push(var_context(), stmt));
    stmt->be_decls = copy_list(be_decls);
    stmt->be_stmts = copy_list(be_stmts);
    set_var_context(vl_context::pop(var_context()));

    if (stmt->be_name) {
        vl_context *cx = var_context();
        if (cx->block()) {
            if (!cx->block()->be_blk_st)
                cx->block()->be_blk_st = new table<vl_stmt*>;
            cx->block()->be_blk_st->insert(stmt->be_name, stmt);
        }
        else if (cx->fjblk()) {
            if (!cx->fjblk()->blk_st())
                cx->fjblk()->set_blk_st(new table<vl_stmt*>);
            cx->fjblk()->blk_st()->insert(stmt->name(), stmt);
        }
        else if (cx->task()) {
            if (!cx->task()->blk_st())
                cx->task()->set_blk_st(new table<vl_stmt*>);
            cx->task()->blk_st()->insert(stmt->name(), stmt);
        }
        else if (cx->function()) {
            if (!cx->function()->blk_st())
                cx->function()->set_blk_st(new table<vl_stmt*>);
            cx->function()->blk_st()->insert(stmt->name(), stmt);
        }
        else if (cx->module()) {
            if (!cx->module()->blk_st())
                cx->module()->set_blk_st(new table<vl_stmt*>);
            cx->module()->blk_st()->insert(stmt->name(), stmt);
        }
        else {
            vl_error("if/else block %s has no parent", stmt->name());
            VS()->abort();
        }
    }
    return (stmt);
}


void
vl_begin_end_stmt::init()
{
    set_var_context(vl_context::push(var_context(), this));
    init_list(be_decls);
    init_list(be_stmts);
    set_var_context(vl_context::pop(var_context()));
}
// End vl_begin_end_stmt functions.


vl_if_else_stmt::vl_if_else_stmt(vl_expr *c, vl_stmt *if_s, vl_stmt *else_s)
{
    st_type = IfElseStmt;
    ie_cond = c;
    ie_if_stmt = if_s;
    ie_else_stmt = else_s;
}


vl_if_else_stmt *
vl_if_else_stmt::copy()
{
    return (new vl_if_else_stmt(chk_copy(ie_cond),
        chk_copy(ie_if_stmt), chk_copy(ie_else_stmt)));
}


void
vl_if_else_stmt::init()
{
    if (ie_if_stmt)
        ie_if_stmt->init();
    if (ie_else_stmt)
        ie_else_stmt->init();
}
// End vl_if_else_stmt functions.


vl_case_stmt::vl_case_stmt(int t, vl_expr *c, lsList<vl_case_item*> *case_it)
{
    st_type = t;
    c_cond = c;
    c_case_items = case_it;
}


vl_case_stmt::~vl_case_stmt()
{
    delete c_cond;
    delete_list(c_case_items);
}


vl_case_stmt *
vl_case_stmt::copy()
{
    return (new vl_case_stmt(st_type, chk_copy(c_cond),
        copy_list(c_case_items)));
}


void
vl_case_stmt::init()
{
    init_list(c_case_items);
}
// End vl_case_stmt functions.


vl_case_item::vl_case_item(int t, lsList<vl_expr*> *e, vl_stmt *s)
{
    st_type = t;
    ci_exprs = e;
    ci_stmt = s;
}


vl_case_item::~vl_case_item()
{
    delete_list(ci_exprs);
    delete ci_stmt;
}


vl_case_item *
vl_case_item::copy()
{
    return (new vl_case_item(st_type, copy_list(ci_exprs), chk_copy(ci_stmt)));
}


void
vl_case_item::init()
{
    if (ci_stmt)
        ci_stmt->init();
}
// End vl_case_item functions.


vl_forever_stmt::vl_forever_stmt(vl_stmt *s)
{
    st_type = ForeverStmt;
    f_stmt = s;
}


vl_forever_stmt *
vl_forever_stmt::copy()
{
    return (new vl_forever_stmt(chk_copy(f_stmt)));
}


void
vl_forever_stmt::init()
{
    if (f_stmt)
        f_stmt->init();
}
// End vl_forever_stmt functions.


vl_repeat_stmt::vl_repeat_stmt(vl_expr *c, vl_stmt *s)
{
    st_type = RepeatStmt;
    r_count = c;
    r_cur_count = 0;
    r_stmt = s;
}


vl_repeat_stmt *
vl_repeat_stmt::copy()
{
    return (new vl_repeat_stmt(chk_copy(r_count), chk_copy(r_stmt)));
}


void
vl_repeat_stmt::init()
{
    if (r_stmt)
        r_stmt->init();
}
// End vl_repeat_stmt functions.


vl_while_stmt::vl_while_stmt(vl_expr *c, vl_stmt *s)
{
    st_type = WhileStmt;
    w_cond = c;
    w_stmt = s;
}


vl_while_stmt *
vl_while_stmt::copy()
{
    return (new vl_while_stmt(chk_copy(w_cond), chk_copy(w_stmt)));
}


void
vl_while_stmt::init()
{
    if (w_stmt)
        w_stmt->init();
}
// End vl_while_stmt functions.


vl_for_stmt::vl_for_stmt(vl_bassign_stmt *i, vl_expr *c, vl_bassign_stmt *e,
    vl_stmt *s)
{
    st_type = ForStmt;
    f_initial = i;
    f_cond = c;
    f_end = e;
    f_stmt = s;
}


vl_for_stmt *
vl_for_stmt::copy()
{
    return (new vl_for_stmt(chk_copy(f_initial), chk_copy(f_cond),
        chk_copy(f_end), chk_copy(f_stmt)));
}


void
vl_for_stmt::init()
{
    if (f_stmt)
        f_stmt->init();
}
// End vl_for_stmt functions.


vl_delay_control_stmt::vl_delay_control_stmt(vl_delay *del, vl_stmt *s)
{
    st_type = DelayControlStmt;
    d_delay = del;
    d_stmt = s;
}


vl_delay_control_stmt *
vl_delay_control_stmt::copy()
{
    return (new vl_delay_control_stmt(chk_copy(d_delay), chk_copy(d_stmt)));
}


void
vl_delay_control_stmt::init()
{
    if (d_stmt)
        d_stmt->init();
}
// End vl_delay_control_stmt functions.


vl_event_control_stmt::vl_event_control_stmt(vl_event_expr *e, vl_stmt *s)
{
    st_type = EventControlStmt;
    ec_event = e;
    ec_stmt = s;
}


vl_event_control_stmt *
vl_event_control_stmt::copy()
{
    return (new vl_event_control_stmt(chk_copy(ec_event), chk_copy(ec_stmt)));
}


// Add a dummy event to the source variable(s) to indicate that somebody
// is listening for changes, and evaluate each expression to set the
// initial value
//
void
vl_event_control_stmt::init()
{
    if (ec_stmt)
        ec_stmt->init();
    if (ec_event)
        ec_event->init();
}
// End vl_event_control_stmt functions.


vl_wait_stmt::vl_wait_stmt(vl_expr *c, vl_stmt *s)
{
    st_type = WaitStmt;
    w_cond = c;
    w_stmt = s;                
    w_event = 0;
}


vl_wait_stmt *
vl_wait_stmt::copy()
{
    return (new vl_wait_stmt(chk_copy(w_cond), chk_copy(w_stmt)));
}


void
vl_wait_stmt::init()
{
    if (w_stmt)
        w_stmt->init();
}
// End vl_wait_stmt functions.


vl_send_event_stmt::vl_send_event_stmt(char *n)
{
    st_type = SendEventStmt;
    se_name = n;
}


vl_send_event_stmt *
vl_send_event_stmt::copy()
{
    return (new vl_send_event_stmt(vl_strdup(se_name)));
}
// End vl_send_event_stmt functions.


vl_fork_join_stmt::vl_fork_join_stmt(char *n, lsList<vl_decl*> *d,
    lsList<vl_stmt*> *s)
{
    st_type = ForkJoinStmt;
    fj_name = n;
    fj_decls = d;
    fj_stmts = s;
    fj_sig_st = 0;
    fj_blk_st = 0;
    fj_endcnt = 0;
}


vl_fork_join_stmt::~vl_fork_join_stmt()
{
    delete [] fj_name;
    delete_list(fj_decls);
    delete_list(fj_stmts);
    delete_table(fj_sig_st);
    delete fj_blk_st;
}


vl_fork_join_stmt *
vl_fork_join_stmt::copy()
{
    vl_fork_join_stmt *stmt = new vl_fork_join_stmt(vl_strdup(fj_name), 0, 0);
    set_var_context(vl_context::push(var_context(), stmt));
    stmt->fj_decls = copy_list(fj_decls);
    stmt->fj_stmts = copy_list(fj_stmts);
    set_var_context(vl_context::pop(var_context()));

    if (stmt->name()) {
        vl_context *cx = var_context();
        if (cx->block()) {
            if (!cx->block()->blk_st())
                cx->block()->set_blk_st(new table<vl_stmt*>);
            cx->block()->blk_st()->insert(stmt->name(), stmt);
        }
        else if (cx->fjblk()) {
            if (!cx->fjblk()->blk_st())
                cx->fjblk()->set_blk_st(new table<vl_stmt*>);
            cx->fjblk()->blk_st()->insert(stmt->name(), stmt);
        }
        else if (cx->task()) {
            if (!cx->task()->blk_st())
                cx->task()->set_blk_st(new table<vl_stmt*>);
            cx->task()->blk_st()->insert(stmt->name(), stmt);
        }
        else if (cx->function()) {
            if (!cx->function()->blk_st())
                cx->function()->set_blk_st(new table<vl_stmt*>);
            cx->function()->blk_st()->insert(stmt->name(), stmt);
        }
        else if (cx->module()) {
            if (!cx->module()->blk_st())
                cx->module()->set_blk_st(new table<vl_stmt*>);
            cx->module()->blk_st()->insert(stmt->name(), stmt);
        }
        else {
            vl_error("fork/join block %s has no parent", stmt->name());
            VS()->abort();
        }
    }
    return (stmt);
}


void
vl_fork_join_stmt::init()
{
    set_var_context(vl_context::push(var_context(), this));
    init_list(fj_decls);
    init_list(fj_stmts);
    set_var_context(vl_context::pop(var_context()));
}
// End vl_fork_join_stmt functions.


vl_task_enable_stmt::vl_task_enable_stmt(int t, char *n, lsList<vl_expr*> *a)
{
    st_type = t;
    te_name = n;
    te_args = a;
    te_task = 0;
}


vl_task_enable_stmt::~vl_task_enable_stmt()
{
    delete [] te_name;
    delete_list(te_args);
}


vl_task_enable_stmt *
vl_task_enable_stmt::copy()
{
    return (new vl_task_enable_stmt(st_type, vl_strdup(te_name),
        copy_list(te_args)));
}
// End vl_task_enable_stmt functions.


vl_disable_stmt::vl_disable_stmt(char *n)
{
    st_type = DisableStmt;
    d_name = n;
    d_target = 0;
}


vl_disable_stmt *
vl_disable_stmt::copy()
{
    return (new vl_disable_stmt(vl_strdup(d_name)));
}
// End vl_disable_stmt functions.


vl_deassign_stmt::vl_deassign_stmt(int t, vl_var *v)
{
    st_type = t;  // DeassignStmt or ReleaseStmt
    d_lhs = v;
    st_flags |= DAS_DEL_VAR;
}


vl_deassign_stmt::~vl_deassign_stmt()
{
    if (st_flags & DAS_DEL_VAR)
        delete d_lhs;
}


vl_deassign_stmt *
vl_deassign_stmt::copy()
{
    return (new vl_deassign_stmt(st_type, chk_copy(d_lhs)));
}


void
vl_deassign_stmt::init()
{
    if (!d_lhs->name()) {
        if (d_lhs->data_type() != Dconcat) {
            vl_error("internal, unnamed variable in deassign");
            VS()->abort();
        }
        return;
    }
    vl_var *nvar = VS()->context()->lookup_var(d_lhs->name(), false);
    if (!nvar) {
        vl_error("undeclared variable %s in deassign", d_lhs->name());
        VS()->abort();
    }
    if (nvar != d_lhs) {
        if (strcmp(nvar->name(), d_lhs->name())) {
            // from another module, don't free it!
            anot_flags(DAS_DEL_VAR);
        }
        delete d_lhs;
        d_lhs = nvar;
    }
    if (d_lhs->flags() & VAR_IN_TABLE)
        anot_flags(DAS_DEL_VAR);
}
// End vl_deassign_stmt functions.


//---------------------------------------------------------------------------
//  Instances
//---------------------------------------------------------------------------

vl_gate_inst::vl_gate_inst()
{
    gi_setup = 0;
    gi_eval = 0;
    gi_set = 0;

    gi_terms = 0;
    gi_outputs = 0;
    gi_string = 0;
    gi_inst_list = 0;
    gi_delay = 0;
    gi_array = 0;
}


vl_gate_inst::vl_gate_inst(char *n, lsList<vl_expr*> *t, vl_range *r)
{
    gi_setup = 0;
    gi_eval = 0;
    gi_set = 0;

    st_type = 0;
    i_name = n;
    gi_terms = t;
    gi_outputs = 0;
    gi_string = 0;
    gi_inst_list = 0;
    gi_delay = 0;
    gi_array = r;
}


vl_gate_inst::~vl_gate_inst()
{
    delete_list(gi_terms);
    delete_list(gi_outputs);
    if (gi_delay) {
        gi_delay->delay1 = 0;
        delete gi_delay;
    }
    delete gi_array;
}


vl_gate_inst *
vl_gate_inst::copy()
{
    vl_gate_inst *g = new vl_gate_inst(vl_strdup(i_name), copy_list(gi_terms),
        chk_copy(gi_array));
    g->set_type(st_type);
    g->gi_setup = gi_setup;
    g->gi_eval = gi_eval;
    g->gi_set = gi_set;
    g->gi_string = gi_string;
    return (g);
}
// End vl_gate_inst functions.


vl_mp_inst::vl_mp_inst(char *n, lsList<vl_port_connect*> *p)
{
    i_name = n;
    pi_ports = p;
    pi_inst_list = 0;
    pi_master = 0;
}


vl_mp_inst::~vl_mp_inst()
{
    delete_list(pi_ports);
    delete pi_master;
}


vl_mp_inst *
vl_mp_inst::copy()
{
    return (new vl_mp_inst(vl_strdup(i_name), copy_list(pi_ports)));
}
// End vl_mp_inst functions.

