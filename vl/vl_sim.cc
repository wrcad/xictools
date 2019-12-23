
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

#include <stdio.h>
#include <math.h>
#include "vl_list.h"
#include "vl_st.h"
#include "vl_defs.h"
#include "vl_types.h"

// needed to find top modules in resolve routines
vl_simulator *vl_context::simulator;

extern bool mmon_start(int);
extern bool mmon_stop();
extern bool mmon_dump(char*);

//---------------------------------------------------------------------------
//  Local
//---------------------------------------------------------------------------

template<class T> void
vl_setup_list(vl_simulator *sim, lsList<T> *stmts)
{
    if (!stmts)
        return;
    lsGen<T> gen(stmts);
    T stmt;
    while (gen.next(&stmt))
        stmt->setup(sim);    
}


template<class T> void
vl_disable_list(lsList<T> *stmts, vl_stmt *s)
{
    if (!stmts)
        return;
    lsGen<T> gen(stmts);
    T stmt;
    while (gen.next(&stmt))
        stmt->disable(s);    
}


// Return the vl_port corresponding to name, or num if name is nil
//
static vl_port *
find_port(lsList<vl_port*> *ports, const char *name, int num = 0)
{
    lsGen<vl_port*> pgen(ports);
    vl_port *port;
    int cnt = 0;
    while (pgen.next(&port)) {
        if (name) {
            if (port->port_exp) {
                if (port->name && !strcmp(port->name, name))
                    return (port);
                lsGen<vl_var*> vgen(port->port_exp);
                vl_var *v;
                if (vgen.next(&v)) {
                    if (v->name && !strcmp(v->name, name))
                        return (port);
                }
            }
        }
        else if (num == cnt)
            return (port);
        cnt++;
    }
    return (0);
}


// Comparison function for combinational primitive tokens
//
static bool
dcmp(unsigned char a, unsigned char b)
{
    if (a == b)
        return (true);
    if (a == PrimQ || b == PrimQ)
        return (true);
    if ((a == PrimB && b <= Prim1) || (b == PrimB && a <= Prim1))
        return (true);
    return (false);
}


// Comparison function for sequential primitive tokens
//
static bool
dcmpseq(unsigned char a, unsigned char l, unsigned char c)
{
    if (c == Prim0)
        return (a == Prim0);
    if (c == Prim1)
        return (a == Prim1);
    if (c == PrimX)
        return (a == PrimX);
    if (c == PrimB)
        return (a == Prim0 || a == Prim1);
    if (c == PrimQ)
        return (a == Prim0 || a == Prim1 || a == PrimX);

    if (c == PrimR)
        return (l == Prim0 && a == Prim1);
    if (c == PrimF)
        return (l == Prim1 && a == Prim0);
    if (c == PrimP)
        return (((l == Prim0 && (a == Prim1 || a == PrimX)) ||
            (l == PrimX && a == Prim1)));
    if (c == PrimN)
        return (((l == Prim1 && (a == Prim0 || a == PrimX)) ||
            (l == PrimX && a == Prim0)));
    if (c == PrimS)
        return (l != a);
    if (c == PrimM)
        return (l == a);
    if (c == Prim0X)
        return (l == Prim0 && a == PrimX);
    if (c == Prim1X)
        return (l == Prim1 && a == PrimX);
    if (c == PrimX0)
        return (l == PrimX && a == Prim0);
    if (c == PrimX1)
        return (l == PrimX && a == Prim1);
    if (c == PrimXB)
        return (l == PrimX && (a == Prim0 || a == Prim1));
    if (c == PrimBX)
        return ((l == Prim0 || l == Prim1) && a == PrimX);
    if (c == PrimBB)
        return ((l == Prim0 && a == Prim1) || (l == Prim1 && a == Prim0));
    if (c == PrimQ0)
        return ((l == Prim1 || l == PrimX) && a == Prim0);
    if (c == PrimQ1)
        return ((l == Prim0 || l == PrimX) && a == Prim1);
    if (c == PrimQB)
        return ((l == Prim0 || l == Prim1 || l == PrimX) &&
            (a == Prim0 || a == Prim1) && l != a);
    if (c == Prim0Q)
        return (l == Prim0 && (a == Prim1 || a == PrimX));
    if (c == Prim1Q)
        return (l == Prim1 && (a == Prim0 || a == PrimX));
    if (c == PrimBQ)
        return ((l == Prim0 || l == Prim1) &&
            (a == Prim0 || a == Prim1 || a == PrimX) && l != a);
    return (false);
}


//---------------------------------------------------------------------------
//  Simulator objects
//---------------------------------------------------------------------------

vl_simulator::vl_simulator()
{
    description = 0;
    dmode = DLYtyp;
    stop = VLrun;
    first_point = true;
    monitor_state = fmonitor_state = false;
    monitors = 0;
    fmonitors = 0;
    time = 0;
    context = 0;
    timewheel = 0;
    next_actions = 0;
    fj_end = 0;
    top_modules = 0;
    dbg_flags = 0;
    channels[0] = 0;
    channels[1] = &cout;
    channels[2] = &cerr;
    for (int i = 3; i < 32; i++) channels[i] = 0;
    dmpfile = 0;
    dmpcx = 0;
    dmpdepth = 0;
    dmpstatus = 0;
    dmpdata = 0;
    dmplast = 0;
    dmpsize = 0;
    dmpindx = 0;
    tfunit = 0;
    tfprec = 0;
    tfsuffix = 0;
    tfwidth = 20;
}


vl_simulator::~vl_simulator()
{
    vl_context::simulator = 0;
    vl_var::simulator = 0;
    if (description && description->simulator == this)
        description->simulator = 0;
    while (monitors) {
        vl_monitor *m = monitors;
        monitors = monitors->next;
        delete m;
    }
    while (fmonitors) {
        vl_monitor *m = fmonitors;
        monitors = fmonitors->next;
        delete m;
    }
    vl_context::destroy(context);
    delete timewheel;
    vl_action_item::destroy(next_actions);
    vl_action_item::destroy(fj_end);

    if (top_modules) {
        for (int i = 0; i < top_modules->num; i++)
            delete top_modules->mods[i];
    }
    delete top_modules;

    // close any open files
    close_files();

    delete dmpcx;
    delete [] dmplast;
    delete dmpdata;
}


// Initialize the simulator, call before simulation, call with desc = 0
// to clear/free everything
//
bool
vl_simulator::initialize(vl_desc *desc, VLdelayType dly, int dbg)
{
    const vl_simulator *vlsim = this;
    if (!vlsim)
        return (false);

    description = 0;
    dmode = DLYtyp;
    stop = VLrun;
    monitor_state = fmonitor_state = false;
    while (monitors) {
        vl_monitor *m = monitors;
        monitors = monitors->next;
        delete m;
    }
    while (fmonitors) {
        vl_monitor *m = fmonitors;
        monitors = fmonitors->next;
        delete m;
    }
    time = 0;
    steptime = 0;
    vl_context::destroy(context);
    context = 0;
    delete timewheel;
    timewheel = 0;
    vl_action_item::destroy(next_actions);
    next_actions = 0;
    vl_action_item::destroy(fj_end);
    fj_end = 0;
    if (top_modules) {
        for (int i = 0; i < top_modules->num; i++)
            delete top_modules->mods[i];
    }
    delete top_modules;
    top_modules = 0;
    dbg_flags = 0;
    time_data.u.t = 0;

    close_files();

    delete dmpcx;
    dmpcx = 0;
    dmpdepth = 0;
    dmpstatus = 0;
    delete dmpdata;
    dmpdata = 0;
    delete [] dmplast;
    dmplast = 0;
    dmpsize = 0;
    dmpindx = 0;
    tfunit = 0;
    tfprec = 0;
    delete [] tfsuffix;
    tfsuffix = 0;
    tfwidth = 20;
    var_factory.clear();

    if (!desc)
        return (true);

    dmode = dly;
    dbg_flags = dbg;
    description = desc;
    vl_context::simulator = this;
    vl_var::simulator = this;
    timewheel = new vl_timeslot(0);
    vl_module *mod;
    lsGen<vl_module*> gen(desc->modules);
    int count = 0;
    while (gen.next(&mod))
        if (!mod->inst_count && !mod->ports)
            count++;
    if (count <= 0) {
        vl_error("no top module!");
        return (false);
    }
    top_modules = new vl_top_mod_list;
    top_modules->num = count;
    top_modules->mods = new vl_module*[count];
    gen.reset(desc->modules);
    count = 0;
    while (gen.next(&mod)) {
        if (!mod->inst_count && !mod->ports) {
            // copy the module, fills in blk_st and task_st
            top_modules->mods[count] = mod->copy();
            count++;
        }
    }
    for (int i = 0; i < top_modules->num; i++) {
        context = context->push(top_modules->mods[i]);
        top_modules->mods[i]->init();
        context = context->pop();
    }
    for (int i = 0; i < top_modules->num; i++) {
        context = context->push(top_modules->mods[i]);
        top_modules->mods[i]->setup(this);
        context = context->pop();
    }
    if (dbg_flags & DBG_mod_dmp) {
        for (int i = 0; i < top_modules->num; i++)
                top_modules->mods[i]->dump(cout);
    }
    monitor_state = true;
    fmonitor_state = true;
    context = 0;
    var_factory.clear();
    time_data.data_type = Dtime;
    time_data.u.t = 0;
    tfunit = (int)(log10(description->tstep) - 0.5);
    return (true);
}


// Perform the simulation
//
bool
vl_simulator::simulate()
{
    vl_context::simulator = this;
    vl_var::simulator = this;
    if (dbg_flags & DBG_desc)
        description->dump(cout);
    while (timewheel && stop == VLrun) {
        vl_timeslot *t = timewheel;
        if (dbg_flags & DBG_tslot) {
            for ( ; t; t = t->next)
                t->print(cout);
            cout << "\n\n";
        }
        var_factory.clear();
        time_data.u.t = timewheel->time;
        timewheel->eval_slot(this);
        if (monitor_state && monitors) {
            for (vl_monitor *m = monitors; m; m = m->next) {
                vl_context *tcx = context;
                context = m->cx;
                if (monitor_change(m->args))
                    display_print(m->args, cout, m->dtype, 0);
                context = tcx;
            }
        }
        if (fmonitor_state && fmonitors) {
            for (vl_monitor *m = fmonitors; m; m = m->next) {
                vl_context *tcx = context;
                context = m->cx;
                if (monitor_change(m->args))
                    fdisplay_print(m->args, m->dtype, 0);
                context = tcx;
            }
        }
        if (dmpstatus & DMP_ACTIVE)
            do_dump();
        t = timewheel->next;
        delete timewheel;
        timewheel = t;
        first_point = false;
    }
    // close any open files
    close_files();
    return (true);
}


// Step one time point
//
VLstopType
vl_simulator::step()
{
    vl_context::simulator = this;
    vl_var::simulator = this;
    while (timewheel && stop == VLrun && timewheel->time <= steptime) {
        time_data.u.t = timewheel->time;
        timewheel->eval_slot(this);
        if (monitor_state && monitors) {
            for (vl_monitor *m = monitors; m; m = m->next) {
                vl_context *tcx = context;
                context = m->cx;
                if (monitor_change(m->args))
                    display_print(m->args, cout, m->dtype, 0);
                context = tcx;
            }
        }
        if (fmonitor_state && fmonitors) {
            for (vl_monitor *m = fmonitors; m; m = m->next) {
                vl_context *tcx = context;
                context = m->cx;
                if (monitor_change(m->args))
                    fdisplay_print(m->args, m->dtype, 0);
                context = tcx;
            }
        }
        if (dmpstatus & DMP_ACTIVE)
            do_dump();
        vl_timeslot *t = timewheel->next;
        delete timewheel;
        timewheel = t;
        first_point = false;
        var_factory.clear();
    }
    steptime++;

    // Always keep a non-nil timewheel when stepping.
    if (!timewheel) {
        vl_timeslot *ts = new vl_timeslot(0);
        ts->next = timewheel;
        timewheel = ts;
    }

    if (stop != VLrun)
        close_files();
    return (stop);
}


// Close all open files.
//
void
vl_simulator::close_files()
{
    delete dmpfile;
    dmpfile = 0;
    for (int i = 3; i < 32; i++) {
        delete channels[i];
        channels[i] = 0;
    }
}


// Flush all open files.
//
void
vl_simulator::flush_files()
{
    if (dmpfile)
        dmpfile->flush();
    for (int i = 3; i < 32; i++) {
        if (channels[i])
            channels[i]->flush();
    }
}
// End vl_simulator functions


// Copy context
//
vl_context *
vl_context::copy()
{
    vl_context *cx0 = 0, *cx = 0;
    for (vl_context *c = this; c; c = c->parent) {
        if (!cx0)
            cx = cx0 = new vl_context(*c);
        else {
            cx->parent = new vl_context(*c);
            cx = cx->parent;
        }
        cx->parent = 0;
    }
    return (cx0);
}


// Push to new context, return the new context
//
vl_context *
vl_context::push()
{
    vl_context *cx = new vl_context();
    cx->parent = this;
    return (cx);
}


vl_context *
vl_context::push(vl_mp *m)
{
    vl_context *cx = push();
    if (m->type == ModDecl)
        cx->module = (vl_module*)m;
    else if (m->type == CombPrimDecl || m->type == SeqPrimDecl)
        cx->primitive = (vl_primitive*)m;
    else {
        vl_error("internal, bad object type %d (not mod/prim)", m->type);
        simulator->abort();
    }
    return (cx);
}


vl_context *
vl_context::push(vl_module *m)
{
    vl_context *cx = push();
    if (m->type == ModDecl)
        cx->module = m;
    else {
        vl_error("internal, bad object type %d (not module)", m->type);
        simulator->abort();
    }
    return (cx);
}


vl_context *
vl_context::push(vl_primitive *p)
{
    vl_context *cx = push();
    if (p->type == CombPrimDecl || p->type == SeqPrimDecl)
        cx->primitive = p;
    else {
        vl_error("internal, bad object type %d (not primitive)", p->type);
        simulator->abort();
    }
    return (cx);
}


vl_context *
vl_context::push(vl_task *t)
{
    vl_context *cx = push();
    if (t->type == TaskDecl)
        cx->task = t;
    else {
        vl_error("internal, bad object type %d (not task)", t->type);
        simulator->abort();
    }
    return (cx);
}


vl_context *
vl_context::push(vl_function *f)
{
    vl_context *cx = push();
    if (f->type >= IntFuncDecl && f->type <= RangeFuncDecl)
        cx->function = f;
    else {
        vl_error("internal, bad object type %d (not function)", f->type);
        simulator->abort();
    }
    return (cx);
}


vl_context *
vl_context::push(vl_begin_end_stmt *b)
{
    vl_context *cx = push();
    if (b->type == BeginEndStmt)
        cx->block = b;
    else {
        vl_error("internal, bad object type %d (not begin/end)", b->type);
        simulator->abort();
    }
    return (cx);
}


vl_context *
vl_context::push(vl_fork_join_stmt *f)
{
    vl_context *cx = push();
    if (f->type == ForkJoinStmt)
        cx->fjblk = f;
    else {
        vl_error("internal, bad object type %d (not fork/join)", f->type);
        simulator->abort();
    }
    return (cx);
}


// Pop to previous context, return the previous context
//
vl_context *
vl_context::pop()
{
    const vl_context *vcx = this;
    if (vcx) {
        vl_context *cx = parent;
        delete this;
        return (cx);
    }
    return (0);
}


// Return true if the passed object is in the context hierarchy
//
bool
vl_context::in_context(vl_stmt *blk)
{
    if (!blk)
        return (false);
    if (blk->type == BeginEndStmt) {
        for (vl_context *cx = this; cx; cx = cx->parent) {
            if (cx->block == blk)
                return (true);
        }
    }
    else if (blk->type == ForkJoinStmt) {
        for (vl_context *cx = this; cx; cx = cx->parent) {
            if (cx->fjblk == blk)
                return (true);
        }
    }
    else if (blk->type == TaskDecl) {
        for (vl_context *cx = this; cx; cx = cx->parent) {
            if (cx->task == blk)
                return (true);
        }
    }
    return (false);
}


// Retrieve a value for the named variable, search present context
// exclusively if thisonly is true
//
vl_var *
vl_context::lookup_var(const char *name, bool thisonly)
{
    const char *bname = name;
    vl_context *cx = this;
    while (cx) {
        table<vl_var*> *st = cx->resolve_st(&bname, false);
        if (st) {
            vl_var *data;
            if (st->lookup(bname, &data) && data)
                return (data);
        }
        if (thisonly || bname != name)
            break;
        if (cx->module || cx->primitive)
            break;
        cx = cx->parent;
    }
    return (0);
}


// Return the named vl_begin_end_stmt or vl_fork_join_stmt
//
vl_stmt *
vl_context::lookup_block(const char *name)
{
    const char *bname = name;
    vl_context cvar;
    if (!resolve_cx(&bname, cvar, false)) {
        if (bname != name)
            // unresolved path prefix
            return (0);
        else
            // no path prefix
            cvar = *this;
    }
    vl_context *cx = &cvar;
    while (cx) {
        vl_stmt *stmt;
        if (cx->block) {
            if (cx->block->name && !strcmp(cx->block->name, bname))
                return (cx->block);
            if (cx->block->blk_st &&
                    cx->block->blk_st->lookup(bname, &stmt) && stmt)
                return (stmt);
        }
        if (cx->fjblk) {
            if (cx->fjblk->name && !strcmp(cx->fjblk->name, bname))
                return (cx->fjblk);
            if (cx->fjblk->blk_st &&
                    cx->fjblk->blk_st->lookup(bname, &stmt) && stmt)
                return (stmt);
        }
        if (cx->function) {
            if (cx->function->blk_st &&
                    cx->function->blk_st->lookup(bname, &stmt) && stmt)
                return (stmt);
        }
        if (cx->task) {
            if (cx->task->blk_st &&
                    cx->task->blk_st->lookup(bname, &stmt) && stmt)
                return (stmt);
        }
        if (cx->module) {
            if (cx->module->blk_st &&
                    cx->module->blk_st->lookup(bname, &stmt) && stmt)
                return (stmt);
        }
        if (bname != name)
            break;
        if (cx->module)
            break;
        cx = cx->parent;
    }
    return (0);
}


// Return the named vl_task
//
vl_task *
vl_context::lookup_task(const char *name)
{
    const char *bname = name;
    table<vl_task*> *st = resolve_task(&bname);
    if (st) {
        vl_task *vtask;
        if (st->lookup(bname, &vtask) && vtask)
            return (vtask);
    }
    return (0);
}


// Return the named vl_function
//
vl_function *
vl_context::lookup_func(const char *name)
{
    const char *bname = name;
    table<vl_function*> *st = resolve_func(&bname);
    if (st) {
        vl_function *func;
        if (st->lookup(bname, &func) && func)
            return (func);
    }
    return (0);
}


vl_inst *
vl_context::lookup_mp(const char *name)
{
    const char *bname = name;
    table<vl_inst*> *st = resolve_mp(&bname);
    if (st) {
        vl_inst *mp;
        if (st->lookup(bname, &mp) && mp)
            return (mp);
    }
    return (0);
}


// Return the symbol table for name, resolving the '.' notation if any,
// if crt is true, create new symbol table if empty.  The base name is
// returned in 'name'
//
table<vl_var*> *
vl_context::resolve_st(const char **name, bool crt)
{
    vl_context cvar;
    const char *bname = *name;
    if (!resolve_cx(name, cvar, false)) {
        if (bname != *name)
            // unresolved path prefix
            return (0);
        else
            // no path prefix
            cvar = *this;
    }
    table<vl_var*> *st = 0;
    if (cvar.module) {
        if (crt && !cvar.module->sig_st)
            cvar.module->sig_st = new table<vl_var*>;
        st = cvar.module->sig_st;
    }
    else if (cvar.primitive) {
        if (crt && !cvar.primitive->sig_st)
            cvar.primitive->sig_st = new table<vl_var*>;
        st = cvar.primitive->sig_st;
    }
    else if (cvar.task) {
        if (crt && !cvar.task->sig_st)
            cvar.task->sig_st = new table<vl_var*>;
        st = cvar.task->sig_st;
    }
    else if (cvar.function) {
        if (crt && !cvar.function->sig_st)
            cvar.function->sig_st = new table<vl_var*>;
        st = cvar.function->sig_st;
    }
    else if (cvar.block) {
        if (crt && !cvar.block->sig_st)
            cvar.block->sig_st = new table<vl_var*>;
        st = cvar.block->sig_st;
    }
    else if (cvar.fjblk) {
        if (crt && !cvar.fjblk->sig_st)
            cvar.fjblk->sig_st = new table<vl_var*>;
        st = cvar.fjblk->sig_st;
    }
    return (st);
}


table<vl_task*> *
vl_context::resolve_task(const char **name)
{
    vl_context cvar;
    const char *bname = *name;
    if (!resolve_cx(name, cvar, false)) {
        if (bname != *name)
            // unresolved path prefix
            return (0);
        else
            // no path prefix
            cvar = *this;
    }
    table<vl_task*> *st = 0;
    vl_context *cx = &cvar;
    while (cx) {
        if (cx->module) {
            st = cx->module->task_st;
            break;
        }
        cx = cx->parent;
    }
    return (st);
}


table<vl_function*> *
vl_context::resolve_func(const char **name)
{
    vl_context cvar;
    const char *bname = *name;
    if (!resolve_cx(name, cvar, false)) {
        if (bname != *name)
            // unresolved path prefix
            return (0);
        else
            // no path prefix
            cvar = *this;
    }
    table<vl_function*> *st = 0;
    vl_context *cx = &cvar;
    while (cx) {
        if (cx->module) {
            st = cx->module->func_st;
            break;
        }
        cx = cx->parent;
    }
    return (st);
}


table<vl_inst*> *
vl_context::resolve_mp(const char **name)
{
    vl_context cvar;
    const char *bname = *name;
    if (!resolve_cx(name, cvar, false)) {
        if (bname != *name)
            // unresolved path prefix
            return (0);
        else
            // no path prefix
            cvar = *this;
    }
    table<vl_inst*> *st = 0;
    vl_context *cx = &cvar;
    while (cx) {
        if (cx->module) {
            st = cx->module->inst_st;
            break;
        }
        cx = cx->parent;
    }
    return (st);
}


// Strip the prefix from name, and return the object which should contain
// the base symbol name in cvar.  If error or no prefix, return false.
// If modonly is true, return the module containing item.
//
bool
vl_context::resolve_cx(const char **name, vl_context &cvar, bool modonly)
{
    const char *r = *name;
    const char *rp = 0;
    while (*r) {
        if (*r == '\\') {
            while (*r && !isspace(*r))
                r++;
            while (*r && isspace(*r))
                r++;
        }
        if (*r == '.')
            rp = r;
        r++;
    }
    r = rp;
    if (!r)
        return (false);
    int os = r - *name;
    char buf[256];
    strcpy(buf, *name);
    *name = r + 1;
    *(buf + os) = 0;
    r = buf;
    char *s = (char*)r;
    while (*s) {
        if (*s == '\\') {
            while (*s && !isspace(*s))
                s++;
            while (*s && isspace(*r))
                r++;
        }
        if (*s == '.')
            break;
        s++;
    }
    bool fdot = false;
    if (*s == '.') {
        *s++ = 0;
        fdot = true;
    }

    // look for top module
    cvar.module = 0;
    cvar.parent = 0;
    for (int i = 0; i < simulator->top_modules->num; i++) {
        if (!strcmp(r, simulator->top_modules->mods[i]->name)) {
            cvar.module = simulator->top_modules->mods[i];
            r = s;
            break;
        }
    }
    if (!cvar.module) {
        vl_context *cx = this;
        while (!cx->module && !cx->primitive && !cx->task && !cx->function)
            cx = cx->parent;
        if (!cx)
            return (false);
        cvar = *cx;
        // put '.' back and move to start
        if (fdot)
            *(s-1) = '.';
        if (cx->task || cx->function) {
            // first search in task/function
            if (cvar.resolve_path(r, modonly))
                return (true);
            while (!cx->module && !cx->primitive)
                cx = cx->parent;
            if (!cx)
                return (false);
            cvar = *cx;
        }
    }
    return (cvar.resolve_path(r, modonly));
}


bool
vl_context::resolve_path(const char *string, bool modonly)
{
    char buf[256];
    strcpy(buf, string);
    char *r, *s;
    for (r = buf; *r; r = s) {
        s = r;
        while (*s) {
            if (*s == '\\') {
                while (*s && !isspace(*s))
                    s++;
                while (*s && isspace(*s))
                    s++;
            }
            if (*s == '.')
                break;
            s++;
        }
        if (*s == '.')
            *s++ = 0;

        bool found = false;
        if (module) {
            vl_inst *inst;
            if (module->inst_st && module->inst_st->lookup(r, &inst)) {
                if (!inst->type) {
                    // not a gate
                    vl_mp_inst *mp = (vl_mp_inst*)inst;
                    if (mp->inst_list) {
                        if (mp->inst_list->mptype == MPmod) {
                            found = true;
                            module = (vl_module*)mp->master;
                        }
                        else if (mp->inst_list->mptype == MPprim) {
                            if (modonly)
                                return (true);
                            found = true;
                            module = 0;
                            primitive = (vl_primitive*)mp->master;
                        }
                    }
                }
                if (!found) {
                    // must have been a gate?
                    module = 0;
                    return (false);
                }
            }
            vl_stmt *stmt;
            if (!found && module->blk_st && module->blk_st->lookup(r, &stmt)) {
                if (modonly)
                    return (true);
                found = true;
                if (stmt->type == BeginEndStmt) {
                    module = 0;
                    block = (vl_begin_end_stmt*)stmt;
                }
                else if (stmt->type == ForkJoinStmt) {
                    module = 0;
                    fjblk = (vl_fork_join_stmt*)stmt;
                }
                else
                    found = false;
            }
        }
        else if (task) {
            vl_stmt *stmt;
            if (task->blk_st && task->blk_st->lookup(r, &stmt)) {
                found = true;
                if (stmt->type == BeginEndStmt) {
                    task = 0;
                    block = (vl_begin_end_stmt*)stmt;
                }
                else if (stmt->type == ForkJoinStmt) {
                    task = 0;
                    fjblk = (vl_fork_join_stmt*)stmt;
                }
                else
                    found = false;
            }
        }
        else if (function) {
            vl_stmt *stmt;
            if (function->blk_st && function->blk_st->lookup(r, &stmt)) {
                found = true;
                if (stmt->type == BeginEndStmt) {
                    function = 0;
                    block = (vl_begin_end_stmt*)stmt;
                }
                else if (stmt->type == ForkJoinStmt) {
                    function = 0;
                    fjblk = (vl_fork_join_stmt*)stmt;
                }
                else
                    found = false;
            }
        }
        else if (block) {
            vl_stmt *stmt;
            if (block->blk_st && block->blk_st->lookup(r, &stmt)) {
                found = true;
                if (stmt->type == BeginEndStmt)
                    block = (vl_begin_end_stmt*)stmt;
                else if (stmt->type == ForkJoinStmt) {
                    block = 0;
                    fjblk = (vl_fork_join_stmt*)stmt;
                }
                else
                    found = false;
            }
        }
        else if (fjblk) {
            vl_stmt *stmt;
            if (fjblk->blk_st && fjblk->blk_st->lookup(r, &stmt)) {
                found = true;
                if (stmt->type == BeginEndStmt) {
                    fjblk = 0;
                    block = (vl_begin_end_stmt*)stmt;
                }
                else if (stmt->type == ForkJoinStmt)
                    fjblk = (vl_fork_join_stmt*)stmt;
                else
                    found = false;
            }
        }
        if (!found) {
            module = 0;
            primitive = 0;
            task = 0;
            function = 0;
            block = 0;
            fjblk = 0;
            return (false);
        }
    }
    return (true);
}


// Return the current module
//
vl_module *
vl_context::currentModule()
{
    vl_context *cx = this;
    while (cx) {
        if (cx->module)
            return (cx->module);
        if (cx->primitive)
            return (0);
        cx = cx->parent;
    }
    return (0);
}


// Return the current primitive
//
vl_primitive *
vl_context::currentPrimitive()
{
    vl_context *cx = this;
    while (cx) {
        if (cx->primitive)
            return (cx->primitive);
        if (cx->module)
            return (0);
        cx = cx->parent;
    }
    return (0);
}


// Return the current function
//
vl_function *
vl_context::currentFunction()
{
    vl_context *cx = this;
    while (cx) {
        if (cx->function)
            return (cx->function);
        if (cx->module || cx->primitive)
            return (0);
        cx = cx->parent;
    }
    return (0);
}


// Return the current task
//
vl_task *
vl_context::currentTask()
{
    vl_context *cx = this;
    while (cx) {
        if (cx->task)
            return (cx->task);
        if (cx->module || cx->primitive)
            return (0);
        cx = cx->parent;
    }
    return (0);
}
// End vl_context functions


vl_action_item::vl_action_item(vl_stmt *s, vl_context *c)
{
    type = ActionItem;
    stmt = s;
    stack = 0;
    event = 0;
    context = c->copy();
    next = 0;
    flags = 0;
}


// Copy an action
//
vl_action_item *
vl_action_item::copy()
{
    return (new vl_action_item(stmt, context));
}


vl_action_item::~vl_action_item()
{
    if (flags & AI_DEL_STMT)
        delete stmt;
    delete stack;
    vl_context::destroy(context);
}


// Remove and free any entries in the actions list with blk in the context
// hierarchy, return the purged list head
//
vl_action_item *
vl_action_item::purge(vl_stmt *blk)
{
    vl_action_item *a0 = this;
    vl_action_item *ap = 0, *an;
    for (vl_action_item *a = a0; a; a = an) {
        an = a->next;
        if (a->stack) {
            for (int i = 0; i < a->stack->num; i++)
                a->stack->acts[i].actions =
                    a->stack->acts[i].actions->purge(blk);
            bool mt = true;
            for (int i = 0; i < a->stack->num; i++) {
                if (a->stack->acts[i].actions) {
                    mt = false;
                    break;
                }
            }
            if (mt) {
                if (!ap)
                    a0 = an;
                else
                    ap->next = an;
                delete a;
                continue;
            }
        }
        else if (a->stmt) {
            if (a->context->in_context(blk)) {
                if (!ap)
                    a0 = an;
                else
                    ap->next = an;
                delete a;
                continue;
            }
        }
        ap = a;
    }
    return (a0);
}


// Evaluate an action
//
EVtype
vl_action_item::eval(vl_event *ev, vl_simulator *sim)
{
    if (stmt) {
        vl_context *cx = sim->context;
        sim->context = context;
        EVtype evt = stmt->eval(ev, sim);
        sim->context = cx;
        return (evt);
    }
    else {
        vl_error("(internal) null statement encountered");
        sim->abort();
    }
    return (EVnone);
}


// Diagnostic print of action
//
void
vl_action_item::print(ostream &outs)
{
    if (stmt) {
        stmt->print(outs);
        outs << stmt->lterm();
    }
    else if (stack)
        stack->print(outs);
    else if (event)
        outs << "<event>\n";
}
// End of vl_action_item functions


vl_stack *
vl_stack::copy()
{
    const vl_stack *stk = this;
    if (!stk)
        return (0);
    return (new vl_stack(acts, num));
}


// Diagnostic print of stack
//
void
vl_stack::print(ostream &outs)
{
    for (int i = num-1; i >= 0; i--) {
        for (vl_action_item *a = acts[i].actions; a; a = a->next) {
            cout << i << '|';
            a->print(outs);
        }
    }
}
// End vl_stack functions


vl_timeslot::vl_timeslot(vl_time_t t) 
{
    time = t;
    actions = a_end = 0;
    trig_actions = t_end = 0;
    zdly_actions = z_end = 0;
    nbau_actions = n_end = 0;
    mon_actions = m_end = 0;
    next = 0;
}


vl_timeslot::~vl_timeslot() 
{
    vl_action_item::destroy(actions);
    vl_action_item::destroy(trig_actions);
    vl_action_item::destroy(zdly_actions);
    vl_action_item::destroy(nbau_actions);
    vl_action_item::destroy(mon_actions);
}
  
// Return the list head corresponding to the indicated time
//
vl_timeslot *
vl_timeslot::find_slot(vl_time_t t)
{
    if (t < time) {
        // This can happen when stepping (WRspice control) if an event
        // is generated by an input signal from WRspice which was not
        // triggered from Verilog.  Handle this by changing this to
        // a dummy timeslot (it is the list head in the simulator) and
        // add a new slot with the previous contents.

        vl_timeslot *ts = new vl_timeslot(*this);
        ts->next = next;
        *this = vl_timeslot(t);
        next = ts;
        return (this);

        /*
        vl_error("negative delay encountered, time=%g timewheel start=%g",
            (double)t, (double)time);
        vl_var::simulator->abort();
        return (this);
        */
    }
    vl_timeslot *s = this, *sp = 0;
    while (s && s->time < t)
        sp = s, s = s->next;
    if (s) {
        if (s->time > t) {
            vl_timeslot *ts = new vl_timeslot(t);
            ts->next = s;
            sp->next = ts;
            s = ts;
        }
    }
    else {
        sp->next = new vl_timeslot(t);
        s = sp->next;
    }
    return (s);
}


// Append an action at time t
//
void
vl_timeslot::append(vl_time_t t, vl_action_item *a)
{
    vl_timeslot *s = find_slot(t);
    for (vl_action_item *aa = s->actions; aa; aa = aa->next) {
        if (aa->stmt && aa->stmt == a->stmt) {
            delete a;
            return;
        }
    }
    if (!s->actions)
        s->actions = s->a_end = a;
    else
        s->a_end->next = a;
    while (s->a_end->next)
        s->a_end = s->a_end->next;
}


// Append an action to the list of triggered events at time t
//
void
vl_timeslot::append_trig(vl_time_t t, vl_action_item *a)
{
    vl_timeslot *s = find_slot(t);
    for (vl_action_item *aa = s->trig_actions; aa; aa = aa->next) {
        if (aa->stmt && aa->stmt == a->stmt) {
            delete a;
            return;
        }
    }
    if (!s->trig_actions)
        s->trig_actions = s->t_end = a;
    else
        s->t_end->next = a;
    while (s->t_end->next)
        s->t_end = s->t_end->next;
}


// Append an "inactive" event at time t (for #0 ...)
//
void
vl_timeslot::append_zdly(vl_time_t t, vl_action_item *a)
{
    vl_timeslot *s = find_slot(t);
    for (vl_action_item *aa = s->zdly_actions; aa; aa = aa->next) {
        if (aa->stmt && aa->stmt == a->stmt) {
            delete a;
            return;
        }
    }
    if (!s->zdly_actions)
        s->zdly_actions = s->z_end = a;
    else
        s->z_end->next = a;
    while (s->z_end->next)
        s->z_end = s->z_end->next;
}


// Append a "non-blocking assign update" event at time t  (for n-b assign)
//
void
vl_timeslot::append_nbau(vl_time_t t, vl_action_item *a)
{
    vl_timeslot *s = find_slot(t);
    for (vl_action_item *aa = s->nbau_actions; aa; aa = aa->next) {
        if (aa->stmt && aa->stmt == a->stmt) {
            delete a;
            return;
        }
    }
    if (!s->nbau_actions)
        s->nbau_actions = s->n_end = a;
    else
        s->n_end->next = a;
    while (s->n_end->next)
        s->n_end = s->n_end->next;
}


// Append a "monitor" event at time t  (for $monitor/$strobe)
//
void
vl_timeslot::append_mon(vl_time_t t, vl_action_item *a)
{
    vl_timeslot *s = find_slot(t);
    for (vl_action_item *aa = s->mon_actions; aa; aa = aa->next) {
        if (aa->stmt && aa->stmt == a->stmt) {
            delete a;
            return;
        }
    }
    if (!s->mon_actions)
        s->mon_actions = s->m_end = a;
    else
        s->m_end->next = a;
    while (s->m_end->next)
        s->m_end = s->m_end->next;
}


// Process the actions at the current time, servicing events, and
// resheduling propagating actions
//
void
vl_timeslot::eval_slot(vl_simulator *sim)
{
    sim->time = time;
    sim->time_data.trigger();  // for @($time)
    add_next_actions(sim);
    while (actions || zdly_actions || nbau_actions || mon_actions) {
        if (sim->stop != VLrun)
            break;
        if (!actions) {
            if (zdly_actions) {
                actions = zdly_actions;
                zdly_actions = 0;
            }
            else if (nbau_actions) {
                actions = nbau_actions;
                nbau_actions = 0;
            }
            else if (mon_actions) {
                actions = mon_actions;
                mon_actions = 0;
            }
            else
                break;
        }
        do_actions(sim);
    }
}


// Add an action to be performed at the next time point
//
void
vl_timeslot::add_next_actions(vl_simulator *sim)
{
    if (sim->next_actions) {
        vl_action_item *a = sim->next_actions;
        while (a->next)
            a = a->next;
        a->next = actions;
        actions = sim->next_actions;
        sim->next_actions = 0;
    }
}


//#define DEBUG_CONTEXT
#ifdef DEBUG_CONTEXT

static bool
check_cx(vl_context *cx)
{
    while (cx) {
        if (cx->block && cx->block->type != BeginEndStmt)
            return (false);
        if (cx->fjblk && cx->fjblk->type != ForkJoinStmt)
            return (false);
        if (cx->task && cx->task->type != TaskDecl)
            return (false);
        if (cx->module && cx->module->type != ModDecl)
            return (false);
        cx = cx->parent;
    }
    return (true);
}

#endif


#define STACK_DEPTH 256

// Loop through and evaluate the actions.
//
void
vl_timeslot::do_actions(vl_simulator *sim)
{
    vl_sblk acts[STACK_DEPTH];
    int sp = 0;
    acts[sp].actions = actions;
    acts[sp].type = Fence;
    actions = 0;
    bool doing_trig = false;
    int trig_sp = 0;

    while (sp >= 0) {
        vl_action_item *a = acts[sp].actions;

        if (trig_actions && trig_actions->stmt && a && a->stmt &&
                (a->stmt->type == InitialStmt ||
                a->stmt->type == AlwaysStmt ||
                a->stmt->type == BeginEndStmt ||
                a->stmt->type == ForkJoinStmt)) {

            // We're about to start a new block, and the leading action
            // has a stmt rather than a stack, so is an assignment rather
            // than a continuation.  Do the leading assignments now.
            //
            // Getting the scheduling right is a challenge, taking lots
            // of experimentation.  Here are some of the problems:
            //
            // If trig actions are handled immediately, sio85.v and
            // task1.v don't work.  These files assume that statements
            // in the current block continue to execute before events
            // are handled.  Handling trig actions when stalled (a == 0
            // and sp == 0, see block below) solves this.
            //
            // Without the present block, cff.v hangs.  This is due to
            // the intermediate state in the continuous assign generating
            // events which recursively retrigger.  The present block, if
            // applied to all trig actions on InitialStmts only, fixes
            // that problem.
            // 
            // The schedule.v file still fails, but works when AlwaysStmts
            // are handled in the present block.  This, however, breaks
            // onehot.v.  If only the trig actions with stmts are handled
            // here, all known files run correctly.  The begin/end and
            // fork/join handling was added to unify block handling, and
            // may not be needed.

            sp++;
            acts[sp].actions = trig_actions;
            vl_action_item *tp;
            do {
                tp = trig_actions;
                trig_actions = trig_actions->next;
            } while (trig_actions && trig_actions->stmt);
            tp->next = 0;

            acts[sp].type = Sequential;
            trig_sp = sp;
            doing_trig = true;
            continue;
        }

        if (!a && sp == 0) {
            // We're stalled, time to do the accumulated events triggered
            // by the previous actions
            a = trig_actions;
            trig_actions = 0;
            doing_trig = true;
            trig_sp = 0;
        }
        if (!a) {
            sp--;
            continue;
        }
        acts[sp].actions = a->next;

        // If the action has a stack, unwind the stack and store a
        // place holder so as to keep this frame separate if there is
        // an event or delay encountered
        if (a->stack) {
            acts[sp].type = Fence;
            for (int i = 0; i < a->stack->num; i++) {
                acts[++sp] = a->stack->acts[i];
                a->stack->acts[i].actions = 0;
            }
            delete a;
            continue;
        }

        if (sp < trig_sp)
            doing_trig = false;
        if (sim->dbg_flags & DBG_action) {
            if (doing_trig)
                cout << '*';
            cout << "---- " << sim->time << " " << sp << '\n';
            a->print(cout);
        }

#ifdef DEBUG_CONTEXT
        if (!check_cx(sim->context)) {
            cout << "at time=" << sim->time << " CONTEXT IS BAD!\n";
            cout << a->stmt << '\n';
            cout << "---\n";
        }
#endif
        if (a->stmt->type == ForkJoinStmt) {
            // We're about to start on a fork/join block.  To deal
            // with completion, save a list of actions, which are
            // updated when each thread completes (in vl_fj_break::eval()).
            // The actions are given a stack along the way
            //
            vl_action_item *anew = new vl_action_item(a->stmt, 0);
            anew->next = sim->fj_end;
            sim->fj_end = anew;
        }

        vl_event ev;
        EVtype evt = a->eval(&ev, sim);
        if (sim->stop != VLrun)
            return;

        if (a->stmt->type == DisableStmt) {
            // just evaluated a disable statement, target should be
            // filled in.  Purge all pending actions for this block
            vl_stmt *blk = ((vl_disable_stmt*)a->stmt)->target;
            if (blk) {
                sim->timewheel->purge(blk);
                for (int i = 0; i <= sp; i++)
                    acts[i].actions = acts[i].actions->purge(blk);
                // this purges the events under blk, can generate
                // f/j termination events
                blk->disable(blk);
            }
        }
        if (actions) {
            if (a->stmt->type == BeginEndStmt ||
                    a->stmt->type == ForkJoinStmt ||
                    a->stmt->type == InitialStmt ||
                    a->stmt->type == AlwaysStmt) {
                // The action created more actions, push these into a new
                // stack block, and increment the stack pointer
                ABtype ntype;
                if (a->stmt->type == ForkJoinStmt)
                    ntype = Fork;
                else
                    ntype = Sequential;
                if (acts[sp].actions || acts[sp].type != ntype)
                    sp++;
                if (sp >= STACK_DEPTH) {
                    vl_error("(internal) stack depth exceeded");
                    sim->abort();
                    return;
                }
                acts[sp].actions = actions;
                acts[sp].type = ntype;
                acts[sp].fjblk = (ntype == Fork ? a->stmt : 0);
                actions = 0;
            }
            else {
                a_end->next = acts[sp].actions;
                acts[sp].actions = actions;
                actions = 0;
            }
        }

        // If a delay or event is returned, save the frame for the event
        // in a new action, then dispatch the action to the appropriate
        // list and fix the stack pointer
        //
        if (evt == EVdelay) {
            int lsp = sp;
            if (!acts[lsp].actions)
                lsp--;
            int tsp = lsp;
            while (tsp > 0 && acts[tsp].type == Sequential)
                tsp--;
            if (tsp != lsp) {
                vl_action_item *anew = new vl_action_item(0, 0);
                anew->stack = new vl_stack(acts + tsp + 1, lsp - tsp);
                if (ev.time == sim->time)
                    sim->timewheel->append_zdly(ev.time, anew);
                else
                    sim->timewheel->append(ev.time, anew);
                for (int i = tsp+1; i <= lsp; i++)
                    acts[i].actions = 0;
                sp = tsp;
            }
        }
        else if (evt == EVevent) {
            int lsp = sp;
            if (!acts[lsp].actions)
                lsp--;
            int tsp = lsp;
            while (tsp > 0 && acts[tsp].type == Sequential)
                tsp--;
            if (tsp != lsp) {
                vl_action_item *anew = new vl_action_item(0, 0);
                anew->stack = new vl_stack(acts + tsp + 1, lsp - tsp);
                anew->event = ev.event;
                // put in event queue
                anew->event->chain(anew);
                delete anew;
                for (int i = tsp+1; i <= lsp; i++)
                    acts[i].actions = 0;
                sp = tsp;
            }
        }
        if (acts[sp].type == Fork && !acts[sp].actions) {
            // We're done setting up a fork/join block, and can now
            // provide a stack to the action item in the fork/join
            // completion list.

            vl_stmt *fj = acts[sp].fjblk;
            acts[sp].fjblk = 0;

            vl_action_item *af = 0;
            if (fj && fj->type == ForkJoinStmt) {
                for (af = sim->fj_end; af; af = af->next) {
                    if (af->stmt == fj)
                        break;
                }
            }
            if (af) {
                // Add the stack.  If there is no stack, remove the
                // action from the list.  The threads may or may not
                // have completed.  If so, remove the action and add
                // it to the queue.
                int tsp = --sp;
                while (tsp > 0 && acts[tsp].type == Sequential)
                    tsp--;
                if (tsp != sp) {
                    af->stack = new vl_stack(acts + tsp + 1, sp - tsp);
                    for (int i = tsp+1; i <= sp; i++)
                        acts[i].actions = 0;
                    sp = tsp;
                }
                if (!af->stack || ((vl_fork_join_stmt*)fj)->endcnt <= 0) {
                    // remove from list
                    vl_action_item *ap = 0;
                    for (vl_action_item *aa = sim->fj_end; aa; aa = aa->next) {
                        if (aa == af) {
                            if (!ap)
                                sim->fj_end = aa->next;
                            else
                                ap->next = aa->next;
                            break;
                        }
                        ap = aa;
                    }
                }
                if (!af->stack)
                    delete af;
                else if (((vl_fork_join_stmt*)fj)->endcnt <= 0) {
                    af->stmt = 0;
                    sim->timewheel->append(sim->time, af);
                }
            }
            else {
                vl_error("internal, lost fork/join return pointer");
                sim->abort();
            }
        }
        delete a;
        sim->var_factory.clear();
    }
}


// Get rid of all pending actions with blk in the context hierarchy
//
void
vl_timeslot::purge(vl_stmt *blk)
{
    for (vl_timeslot *ts = this; ts; ts = ts->next) {
        ts->actions = ts->actions->purge(blk);
        ts->a_end = ts->actions;
        if (ts->a_end) {
            while (ts->a_end->next)
                ts->a_end = ts->a_end->next;
        }
        ts->zdly_actions = ts->zdly_actions->purge(blk);
        ts->z_end = ts->zdly_actions;
        if (ts->z_end) {
            while (ts->z_end->next)
                ts->z_end = ts->z_end->next;
        }
        ts->nbau_actions = ts->nbau_actions->purge(blk);
        ts->n_end = ts->nbau_actions;
        if (ts->n_end) {
            while (ts->n_end->next)
                ts->n_end = ts->n_end->next;
        }
        ts->mon_actions = ts->mon_actions->purge(blk);
        ts->m_end = ts->mon_actions;
        if (ts->m_end) {
            while (ts->m_end->next)
                ts->m_end = ts->m_end->next;
        }
    }
}


// Diagnostic printout for current time slot
//
void
vl_timeslot::print(ostream &outs)
{
    outs << "time " << (int)time << '\n';
    for (vl_action_item *a = actions; a; a = a->next)
        a->print(outs);
    for (vl_action_item *a = zdly_actions; a; a = a->next)
        a->print(outs);
    for (vl_action_item *a = nbau_actions; a; a = a->next)
        a->print(outs);
}
// End vl_timeslot functions


vl_monitor::~vl_monitor()
{
    // delete args ?
    vl_context::destroy(cx);
}
// End vl_monitor functions


//---------------------------------------------------------------------------
//  Verilog description objects
//---------------------------------------------------------------------------

vl_desc::vl_desc()
{
    tstep = 1.0;
    modules = new lsList<vl_module*>;
    primitives = new lsList<vl_primitive*>;
    mp_st = new table<vl_mp*>;
    mp_undefined = set_empty();
    simulator = 0;
}


vl_desc::~vl_desc()
{
    if (simulator && simulator->description == this)
        simulator->description = 0;
    delete modules;  // modules deleted in mp_st
    delete primitives;
    if (mp_st) {
        table_gen<vl_mp*> gen(mp_st);
        vl_mp *mp;
        const char *key;
        while (gen.next(&key, &mp))
            delete mp;
        delete mp_st;
    }
    delete mp_undefined;
    simulator->var_factory.clear();
}


void
vl_desc::dump(ostream &outs)
{
    vl_module *mod;
    lsGen<vl_module*> gen(modules);
    while (gen.next(&mod))
        mod->dump(outs);
}


vl_gate_inst_list *
vl_desc::add_gate_inst(short t, vl_dlstr *dlstr, lsList<vl_gate_inst*> *g)
{
    // set up pointers to evaluation/setup functions
    lsGen<vl_gate_inst*> gen(g);
    vl_gate_inst *gate;
    while (gen.next(&gate))
        gate->set_type(t);
    return (new vl_gate_inst_list(t, dlstr, g));
}


vl_stmt *
vl_desc::add_mp_inst(vl_desc *desc, char *mp_name, vl_dlstr *dlstr,
    lsList<vl_mp_inst*> *instances)
{
    // if the module hasn't been parsed yet, create and add to symbol
    // table, and also to undefined list
    vl_mp *mod_pri;
    vl_stmt *retval = 0;
    if (!mp_st->lookup(mp_name, &mod_pri) || !mod_pri) {
        if (!desc->mp_undefined->set_find(mp_name))
            desc->mp_undefined->set_add(vl_strdup(mp_name));
        retval = new vl_mp_inst_list(MPundef, mp_name, dlstr, instances);
    }
    else {
        if (mod_pri->type == ModDecl) {
            retval = new vl_mp_inst_list(MPmod, mp_name, dlstr, instances);
            mod_pri->inst_count++;
        }
        else if (mod_pri->type == CombPrimDecl ||
                mod_pri->type == SeqPrimDecl) {
            retval = new vl_mp_inst_list(MPprim, mp_name, dlstr, instances);
            mod_pri->inst_count++;
        }
        else
            VP.error(ERR_COMPILE, "unknown module/primitive");
    }
    return (retval);
}
// End vl_desc functions


void
vl_module::dump(ostream &outs)
{
    int indnt = 0;
    outs << "Module " << name << ' ' << (void*)this << '\n';

    bool found = false;
    table_gen<vl_var*> sig_g(sig_st);
    vl_var *data;
    const char *vname;
    while (sig_g.next(&vname, &data)) {
        if (!found) {
            outs << "  Data symbols\n";
            found = true;
        }
        outs << "    " << vname << ' ' << (void*)data << '\n';
    }

    found = false;
    lsGen<vl_port*> pgen(ports);
    vl_port *p;
    while (pgen.next(&p)) {
        lsGen<vl_var*> vgen(p->port_exp);
        vl_var *v;
        while (vgen.next(&v)) {
            if (!found) {
                outs << "  Ports table\n";
                found = true;
            }
            outs << "    " << v->name << ' ' << (void*)v << '\n';
        }
    }

    found = false;
    table_gen<vl_inst*> inst_g(inst_st);
    vl_inst *inst;
    while (inst_g.next(&vname, &inst)) {
        if (!found) {
            outs << "  Instance table\n";
            found = true;
        }
        outs << "    " << vname << ' ' << (void*)inst << '\n';
    }

    found = false;
    table_gen<vl_task*> task_g(task_st);
    vl_task *task;
    while (task_g.next(&vname, &task)) {
        if (!found) {
            outs << "  Task table\n";
            found = true;
        }
        task->dump(outs, indnt + 2);
    }

    found = false;
    table_gen<vl_function*> func_g(func_st);
    vl_function *func;
    while (func_g.next(&vname, &func)) {
        if (!found) {
            outs << "  Function table\n";
            found = true;
        }
        func->dump(outs, indnt + 2);
    }

    found = false;
    table_gen<vl_stmt*> block_g(blk_st);
    vl_stmt *blk;
    while (block_g.next(&vname, &blk)) {
        if (!found) {
            outs << "  Block table\n";
            found = true;
        }
        blk->dump(outs, indnt + 2);
    }

    outs << '\n';

    lsGen<vl_stmt*> mgen(mod_items);
    vl_stmt *item;
    while (mgen.next(&item)) {
        if (item->type == ModPrimInst) {
            vl_mp_inst_list *list = (vl_mp_inst_list*)item;
            lsGen<vl_mp_inst*> mpgen(list->mps);
            vl_mp_inst *mpinst;
            while (mpgen.next(&mpinst)) {
                cout << "Instance " << mpinst->name << '\n';
                if (mpinst->master)
                    mpinst->master->dump(outs);
            }
        }
    }
}


// Sort the mod items into the following order:
//  vl_decl except Defparam
//  vl_mp_inst_list
//  vl_gate_list
//  vl_specify_block
//  vl_task
//  vl_function
//  vl_decl (Defparam)
//  vl_cont_assign
//  vl_procstmt
// This should (hopefully) ensure thing are defined when needed
//
void
vl_module::sort_moditems()
{
    if (!mod_items)
        return;
    lsList<vl_stmt*> *nlist = new lsList<vl_stmt*>;

    lsGen<vl_stmt*> gen(mod_items);
    vl_stmt *stmt;
    while (gen.next(&stmt)) {
        if (stmt->type >= RealDecl && stmt->type <= TriregDecl &&
                stmt->type != DefparamDecl)
            nlist->newEnd(stmt);
    }
    gen.reset(mod_items);
    while (gen.next(&stmt)) {
        if (stmt->type == ModPrimInst)
            nlist->newEnd(stmt);
    }
    gen.reset(mod_items);
    while (gen.next(&stmt)) {
        if (stmt->type >= AndGate && stmt->type <= Rtranif1Gate)
            nlist->newEnd(stmt);
    }
    gen.reset(mod_items);
    while (gen.next(&stmt)) {
        if (stmt->type == SpecBlock)
            nlist->newEnd(stmt);
    }
    gen.reset(mod_items);
    while (gen.next(&stmt)) {
        if (stmt->type == TaskDecl)
            nlist->newEnd(stmt);
    }
    gen.reset(mod_items);
    while (gen.next(&stmt)) {
        if (stmt->type >= IntFuncDecl && stmt->type <= RangeFuncDecl)
            nlist->newEnd(stmt);
    }
    gen.reset(mod_items);
    while (gen.next(&stmt)) {
        if (stmt->type == DefparamDecl)
            nlist->newEnd(stmt);
    }
    gen.reset(mod_items);
    while (gen.next(&stmt)) {
        if (stmt->type == ContAssign)
            nlist->newEnd(stmt);
    }
    gen.reset(mod_items);
    while (gen.next(&stmt)) {
        if (stmt->type == AlwaysStmt || stmt->type == InitialStmt)
            nlist->newEnd(stmt);
    }
    if (nlist->length() != mod_items->length()) {
        vl_error("internal, something strange in mod items list");
        vl_var::simulator->abort();
    }
    delete mod_items;
    mod_items = nlist;
}


// Start processing a module.  The module is a copy of a module obtained
// from the parser.  The copy function performs some crucial pre-setup,
// such as putting declared variables in the symbol table.
//
void
vl_module::setup(vl_simulator *sim)
{
    // If this is not a top-level module, do the #(...) parameter
    // overrides.  The parameters are in the delay struct 'params'
    // in the vl_mod_inst_list.  Do this before calling setup_list
    // so the parameter setting will be top-down
    //
    if (instance && instance->inst_list &&
            instance->inst_list->params_or_delays) {
        vl_delay *params = instance->inst_list->params_or_delays;
        if (params->delay1) {
            lsGen<vl_stmt*> gen(mod_items);
            vl_stmt *stmt;
            while (gen.next(&stmt)) {
                if (stmt->type == ParamDecl) {
                    vl_decl *decl = (vl_decl*)stmt;
                    lsGen<vl_bassign_stmt*> bgen(decl->list);
                    vl_bassign_stmt *bs;
                    if (bgen.next(&bs))
                        *bs->lhs = params->delay1->eval();
                    break;
                }
            }
        }
        else if (params->list) {
            lsGen<vl_expr*> gen(params->list);
            vl_expr *e;
            lsGen<vl_stmt*> sgen(mod_items);
            vl_stmt *stmt;
            while (sgen.next(&stmt)) {
                if (stmt->type == ParamDecl) {
                    vl_decl *decl = (vl_decl*)stmt;
                    lsGen<vl_bassign_stmt*> bgen(decl->list);
                    vl_bassign_stmt *bs;
                    while (bgen.next(&bs)) {
                        if (!gen.next(&e))
                            break;
                        *bs->lhs = e->eval();
                    }
                }
            }
        }
    }

    sim->context = sim->context->push(this);
    vl_setup_list(sim, mod_items);
    sim->context = sim->context->pop();
}
// End vl_module functions


void
vl_primitive::dump(ostream &outs)
{
    outs << "Primitive " << name << ' ' << (void*)this << '\n';

    bool found = false;
    table_gen<vl_var*> sig_g(sig_st);
    vl_var *data;
    const char *vname;
    while (sig_g.next(&vname, &data)) {
        if (!found) {
            outs << "  Data symbols\n";
            found = true;
        }
        outs << "    " << vname << ' ' << (void*)data << '\n';
    }

    found = false;
    lsGen<vl_port*> pgen(ports);
    vl_port *p;
    while (pgen.next(&p)) {
        lsGen<vl_var*> vgen(p->port_exp);
        vl_var *v;
        while (vgen.next(&v)) {
            if (!found) {
                outs << "  Ports table\n";
                found = true;
            }
            outs << "    " << v->name << ' ' << (void*)v << '\n';
        }
    }
    outs << '\n';
}


void
vl_primitive::setup(vl_simulator *sim)
{
    sim->context = sim->context->push(this);
    vl_setup_list(sim, decls);
    if (type == SeqPrimDecl && initial)
        initial->setup(sim);
    sim->context = sim->context->pop();
}


bool
vl_primitive::primtest(unsigned char t, bool isout)
{
    switch (t) {
    case PrimNone:
        return(true);
    case Prim0:
        return(true);
    case Prim1:
        return(true);
    case PrimX:
    case PrimB:
    case PrimQ:
        if (isout) {
            vl_error("%s not permitted in primitive output field", symbol(t));
            return (false);
        }
        return (true);
    case PrimM:
        if (type == CombPrimDecl) {
            vl_error("%d not permitted in combinational UDP", symbol(t));
            return (false);
        }
        if (!isout) {
            vl_error("%s not permitted in primitive input field", symbol(t));
            return (false);
        }
        return (true);
    case PrimR:
    case PrimF:
    case PrimP:
    case PrimN:
    case PrimS:
    case Prim0X:
    case Prim1X:
    case PrimX0:
    case PrimX1:
    case PrimXB:
    case PrimBX:
    case PrimBB:
    case PrimQ0:
    case PrimQ1:
    case PrimQB:
    case Prim0Q:
    case Prim1Q:
    case PrimBQ:
        if (type == CombPrimDecl) {
            vl_error("%d not permitted in combinational UDP", symbol(t));
            return (false);
        }
        if (isout) {
            vl_error("%s not permitted in primitive output field", symbol(t));
            return (false);
        }
    }
    return (true);
}
// End vl_primitive functions


//---------------------------------------------------------------------------
//  Module items
//---------------------------------------------------------------------------

void
vl_decl::setup(vl_simulator *sim)
{
    // setup_vars() has alrady been called, all vars are in symbol table
    if (type == DefparamDecl) {
        // This evaluation is deferred until now
        lsGen<vl_bassign_stmt*> gen(list);
        vl_bassign_stmt *assign;
        while (gen.next(&assign)) {
            vl_var *v = sim->context->lookup_var(assign->lhs->name, false);
            if (!v) {
                vl_error("can not resolve defparam %s", assign->lhs->name);
                sim->abort();
            }
            else {
                if (v != assign->lhs) {
                    delete assign->lhs;
                    assign->lhs = v;
                }
                assign->flags |= BAS_SAVE_LHS;
                *assign->lhs = assign->rhs->eval();
            }
        }
        return;
    }
    if (type == ParamDecl)
        return;
    if (list) {
        // declared with assignment
        lsGen<vl_bassign_stmt*> agen(list);
        vl_bassign_stmt *bs;
        while (agen.next(&bs)) {
            bs->lhs->strength = strength;
            // set up implicit continuous assignment
            bs->wait = delay;
            bs->rhs->chain(bs);
            bs->setup(sim);
        }
    }
}
// End vl_decl functions


void
vl_procstmt::setup(vl_simulator *sim)
{
    if (stmt)
        sim->timewheel->append(sim->time,
            new vl_action_item(this, sim->context));
    lasttime = (vl_time_t)-1;
}


EVtype
vl_procstmt::eval(vl_event*, vl_simulator *sim)
{
    if (type == AlwaysStmt) {
        // If the statement is one of the following, do one loop per time
        // point rather than looping infinitely

        bool single_pass = false;
        switch (stmt->type) {
        case SendEventStmt:
        case BassignStmt:
        case NbassignStmt:
        case DelayBassignStmt:
        case EventBassignStmt:
        case DelayNbassignStmt:
        case EventNbassignStmt:
        case AssignStmt:
        case ForceStmt:
        case SysTaskEnableStmt:
        case DisableStmt:
        case DeassignStmt:
        case ReleaseStmt:
            single_pass = true;
            break;
        default:
            break;
        }

        if (lasttime != sim->time || !single_pass) {
            // have to loop around when finished
            lasttime = sim->time;
            stmt->setup(sim);
            sim->timewheel->append(sim->time,
                new vl_action_item(this, sim->context));
        }
        else {
            // enable another run-through at next time point
            vl_action_item *anew = new vl_action_item(this, sim->context);
            if (!sim->next_actions)
                sim->next_actions = anew;
            else {
                for (vl_action_item *a = sim->next_actions; a; a = a->next) {
                    if (a->stmt && a->stmt == anew->stmt) {
                        delete anew;
                        break;
                    }
                    if (!a->next) {
                        a->next = anew;
                        break;
                    }
                }
            }
        }
    }
    else if (type == InitialStmt)
        stmt->setup(sim);
    return (EVnone);
}
// End vl_procstmt functions


void
vl_cont_assign::setup(vl_simulator *sim)
{
    // this is a "mod item", applies only to nets
    vl_bassign_stmt *stmt;
    lsGen<vl_bassign_stmt*> gen(assigns);
    while (gen.next(&stmt)) {
        stmt->setup(sim);
        if (!stmt->lhs->check_net_type(REGwire)) {
            vl_error("continuous assignment to non-net %s",
                stmt->lhs->name ? stmt->lhs->name : "concatenation");
            sim->abort();
            return;
        }
        // The drive strength is stored in the vl_var part of the
        // driving expression, since it is unique to this assignment
        stmt->rhs->strength = strength;
        if (delay)
            stmt->wait = delay;
        stmt->rhs->chain(stmt);
    }
}
// End vl_cont_assign functions


void
vl_task::disable(vl_stmt *s)
{
    vl_disable_list(stmts, s);
}


void
vl_task::dump(ostream &outs, int indnt)
{
    char buf[128];
    for (int i = 0; i < 2*indnt; i++)
        buf[i] = ' ';
    buf[2*indnt] = 0;

    outs << buf << name << " " << (void*)this << '\n';
    bool found = false;
    table_gen<vl_var*> sig_g(sig_st);
    vl_var *data;
    const char *vname;
    while (sig_g.next(&vname, &data)) {
        if (!found) {
            outs << buf << "  Data symbols\n";
            found = true;
        }
        outs << buf << "    " << vname << ' ' << (void*)data << '\n';
    }

    found = false;
    table_gen<vl_stmt*> block_g(blk_st);
    vl_stmt *blk;
    while (block_g.next(&vname, &blk)) {
        if (!found) {
            outs << buf << "  Block table\n";
            found = true;
        }
        blk->dump(outs, indnt+2);
    }
}
// End vl_task functions


// This is called when a FuncExpr is encountered, takes care of function
// evaluation.  The function is evaluated completely here, i.e., the
// return value is saved in outport
//
void
vl_function::eval_func(vl_var *out, lsList<vl_expr*> *args)
{
    if (!sig_st)
        sig_st = new table<vl_var*>;
    vl_var *outvar;
    if (!sig_st->lookup(name, &outvar)) {
        outvar = new vl_var;
        outvar->name = vl_strdup(name);
        if (type == RangeFuncDecl)
            outvar->configure(range, RegDecl);
        else if (type == RealFuncDecl)
            outvar->configure(0, RealDecl);
        else if (type == IntFuncDecl)
            outvar->configure(0, IntDecl);
        else
            outvar->configure(0, RegDecl);
        sig_st->insert(outvar->name, outvar);
        outvar->flags |= VAR_IN_TABLE;
    }

    vl_simulator *sim = outvar->simulator;
    vl_action_item *atmp = sim->timewheel->actions;
    sim->timewheel->actions = 0;

    sim->context = sim->context->push(this);
    vl_setup_list(sim, decls);
    sim->context = sim->context->pop();

    lsGen<vl_expr*> agen(args);
    lsGen<vl_decl*> dgen(decls);
    vl_expr *ae;
    vl_decl *decl;
    bool done = false;
    while (dgen.next(&decl)) {
        lsGen<vl_var*> vgen(decl->ids);
        vl_var *av;
        while (vgen.next(&av)) {
            if (av->net_type >= REGwire)
                av->net_type = REGreg;
            if (agen.next(&ae)) {
                if (av->io_type == IOoutput || av->io_type == IOinout) {
                    vl_error("function %s can not have output port %s",
                        name, av->name);
                    sim->abort();
                    continue;
                }
                if (av->io_type == IOinput) {
                    vl_bassign_stmt *bs =
                        new vl_bassign_stmt(BassignStmt, av, 0, 0, ae);
                    bs->flags |= (SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
                    vl_action_item *a = new vl_action_item(bs, sim->context);
                    a->flags |= AI_DEL_STMT;
                    sim->timewheel->append(sim->time, a);
                }
                else {
                    vl_warn("too many args to function %s", name);
                    done = true;
                    break;
                }
            }
            else {
                if (av->io_type == IOinput)
                    vl_warn("too few args to function %s", name);
                done = true;
                break;
            }
        }
        if (done)
            break;
    }
    sim->context = sim->context->push(this);
    if (stmts)
        vl_setup_list(sim, stmts);
    while (sim->timewheel->actions)
        sim->timewheel->do_actions(sim);
    sim->timewheel->actions = atmp;
    sim->context = sim->context->pop();
    *out = *outvar;
}


void
vl_function::dump(ostream &outs, int indnt)
{
    char buf[128];
    for (int i = 0; i < 2*indnt; i++)
        buf[i] = ' ';
    buf[2*indnt] = 0;

    outs << buf << name << " " << (void*)this << '\n';
    bool found = false;
    table_gen<vl_var*> sig_g(sig_st);
    vl_var *data;
    const char *vname;
    while (sig_g.next(&vname, &data)) {
        if (!found) {
            outs << buf << "  Data symbols\n";
            found = true;
        }
        outs << buf << "    " << vname << ' ' << (void*)data << '\n';
    }

    found = false;
    table_gen<vl_stmt*> block_g(blk_st);
    vl_stmt *blk;
    while (block_g.next(&vname, &blk)) {
        if (!found) {
            outs << buf << "  Block table\n";
            found = true;
        }
        blk->dump(outs, indnt+2);
    }
}
// End vl_function functions


void
vl_gate_inst_list::setup(vl_simulator *sim)
{
    vl_setup_list(sim, gates);
}
// End vl_gate_inst_list functions


void
vl_mp_inst_list::setup(vl_simulator *sim)
{
    if (mptype == MPundef) {
        vl_mp *mp;
        if (!sim->description->mp_st->lookup(name, &mp) || !mp) {
            vl_error("no master found for %s", name);
            sim->abort();
            return;
        }
        if (mp->type == ModDecl)
            mptype = MPmod;
        else
            mptype = MPprim;
    }
    vl_setup_list(sim, mps);
}
// End vl_mp_inst_list functions


//---------------------------------------------------------------------------
//  Statements
//---------------------------------------------------------------------------

void
vl_bassign_stmt::setup(vl_simulator *sim)
{
    if (lhs->name && lhs->data_type == Dnone && lhs->net_type == REGnone) {
        if (!sim->context) {
            vl_error("internal, no current context!");
            sim->abort();
            return;
        }
        vl_var *nvar = sim->context->lookup_var(lhs->name, false);
        if (!nvar) {
            vl_warn("implicit declaration of %s", lhs->name);
            vl_module *cmod = sim->context->currentModule();
            if (cmod) {
                nvar = new vl_var;
                nvar->name = vl_strdup(lhs->name);
                if (!cmod->sig_st)
                    cmod->sig_st = new table<vl_var*>;
                cmod->sig_st->insert(nvar->name, nvar);
                nvar->flags |= VAR_IN_TABLE;
            }
            else {
                vl_error("internal, no current module!");
                sim->abort();
                return;
            }
        }
        if (nvar != lhs) {
            if (strcmp(nvar->name, lhs->name))
                // from another module, don't free it!
                flags |= BAS_SAVE_LHS;
            vl_var *olhs = lhs;
            lhs = nvar;
            range = olhs->range;
            olhs->range = 0;
            delete olhs;
        }
    }
    if (lhs->flags & VAR_IN_TABLE)
        flags |= BAS_SAVE_LHS;
    if (lhs->net_type >= REGwire && lhs->delay)
        wait = lhs->delay;
    sim->timewheel->append(sim->time,
        new vl_action_item(this, sim->context));
}


EVtype
vl_bassign_stmt::eval(vl_event *ev, vl_simulator *sim)
{
    // The wait field is lhs->delay() for wires, and is set only if
    // the vl_bassign_stmt::setup() function is called, which is only
    // true for assigns that are original text statements.  The
    // assigns that are created due to delays and events and queued in
    // the actions list never call setup.
    //
    // Since wires receiving an assignment will save the assigning
    // variable in the driver list, the drivers must not be freed. 
    // The tmpvar is used for this purpose.  This variable is created
    // when necessary, and passed as the rhs of spawned assigns.

    if (type == AssignStmt) {
        if (!lhs->check_net_type(REGreg)) {
            vl_error("non-reg %s in procedural continuous assign",
                lhs->name ? lhs->name : "in concatenation");
            sim->abort();
            return (EVnone);
        }
        // procedural continuous assign 
        if (range)
            vl_warn("bit or part select in procedural continuous "
                "assignment ignored");
        lhs->set_assigned(this);
    }
    else if (type == ForceStmt) {
        if (range)
            vl_warn("bit or part select in force assignment ignored");
        lhs->set_forced(this);
    }
    else if (type == BassignStmt) {
        // a = ( )
        if (wait) {
            // Delayed continuous assignment, wait time td before
            // evaluating and assigning rhs
            //
            vl_bassign_stmt *bs =
                new vl_bassign_stmt(BassignStmt, lhs, 0, 0, rhs);
            bs->flags |= (SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
            bs->freeze_indices(range);
            vl_action_item *a = new vl_action_item(bs, sim->context);
            a->flags |= AI_DEL_STMT;
            vl_time_t td = wait->eval();
            if (td > 0)
                sim->timewheel->append(sim->time + td, a);
            else
                sim->timewheel->append_zdly(sim->time, a);
        }
        else
            lhs->assign(range, &rhs->eval(), 0);
    }
    else if (type == DelayBassignStmt) {
        // a = #xx ( )
        // Delayed assignment, compute the present rhs value and save
        // it in a temp variable.  Apply the value after the delay.
        // The next statement executes after the delay.
        //
        if (!tmpvar)
            tmpvar = new vl_var;
        *tmpvar = rhs->eval();
        vl_bassign_stmt *bs =
            new vl_bassign_stmt(BassignStmt, lhs, 0, 0, tmpvar);
        bs->flags |= (SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
        bs->wait = wait;  // propagate wire delay
        bs->freeze_indices(range);
        vl_action_item *a = new vl_action_item(bs, sim->context);
        a->flags |= AI_DEL_STMT;
        vl_time_t td = delay->eval();
        sim->timewheel->append(sim->time, a);
        ev->time = sim->time + td;
        return (EVdelay);
    }
    else if (type == EventBassignStmt) {
        // a = @( )
        // Event-triggered assignment, compute the rhs value and save
        // it in a temp variable.  Apply the value when triggered.  The
        // next statement executes after trigger
        //
        if (!tmpvar)
            tmpvar = new vl_var;
        *tmpvar = rhs->eval();
        vl_bassign_stmt *bs =
            new vl_bassign_stmt(BassignStmt, lhs, 0, 0, tmpvar);
        bs->flags |= (SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
        bs->wait = wait;  // propagate wire delay
        bs->freeze_indices(range);
        vl_action_item *a = new vl_action_item(bs, sim->context);
        a->flags |= AI_DEL_STMT;
        sim->timewheel->append(sim->time, a);
        ev->event = event;
        return (EVevent);
    }
    else if (type == NbassignStmt) {
        // a <= ( )
        // Non-blocking assignment, compute the rhs and save it in a
        // temp variable.  Apply the value after other events have been
        // processed, by appending the action to the nbau list
        //
        vl_var *src;
        if (flags & SIM_INTERNAL)
            // This was spawned by a DelayBassign or EventBassign, use
            // the parent's temp variable.  Since the rhs may be added
            // to the lhs driver list (for nets), we have to drive with
            // a variable that isn't freed
            src = rhs;
        else {
            if (!tmpvar)
                tmpvar = new vl_var;
            *tmpvar = rhs->eval();
            src = tmpvar;
        }
        vl_time_t td;
        if (wait && (td = wait->eval()) > 0) {
            vl_bassign_stmt *bs =
                new vl_bassign_stmt(NbassignStmt, lhs, 0, 0, src);
            bs->flags |= (SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
            vl_time_t ttd = wait->eval();
            bs->freeze_indices(range);
            vl_action_item *a = new vl_action_item(bs, sim->context);
            a->flags |= AI_DEL_STMT;
            sim->timewheel->append(sim->time + ttd, a);
        }
        else {
            vl_bassign_stmt *bs =
                new vl_bassign_stmt(BassignStmt, lhs, 0, 0, src);
            bs->flags |= (SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
            bs->freeze_indices(range);
            vl_action_item *a = new vl_action_item(bs, sim->context);
            a->flags |= AI_DEL_STMT;
            sim->timewheel->append_nbau(sim->time, a);
        }
    }
    else if (type == DelayNbassignStmt) {
        // a <= #xx ( )
        // Delayed non-blocking assignment, compute the rhs and save it in
        // a temporary variable.  Spawn a regular non-blocking assignment
        // after the delay
        //
        if (!tmpvar)
            tmpvar = new vl_var;
        *tmpvar = rhs->eval();
        vl_time_t td = delay->eval();
        if (td) {
            vl_bassign_stmt *bs =
                new vl_bassign_stmt(NbassignStmt, lhs, 0, 0, tmpvar);
            bs->flags |= (SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
            bs->wait = wait;  // propagate wire delay
            bs->freeze_indices(range);
            vl_action_item *a = new vl_action_item(bs, sim->context);
            a->flags |= AI_DEL_STMT;
            sim->timewheel->append(sim->time + td, a);
        }
        else {
            vl_bassign_stmt *bs =
                new vl_bassign_stmt(BassignStmt, lhs, 0, 0, tmpvar);
            bs->flags |= (SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
            bs->wait = wait;  // propagate wire delay
            bs->freeze_indices(range);
            vl_action_item *a = new vl_action_item(bs, sim->context);
            a->flags |= AI_DEL_STMT;
            sim->timewheel->append_nbau(sim->time, a);
        }
    }
    else if (type == EventNbassignStmt) {
        // a <= @( )
        // Event-triggered non-blocking assignment, compute the rhs and
        // save it in a temp variable.  Spawn a regular non-blocking
        // assignment when triggered
        //
        if (!tmpvar)
            tmpvar = new vl_var;
        *tmpvar = rhs->eval();
        vl_bassign_stmt *bs =
            new vl_bassign_stmt(NbassignStmt, lhs, 0, 0, tmpvar);
        bs->flags |= (SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
        bs->wait = wait;  // propagate wire delay
        bs->freeze_indices(range);
        if (event && event->repeat)
            event->count = (int)event->repeat->eval();
        vl_action_item *a = new vl_action_item(bs, sim->context);
        a->flags |= AI_DEL_STMT;
        a->event = event;
        event->chain(a);
    }
    return (EVnone);
}


// For a delayed assignment, evaluate any indices used in the lhs.  The
// range is the quantity that would normally be copied to the stmt
//
void
vl_bassign_stmt::freeze_indices(vl_range *rng)
{
    rng->eval(&range);
    if (lhs->data_type == Dconcat) {
        lhs = lhs->copy();
        lhs->freeze_concat();
        flags &= ~BAS_SAVE_LHS;
    }
}
// End vl_bassign_stmt functions


void
vl_sys_task_stmt::setup(vl_simulator *sim)
{
    if (flags & SYSafter)
        sim->timewheel->append_mon(sim->time, new vl_action_item(this,
            sim->context));
    else
        sim->timewheel->append(sim->time, new vl_action_item(this,
            sim->context));
}


EVtype
vl_sys_task_stmt::eval(vl_event*, vl_simulator *sim)
{
    (sim->*this->action)(this, args);
    return (EVnone);
}
// End vl_sys_task_stmt functions


void
vl_begin_end_stmt::setup(vl_simulator *sim)
{
    sim->context = sim->context->push(this);
    vl_setup_list(sim, decls);
    sim->timewheel->append(sim->time, new vl_action_item(this, sim->context));
    sim->context = sim->context->pop();
}


EVtype
vl_begin_end_stmt::eval(vl_event*, vl_simulator *sim)
{
    vl_setup_list(sim, stmts);
    return (EVnone);
}


void
vl_begin_end_stmt::disable(vl_stmt *s)
{
    // If the last stmt is a vl_fj_break, have to notify of termination
    vl_stmt *fj;
    stmts->lastItem(&fj);
    if (fj && fj->type == ForkJoinBreak)
        fj->setup(vl_var::simulator);
    vl_disable_list(stmts, s);
}


void
vl_begin_end_stmt::dump(ostream &outs, int indnt)
{
    char buf[128];
    for (int i = 0; i < 2*indnt; i++)
        buf[i] = ' ';
    buf[2*indnt] = 0;

    outs << buf << name << " (begin/end) " << (void*)this << '\n';
    bool found = false;
    table_gen<vl_var*> sig_g(sig_st);
    vl_var *data;
    const char *vname;
    while (sig_g.next(&vname, &data)) {
        if (!found) {
            outs << buf << "  Data symbols\n";
            found = true;
        }
        outs << buf << "    " << vname << ' ' << (void*)data << '\n';
    }

    found = false;
    table_gen<vl_stmt*> block_g(blk_st);
    vl_stmt *blk;
    while (block_g.next(&vname, &blk)) {
        if (!found) {
            outs << buf << "  Block table\n";
            found = true;
        }
        blk->dump(outs, indnt+2);
    }
}
// End vl_begin_end_stmt functions


void
vl_if_else_stmt::setup(vl_simulator *sim)
{
    if (cond)
        sim->timewheel->append(sim->time,
            new vl_action_item(this, sim->context));
}


EVtype
vl_if_else_stmt::eval(vl_event*, vl_simulator *sim)
{
    if ((int)cond->eval()) {
        if (if_stmt)
            if_stmt->setup(sim);
    }
    else {
        if (else_stmt)
            else_stmt->setup(sim);
    }
    return (EVnone);
}


void
vl_if_else_stmt::disable(vl_stmt *s)
{
    if (if_stmt)
        if_stmt->disable(s);
    if (else_stmt)
        else_stmt->disable(s);
}
// End vl_if_else_stmt functions


void
vl_case_stmt::setup(vl_simulator *sim)
{
    sim->timewheel->append(sim->time,
        new vl_action_item(this, sim->context));
}


EVtype
vl_case_stmt::eval(vl_event*, vl_simulator *sim)
{
    vl_var d = cond->eval();
    lsGen<vl_case_item*> gen(case_items);
    vl_case_item *item;
    while (gen.next(&item)) {
        if (item->type == CaseItem) {
            lsGen<vl_expr*> egen(item->exprs);
            vl_expr *e;
            while (egen.next(&e)) {
                vl_var z;
                if (type == CaseStmt)
                    z = case_eq(d, e->eval());
                else if (type == CasexStmt)
                    z = casex_eq(d, e->eval());
                else if (type == CasezStmt)
                    z = casez_eq(d, e->eval());
                else {
                    vl_error("(internal) unknown case type");
                    sim->abort();
                    return (EVnone);
                }
                if (z.u.s[0] == BitH) {
                    if (item->stmt)
                        item->stmt->setup(sim);
                    return (EVnone);
                }
            }
        }
    }
    gen = lsGen<vl_case_item*>(case_items);
    while (gen.next(&item)) {
        if (item->type == DefaultItem && item->stmt)
            item->stmt->setup(sim);
    }
    return (EVnone);
}


void
vl_case_stmt::disable(vl_stmt *s)
{
    vl_disable_list(case_items, s);
}


void
vl_case_item::disable(vl_stmt *s)
{
    if (stmt)
        stmt->disable(s);
}
// End vl_case_stmt functions


void
vl_forever_stmt::setup(vl_simulator *sim)
{
    if (stmt)
        sim->timewheel->append(sim->time,
            new vl_action_item(this, sim->context));
}


EVtype
vl_forever_stmt::eval(vl_event*, vl_simulator *sim)
{
    stmt->setup(sim);
    sim->timewheel->append(sim->time,
        new vl_action_item(this, sim->context));
    return (EVnone);
}


void
vl_forever_stmt::disable(vl_stmt *s)
{
    if (stmt)
        stmt->disable(s);
}
// End vl_forever_stmt functions


void
vl_repeat_stmt::setup(vl_simulator *sim)
{
    cur_count = -1;
    if (count && stmt)
        sim->timewheel->append(sim->time,
            new vl_action_item(this, sim->context));
}


EVtype
vl_repeat_stmt::eval(vl_event*, vl_simulator *sim)
{
    if (cur_count < 0)
        cur_count = (int)count->eval();
    if (cur_count > 0) {
        stmt->setup(sim);
        cur_count--;
        if (cur_count > 0)
            sim->timewheel->append(sim->time,
                new vl_action_item(this, sim->context));
    }
    return (EVnone);
}


void
vl_repeat_stmt::disable(vl_stmt *s)
{
    if (stmt)
        stmt->disable(s);
}
// End vl_repeat_stmt functions


void
vl_while_stmt::setup(vl_simulator *sim)
{
    sim->timewheel->append(sim->time,
        new vl_action_item(this, sim->context));
}


EVtype
vl_while_stmt::eval(vl_event*, vl_simulator *sim)
{
    if (!cond || (int)cond->eval()) {
        if (stmt)
            stmt->setup(sim);
        sim->timewheel->append(sim->time,
            new vl_action_item(this, sim->context));
    }
    return (EVnone);
}


void
vl_while_stmt::disable(vl_stmt *s)
{
    if (stmt)
        stmt->disable(s);
}
// End vl_while_stmt functions


void
vl_for_stmt::setup(vl_simulator *sim)
{
    if (initial)
        initial->setup(sim);
    sim->timewheel->append(sim->time,
        new vl_action_item(this, sim->context));
}


EVtype
vl_for_stmt::eval(vl_event*, vl_simulator *sim)
{
    if (!cond || (int)cond->eval()) {
        if (stmt)
            stmt->setup(sim);
        if (end)
            end->setup(sim);
        sim->timewheel->append(sim->time,
            new vl_action_item(this, sim->context));
    }
    return (EVnone);
}


void
vl_for_stmt::disable(vl_stmt *s)
{
    if (stmt)
        stmt->disable(s);
}
// End vl_for_stmt functions


void
vl_delay_control_stmt::setup(vl_simulator *sim)
{
    sim->timewheel->append(sim->time,
        new vl_action_item(this, sim->context));
}


EVtype
vl_delay_control_stmt::eval(vl_event *ev, vl_simulator *sim)
{
    ev->time = sim->time + delay->eval();
    if (stmt)
        stmt->setup(sim);
    return (EVdelay);
}


void
vl_delay_control_stmt::disable(vl_stmt *s)
{
    if (stmt)
        stmt->disable(s);
}
// End vl_delay_control_stmt functions


void
vl_event_control_stmt::setup(vl_simulator *sim)
{
    if (!event)
        return;
    sim->timewheel->append(sim->time,
        new vl_action_item(this, sim->context));
}


EVtype
vl_event_control_stmt::eval(vl_event *ev, vl_simulator *sim)
{
    if (stmt)
        stmt->setup(sim);
    ev->event = event;
    return (EVevent);
}


void
vl_event_control_stmt::disable(vl_stmt *s)
{
    event->unchain_disabled(s);
    if (stmt)
        stmt->disable(s);
}
// End vl_event_control_stmt functions


void
vl_wait_stmt::setup(vl_simulator *sim)
{
    if (!cond)
        return;
    sim->timewheel->append(sim->time,
        new vl_action_item(this, sim->context));
}


EVtype
vl_wait_stmt::eval(vl_event *ev, vl_simulator *sim)
{
    if (stmt)
        stmt->setup(sim);
    if ((int)cond->eval())
        return (EVnone);
    if (!event)
        event = new vl_event_expr(LevelEventExpr, cond);
    ev->event = event;
    return (EVevent);
}


void
vl_wait_stmt::disable(vl_stmt *s)
{
    if (event)
        event->unchain_disabled(s);
    if (stmt)
        stmt->disable(s);
}
// End vl_wait_stmt functions


void
vl_send_event_stmt::setup(vl_simulator *sim)
{
    sim->timewheel->append(sim->time, new vl_action_item(this, sim->context));
}


EVtype
vl_send_event_stmt::eval(vl_event*, vl_simulator *sim)
{
    vl_var *d = sim->context->lookup_var(name, false);
    if (!d) {
        vl_error("send-event %s not found", name);
        sim->abort();
        return (EVnone);
    }
    if (d->net_type != REGevent) {
        vl_error("send-event %s is not an event", name);
        sim->abort();
        return (EVnone);
    }
    d->trigger();
    return (EVnone);
}
// End vl_send_event_stmt functions


void
vl_fork_join_stmt::setup(vl_simulator *sim)
{
    endcnt = 0;
    sim->context = sim->context->push(this);
    vl_setup_list(sim, decls);
    sim->timewheel->append(sim->time, new vl_action_item(this, sim->context));
    sim->context = sim->context->pop();
}


EVtype
vl_fork_join_stmt::eval(vl_event*, vl_simulator *sim)
{
    if (stmts) {
        lsGen<vl_stmt*> gen(stmts);
        vl_stmt *stmt;
        while (gen.next(&stmt)) {
            if (stmt->type == BeginEndStmt) {
                vl_begin_end_stmt *be = (vl_begin_end_stmt*)stmt;
                vl_stmt *st;
                be->stmts->lastItem(&st);
                if (st->type != ForkJoinBreak) {
                    vl_fj_break *bk = new vl_fj_break(this, be);
                    be->stmts->newEnd(bk);
                }
                be->setup(sim);
            }
            else {
                lsList<vl_stmt*> *lst = new lsList<vl_stmt*>;
                lst->newEnd(stmt);
                vl_begin_end_stmt *be = new vl_begin_end_stmt(0, 0, lst);
                vl_fj_break *bk = new vl_fj_break(this, be);
                lst->newEnd(bk);
                be->setup(sim);
                stmts->replace(stmt, be);
            }
            endcnt++;
        }
    }
    return (EVnone);
}


void
vl_fork_join_stmt::disable(vl_stmt *s)
{
    vl_disable_list(stmts, s);
}


void
vl_fork_join_stmt::dump(ostream &outs, int indnt)
{
    char buf[128];
    for (int i = 0; i < 2*indnt; i++)
        buf[i] = ' ';
    buf[2*indnt] = 0;

    outs << buf << name << " (fork/join) " << (void*)this << '\n';

    bool found = false;
    table_gen<vl_var*> sig_g(sig_st);
    vl_var *data;
    const char *vname;
    while (sig_g.next(&vname, &data)) {
        if (!found) {
            outs << buf << "  Data symbols\n";
            found = true;
        }
        outs << buf << "    " << vname << ' ' << (void*)data << '\n';
    }

    found = false;
    table_gen<vl_stmt*> block_g(blk_st);
    vl_stmt *blk;
    while (block_g.next(&vname, &blk)) {
        if (!found) {
            outs << buf << "  Block table\n";
            found = true;
        }
        blk->dump(outs, indnt+2);
    }
}
// End vl_fork_join_stmt functions


void
vl_fj_break::setup(vl_simulator *sim)
{
    sim->timewheel->append(sim->time, new vl_action_item(this, sim->context));
}


// This is evaluated after each fork/join thread completes.  Run
// through the list of fork/join return contexts to find the right one
// and decrement its thread counter.  When the thread count reaches 0,
// put the context on the queue if it has a stack.  The stack may not
// be set yet, in which case just return
//
EVtype
vl_fj_break::eval(vl_event*, vl_simulator *sim)
{
    vl_action_item *ap = 0, *an;
    for (vl_action_item *a = sim->fj_end; a; a = an) {
        an = a->next;
        if (a->stmt == fjblock) {
            vl_fork_join_stmt *fj = (vl_fork_join_stmt*)a->stmt;
            fj->endcnt--;
            if (fj->endcnt <= 0) {
                if (a->stack) {
                    if (!ap)
                        sim->fj_end = an;
                    else
                        ap->next = an;
                    a->stmt = 0;
                    sim->timewheel->append(sim->time, a);
                }
                break;
            }
            ap = a;
        }
    }
    return (EVnone);
}


void
vl_task_enable_stmt::setup(vl_simulator *sim)
{
    if (!task) {
        task = sim->context->lookup_task(name);
        if (!task) {
            vl_error("can't find task %s", name);
            sim->abort();
            return;
        }
    }

    // Have to be careful with context switch, task name can have
    // prepended path
    vl_context cvar;
    const char *n = name;
    if (!sim->context->resolve_cx(&n, cvar, false))
        cvar = *sim->context;
    vl_context *tcx = sim->context;
    sim->context = &cvar;

    sim->context = sim->context->push(task);
    vl_setup_list(sim, task->decls);
    sim->context = sim->context->pop();

    // setup ports
    lsGen<vl_expr*> agen(args);
    lsGen<vl_decl*> dgen(task->decls);
    vl_expr *ae;
    vl_decl *decl;
    bool done = false;
    while (dgen.next(&decl)) {
        lsGen<vl_var*> vgen(decl->ids);
        vl_var *av;
        while (vgen.next(&av)) {
            if (av->net_type >= REGwire)
                av->net_type = REGreg;
            if (agen.next(&ae)) {
                if (av->io_type == IOinput || av->io_type == IOinout) {
                    // set formal = actual
                    vl_bassign_stmt *bs =
                        new vl_bassign_stmt(BassignStmt, av, 0, 0, ae);
                    bs->flags |= (SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
                    vl_action_item *a = new vl_action_item(bs, sim->context);
                    a->flags |= AI_DEL_STMT;
                    sim->timewheel->append(sim->time, a);
                }
                if (av->io_type == IOnone) {
                    vl_warn("too many args to task %s", task->name);
                    done = true;
                    break;
                }
            }
            else {
                if (av->io_type != IOnone)
                    vl_warn("too few args to task %s", task->name);
                done = true;
                break;
            }
        }
        if (done)
            break;
    }

    sim->context = sim->context->push(task);
    sim->timewheel->append(sim->time,
        new vl_action_item(this, sim->context));
    sim->context = sim->context->pop();

    agen = lsGen<vl_expr*>(args);
    dgen = lsGen<vl_decl*>(task->decls);
    done = false;
    while (dgen.next(&decl)) {
        lsGen<vl_var*> vgen(decl->ids);
        vl_var *av;
        while (vgen.next(&av)) {
            if (agen.next(&ae)) {
                if (av->io_type == IOoutput || av->io_type == IOinout) {
                    vl_var *vo = ae->source();
                    if (!vo) {
                        vl_error("internal, variable not found in expr");
                        sim->abort();
                        sim->context = tcx;
                        return;
                    }
                    // set actual = formal
                    vl_bassign_stmt *bs =
                        new vl_bassign_stmt(BassignStmt, vo, 0, 0, av);
                    bs->flags |= (SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
                    bs->range = ae->source_range()->copy();
                    vl_action_item *a = new vl_action_item(bs, sim->context);
                    a->flags |= AI_DEL_STMT;
                    sim->timewheel->append(sim->time, a);
                }
                if (av->io_type == IOnone) {
                    done = true;
                    break;
                }
            }
            else {
                done = true;
                break;
            }
        }
        if (done)
            break;
    }
    sim->context = tcx;
}


EVtype
vl_task_enable_stmt::eval(vl_event*, vl_simulator *sim)
{
    if (task)
        vl_setup_list(sim, task->stmts);
    return (EVnone);
}
// End vl_task_enable_stmt functions


void
vl_disable_stmt::setup(vl_simulator *sim)
{
    sim->timewheel->append(sim->time,
        new vl_action_item(this, sim->context));
}


EVtype
vl_disable_stmt::eval(vl_event*, vl_simulator *sim)
{
    if (!name) {
        if (sim->context->block)
            target = sim->context->block;
        else if (sim->context->fjblk)
            target = sim->context->fjblk;
        else if (sim->context->task)
            target = sim->context->task;
    }
    else {
        vl_stmt *s = sim->context->lookup_block(name);
        if (s) {
            if (s->type == BeginEndStmt || s->type == ForkJoinStmt)
                target = s;
            return (EVnone);
        }
        vl_task *t = sim->context->lookup_task(name);
        if (t) {
            target = t;
            return (EVnone);
        }
        vl_error("in disable, block/task %s not found", name);
        sim->abort();
    }
    return (EVnone);
}
// End vl_disable_stmt functions


void
vl_deassign_stmt::setup(vl_simulator *sim)
{
    sim->timewheel->append(sim->time,
        new vl_action_item(this, sim->context));
}


EVtype
vl_deassign_stmt::eval(vl_event*, vl_simulator*)
{
    if (type == DeassignStmt)
        lhs->set_assigned(0);
    else if (type == ReleaseStmt)
        lhs->set_forced(0);
    return (EVnone);
}
// End vl_deassign_stmt functions


//---------------------------------------------------------------------------
//  Instances
//---------------------------------------------------------------------------

void
vl_gate_inst::setup(vl_simulator *sim)
{
    if (!terms) {
        vl_error("too few terminals for gate %s", name);
        sim->abort();
        return;
    }
    if (!gsetup) {
        vl_error("(internal) gate not initialized");
        sim->abort();
        return;
    }
    if (!(*gsetup)(sim, this))
        sim->abort();
}


EVtype
vl_gate_inst::eval(vl_event*, vl_simulator *sim)
{
    if (!geval) {
        vl_error("(internal) gate not initialized");
        sim->abort();
        return (EVnone);
    }
    if (!(*geval)(sim, gset, this))
        // error
        sim->abort();
    return (EVnone);
}
// End vl_gate_inst functions


void
vl_mp_inst::setup(vl_simulator *sim)
{
    if (inst_list) {
        if (inst_list->mptype == MPundef) {
            vl_error("no master found for %s", inst_list->name);
            sim->abort();
            return;
        }
        if (!master) {
            vl_mp *mp;
            if (!sim->description->mp_st->lookup(inst_list->name, &mp) ||
                    !mp) {
                vl_error("no master found for %s", inst_list->name);
                sim->abort();
                return;
            }
            mp->inst_count++;
            mp = mp->copy();
            mp->instance = this;
            master = mp;
            sim->abort();
            return;
        }
        master->setup(sim);
        link_ports(sim);
    }
}


EVtype
vl_mp_inst::eval(vl_event*, vl_simulator *sim)
{
    if (inst_list && inst_list->mptype == MPprim) {
        vl_primitive *prim = (vl_primitive*)master;
        unsigned char s[MAXPRIMLEN];
        if (prim->type == CombPrimDecl) {
            for (int i = 1; i < MAXPRIMLEN-1; i++) {
                if (prim->iodata[i])
                    s[i-1] = prim->iodata[i]->u.s[0];
                else
                    break;
                // s = { in... }
            }

            if (!prim->seq_init) {
                prim->seq_init = true;
                // sanity test for table entries
                unsigned char *row = prim->ptable;
                for (int i = 0; i < prim->rows; i++) {
                    unsigned char *col = row;
                    for (int j = 0; j < MAXPRIMLEN; j++) {
                        if (col[j] == PrimNone)
                            break;
                        if (!prim->primtest(col[j], !j)) {
                            sim->abort();
                            return (EVnone);
                        }
                    }
                }
            }

            unsigned char *row = prim->ptable;
            for (int i = 0; i < prim->rows; i++) {
                unsigned char *col = row + 1;
                bool match = true;
                for (int j = 0; j < MAXPRIMLEN - 1; j++) {
                    if (col[j] == PrimNone)
                        break;
                    if (dcmp(s[j], col[j]))
                        continue;
                    match = false;
                    break;
                }
                if (match) {
                    // set output to row[0];
                    int x = prim->iodata[0]->u.s[0];
                    prim->iodata[0]->u.s[0] = row[0];
                    if (x != row[0])
                        prim->iodata[0]->trigger();
                    return (EVnone);
                }
                row += MAXPRIMLEN;
            }
            // set output to 'x'
            int x = prim->iodata[0]->u.s[0];
            prim->iodata[0]->u.s[0] = BitDC;
            if (x != BitDC)
                prim->iodata[0]->trigger();
        }
        else if (prim->type == SeqPrimDecl) {
            for (int i = 0; i < MAXPRIMLEN; i++) {
                if (prim->iodata[i])
                    s[i] = prim->iodata[i]->u.s[0];
                else
                    break;
                // s = { out, in... }
            }
            if (!prim->seq_init) {
                prim->lastvals[0] = prim->iodata[0]->u.s[0];
                prim->seq_init = true;

                // sanity test for table entries
                unsigned char *row = prim->ptable;
                for (int i = 0; i < prim->rows; i++) {
                    unsigned char *col = row;
                    for (int j = 0; j < MAXPRIMLEN; j++) {
                        if (col[j] == PrimNone)
                            break;
                        if (!prim->primtest(col[j], !j)) {
                            sim->abort();
                            return (EVnone);
                        }
                    }
                }
            }

            unsigned char *row = prim->ptable;
            for (int i = 0; i < prim->rows; i++) {
                unsigned char *col = row + 1;
                bool match = true;
                for (int j = 0; j < MAXPRIMLEN - 1; j++) {
                    if (col[j] == PrimNone)
                        break;
                    if (dcmpseq(s[j], prim->lastvals[j], col[j]))
                        continue;
                    match = false;
                    break;
                }
                if (match) {
                    // set output to row[0];
                    if (row[0] != PrimM) {
                        int x = prim->iodata[0]->u.s[0];
                        prim->iodata[0]->u.s[0] = row[0];
                        if (x != row[0])
                            prim->iodata[0]->trigger();
                    }
                    memcpy(prim->lastvals+1, s+1, MAXPRIMLEN-1);
                    prim->lastvals[0] = prim->iodata[0]->u.s[0];
                    return (EVnone);
                }
                row += MAXPRIMLEN;
            }
            // set output to 'x'
            int x = prim->iodata[0]->u.s[0];
            prim->iodata[0]->u.s[0] = BitDC;
            if (x != BitDC)
                prim->iodata[0]->trigger();
            memcpy(prim->lastvals+1, s+1, MAXPRIMLEN-1);
            prim->lastvals[0] = prim->iodata[0]->u.s[0];
        }
    }
    return (EVnone);
}


// Establish formal/actual port linkage, called from setup
//
void
vl_mp_inst::link_ports(vl_simulator *sim)
{
    // sim->context -> calling module
    if (inst_list->mptype == MPmod) {
        vl_module *mod = (vl_module*)master;
        if (!mod->ports || !ports)
            return;

        lsGen<vl_port_connect*> pcgen(ports);
        // make sure ports are either all named, or none are named
        vl_port_connect *pc;
        bool named = false;
        if (pcgen.next(&pc)) {
            named = pc->name;
            while (pcgen.next(&pc)) {
                if ((named && !pc->name) || (!named && pc->name)) {
                    vl_error("in module %s, mix of named and unnamed ports",
                        name);
                    sim->abort();
                    return;
                }
            }
        }
        if (named) {
            pcgen = lsGen<vl_port_connect*>(ports);
            int argcnt = 1;
            while (pcgen.next(&pc)) {
                if (pc->expr) {
                    vl_port *port = find_port(mod->ports, pc->name);
                    if (port)
                        port_setup(sim, pc, port, argcnt);
                    else {
                        vl_error("can't find port %s in module %s", pc->name,
                            name);
                        sim->abort();
                        return;
                    }
                    argcnt++;
                }
            }
        }
        else {
            pcgen = lsGen<vl_port_connect*>(ports);
            lsGen<vl_port*> pgen(mod->ports);
            vl_port *port;
            int argcnt = 1;
            while (pcgen.next(&pc) && pgen.next(&port)) {
                if (pc->expr && port->port_exp)
                    port_setup(sim, pc, port, argcnt);
                argcnt++;
            }
        }
    }
    else if (inst_list->mptype == MPprim) {
        vl_primitive *prm = (vl_primitive*)master;
        if (!prm->ports || !ports)
            return;
        lsGen<vl_port_connect*> pcgen(ports);
        lsGen<vl_port*> pgen(prm->ports);
        vl_port *port;
        vl_port_connect *pc;
        int argcnt = 1;
        while (pcgen.next(&pc) && pgen.next(&port)) {
            if (pc->expr && port->port_exp)
                port_setup(sim, pc, port, argcnt);
            argcnt++;
        }
    }
}


// Establish linkage to a module/primitive instance port, argcnt is a
// *1 based* port index.  The context is in the caller, pc is the actual
// arg, the port contains the formal arg
//
void
vl_mp_inst::port_setup(vl_simulator *sim, vl_port_connect *pc, vl_port *port,
    int argcnt)
{
    if (pc->i_assign || pc->o_assign)
        // already set up
        return;
    char buf[32];
    const char *portname = pc->name;
    if (!portname) {
        portname = buf;
        sprintf(buf, "%d", argcnt);
    }
    bool isprim = inst_list->mptype == MPprim ? true : false;
    const char *modpri = isprim ? "primitive" : "module";
    vl_primitive *primitive = isprim ? (vl_primitive*)master : 0;

    if (port->port_exp->length() != 1) {
        // the formal arg is a concatenation
        if (isprim) {
            vl_error("in %s instance %s, port %s, found concatenation, "
                "not allowed", modpri, name, portname);
            sim->abort();
            return;
        }
        vl_var *nvar = new vl_var;
        nvar->data_type = Dconcat;
        nvar->u.c = new lsList<vl_expr*>;
        lsGen<vl_var*> gen(port->port_exp);

        IOtype tt = IOnone;
        vl_var *var;
        while (gen.next(&var)) {
            vl_var *catvar;
            if (!master->sig_st->lookup(var->name, &catvar)) {
                vl_error("in %s instance %s, port %s, actual arg %s "
                    "is undeclared", modpri, name, portname, var->name);
                sim->abort();
                return;
            }
            vl_range *range = var->range;
            var = catvar;
            vl_expr *xp = new vl_expr(var);
            if (range) {
                xp->etype = range->right ? PartSelExpr : BitSelExpr;
                xp->ux.ide.range = range->copy();
            }
            IOtype nt = var->io_type;
            if (tt == IOnone)
                tt = nt;
            else if (nt != tt) {
                vl_error("in %s instance %s, port %s, type mismatch in "
                    "concatenation", modpri, name, portname);
                sim->abort();
                return;
            }
            if (nt == IOinput || nt == IOinout) {
                if (var->net_type < REGwire) {
                    if (var->net_type == REGnone)
                        var->net_type = REGwire;
                    else {
                        vl_error("in %s instance %s, port %s, formal arg %s "
                            "is not a net", modpri, name, portname, var->name);
                        sim->abort();
                        return;
                    }
                }
            }
            nvar->u.c->newEnd(xp);
        }

        if (tt == IOinput || tt == IOinout) {
            // set formal = actual
            pc->i_assign =
                new vl_bassign_stmt(BassignStmt, nvar, 0, 0, pc->expr);
            pc->i_assign->flags |= BAS_SAVE_RHS;
            pc->expr->chain(pc->i_assign);
            // initialize
            sim->timewheel->append(sim->time,
                new vl_action_item(pc->i_assign, sim->context));
        }
        if (tt == IOoutput || tt == IOinout) {
            // set actual = formal
            vl_expr *assign_expr = new vl_expr;
            assign_expr->etype = ConcatExpr;
            assign_expr->ux.mcat.var = nvar;
            nvar = pc->expr->source();
            // actual must be net
            if (!nvar || !nvar->check_net_type(REGwire)) {
                vl_error("in %s instance %s, formal connection "
                    "to port %s is not a net", modpri, name, portname);
                sim->abort();
                return;
            }
            pc->o_assign = new vl_bassign_stmt(BassignStmt, nvar, 0, 0,
                assign_expr);
            pc->o_assign->flags |= BAS_SAVE_LHS;
            pc->expr->source_range()->eval(&pc->o_assign->range);
            assign_expr->chain(pc->o_assign);
            sim->timewheel->append(sim->time,
                new vl_action_item(pc->o_assign, sim->context));
        }
        return;
    }

    lsGen<vl_var*> gen(port->port_exp);
    vl_var *var = 0;
    gen.next(&var);
    vl_var *nvar = 0;
    if (!master->sig_st->lookup(var->name, &nvar)) {
        vl_error("in %s instance %s, port %s, actual arg %s "
            "is undeclared", modpri, name, portname, var->name);
        sim->abort();
        return;
    }
    port->port_exp->replace(var, nvar);
    vl_range *range = var->range;
    var->range = 0;
    delete var;
    var = nvar;

    if (isprim) {
        if (range)
            vl_warn("in %s instance %s, port %s, actual arg %s "
                "has range, ignored", modpri, name, portname, var->name);
        if (var->data_type != Dbit || var->bits.size != 1) {
            vl_error("in %s instance %s, port %s, actual arg %s "
                "is not unit width", modpri, name, portname, var->name);
            sim->abort();
            delete range;
            return;
        }
    }
    IOtype tt = var->io_type;
    if (isprim) {
        if (argcnt == 1 && tt != IOoutput) {
            vl_error("in %s instance %s, port %s, actual arg %s "
                "is not an output", modpri, name, portname, var->name);
            sim->abort();
            delete range;
            return;
        }
        if (argcnt > 1 && tt != IOinput) {
            vl_error("in %s instance %s, port %s, actual arg %s "
                "is not an input", modpri, name, portname, var->name);
            sim->abort();
            delete range;
            return;
        }
    }
    if (tt == IOnone) {
        vl_warn("formal arg %s in %s instance %s is not declared\n"
            "input/output/inout, assuming inout",
            portname, modpri, name);
        tt = IOinout;
    }
    if (tt == IOinput || tt == IOinout) {
        // set formal = actual
        // formal must be net
        if (!var->check_net_type(REGwire)) {
            vl_error("in %s instance %s, actual connection "
                "to port %s is not a net", modpri, name, portname);
            sim->abort();
            delete range;
            return;
        }
        pc->i_assign = new vl_bassign_stmt(BassignStmt, var, 0, 0, pc->expr);
        pc->i_assign->flags |= (BAS_SAVE_LHS | BAS_SAVE_RHS);
        range->eval(&pc->i_assign->range);
        pc->expr->chain(pc->i_assign);
        if (tt == IOinout)
            // prevent driver loop
            pc->expr->flags |= VAR_PORT_DRIVER;
        sim->timewheel->append(sim->time,
            new vl_action_item(pc->i_assign, sim->context));
        // bs freed with pc
    }
    if (tt == IOoutput || tt == IOinout) {
        // set actual = formal
        vl_var *vnew = pc->expr->source();
        // actual must be net
        if (!vnew || !vnew->check_net_type(REGwire)) {
            vl_error("in %s instance %s, formal connection "
                "to port %s is not a net", modpri, name, portname);
            sim->abort();
            delete range;
            return;
        }
        if (range) {
            vl_expr *xp = new vl_expr(var);
            xp->etype = range->right ? PartSelExpr : BitSelExpr;
            xp->ux.ide.range = range;
            var = xp;
        }
        pc->o_assign = new vl_bassign_stmt(BassignStmt, vnew, 0, 0, var);
        pc->o_assign->flags |= BAS_SAVE_LHS;
        if (!range)
            pc->o_assign->flags |= BAS_SAVE_RHS;
        pc->expr->source_range()->eval(&pc->o_assign->range);
        var->chain(pc->o_assign);
        if (tt == IOinout)
            // prevent driver loop
            var->flags |= VAR_PORT_DRIVER;
        sim->timewheel->append(sim->time,
            new vl_action_item(pc->o_assign, sim->context));
    }
    if (isprim) {
        primitive->iodata[argcnt-1] = var;
        sim->context = sim->context->push(primitive);
        if (argcnt > 1)
            var->chain(this);
        sim->context = sim->context->pop();
    }
}
// End vl_mp_inst functions

