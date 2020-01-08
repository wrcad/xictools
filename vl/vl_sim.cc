
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


namespace {
    // Return the vl_port corresponding to name, or num if name is nil.
    //
    vl_port *find_port(lsList<vl_port*> *ports, const char *name, int num = 0)
    {
        lsGen<vl_port*> pgen(ports);
        vl_port *port;
        int cnt = 0;
        while (pgen.next(&port)) {
            if (name) {
                if (port->port_exp()) {
                    if (port->name() && !strcmp(port->name(), name))
                        return (port);
                    lsGen<vl_var*> vgen(port->port_exp());
                    vl_var *v;
                    if (vgen.next(&v)) {
                        if (v->name() && !strcmp(v->name(), name))
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


    // Comparison function for combinational primitive tokens.
    //
    bool dcmp(unsigned char a, unsigned char b)
    {
        if (a == b)
            return (true);
        if (a == PrimQ || b == PrimQ)
            return (true);
        if ((a == PrimB && b <= Prim1) || (b == PrimB && a <= Prim1))
            return (true);
        return (false);
    }


    // Comparison function for sequential primitive tokens.
    //
    bool dcmpseq(unsigned char a, unsigned char l, unsigned char c)
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
}


//---------------------------------------------------------------------------
//  Simulator objects
//---------------------------------------------------------------------------

vl_simulator *vl_simulator::s_simulator = 0;

vl_simulator::vl_simulator()
{
    s_description = 0;
    s_dmode = DLYtyp;
    s_stop = VLrun;
    s_first_point = true;
    s_monitor_state = false;
    s_fmonitor_state = false;
    s_monitors = 0;
    s_fmonitors = 0;
    s_time = 0;
    s_steptime = 0;
    s_context = 0;
    s_timewheel = 0;
    s_next_actions = 0;
    s_fj_end = 0;
    s_top_modules = 0;
    s_dbg_flags = 0;
    s_channels[0] = 0;
    s_channels[1] = &cout;
    s_channels[2] = &cerr;
    for (int i = 3; i < 32; i++)
        s_channels[i] = 0;
    s_dmpfile = 0;
    s_dmpcx = 0;
    s_dmpdepth = 0;
    s_dmpstatus = 0;
    s_dmpdata = 0;
    s_dmplast = 0;
    s_dmpsize = 0;
    s_dmpindx = 0;
    s_tfunit = 0;
    s_tfprec = 0;
    s_tfsuffix = 0;
    s_tfwidth = 20;
}


void
vl_simulator::on_null_ptr()
{
    vl_error("null simulator pointer.");
    exit(1);
}


vl_simulator::~vl_simulator()
{
    while (s_monitors) {
        vl_monitor *m = s_monitors;
        s_monitors = s_monitors->next();
        delete m;
    }
    while (s_fmonitors) {
        vl_monitor *m = s_fmonitors;
        s_monitors = s_fmonitors->next();
        delete m;
    }
    vl_context::destroy(s_context);
    delete s_timewheel;
    vl_action_item::destroy(s_next_actions);
    vl_action_item::destroy(s_fj_end);

    if (s_top_modules) {
        for (int i = 0; i < s_top_modules->num(); i++)
            delete s_top_modules->mod(i);
    }
    delete s_top_modules;

    // close any open files
    close_files();

    delete s_dmpcx;
    delete [] s_dmplast;
    delete s_dmpdata;

    if (s_simulator == this)
        s_simulator = 0;
}


// Initialize the simulator, call before simulation, call with desc = 0
// to clear/free everything.
//
bool
vl_simulator::initialize(vl_desc *desc, VLdelayType dly, int dbg)
{
    if (s_simulator) {
        vl_error("non-reentrant initialize function called");
        return (false);
    }
    s_simulator = this;

    s_description = 0;
    s_dmode = DLYtyp;
    s_stop = VLrun;
    s_monitor_state = false;
    s_fmonitor_state = false;
    while (s_monitors) {
        vl_monitor *m = s_monitors;
        s_monitors = s_monitors->next();
        delete m;
    }
    while (s_fmonitors) {
        vl_monitor *m = s_fmonitors;
        s_monitors = s_fmonitors->next();
        delete m;
    }
    s_time = 0;
    s_steptime = 0;
    vl_context::destroy(s_context);
    s_context = 0;
    delete s_timewheel;
    s_timewheel = 0;
    vl_action_item::destroy(s_next_actions);
    s_next_actions = 0;
    vl_action_item::destroy(s_fj_end);
    s_fj_end = 0;
    if (s_top_modules) {
        for (int i = 0; i < s_top_modules->num(); i++)
            delete s_top_modules->mod(i);
    }
    delete s_top_modules;
    s_top_modules = 0;
    s_dbg_flags = 0;
    s_time_data.set_data_t(0);

    close_files();

    delete s_dmpcx;
    s_dmpcx = 0;
    s_dmpdepth = 0;
    s_dmpstatus = 0;
    delete s_dmpdata;
    s_dmpdata = 0;
    delete [] s_dmplast;
    s_dmplast = 0;
    s_dmpsize = 0;
    s_dmpindx = 0;
    s_tfunit = 0;
    s_tfprec = 0;
    delete [] s_tfsuffix;
    s_tfsuffix = 0;
    s_tfwidth = 20;
    var_factory.clear();

    if (!desc) {
        s_simulator = 0;
        return (true);
    }

    s_dmode = dly;
    s_dbg_flags = dbg;
    s_description = desc;
    s_timewheel = new vl_timeslot(0);
    vl_module *mod;
    lsGen<vl_module*> gen(desc->modules());
    int count = 0;
    while (gen.next(&mod))
        if (!mod->inst_count() && !mod->ports())
            count++;
    if (count <= 0) {
        vl_error("no top module!");
        s_simulator = 0;
        return (false);
    }
    s_top_modules = new vl_top_mod_list(count, new vl_module*[count]);
    gen.reset(desc->modules());
    count = 0;
    while (gen.next(&mod)) {
        if (!mod->inst_count() && !mod->ports()) {
            // copy the module, fills in blk_st and task_st
            s_top_modules->set_mod(count, chk_copy(mod));
            count++;
        }
    }
    for (int i = 0; i < s_top_modules->num(); i++) {
        push_context(s_top_modules->mod(i));
        s_top_modules->mod(i)->init();
        pop_context();
    }
    for (int i = 0; i < s_top_modules->num(); i++) {
        push_context(s_top_modules->mod(i));
        s_top_modules->mod(i)->setup(this);
        pop_context();
    }
    if (s_dbg_flags & DBG_mod_dmp) {
        for (int i = 0; i < s_top_modules->num(); i++)
            s_top_modules->mod(i)->dump(cout);
    }
    s_monitor_state = true;
    s_fmonitor_state = true;
    s_context = 0;
    var_factory.clear();
    s_time_data.set_data_type(Dtime);
    s_time_data.set_data_t(0);
    s_tfunit = (int)(log10(s_description->tstep()) - 0.5);
    s_simulator = 0;
    return (true);
}


// Perform the simulation.
//
bool
vl_simulator::simulate()
{
    if (s_simulator) {
        vl_error("non-reentrant simulate function called");
        return (false);
    }
    s_simulator = this;

    if (s_dbg_flags & DBG_desc)
        s_description->dump(cout);
    while (s_timewheel && s_stop == VLrun) {
        vl_timeslot *t = s_timewheel;
        if (s_dbg_flags & DBG_tslot) {
            for ( ; t; t = t->next())
                t->print(cout);
            cout << "\n\n";
        }
        var_factory.clear();
        s_time_data.set_data_t(s_timewheel->time());
        s_timewheel->eval_slot(this);
        if (s_monitor_state && s_monitors) {
            for (vl_monitor *m = s_monitors; m; m = m->next()) {
                vl_context *tcx = s_context;
                s_context = m->cx();
                if (monitor_change(m->args()))
                    display_print(m->args(), cout, m->dtype(), 0);
                s_context = tcx;
            }
        }
        if (s_fmonitor_state && s_fmonitors) {
            for (vl_monitor *m = s_fmonitors; m; m = m->next()) {
                vl_context *tcx = s_context;
                s_context = m->cx();
                if (monitor_change(m->args()))
                    fdisplay_print(m->args(), m->dtype(), 0);
                s_context = tcx;
            }
        }
        if (s_dmpstatus & DMP_ACTIVE)
            do_dump();
        t = s_timewheel->next();
        delete s_timewheel;
        s_timewheel = t;
        s_first_point = false;
    }
    // close any open files
    close_files();
    s_simulator = 0;
    return (true);
}


// Step one time point.
//
VLstopType
vl_simulator::step()
{
    if (s_simulator) {
        vl_error("non-reentrant step function called");
        return (VLabort);
    }
    s_simulator = this;

    while (s_timewheel && s_stop == VLrun &&
            s_timewheel->time() <= s_steptime) {
        s_time_data.set_data_t(s_timewheel->time());
        s_timewheel->eval_slot(this);
        if (s_monitor_state && s_monitors) {
            for (vl_monitor *m = s_monitors; m; m = m->next()) {
                vl_context *tcx = s_context;
                s_context = m->cx();
                if (monitor_change(m->args()))
                    display_print(m->args(), cout, m->dtype(), 0);
                s_context = tcx;
            }
        }
        if (s_fmonitor_state && s_fmonitors) {
            for (vl_monitor *m = s_fmonitors; m; m = m->next()) {
                vl_context *tcx = s_context;
                s_context = m->cx();
                if (monitor_change(m->args()))
                    fdisplay_print(m->args(), m->dtype(), 0);
                s_context = tcx;
            }
        }
        if (s_dmpstatus & DMP_ACTIVE)
            do_dump();
        vl_timeslot *t = s_timewheel->next();
        delete s_timewheel;
        s_timewheel = t;
        s_first_point = false;
        var_factory.clear();

        // When stepping, we generally have to move the time wheel
        // forward explicitly as if "always #1;" was given.  If we
        // stall, add an explicit 1-count delay to keep things
        // running.

        if (s_timewheel == 0) {
            vl_expr *ex = new vl_expr(IntExpr, 1, 0.0, 0, 0, 0);
            vl_delay *exdly = new vl_delay(ex);
            vl_delay_control_stmt  *vc = new vl_delay_control_stmt(exdly, 0);
            push_context(s_top_modules->mod(0));
            vl_action_item *ai = new vl_action_item(vc, s_context);
            pop_context();

            vl_timeslot *ts = new vl_timeslot(s_steptime+1);
            ts->set_actions(ai);
            ts->set_next(s_timewheel);
            s_timewheel = ts;
        }
    }

    s_steptime++;

    if (s_stop != VLrun)
        close_files();
    s_simulator = 0;
    return (s_stop);
}


// Close all open files.
//
void
vl_simulator::close_files()
{
    delete s_dmpfile;
    s_dmpfile = 0;
    for (int i = 3; i < 32; i++) {
        delete s_channels[i];
        s_channels[i] = 0;
    }
}


// Flush all open files.
//
void
vl_simulator::flush_files()
{
    if (s_dmpfile)
        s_dmpfile->flush();
    for (int i = 3; i < 32; i++) {
        if (s_channels[i])
            s_channels[i]->flush();
    }
}


// This is for WRspice interface.  Take val, subtract offs, and quantize
// with bitsize quan.  Use the resulting int to set data.  Note, if the
// data type is real, just save val.  If m and l are >= 0, set the
// indicated bits, if a bit field.
//
bool
vl_simulator::assign_to(vl_var *var, double val, double offs, double quan,
    int m, int l)
{
    if (s_simulator) {
        vl_error("non-reentrant assign_to function called");
        return (false);
    }
    s_simulator = this;

    if (var->data_type() == Dnone) {
        var->set_data_type(Dreal);
        var->set_data_r(val);
        var->trigger();
    }
    else if (var->data_type() == Dreal) {
        if (var->data_r() != val) {
            var->set_data_r(val);
            var->trigger();
        }
    }
    else if (var->data_type() == Dint) {
        val -= offs;
        int b;
        if (quan != 0.0)
            b = (int)((val + 0.5*(val > 0.0 ? quan : -quan))/quan);
        else
            b = (int)val;
        if (var->data_i() != b) {
            var->set_data_i(b);
            var->trigger();
        }
    }
    else if (var->data_type() == Dtime) {
        val -= offs;
        vl_time_t t;
        if (quan != 0.0)
            t = (vl_time_t)((val + 0.5*(val > 0.0 ? quan : -quan))/quan);
        else
            t = (vl_time_t)val;
        if (var->data_t() != t) {
            var->set_data_t(t);
            var->trigger();
        }
    }
    else if (var->data_type() == Dstring) {
        ;
    }
    else if (var->data_type() == Dbit) {
        int ival;
        val -= offs;
        if (quan != 0.0)
            ival = (int)((val + 0.5*(val > 0.0 ? quan : -quan))/quan);
        else
            ival = (int)val;
        bool arm_trigger = false;
        if (m >= 0 && l >= 0) {
            if (var->bits().check_range(&m, &l)) {
                if (m >= l) {
                    for (int i = l; i <= m; i++) {
                        int b = (ival & (1 << (i-l))) ? BitH : BitL;
                        if (var->data_s()[i] != b) {
                            arm_trigger = true;
                            var->data_s()[i] = b;
                        }
                    }
                }
                else {
                    for (int i = m; i <= l; i++) {
                        int b = (ival & (1 << (i-m))) ? BitH : BitL;
                        if (var->data_s()[i] != b) {
                            arm_trigger = true;
                            var->data_s()[i] = b;
                        }
                    }
                }
                if (arm_trigger && var->events())
                    var->trigger();
            }
        }
        else {
            for (int i = 0; i < var->bits().size(); i++) {
                int b = (ival & (1 << i)) ? BitH : BitL;
                if (var->data_s()[i] != b) {
                    arm_trigger = true;
                    var->data_s()[i] = b;
                }
            }
            if (arm_trigger && var->events())
                var->trigger();
        }
    }
    s_simulator = 0;
    return (true);
}


void
vl_simulator::push_context(vl_mp *m)
{
    s_context = new vl_context(s_context);
    if (m->type() == ModDecl)
        s_context->set_module((vl_module*)m);
    else if (m->type() == CombPrimDecl || m->type() == SeqPrimDecl)
        s_context->set_primitive((vl_primitive*)m);
    else {
        vl_error("internal, bad object type %d (not mod/prim)", m->type());
        abort();
    }
}


void
vl_simulator::push_context(vl_module *m)
{
    s_context = new vl_context(s_context);
    if (m->type() == ModDecl)
        s_context->set_module(m);
    else {
        vl_error("internal, bad object type %d (not module)", m->type());
        abort();
    }
}


void
vl_simulator::push_context(vl_primitive *p)
{
    s_context = new vl_context(s_context);
    if (p->type() == CombPrimDecl || p->type() == SeqPrimDecl)
        s_context->set_primitive(p);
    else {
        vl_error("internal, bad object type %d (not primitive)", p->type());
        abort();
    }
}


void
vl_simulator::push_context(vl_task *t)
{
    s_context = new vl_context(s_context);
    if (t->type() == TaskDecl)
        s_context->set_task(t);
    else {
        vl_error("internal, bad object type %d (not task)", t->type());
        abort();
    }
}


void
vl_simulator::push_context(vl_function *f)
{
    s_context = new vl_context(s_context);
    if (f->type() >= IntFuncDecl && f->type() <= RangeFuncDecl)
        s_context->set_function(f);
    else {
        vl_error("internal, bad object type %d (not function)", f->type());
        abort();
    }
}


void
vl_simulator::push_context(vl_begin_end_stmt *b)
{
    s_context = new vl_context(s_context);
    if (b->type() == BeginEndStmt)
        s_context->set_block(b);
    else {
        vl_error("internal, bad object type %d (not begin/end)", b->type());
        abort();
    }
}


void
vl_simulator::push_context(vl_fork_join_stmt *f)
{
    s_context = new vl_context(s_context);
    if (f->type() == ForkJoinStmt)
        s_context->set_fjblk(f);
    else {
        vl_error("internal, bad object type %d (not fork/join)", f->type());
        abort();
    }
}


void
vl_simulator::pop_context()
{
    if (s_context) {
        vl_context *cx = s_context;
        s_context = cx->parent();
        delete cx;
    }
}
// End vl_simulator functions.


vl_context *
vl_context::copy()
{
    vl_context *cx0 = 0, *cx = 0;
    for (vl_context *c = this; c; c = c->c_parent) {
        if (!cx0)
            cx = cx0 = new vl_context(*c);
        else {
            cx->c_parent = new vl_context(*c);
            cx = cx->c_parent;
        }
        cx->c_parent = 0;
    }
    return (cx0);
}


// Return true if the passed object is in the context hierarchy.
//
bool
vl_context::in_context(vl_stmt *blk)
{
    if (!blk)
        return (false);
    if (blk->type() == BeginEndStmt) {
        for (vl_context *cx = this; cx; cx = cx->c_parent) {
            if (cx->c_block == blk)
                return (true);
        }
    }
    else if (blk->type() == ForkJoinStmt) {
        for (vl_context *cx = this; cx; cx = cx->c_parent) {
            if (cx->c_fjblk == blk)
                return (true);
        }
    }
    else if (blk->type() == TaskDecl) {
        for (vl_context *cx = this; cx; cx = cx->c_parent) {
            if (cx->c_task == blk)
                return (true);
        }
    }
    return (false);
}


// Retrieve a value for the named variable, search present context
// exclusively if thisonly is true.
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
        if (cx->c_module || cx->c_primitive)
            break;
        cx = cx->c_parent;
    }
    return (0);
}


// Return the named vl_begin_end_stmt or vl_fork_join_stmt.
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
        if (cx->c_block) {
            if (cx->c_block->name() && !strcmp(cx->c_block->name(), bname))
                return (cx->c_block);
            if (cx->c_block->blk_st() &&
                    cx->c_block->blk_st()->lookup(bname, &stmt) && stmt)
                return (stmt);
        }
        if (cx->c_fjblk) {
            if (cx->c_fjblk->name() && !strcmp(cx->c_fjblk->name(), bname))
                return (cx->c_fjblk);
            if (cx->c_fjblk->blk_st() &&
                    cx->c_fjblk->blk_st()->lookup(bname, &stmt) && stmt)
                return (stmt);
        }
        if (cx->c_function) {
            if (cx->c_function->blk_st() &&
                    cx->c_function->blk_st()->lookup(bname, &stmt) && stmt)
                return (stmt);
        }
        if (cx->c_task) {
            if (cx->c_task->blk_st() &&
                    cx->c_task->blk_st()->lookup(bname, &stmt) && stmt)
                return (stmt);
        }
        if (cx->c_module) {
            if (cx->c_module->blk_st() &&
                    cx->c_module->blk_st()->lookup(bname, &stmt) && stmt)
                return (stmt);
        }
        if (bname != name)
            break;
        if (cx->c_module)
            break;
        cx = cx->c_parent;
    }
    return (0);
}


// Return the named vl_task.
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


// Return the named vl_function.
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
// returned in 'name'.
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
    if (cvar.c_module) {
        if (crt && !cvar.c_module->sig_st())
            cvar.c_module->set_sig_st(new table<vl_var*>);
        st = cvar.c_module->sig_st();
    }
    else if (cvar.c_primitive) {
        if (crt && !cvar.c_primitive->sig_st())
            cvar.c_primitive->set_sig_st(new table<vl_var*>);
        st = cvar.c_primitive->sig_st();
    }
    else if (cvar.c_task) {
        if (crt && !cvar.c_task->sig_st())
            cvar.c_task->set_sig_st(new table<vl_var*>);
        st = cvar.c_task->sig_st();
    }
    else if (cvar.c_function) {
        if (crt && !cvar.c_function->sig_st())
            cvar.c_function->set_sig_st(new table<vl_var*>);
        st = cvar.c_function->sig_st();
    }
    else if (cvar.c_block) {
        if (crt && !cvar.c_block->sig_st())
            cvar.c_block->set_sig_st(new table<vl_var*>);
        st = cvar.c_block->sig_st();
    }
    else if (cvar.c_fjblk) {
        if (crt && !cvar.c_fjblk->sig_st())
            cvar.c_fjblk->set_sig_st(new table<vl_var*>);
        st = cvar.c_fjblk->sig_st();
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
        if (cx->c_module) {
            st = cx->c_module->task_st();
            break;
        }
        cx = cx->c_parent;
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
        if (cx->c_module) {
            st = cx->c_module->func_st();
            break;
        }
        cx = cx->c_parent;
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
        if (cx->c_module) {
            st = cx->c_module->inst_st();
            break;
        }
        cx = cx->c_parent;
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
    cvar.c_module = 0;
    cvar.c_parent = 0;
    for (int i = 0; i < VS()->top_modules()->num(); i++) {
        if (!strcmp(r, VS()->top_modules()->mod(i)->name())) {
            cvar.c_module = VS()->top_modules()->mod(i);
            r = s;
            break;
        }
    }
    if (!cvar.c_module) {
        vl_context *cx = this;
        while (!cx->c_module && !cx->c_primitive && !cx->c_task &&
                !cx->c_function)
            cx = cx->c_parent;
        if (!cx)
            return (false);
        cvar = *cx;
        // put '.' back and move to start
        if (fdot)
            *(s-1) = '.';
        if (cx->c_task || cx->c_function) {
            // first search in task/function
            if (cvar.resolve_path(r, modonly))
                return (true);
            while (!cx->c_module && !cx->c_primitive)
                cx = cx->c_parent;
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
        if (c_module) {
            vl_inst *inst;
            if (c_module->inst_st() && c_module->inst_st()->lookup(r, &inst)) {
                if (!inst->type()) {
                    // not a gate
                    vl_mp_inst *mp = (vl_mp_inst*)inst;
                    if (mp->inst_list()) {
                        if (mp->inst_list()->mptype() == MPmod) {
                            found = true;
                            c_module = (vl_module*)mp->master();
                        }
                        else if (mp->inst_list()->mptype() == MPprim) {
                            if (modonly)
                                return (true);
                            found = true;
                            c_module = 0;
                            c_primitive = (vl_primitive*)mp->master();
                        }
                    }
                }
                if (!found) {
                    // must have been a gate?
                    c_module = 0;
                    return (false);
                }
            }
            vl_stmt *stmt;
            if (!found && c_module->blk_st() &&
                    c_module->blk_st()->lookup(r, &stmt)) {
                if (modonly)
                    return (true);
                found = true;
                if (stmt->type() == BeginEndStmt) {
                    c_module = 0;
                    c_block = (vl_begin_end_stmt*)stmt;
                }
                else if (stmt->type() == ForkJoinStmt) {
                    c_module = 0;
                    c_fjblk = (vl_fork_join_stmt*)stmt;
                }
                else
                    found = false;
            }
        }
        else if (c_task) {
            vl_stmt *stmt;
            if (c_task->blk_st() && c_task->blk_st()->lookup(r, &stmt)) {
                found = true;
                if (stmt->type() == BeginEndStmt) {
                    c_task = 0;
                    c_block = (vl_begin_end_stmt*)stmt;
                }
                else if (stmt->type() == ForkJoinStmt) {
                    c_task = 0;
                    c_fjblk = (vl_fork_join_stmt*)stmt;
                }
                else
                    found = false;
            }
        }
        else if (c_function) {
            vl_stmt *stmt;
            if (c_function->blk_st() &&
                    c_function->blk_st()->lookup(r, &stmt)) {
                found = true;
                if (stmt->type() == BeginEndStmt) {
                    c_function = 0;
                    c_block = (vl_begin_end_stmt*)stmt;
                }
                else if (stmt->type() == ForkJoinStmt) {
                    c_function = 0;
                    c_fjblk = (vl_fork_join_stmt*)stmt;
                }
                else
                    found = false;
            }
        }
        else if (c_block) {
            vl_stmt *stmt;
            if (c_block->blk_st() && c_block->blk_st()->lookup(r, &stmt)) {
                found = true;
                if (stmt->type() == BeginEndStmt)
                    c_block = (vl_begin_end_stmt*)stmt;
                else if (stmt->type() == ForkJoinStmt) {
                    c_block = 0;
                    c_fjblk = (vl_fork_join_stmt*)stmt;
                }
                else
                    found = false;
            }
        }
        else if (c_fjblk) {
            vl_stmt *stmt;
            if (c_fjblk->blk_st() && c_fjblk->blk_st()->lookup(r, &stmt)) {
                found = true;
                if (stmt->type() == BeginEndStmt) {
                    c_fjblk = 0;
                    c_block = (vl_begin_end_stmt*)stmt;
                }
                else if (stmt->type() == ForkJoinStmt)
                    c_fjblk = (vl_fork_join_stmt*)stmt;
                else
                    found = false;
            }
        }
        if (!found) {
            c_module = 0;
            c_primitive = 0;
            c_task = 0;
            c_function = 0;
            c_block = 0;
            c_fjblk = 0;
            return (false);
        }
    }
    return (true);
}


// Return the current module.
//
vl_module *
vl_context::currentModule()
{
    vl_context *cx = this;
    while (cx) {
        if (cx->c_module)
            return (cx->c_module);
        if (cx->c_primitive)
            return (0);
        cx = cx->c_parent;
    }
    return (0);
}


// Return the current primitive.
//
vl_primitive *
vl_context::currentPrimitive()
{
    vl_context *cx = this;
    while (cx) {
        if (cx->c_primitive)
            return (cx->c_primitive);
        if (cx->c_module)
            return (0);
        cx = cx->c_parent;
    }
    return (0);
}


// Return the current function.
//
vl_function *
vl_context::currentFunction()
{
    vl_context *cx = this;
    while (cx) {
        if (cx->c_function)
            return (cx->c_function);
        if (cx->c_module || cx->c_primitive)
            return (0);
        cx = cx->c_parent;
    }
    return (0);
}


// Return the current task.
//
vl_task *
vl_context::currentTask()
{
    vl_context *cx = this;
    while (cx) {
        if (cx->c_task)
            return (cx->c_task);
        if (cx->c_module || cx->c_primitive)
            return (0);
        cx = cx->c_parent;
    }
    return (0);
}
// End vl_context functions.


vl_action_item::vl_action_item(vl_stmt *s, vl_context *c)
{
    st_type = ActionItem;
    ai_stmt = s;
    ai_stack = 0;
    ai_event = 0;
    ai_context = chk_copy(c);
    ai_next = 0;
}


vl_action_item::~vl_action_item()
{
    if (st_flags & AI_DEL_STMT)
        delete ai_stmt;
    delete ai_stack;
    vl_context::destroy(ai_context);
}


// Copy an action.
//
vl_action_item *
vl_action_item::copy()
{
    return (new vl_action_item(ai_stmt, ai_context));
}


// Remove and free any entries in the actions list with blk in the
// context hierarchy, return the purged list head.
//
vl_action_item *
vl_action_item::purge(vl_stmt *blk)
{
    vl_action_item *a0 = this;
    vl_action_item *ap = 0, *an;
    for (vl_action_item *a = a0; a; a = an) {
        an = a->ai_next;
        if (a->ai_stack) {
            for (int i = 0; i < a->ai_stack->num(); i++) {
                if (a->ai_stack->act(i).actions()) {
                    a->ai_stack->act(i).set_actions(
                        a->ai_stack->act(i).actions()->purge(blk));
                }
            }
            bool mt = true;
            for (int i = 0; i < a->ai_stack->num(); i++) {
                if (a->ai_stack->act(i).actions()) {
                    mt = false;
                    break;
                }
            }
            if (mt) {
                if (!ap)
                    a0 = an;
                else
                    ap->ai_next = an;
                delete a;
                continue;
            }
        }
        else if (a->ai_stmt) {
            if (a->ai_context->in_context(blk)) {
                if (!ap)
                    a0 = an;
                else
                    ap->ai_next = an;
                delete a;
                continue;
            }
        }
        ap = a;
    }
    return (a0);
}


// Evaluate an action.
//
EVtype
vl_action_item::eval(vl_event *ev, vl_simulator *sim)
{
    if (ai_stmt) {
        vl_context *cx = sim->context();
        sim->set_context(ai_context);
        EVtype evt = ai_stmt->eval(ev, sim);
        sim->set_context(cx);
        return (evt);
    }
    else {
        vl_error("(internal) null statement encountered");
        sim->abort();
    }
    return (EVnone);
}


// Diagnostic print of action.
//
void
vl_action_item::print(ostream &outs)
{
    if (ai_stmt) {
        ai_stmt->print(outs);
        outs << ai_stmt->lterm();
    }
    else if (ai_stack)
        ai_stack->print(outs);
    else if (ai_event)
        outs << "<event>\n";
}
// End of vl_action_item functions.


vl_stack *
vl_stack::copy()
{
    return (new vl_stack(st_acts, st_num));
}


// Diagnostic print of stack.
//
void
vl_stack::print(ostream &outs)
{
    for (int i = st_num-1; i >= 0; i--) {
        for (vl_action_item *a = st_acts[i].actions(); a; a = a->next()) {
            cout << i << '|';
            a->print(outs);
        }
    }
}
// End vl_stack functions.


vl_timeslot::vl_timeslot(vl_time_t t) 
{
    ts_next = 0;
    ts_time = t;
    ts_actions = ts_a_end = 0;
    ts_trig_actions = ts_t_end = 0;
    ts_zdly_actions = ts_z_end = 0;
    ts_nbau_actions = ts_n_end = 0;
    ts_mon_actions = ts_m_end = 0;
}


vl_timeslot::~vl_timeslot() 
{
    vl_action_item::destroy(ts_actions);
    vl_action_item::destroy(ts_trig_actions);
    vl_action_item::destroy(ts_zdly_actions);
    vl_action_item::destroy(ts_nbau_actions);
    vl_action_item::destroy(ts_mon_actions);
}
  

// Return the list head corresponding to the indicated time.
//
vl_timeslot *
vl_timeslot::find_slot(vl_time_t t)
{
    if (t < ts_time) {
        // This can happen when stepping (WRspice control) if an event
        // is generated by an input signal from WRspice which was not
        // triggered from Verilog.  Handle this by changing this to
        // a dummy timeslot (it is the list head in the simulator) and
        // add a new slot with the previous contents.

        vl_timeslot *ts = new vl_timeslot(*this);
        ts->ts_next = ts_next;
        *this = vl_timeslot(t);
        ts_next = ts;
        return (this);

        /*
        vl_error("negative delay encountered, time=%g timewheel start=%g",
            (double)t, (double)time);
        vl_var::simulator->abort();
        return (this);
        */
    }
    vl_timeslot *s = this, *sp = 0;
    while (s && s->ts_time < t)
        sp = s, s = s->ts_next;
    if (s) {
        if (s->ts_time > t) {
            vl_timeslot *ts = new vl_timeslot(t);
            ts->ts_next = s;
            sp->ts_next = ts;
            s = ts;
        }
    }
    else {
        sp->set_next(new vl_timeslot(t));
        s = sp->next();
    }
    return (s);
}


// Append an action at time t.
//
void
vl_timeslot::append(vl_time_t t, vl_action_item *a)
{
    vl_timeslot *s = find_slot(t);
    for (vl_action_item *aa = s->ts_actions; aa; aa = aa->next()) {
        if (aa->stmt() && aa->stmt() == a->stmt()) {
            delete a;
            return;
        }
    }
    if (!s->ts_actions)
        s->ts_actions = s->ts_a_end = a;
    else
        s->ts_a_end->set_next(a);
    while (s->ts_a_end->next())
        s->ts_a_end = s->ts_a_end->next();
}


// Append an action to the list of triggered events at time t.
//
void
vl_timeslot::append_trig(vl_time_t t, vl_action_item *a)
{
    vl_timeslot *s = find_slot(t);
    for (vl_action_item *aa = s->ts_trig_actions; aa; aa = aa->next()) {
        if (aa->stmt() && aa->stmt() == a->stmt()) {
            delete a;
            return;
        }
    }
    if (!s->ts_trig_actions)
        s->ts_trig_actions = s->ts_t_end = a;
    else
        s->ts_t_end->set_next(a);
    while (s->ts_t_end->next())
        s->ts_t_end = s->ts_t_end->next();
}


// Append an "inactive" event at time t (for #0 ...).
//
void
vl_timeslot::append_zdly(vl_time_t t, vl_action_item *a)
{
    vl_timeslot *s = find_slot(t);
    for (vl_action_item *aa = s->ts_zdly_actions; aa; aa = aa->next()) {
        if (aa->stmt() && aa->stmt() == a->stmt()) {
            delete a;
            return;
        }
    }
    if (!s->ts_zdly_actions)
        s->ts_zdly_actions = s->ts_z_end = a;
    else
        s->ts_z_end->set_next(a);
    while (s->ts_z_end->next())
        s->ts_z_end = s->ts_z_end->next();
}


// Append a "non-blocking assign update" event at time t  (for n-b assign).
//
void
vl_timeslot::append_nbau(vl_time_t t, vl_action_item *a)
{
    vl_timeslot *s = find_slot(t);
    for (vl_action_item *aa = s->ts_nbau_actions; aa; aa = aa->next()) {
        if (aa->stmt() && aa->stmt() == a->stmt()) {
            delete a;
            return;
        }
    }
    if (!s->ts_nbau_actions)
        s->ts_nbau_actions = s->ts_n_end = a;
    else
        s->ts_n_end->set_next(a);
    while (s->ts_n_end->next())
        s->ts_n_end = s->ts_n_end->next();
}


// Append a "monitor" event at time t  (for $monitor/$strobe).
//
void
vl_timeslot::append_mon(vl_time_t t, vl_action_item *a)
{
    vl_timeslot *s = find_slot(t);
    for (vl_action_item *aa = s->ts_mon_actions; aa; aa = aa->next()) {
        if (aa->stmt() && aa->stmt() == a->stmt()) {
            delete a;
            return;
        }
    }
    if (!s->ts_mon_actions)
        s->ts_mon_actions = s->ts_m_end = a;
    else
        s->ts_m_end->set_next(a);
    while (s->ts_m_end->next())
        s->ts_m_end = s->ts_m_end->next();
}


// Process the actions at the current time, servicing events, and
// resheduling propagating actions.
//
void
vl_timeslot::eval_slot(vl_simulator *sim)
{
    sim->set_time(ts_time);
    sim->time_data().trigger();  // for @($time)
    add_next_actions(sim);
    while (ts_actions || ts_zdly_actions || ts_nbau_actions || ts_mon_actions) {
        if (sim->stop() != VLrun)
            break;
        if (!ts_actions) {
            if (ts_zdly_actions) {
                ts_actions = ts_zdly_actions;
                ts_zdly_actions = 0;
            }
            else if (ts_nbau_actions) {
                ts_actions = ts_nbau_actions;
                ts_nbau_actions = 0;
            }
            else if (ts_mon_actions) {
                ts_actions = ts_mon_actions;
                ts_mon_actions = 0;
            }
            else
                break;
        }
        do_actions(sim);
    }
}


// Add an action to be performed at the next time point.
//
void
vl_timeslot::add_next_actions(vl_simulator *sim)
{
    if (sim->next_actions()) {
        vl_action_item *a = sim->next_actions();
        while (a->next())
            a = a->next();
        a->set_next(ts_actions);
        ts_actions = sim->next_actions();
        sim->set_next_actions(0);
    }
}


//#define DEBUG_CONTEXT
#ifdef DEBUG_CONTEXT

namespace {
    bool check_cx(vl_context *cx)
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
    acts[sp].set_actions(ts_actions);
    acts[sp].set_type(Fence);
    ts_actions = 0;
    bool doing_trig = false;
    int trig_sp = 0;

    while (sp >= 0) {
        vl_action_item *a = acts[sp].actions();

        if (ts_trig_actions && ts_trig_actions->stmt() && a && a->stmt() &&
                (a->stmt()->type() == InitialStmt ||
                a->stmt()->type() == AlwaysStmt ||
                a->stmt()->type() == BeginEndStmt ||
                a->stmt()->type() == ForkJoinStmt)) {

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
            acts[sp].set_actions(ts_trig_actions);
            vl_action_item *tp;
            do {
                tp = ts_trig_actions;
                ts_trig_actions = ts_trig_actions->next();
            } while (ts_trig_actions && ts_trig_actions->stmt());
            tp->set_next(0);

            acts[sp].set_type(Sequential);
            trig_sp = sp;
            doing_trig = true;
            continue;
        }

        if (!a && sp == 0) {
            // We're stalled, time to do the accumulated events triggered
            // by the previous actions
            a = ts_trig_actions;
            ts_trig_actions = 0;
            doing_trig = true;
            trig_sp = 0;
        }
        if (!a) {
            sp--;
            continue;
        }
        acts[sp].set_actions(a->next());

        // If the action has a stack, unwind the stack and store a
        // place holder so as to keep this frame separate if there is
        // an event or delay encountered.

        if (a->stack()) {
            acts[sp].set_type(Fence);
            for (int i = 0; i < a->stack()->num(); i++) {
                acts[++sp] = a->stack()->act(i);
                a->stack()->act(i).set_actions(0);
            }
            delete a;
            continue;
        }

        if (sp < trig_sp)
            doing_trig = false;
        if (sim->dbg_flags() & DBG_action) {
            if (doing_trig)
                cout << '*';
            cout << "---- " << sim->time() << " " << sp << '\n';
            a->print(cout);
        }

#ifdef DEBUG_CONTEXT
        if (!check_cx(sim->context)) {
            cout << "at time=" << sim->time << " CONTEXT IS BAD!\n";
            cout << a->stmt() << '\n';
            cout << "---\n";
        }
#endif
        if (a->stmt()->type() == ForkJoinStmt) {
            // We're about to start on a fork/join block.  To deal
            // with completion, save a list of actions, which are
            // updated when each thread completes (in vl_fj_break::eval()).
            // The actions are given a stack along the way.

            vl_action_item *anew = new vl_action_item(a->stmt(), 0);
            anew->set_next(sim->fj_end());
            sim->set_fj_end(anew);
        }

        vl_event ev;
        EVtype evt = a->eval(&ev, sim);
        if (sim->stop() != VLrun)
            return;

        if (a->stmt()->type() == DisableStmt) {
            // just evaluated a disable statement, target should be
            // filled in.  Purge all pending actions for this block.

            vl_stmt *blk = ((vl_disable_stmt*)a->stmt())->target();
            if (blk) {
                if (sim->timewheel())
                    sim->timewheel()->purge(blk);
                for (int i = 0; i <= sp; i++) {
                    if (acts[i].actions())
                        acts[i].set_actions(acts[i].actions()->purge(blk));
                }
                // this purges the events under blk, can generate
                // f/j termination events
                blk->disable(blk);
            }
        }
        if (ts_actions) {
            if (a->stmt()->type() == BeginEndStmt ||
                    a->stmt()->type() == ForkJoinStmt ||
                    a->stmt()->type() == InitialStmt ||
                    a->stmt()->type() == AlwaysStmt) {
                // The action created more actions, push these into a new
                // stack block, and increment the stack pointer.

                ABtype ntype;
                if (a->stmt()->type() == ForkJoinStmt)
                    ntype = Fork;
                else
                    ntype = Sequential;
                if (acts[sp].actions() || acts[sp].type() != ntype)
                    sp++;
                if (sp >= STACK_DEPTH) {
                    vl_error("(internal) stack depth exceeded");
                    sim->abort();
                    return;
                }
                acts[sp].set_actions(ts_actions);
                acts[sp].set_type(ntype);
                acts[sp].set_fjblk((ntype == Fork ? a->stmt() : 0));
                ts_actions = 0;
            }
            else {
                ts_a_end->set_next(acts[sp].actions());
                acts[sp].set_actions(ts_actions);
                ts_actions = 0;
            }
        }

        // If a delay or event is returned, save the frame for the event
        // in a new action, then dispatch the action to the appropriate
        // list and fix the stack pointer.

        if (evt == EVdelay) {
            int lsp = sp;
            if (!acts[lsp].actions())
                lsp--;
            int tsp = lsp;
            while (tsp > 0 && acts[tsp].type() == Sequential)
                tsp--;
            if (tsp != lsp) {
                vl_action_item *anew = new vl_action_item(0, 0);
                anew->set_stack(new vl_stack(acts + tsp + 1, lsp - tsp));
                if (ev.time == sim->time())
                    sim->timewheel()->append_zdly(ev.time, anew);
                else
                    sim->timewheel()->append(ev.time, anew);
                for (int i = tsp+1; i <= lsp; i++)
                    acts[i].set_actions(0);
                sp = tsp;
            }
        }
        else if (evt == EVevent) {
            int lsp = sp;
            if (!acts[lsp].actions())
                lsp--;
            int tsp = lsp;
            while (tsp > 0 && acts[tsp].type() == Sequential)
                tsp--;
            if (tsp != lsp) {
                vl_action_item *anew = new vl_action_item(0, 0);
                anew->set_stack(new vl_stack(acts + tsp + 1, lsp - tsp));
                anew->set_event(ev.event);
                // put in event queue
                anew->event()->chain(anew);
                delete anew;
                for (int i = tsp+1; i <= lsp; i++)
                    acts[i].set_actions(0);
                sp = tsp;
            }
        }
        if (acts[sp].type() == Fork && !acts[sp].actions()) {
            // We're done setting up a fork/join block, and can now
            // provide a stack to the action item in the fork/join
            // completion list.

            vl_stmt *fj = acts[sp].fjblk();
            acts[sp].set_fjblk(0);

            vl_action_item *af = 0;
            if (fj && fj->type() == ForkJoinStmt) {
                for (af = sim->fj_end(); af; af = af->next()) {
                    if (af->stmt() == fj)
                        break;
                }
            }
            if (af) {
                // Add the stack.  If there is no stack, remove the
                // action from the list.  The threads may or may not
                // have completed.  If so, remove the action and add
                // it to the queue.

                int tsp = --sp;
                while (tsp > 0 && acts[tsp].type() == Sequential)
                    tsp--;
                if (tsp != sp) {
                    af->set_stack(new vl_stack(acts + tsp + 1, sp - tsp));
                    for (int i = tsp+1; i <= sp; i++)
                        acts[i].set_actions(0);
                    sp = tsp;
                }
                if (!af->stack() || ((vl_fork_join_stmt*)fj)->endcnt() <= 0) {
                    // remove from list
                    vl_action_item *ap = 0;
                    for (vl_action_item *aa = sim->fj_end();
                            aa; aa = aa->next()) {
                        if (aa == af) {
                            if (!ap)
                                sim->set_fj_end(aa->next());
                            else
                                ap->set_next(aa->next());
                            break;
                        }
                        ap = aa;
                    }
                }
                if (!af->stack())
                    delete af;
                else if (((vl_fork_join_stmt*)fj)->endcnt() <= 0) {
                    af->set_stmt(0);
                    sim->timewheel()->append(sim->time(), af);
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


// Get rid of all pending actions with blk in the context hierarchy.
//
void
vl_timeslot::purge(vl_stmt *blk)
{
    for (vl_timeslot *ts = this; ts; ts = ts->ts_next) {
        if (ts->ts_actions) {
            ts->ts_actions = ts->ts_actions->purge(blk);
            ts->ts_a_end = ts->ts_actions;
            if (ts->ts_a_end) {
                while (ts->ts_a_end->next())
                    ts->ts_a_end = ts->ts_a_end->next();
            }
        }
        if (ts->ts_zdly_actions) {
            ts->ts_zdly_actions = ts->ts_zdly_actions->purge(blk);
            ts->ts_z_end = ts->ts_zdly_actions;
            if (ts->ts_z_end) {
                while (ts->ts_z_end->next())
                    ts->ts_z_end = ts->ts_z_end->next();
            }
        }
        if (ts->ts_nbau_actions) {
            ts->ts_nbau_actions = ts->ts_nbau_actions->purge(blk);
            ts->ts_n_end = ts->ts_nbau_actions;
            if (ts->ts_n_end) {
                while (ts->ts_n_end->next())
                    ts->ts_n_end = ts->ts_n_end->next();
            }
        }
        if (ts->ts_mon_actions) {
            ts->ts_mon_actions = ts->ts_mon_actions->purge(blk);
            ts->ts_m_end = ts->ts_mon_actions;
            if (ts->ts_m_end) {
                while (ts->ts_m_end->next())
                    ts->ts_m_end = ts->ts_m_end->next();
            }
        }
    }
}


// Diagnostic printout for current time slot.
//
void
vl_timeslot::print(ostream &outs)
{
    outs << "time " << (int)ts_time << '\n';
    for (vl_action_item *a = ts_actions; a; a = a->next())
        a->print(outs);
    for (vl_action_item *a = ts_zdly_actions; a; a = a->next())
        a->print(outs);
    for (vl_action_item *a = ts_nbau_actions; a; a = a->next())
        a->print(outs);
}
// End vl_timeslot functions.


/*
vl_monitor::~vl_monitor()
{
    // delete args ?
    vl_context::destroy(m_cx);
}
*/
// End vl_monitor functions.


//---------------------------------------------------------------------------
//  Verilog description objects
//---------------------------------------------------------------------------

vl_desc::vl_desc()
{
    d_tstep = 1.0;
    d_modules = new lsList<vl_module*>;
    d_primitives = new lsList<vl_primitive*>;
    d_mp_st = new table<vl_mp*>;
    d_mp_undefined = set_empty();
}


vl_desc::~vl_desc()
{
    delete d_modules;   // modules deleted in mp_st
    delete d_primitives;
    if (d_mp_st) {
        table_gen<vl_mp*> gen(d_mp_st);
        vl_mp *mp;
        const char *key;
        while (gen.next(&key, &mp))
            delete mp;
        delete d_mp_st;
    }
    delete d_mp_undefined;
}


void
vl_desc::dump(ostream &outs)
{
    vl_module *mod;
    lsGen<vl_module*> gen(d_modules);
    while (gen.next(&mod))
        mod->dump(outs);
}


// Static function.
//
vl_gate_inst_list *
vl_desc::add_gate_inst(int t, vl_dlstr *dlstr, lsList<vl_gate_inst*> *g)
{
    // set up pointers to evaluation/setup functions
    lsGen<vl_gate_inst*> gen(g);
    vl_gate_inst *gate;
    while (gen.next(&gate))
        gate->set_type(t);
    return (new vl_gate_inst_list(t, dlstr, g));
}


vl_stmt *
vl_desc::add_mp_inst(char *mp_name, vl_dlstr *dlstr,
    lsList<vl_mp_inst*> *instances)
{
    // If the module hasn't been parsed yet, create and add to symbol
    // table, and also to undefined list.

    vl_mp *mod_pri;
    vl_stmt *retval = 0;
    if (!d_mp_st->lookup(mp_name, &mod_pri) || !mod_pri) {
        if (!d_mp_undefined->set_find(mp_name))
            d_mp_undefined->set_add(vl_strdup(mp_name));
        retval = new vl_mp_inst_list(MPundef, mp_name, dlstr, instances);
    }
    else {
        if (mod_pri->type() == ModDecl) {
            retval = new vl_mp_inst_list(MPmod, mp_name, dlstr, instances);
            mod_pri->inc_inst_count();
        }
        else if (mod_pri->type() == CombPrimDecl ||
                mod_pri->type() == SeqPrimDecl) {
            retval = new vl_mp_inst_list(MPprim, mp_name, dlstr, instances);
            mod_pri->inc_inst_count();
        }
        else
            VP()->error(ERR_COMPILE, "unknown module/primitive");
    }
    return (retval);
}
// End vl_desc functions.


void
vl_module::dump(ostream &outs)
{
    int indnt = 0;
    outs << "Module " << mp_name << ' ' << (void*)this << '\n';

    bool found = false;
    table_gen<vl_var*> sig_g(mp_sig_st);
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
    lsGen<vl_port*> pgen(mp_ports);
    vl_port *p;
    while (pgen.next(&p)) {
        lsGen<vl_var*> vgen(p->port_exp());
        vl_var *v;
        while (vgen.next(&v)) {
            if (!found) {
                outs << "  Ports table\n";
                found = true;
            }
            outs << "    " << v->name() << ' ' << (void*)v << '\n';
        }
    }

    found = false;
    table_gen<vl_inst*> inst_g(m_inst_st);
    vl_inst *inst;
    while (inst_g.next(&vname, &inst)) {
        if (!found) {
            outs << "  Instance table\n";
            found = true;
        }
        outs << "    " << vname << ' ' << (void*)inst << '\n';
    }

    found = false;
    table_gen<vl_task*> task_g(m_task_st);
    vl_task *task;
    while (task_g.next(&vname, &task)) {
        if (!found) {
            outs << "  Task table\n";
            found = true;
        }
        task->dump(outs, indnt + 2);
    }

    found = false;
    table_gen<vl_function*> func_g(m_func_st);
    vl_function *func;
    while (func_g.next(&vname, &func)) {
        if (!found) {
            outs << "  Function table\n";
            found = true;
        }
        func->dump(outs, indnt + 2);
    }

    found = false;
    table_gen<vl_stmt*> block_g(m_blk_st);
    vl_stmt *blk;
    while (block_g.next(&vname, &blk)) {
        if (!found) {
            outs << "  Block table\n";
            found = true;
        }
        blk->dump(outs, indnt + 2);
    }

    outs << '\n';

    lsGen<vl_stmt*> mgen(m_mod_items);
    vl_stmt *item;
    while (mgen.next(&item)) {
        if (item->type() == ModPrimInst) {
            vl_mp_inst_list *list = (vl_mp_inst_list*)item;
            lsGen<vl_mp_inst*> mpgen(list->mps());
            vl_mp_inst *mpinst;
            while (mpgen.next(&mpinst)) {
                cout << "Instance " << mpinst->name() << '\n';
                if (mpinst->master())
                    mpinst->master()->dump(outs);
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
// This should (hopefully) ensure thing are defined when needed.
//
void
vl_module::sort_moditems()
{
    if (!m_mod_items)
        return;
    lsList<vl_stmt*> *nlist = new lsList<vl_stmt*>;

    lsGen<vl_stmt*> gen(m_mod_items);
    vl_stmt *stmt;
    while (gen.next(&stmt)) {
        if (stmt->type() >= RealDecl && stmt->type() <= TriregDecl &&
                stmt->type() != DefparamDecl)
            nlist->newEnd(stmt);
    }
    gen.reset(m_mod_items);
    while (gen.next(&stmt)) {
        if (stmt->type() == ModPrimInst)
            nlist->newEnd(stmt);
    }
    gen.reset(m_mod_items);
    while (gen.next(&stmt)) {
        if (stmt->type() >= AndGate && stmt->type() <= Rtranif1Gate)
            nlist->newEnd(stmt);
    }
    gen.reset(m_mod_items);
    while (gen.next(&stmt)) {
        if (stmt->type() == SpecBlock)
            nlist->newEnd(stmt);
    }
    gen.reset(m_mod_items);
    while (gen.next(&stmt)) {
        if (stmt->type() == TaskDecl)
            nlist->newEnd(stmt);
    }
    gen.reset(m_mod_items);
    while (gen.next(&stmt)) {
        if (stmt->type() >= IntFuncDecl && stmt->type() <= RangeFuncDecl)
            nlist->newEnd(stmt);
    }
    gen.reset(m_mod_items);
    while (gen.next(&stmt)) {
        if (stmt->type() == DefparamDecl)
            nlist->newEnd(stmt);
    }
    gen.reset(m_mod_items);
    while (gen.next(&stmt)) {
        if (stmt->type() == ContAssign)
            nlist->newEnd(stmt);
    }
    gen.reset(m_mod_items);
    while (gen.next(&stmt)) {
        if (stmt->type() == AlwaysStmt || stmt->type() == InitialStmt)
            nlist->newEnd(stmt);
    }
    if (nlist->length() != m_mod_items->length()) {
        vl_error("internal, something strange in mod items list");
        VS()->abort();
    }
    delete m_mod_items;
    m_mod_items = nlist;
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
    // so the parameter setting will be top-down.

    if (mp_instance && mp_instance->inst_list() &&
            mp_instance->inst_list()->prms_or_dlys()) {
        vl_delay *params = mp_instance->inst_list()->prms_or_dlys();
        if (params->delay1()) {
            lsGen<vl_stmt*> gen(m_mod_items);
            vl_stmt *stmt;
            while (gen.next(&stmt)) {
                if (stmt->type() == ParamDecl) {
                    vl_decl *decl = (vl_decl*)stmt;
                    lsGen<vl_bassign_stmt*> bgen(decl->list());
                    vl_bassign_stmt *bs;
                    if (bgen.next(&bs))
                        *bs->lhs() = params->delay1()->eval();
                    break;
                }
            }
        }
        else if (params->list()) {
            lsGen<vl_expr*> gen(params->list());
            vl_expr *e;
            lsGen<vl_stmt*> sgen(m_mod_items);
            vl_stmt *stmt;
            while (sgen.next(&stmt)) {
                if (stmt->type() == ParamDecl) {
                    vl_decl *decl = (vl_decl*)stmt;
                    lsGen<vl_bassign_stmt*> bgen(decl->list());
                    vl_bassign_stmt *bs;
                    while (bgen.next(&bs)) {
                        if (!gen.next(&e))
                            break;
                        *bs->lhs() = e->eval();
                    }
                }
            }
        }
    }

    sim->push_context(this);
    vl_setup_list(sim, m_mod_items);
    sim->pop_context();
}
// End vl_module functions.


void
vl_primitive::dump(ostream &outs)
{
    outs << "Primitive " << mp_name << ' ' << (void*)this << '\n';

    bool found = false;
    table_gen<vl_var*> sig_g(mp_sig_st);
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
    lsGen<vl_port*> pgen(mp_ports);
    vl_port *p;
    while (pgen.next(&p)) {
        lsGen<vl_var*> vgen(p->port_exp());
        vl_var *v;
        while (vgen.next(&v)) {
            if (!found) {
                outs << "  Ports table\n";
                found = true;
            }
            outs << "    " << v->name() << ' ' << (void*)v << '\n';
        }
    }
    outs << '\n';
}


void
vl_primitive::setup(vl_simulator *sim)
{
    sim->push_context(this);
    vl_setup_list(sim, p_decls);
    if (mp_type == SeqPrimDecl && p_initial)
        p_initial->setup(sim);
    sim->pop_context();
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
        if (mp_type == CombPrimDecl) {
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
        if (mp_type == CombPrimDecl) {
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
// End vl_primitive functions.


//---------------------------------------------------------------------------
//  Module items
//---------------------------------------------------------------------------

void
vl_decl::setup(vl_simulator *sim)
{
    // setup_vars() has alrady been called, all vars are in symbol table
    if (st_type == DefparamDecl) {
        // This evaluation is deferred until now
        lsGen<vl_bassign_stmt*> gen(d_list);
        vl_bassign_stmt *assign;
        while (gen.next(&assign)) {
            vl_var *v =
                sim->context()->lookup_var(assign->lhs()->name(), false);
            if (!v) {
                vl_error("can not resolve defparam %s", assign->lhs()->name());
                sim->abort();
            }
            else {
                if (v != assign->lhs()) {
                    delete assign->lhs();
                    assign->set_lhs(v);
                }
                assign->or_flags(BAS_SAVE_LHS);
                *assign->lhs() = assign->rhs()->eval();
            }
        }
        return;
    }
    if (st_type == ParamDecl)
        return;
    if (d_list) {
        // declared with assignment
        lsGen<vl_bassign_stmt*> agen(d_list);
        vl_bassign_stmt *bs;
        while (agen.next(&bs)) {
            bs->lhs()->set_strength(d_strength);
            // set up implicit continuous assignment
            bs->set_wait(d_delay);
            bs->rhs()->chain(bs);
            bs->setup(sim);
        }
    }
}
// End vl_decl functions.


void
vl_procstmt::setup(vl_simulator *sim)
{
    if (pr_stmt) {
        sim->timewheel()->append(sim->time(),
            new vl_action_item(this, sim->context()));
    }
    pr_lasttime = (vl_time_t)-1;
}


EVtype
vl_procstmt::eval(vl_event*, vl_simulator *sim)
{
    if (st_type == AlwaysStmt) {
        // If the statement is one of the following, do one loop per time
        // point rather than looping infinitely.

        bool single_pass = false;
        switch (pr_stmt->type()) {
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

        if (pr_lasttime != sim->time() || !single_pass) {
            // have to loop around when finished
            pr_lasttime = sim->time();
            pr_stmt->setup(sim);
            sim->timewheel()->append(sim->time(),
                new vl_action_item(this, sim->context()));
        }
        else {
            // enable another run-through at next time point
            vl_action_item *anew = new vl_action_item(this, sim->context());
            if (!sim->next_actions())
                sim->set_next_actions(anew);
            else {
                for (vl_action_item *a = sim->next_actions(); a;
                        a = a->next()) {
                    if (a->stmt() && a->stmt() == anew->stmt()) {
                        delete anew;
                        break;
                    }
                    if (!a->next()) {
                        a->set_next(anew);
                        break;
                    }
                }
            }
        }
    }
    else if (st_type == InitialStmt)
        pr_stmt->setup(sim);
    return (EVnone);
}
// End vl_procstmt functions.


void
vl_cont_assign::setup(vl_simulator *sim)
{
    // this is a "mod item", applies only to nets
    vl_bassign_stmt *stmt;
    lsGen<vl_bassign_stmt*> gen(c_assigns);
    while (gen.next(&stmt)) {
        stmt->setup(sim);
        if (!stmt->lhs()->check_net_type(REGwire)) {
            vl_error("continuous assignment to non-net %s",
                stmt->lhs()->name() ? stmt->lhs()->name() : "concatenation");
            sim->abort();
            return;
        }
        // The drive strength is stored in the vl_var part of the
        // driving expression, since it is unique to this assignment.

        stmt->rhs()->set_strength(c_strength);
        if (c_delay)
            stmt->set_wait(c_delay);
        stmt->rhs()->chain(stmt);
    }
}
// End vl_cont_assign functions.


void
vl_task::disable(vl_stmt *s)
{
    vl_disable_list(t_stmts, s);
}


void
vl_task::dump(ostream &outs, int indnt)
{
    char buf[128];
    for (int i = 0; i < 2*indnt; i++)
        buf[i] = ' ';
    buf[2*indnt] = 0;

    outs << buf << t_name << " " << (void*)this << '\n';
    bool found = false;
    table_gen<vl_var*> sig_g(t_sig_st);
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
    table_gen<vl_stmt*> block_g(t_blk_st);
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
// return value is saved in outport.
//
void
vl_function::eval_func(vl_var *out, lsList<vl_expr*> *args)
{
    if (!f_sig_st)
        f_sig_st = new table<vl_var*>;
    vl_var *outvar;
    if (!f_sig_st->lookup(f_name, &outvar)) {
        outvar = new vl_var;
        outvar->set_name(vl_strdup(f_name));
        if (st_type == RangeFuncDecl)
            outvar->configure(f_range, RegDecl);
        else if (st_type == RealFuncDecl)
            outvar->configure(0, RealDecl);
        else if (st_type == IntFuncDecl)
            outvar->configure(0, IntDecl);
        else
            outvar->configure(0, RegDecl);
        f_sig_st->insert(outvar->name(), outvar);
        outvar->or_flags(VAR_IN_TABLE);
    }

    vl_simulator *sim = VS();
    vl_action_item *atmp = sim->timewheel()->actions();
    sim->timewheel()->set_actions(0);

    sim->push_context(this);
    vl_setup_list(sim, f_decls);
    sim->pop_context();

    lsGen<vl_expr*> agen(args);
    lsGen<vl_decl*> dgen(f_decls);
    vl_expr *ae;
    vl_decl *decl;
    bool done = false;
    while (dgen.next(&decl)) {
        lsGen<vl_var*> vgen(decl->ids());
        vl_var *av;
        while (vgen.next(&av)) {
            if (av->net_type() >= REGwire)
                av->set_net_type(REGreg);
            if (agen.next(&ae)) {
                if (av->io_type() == IOoutput || av->io_type() == IOinout) {
                    vl_error("function %s can not have output port %s",
                        f_name, av->name());
                    sim->abort();
                    continue;
                }
                if (av->io_type() == IOinput) {
                    vl_bassign_stmt *bs =
                        new vl_bassign_stmt(BassignStmt, av, 0, 0, ae);
                    bs->or_flags(SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
                    vl_action_item *a = new vl_action_item(bs, sim->context());
                    a->or_flags(AI_DEL_STMT);
                    sim->timewheel()->append(sim->time(), a);
                }
                else {
                    vl_warn("too many args to function %s", f_name);
                    done = true;
                    break;
                }
            }
            else {
                if (av->io_type() == IOinput)
                    vl_warn("too few args to function %s", f_name);
                done = true;
                break;
            }
        }
        if (done)
            break;
    }
    sim->push_context(this);
    if (f_stmts)
        vl_setup_list(sim, f_stmts);
    while (sim->timewheel()->actions())
        sim->timewheel()->do_actions(sim);
    sim->timewheel()->set_actions(atmp);
    sim->pop_context();
    *out = *outvar;
}


void
vl_function::dump(ostream &outs, int indnt)
{
    char buf[128];
    for (int i = 0; i < 2*indnt; i++)
        buf[i] = ' ';
    buf[2*indnt] = 0;

    outs << buf << f_name << " " << (void*)this << '\n';
    bool found = false;
    table_gen<vl_var*> sig_g(f_sig_st);
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
    table_gen<vl_stmt*> block_g(f_blk_st);
    vl_stmt *blk;
    while (block_g.next(&vname, &blk)) {
        if (!found) {
            outs << buf << "  Block table\n";
            found = true;
        }
        blk->dump(outs, indnt+2);
    }
}
// End vl_function functions.


void
vl_gate_inst_list::setup(vl_simulator *sim)
{
    vl_setup_list(sim, g_gates);
}
// End vl_gate_inst_list functions.


void
vl_mp_inst_list::setup(vl_simulator *sim)
{
    if (mp_mptype == MPundef) {
        vl_mp *mp;
        if (!sim->description()->mp_st()->lookup(mp_name, &mp) || !mp) {
            vl_error("no master found for %s", mp_name);
            sim->abort();
            return;
        }
        if (mp->type() == ModDecl)
            mp_mptype = MPmod;
        else
            mp_mptype = MPprim;
    }
    vl_setup_list(sim, mp_mps);
}
// End vl_mp_inst_list functions.


//---------------------------------------------------------------------------
//  Statements
//---------------------------------------------------------------------------

void
vl_bassign_stmt::setup(vl_simulator *sim)
{
    if (ba_lhs->name() && ba_lhs->data_type() == Dnone &&
            ba_lhs->net_type() == REGnone) {
        if (!sim->context()) {
            vl_error("internal, no current context!");
            sim->abort();
            return;
        }
        vl_var *nvar = sim->context()->lookup_var(ba_lhs->name(), false);
        if (!nvar) {
            vl_warn("implicit declaration of %s", ba_lhs->name());
            vl_module *cmod = sim->context()->currentModule();
            if (cmod) {
                nvar = new vl_var;
                nvar->set_name(vl_strdup(ba_lhs->name()));
                if (!cmod->sig_st())
                    cmod->set_sig_st(new table<vl_var*>);
                cmod->sig_st()->insert(nvar->name(), nvar);
                nvar->or_flags(VAR_IN_TABLE);
            }
            else {
                vl_error("internal, no current module!");
                sim->abort();
                return;
            }
        }
        if (nvar != ba_lhs) {
            if (strcmp(nvar->name(), ba_lhs->name())) {
                // from another module, don't free it!
                st_flags |= BAS_SAVE_LHS;
            }
            vl_var *olhs = ba_lhs;
            ba_lhs = nvar;
            ba_range = olhs->range();
            olhs->set_range(0);
            delete olhs;
        }
    }
    if (ba_lhs->flags() & VAR_IN_TABLE)
        st_flags |= BAS_SAVE_LHS;
    if (ba_lhs->net_type() >= REGwire && ba_lhs->delay())
        ba_wait = ba_lhs->delay();
    sim->timewheel()->append(sim->time(),
        new vl_action_item(this, sim->context()));
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

    if (st_type == AssignStmt) {
        if (!ba_lhs->check_net_type(REGreg)) {
            vl_error("non-reg %s in procedural continuous assign",
                ba_lhs->name() ? ba_lhs->name() : "in concatenation");
            sim->abort();
            return (EVnone);
        }
        // procedural continuous assign 
        if (ba_range)
            vl_warn("bit or part select in procedural continuous "
                "assignment ignored");
        ba_lhs->set_assigned(this);
    }
    else if (st_type == ForceStmt) {
        if (ba_range)
            vl_warn("bit or part select in force assignment ignored");
        ba_lhs->set_forced(this);
    }
    else if (st_type == BassignStmt) {
        // a = ( )
        if (ba_wait) {
            // Delayed continuous assignment, wait time td before
            // evaluating and assigning rhs.

            vl_bassign_stmt *bs =
                new vl_bassign_stmt(BassignStmt, ba_lhs, 0, 0, ba_rhs);
            bs->or_flags(SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
            bs->freeze_indices(ba_range);
            vl_action_item *a = new vl_action_item(bs, sim->context());
            a->or_flags(AI_DEL_STMT);
            vl_time_t td = ba_wait->eval();
            if (td > 0)
                sim->timewheel()->append(sim->time() + td, a);
            else
                sim->timewheel()->append_zdly(sim->time(), a);
        }
        else
            ba_lhs->assign(ba_range, &ba_rhs->eval(), 0);
    }
    else if (st_type == DelayBassignStmt) {
        // a = #xx ( )
        // Delayed assignment, compute the present rhs value and save
        // it in a temp variable.  Apply the value after the delay.
        // The next statement executes after the delay.

        if (!ba_tmpvar)
            ba_tmpvar = new vl_var;
        *ba_tmpvar = ba_rhs->eval();
        vl_bassign_stmt *bs =
            new vl_bassign_stmt(BassignStmt, ba_lhs, 0, 0, ba_tmpvar);
        bs->or_flags(SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
        bs->ba_wait = ba_wait;  // propagate wire delay
        bs->freeze_indices(ba_range);
        vl_action_item *a = new vl_action_item(bs, sim->context());
        a->or_flags(AI_DEL_STMT);
        vl_time_t td = ba_delay->eval();
        sim->timewheel()->append(sim->time(), a);
        ev->time = sim->time() + td;
        return (EVdelay);
    }
    else if (st_type == EventBassignStmt) {
        // a = @( )
        // Event-triggered assignment, compute the rhs value and save
        // it in a temp variable.  Apply the value when triggered.  The
        // next statement executes after trigger.

        if (!ba_tmpvar)
            ba_tmpvar = new vl_var;
        *ba_tmpvar = ba_rhs->eval();
        vl_bassign_stmt *bs =
            new vl_bassign_stmt(BassignStmt, ba_lhs, 0, 0, ba_tmpvar);
        bs->or_flags(SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
        bs->ba_wait = ba_wait;  // propagate wire delay
        bs->freeze_indices(ba_range);
        vl_action_item *a = new vl_action_item(bs, sim->context());
        a->or_flags(AI_DEL_STMT);
        sim->timewheel()->append(sim->time(), a);
        ev->event = ba_event;
        return (EVevent);
    }
    else if (st_type == NbassignStmt) {
        // a <= ( )
        // Non-blocking assignment, compute the rhs and save it in a
        // temp variable.  Apply the value after other events have been
        // processed, by appending the action to the nbau list.

        vl_var *src;
        if (st_flags & SIM_INTERNAL) {
            // This was spawned by a DelayBassign or EventBassign, use
            // the parent's temp variable.  Since the rhs may be added
            // to the lhs driver list (for nets), we have to drive with
            // a variable that isn't freed.

            src = ba_rhs;
        }
        else {
            if (!ba_tmpvar)
                ba_tmpvar = new vl_var;
            *ba_tmpvar = ba_rhs->eval();
            src = ba_tmpvar;
        }
        vl_time_t td;
        if (ba_wait && (td = ba_wait->eval()) > 0) {
            vl_bassign_stmt *bs =
                new vl_bassign_stmt(NbassignStmt, ba_lhs, 0, 0, src);
            bs->or_flags(SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
            vl_time_t ttd = ba_wait->eval();
            bs->freeze_indices(ba_range);
            vl_action_item *a = new vl_action_item(bs, sim->context());
            a->or_flags(AI_DEL_STMT);
            sim->timewheel()->append(sim->time() + ttd, a);
        }
        else {
            vl_bassign_stmt *bs =
                new vl_bassign_stmt(BassignStmt, ba_lhs, 0, 0, src);
            bs->or_flags(SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
            bs->freeze_indices(ba_range);
            vl_action_item *a = new vl_action_item(bs, sim->context());
            a->or_flags(AI_DEL_STMT);
            sim->timewheel()->append_nbau(sim->time(), a);
        }
    }
    else if (st_type == DelayNbassignStmt) {
        // a <= #xx ( )
        // Delayed non-blocking assignment, compute the rhs and save it in
        // a temporary variable.  Spawn a regular non-blocking assignment
        // after the delay.

        if (!ba_tmpvar)
            ba_tmpvar = new vl_var;
        *ba_tmpvar = ba_rhs->eval();
        vl_time_t td = ba_delay->eval();
        if (td) {
            vl_bassign_stmt *bs =
                new vl_bassign_stmt(NbassignStmt, ba_lhs, 0, 0, ba_tmpvar);
            bs->or_flags(SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
            bs->ba_wait = ba_wait;  // propagate wire delay
            bs->freeze_indices(ba_range);
            vl_action_item *a = new vl_action_item(bs, sim->context());
            a->or_flags(AI_DEL_STMT);
            sim->timewheel()->append(sim->time() + td, a);
        }
        else {
            vl_bassign_stmt *bs =
                new vl_bassign_stmt(BassignStmt, ba_lhs, 0, 0, ba_tmpvar);
            bs->or_flags(SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
            bs->ba_wait = ba_wait;  // propagate wire delay
            bs->freeze_indices(ba_range);
            vl_action_item *a = new vl_action_item(bs, sim->context());
            a->or_flags(AI_DEL_STMT);
            sim->timewheel()->append_nbau(sim->time(), a);
        }
    }
    else if (st_type == EventNbassignStmt) {
        // a <= @( )
        // Event-triggered non-blocking assignment, compute the rhs and
        // save it in a temp variable.  Spawn a regular non-blocking
        // assignment when triggered.

        if (!ba_tmpvar)
            ba_tmpvar = new vl_var;
        *ba_tmpvar = ba_rhs->eval();
        vl_bassign_stmt *bs =
            new vl_bassign_stmt(NbassignStmt, ba_lhs, 0, 0, ba_tmpvar);
        bs->or_flags(SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
        bs->ba_wait = ba_wait;  // propagate wire delay
        bs->freeze_indices(ba_range);
        if (ba_event && ba_event->repeat())
            ba_event->set_count(ba_event->repeat()->eval());
        vl_action_item *a = new vl_action_item(bs, sim->context());
        a->or_flags(AI_DEL_STMT);
        a->set_event(ba_event);
        ba_event->chain(a);
    }
    return (EVnone);
}


// For a delayed assignment, evaluate any indices used in the lhs.  The
// range is the quantity that would normally be copied to the stmt.
//
void
vl_bassign_stmt::freeze_indices(vl_range *rng)
{
    if (rng)
        ba_range = rng->reval();
    if (ba_lhs->data_type() == Dconcat) {
        ba_lhs = chk_copy(ba_lhs);
        ba_lhs->freeze_concat();
        st_flags &= ~BAS_SAVE_LHS;
    }
}
// End vl_bassign_stmt functions.


void
vl_sys_task_stmt::setup(vl_simulator *sim)
{
    if (sy_flags & SYSafter)
        sim->timewheel()->append_mon(sim->time(), new vl_action_item(this,
            sim->context()));
    else
        sim->timewheel()->append(sim->time(), new vl_action_item(this,
            sim->context()));
}


EVtype
vl_sys_task_stmt::eval(vl_event*, vl_simulator *sim)
{
    (sim->*this->action)(this, sy_args);
    return (EVnone);
}
// End vl_sys_task_stmt functions.


void
vl_begin_end_stmt::setup(vl_simulator *sim)
{
    sim->push_context(this);
    vl_setup_list(sim, be_decls);
    sim->timewheel()->append(sim->time(),
        new vl_action_item(this, sim->context()));
    sim->pop_context();
}


EVtype
vl_begin_end_stmt::eval(vl_event*, vl_simulator *sim)
{
    vl_setup_list(sim, be_stmts);
    return (EVnone);
}


void
vl_begin_end_stmt::disable(vl_stmt *s)
{
    // If the last stmt is a vl_fj_break, have to notify of termination.

    vl_stmt *fj;
    be_stmts->lastItem(&fj);
    if (fj && fj->type() == ForkJoinBreak)
        fj->setup(VS());
    vl_disable_list(be_stmts, s);
}


void
vl_begin_end_stmt::dump(ostream &outs, int indnt)
{
    char buf[128];
    for (int i = 0; i < 2*indnt; i++)
        buf[i] = ' ';
    buf[2*indnt] = 0;

    outs << buf << be_name << " (begin/end) " << (void*)this << '\n';
    bool found = false;
    table_gen<vl_var*> sig_g(be_sig_st);
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
    table_gen<vl_stmt*> block_g(be_blk_st);
    vl_stmt *blk;
    while (block_g.next(&vname, &blk)) {
        if (!found) {
            outs << buf << "  Block table\n";
            found = true;
        }
        blk->dump(outs, indnt+2);
    }
}
// End vl_begin_end_stmt functions.


void
vl_if_else_stmt::setup(vl_simulator *sim)
{
    if (ie_cond) {
        sim->timewheel()->append(sim->time(),
            new vl_action_item(this, sim->context()));
    }
}


EVtype
vl_if_else_stmt::eval(vl_event*, vl_simulator *sim)
{
    if ((int)ie_cond->eval()) {
        if (ie_if_stmt)
            ie_if_stmt->setup(sim);
    }
    else {
        if (ie_else_stmt)
            ie_else_stmt->setup(sim);
    }
    return (EVnone);
}


void
vl_if_else_stmt::disable(vl_stmt *s)
{
    if (ie_if_stmt)
        ie_if_stmt->disable(s);
    if (ie_else_stmt)
        ie_else_stmt->disable(s);
}
// End vl_if_else_stmt functions.


void
vl_case_stmt::setup(vl_simulator *sim)
{
    sim->timewheel()->append(sim->time(),
        new vl_action_item(this, sim->context()));
}


EVtype
vl_case_stmt::eval(vl_event*, vl_simulator *sim)
{
    vl_var d = c_cond->eval();
    lsGen<vl_case_item*> gen(c_case_items);
    vl_case_item *item;
    while (gen.next(&item)) {
        if (item->type() == CaseItem) {
            lsGen<vl_expr*> egen(item->exprs());
            vl_expr *e;
            while (egen.next(&e)) {
                vl_var z;
                if (st_type == CaseStmt)
                    z = case_eq(d, e->eval());
                else if (st_type == CasexStmt)
                    z = casex_eq(d, e->eval());
                else if (st_type == CasezStmt)
                    z = casez_eq(d, e->eval());
                else {
                    vl_error("(internal) unknown case type");
                    sim->abort();
                    return (EVnone);
                }
                if (z.data_s()[0] == BitH) {
                    if (item->stmt())
                        item->stmt()->setup(sim);
                    return (EVnone);
                }
            }
        }
    }
    gen = lsGen<vl_case_item*>(c_case_items);
    while (gen.next(&item)) {
        if (item->type() == DefaultItem && item->stmt())
            item->stmt()->setup(sim);
    }
    return (EVnone);
}


void
vl_case_stmt::disable(vl_stmt *s)
{
    vl_disable_list(c_case_items, s);
}


void
vl_case_item::disable(vl_stmt *s)
{
    if (ci_stmt)
        ci_stmt->disable(s);
}
// End vl_case_stmt functions.


void
vl_forever_stmt::setup(vl_simulator *sim)
{
    if (f_stmt) {
        sim->timewheel()->append(sim->time(),
            new vl_action_item(this, sim->context()));
    }
}


EVtype
vl_forever_stmt::eval(vl_event*, vl_simulator *sim)
{
    f_stmt->setup(sim);
    sim->timewheel()->append(sim->time(),
        new vl_action_item(this, sim->context()));
    return (EVnone);
}


void
vl_forever_stmt::disable(vl_stmt *s)
{
    if (f_stmt)
        f_stmt->disable(s);
}
// End vl_forever_stmt functions.


void
vl_repeat_stmt::setup(vl_simulator *sim)
{
    r_cur_count = -1;
    if (r_count && r_stmt)
        sim->timewheel()->append(sim->time(),
            new vl_action_item(this, sim->context()));
}


EVtype
vl_repeat_stmt::eval(vl_event*, vl_simulator *sim)
{
    if (r_cur_count < 0)
        r_cur_count = (int)r_count->eval();
    if (r_cur_count > 0) {
        r_stmt->setup(sim);
        r_cur_count--;
        if (r_cur_count > 0) {
            sim->timewheel()->append(sim->time(),
                new vl_action_item(this, sim->context()));
        }
    }
    return (EVnone);
}


void
vl_repeat_stmt::disable(vl_stmt *s)
{
    if (r_stmt)
        r_stmt->disable(s);
}
// End vl_repeat_stmt functions.


void
vl_while_stmt::setup(vl_simulator *sim)
{
    sim->timewheel()->append(sim->time(),
        new vl_action_item(this, sim->context()));
}


EVtype
vl_while_stmt::eval(vl_event*, vl_simulator *sim)
{
    if (!w_cond || (int)w_cond->eval()) {
        if (w_stmt)
            w_stmt->setup(sim);
        sim->timewheel()->append(sim->time(),
            new vl_action_item(this, sim->context()));
    }
    return (EVnone);
}


void
vl_while_stmt::disable(vl_stmt *s)
{
    if (w_stmt)
        w_stmt->disable(s);
}
// End vl_while_stmt functions.


void
vl_for_stmt::setup(vl_simulator *sim)
{
    if (f_initial)
        f_initial->setup(sim);
    sim->timewheel()->append(sim->time(),
        new vl_action_item(this, sim->context()));
}


EVtype
vl_for_stmt::eval(vl_event*, vl_simulator *sim)
{
    if (!f_cond || (int)f_cond->eval()) {
        if (f_stmt)
            f_stmt->setup(sim);
        if (f_end)
            f_end->setup(sim);
        sim->timewheel()->append(sim->time(),
            new vl_action_item(this, sim->context()));
    }
    return (EVnone);
}


void
vl_for_stmt::disable(vl_stmt *s)
{
    if (f_stmt)
        f_stmt->disable(s);
}
// End vl_for_stmt functions.


void
vl_delay_control_stmt::setup(vl_simulator *sim)
{
    sim->timewheel()->append(sim->time(),
        new vl_action_item(this, sim->context()));
}


EVtype
vl_delay_control_stmt::eval(vl_event *ev, vl_simulator *sim)
{
    ev->time = sim->time() + d_delay->eval();
    if (d_stmt)
        d_stmt->setup(sim);
    return (EVdelay);
}


void
vl_delay_control_stmt::disable(vl_stmt *s)
{
    if (d_stmt)
        d_stmt->disable(s);
}
// End vl_delay_control_stmt functions.


void
vl_event_control_stmt::setup(vl_simulator *sim)
{
    if (!ec_event)
        return;
    sim->timewheel()->append(sim->time(),
        new vl_action_item(this, sim->context()));
}


EVtype
vl_event_control_stmt::eval(vl_event *ev, vl_simulator *sim)
{
    if (ec_stmt)
        ec_stmt->setup(sim);
    ev->event = ec_event;
    return (EVevent);
}


void
vl_event_control_stmt::disable(vl_stmt *s)
{
    ec_event->unchain_disabled(s);
    if (ec_stmt)
        ec_stmt->disable(s);
}
// End vl_event_control_stmt functions.


void
vl_wait_stmt::setup(vl_simulator *sim)
{
    if (!w_cond)
        return;
    sim->timewheel()->append(sim->time(),
        new vl_action_item(this, sim->context()));
}


EVtype
vl_wait_stmt::eval(vl_event *ev, vl_simulator *sim)
{
    if (w_stmt)
        w_stmt->setup(sim);
    if ((int)w_cond->eval())
        return (EVnone);
    if (!w_event)
        w_event = new vl_event_expr(LevelEventExpr, w_cond);
    ev->event = w_event;
    return (EVevent);
}


void
vl_wait_stmt::disable(vl_stmt *s)
{
    if (w_event)
        w_event->unchain_disabled(s);
    if (w_stmt)
        w_stmt->disable(s);
}
// End vl_wait_stmt functions.


void
vl_send_event_stmt::setup(vl_simulator *sim)
{
    sim->timewheel()->append(sim->time(),
        new vl_action_item(this, sim->context()));
}


EVtype
vl_send_event_stmt::eval(vl_event*, vl_simulator *sim)
{
    vl_var *d = sim->context()->lookup_var(se_name, false);
    if (!d) {
        vl_error("send-event %s not found", se_name);
        sim->abort();
        return (EVnone);
    }
    if (d->net_type() != REGevent) {
        vl_error("send-event %s is not an event", se_name);
        sim->abort();
        return (EVnone);
    }
    d->trigger();
    return (EVnone);
}
// End vl_send_event_stmt functions.


void
vl_fork_join_stmt::setup(vl_simulator *sim)
{
    fj_endcnt = 0;
    sim->push_context(this);
    vl_setup_list(sim, fj_decls);
    sim->timewheel()->append(sim->time(),
        new vl_action_item(this, sim->context()));
    sim->pop_context();
}


EVtype
vl_fork_join_stmt::eval(vl_event*, vl_simulator *sim)
{
    if (fj_stmts) {
        lsGen<vl_stmt*> gen(fj_stmts);
        vl_stmt *stmt;
        while (gen.next(&stmt)) {
            if (stmt->type() == BeginEndStmt) {
                vl_begin_end_stmt *be = (vl_begin_end_stmt*)stmt;
                vl_stmt *st;
                be->stmts()->lastItem(&st);
                if (st->type() != ForkJoinBreak) {
                    vl_fj_break *bk = new vl_fj_break(this, be);
                    be->stmts()->newEnd(bk);
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
                fj_stmts->replace(stmt, be);
            }
            fj_endcnt++;
        }
    }
    return (EVnone);
}


void
vl_fork_join_stmt::disable(vl_stmt *s)
{
    vl_disable_list(fj_stmts, s);
}


void
vl_fork_join_stmt::dump(ostream &outs, int indnt)
{
    char buf[128];
    for (int i = 0; i < 2*indnt; i++)
        buf[i] = ' ';
    buf[2*indnt] = 0;

    outs << buf << fj_name << " (fork/join) " << (void*)this << '\n';

    bool found = false;
    table_gen<vl_var*> sig_g(fj_sig_st);
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
    table_gen<vl_stmt*> block_g(fj_blk_st);
    vl_stmt *blk;
    while (block_g.next(&vname, &blk)) {
        if (!found) {
            outs << buf << "  Block table\n";
            found = true;
        }
        blk->dump(outs, indnt+2);
    }
}
// End vl_fork_join_stmt functions.


void
vl_fj_break::setup(vl_simulator *sim)
{
    sim->timewheel()->append(sim->time(),
        new vl_action_item(this, sim->context()));
}


// This is evaluated after each fork/join thread completes.  Run
// through the list of fork/join return contexts to find the right one
// and decrement its thread counter.  When the thread count reaches 0,
// put the context on the queue if it has a stack.  The stack may not
// be set yet, in which case just return.
//
EVtype
vl_fj_break::eval(vl_event*, vl_simulator *sim)
{
    vl_action_item *ap = 0, *an;
    for (vl_action_item *a = sim->fj_end(); a; a = an) {
        an = a->next();
        if (a->stmt() == fjb_fjblock) {
            vl_fork_join_stmt *fj = (vl_fork_join_stmt*)a->stmt();
            fj->dec_endcnt();
            if (fj->endcnt() <= 0) {
                if (a->stack()) {
                    if (!ap)
                        sim->set_fj_end(an);
                    else
                        ap->set_next(an);
                    a->set_stmt(0);
                    sim->timewheel()->append(sim->time(), a);
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
    if (!te_task) {
        te_task = sim->context()->lookup_task(te_name);
        if (!te_task) {
            vl_error("can't find task %s", te_name);
            sim->abort();
            return;
        }
    }

    // Have to be careful with context switch, task name can have
    // prepended path
    vl_context cvar;
    const char *n = te_name;
    if (!sim->context()->resolve_cx(&n, cvar, false))
        cvar = *sim->context();
    vl_context *tcx = sim->context();
    sim->set_context(&cvar);

    sim->push_context(te_task);
    vl_setup_list(sim, te_task->decls());
    sim->pop_context();

    // setup ports
    lsGen<vl_expr*> agen(te_args);
    lsGen<vl_decl*> dgen(te_task->decls());
    vl_expr *ae;
    vl_decl *decl;
    bool done = false;
    while (dgen.next(&decl)) {
        lsGen<vl_var*> vgen(decl->ids());
        vl_var *av;
        while (vgen.next(&av)) {
            if (av->net_type() >= REGwire)
                av->set_net_type(REGreg);
            if (agen.next(&ae)) {
                if (av->io_type() == IOinput || av->io_type() == IOinout) {
                    // set formal = actual
                    vl_bassign_stmt *bs =
                        new vl_bassign_stmt(BassignStmt, av, 0, 0, ae);
                    bs->or_flags(SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
                    vl_action_item *a = new vl_action_item(bs, sim->context());
                    a->or_flags(AI_DEL_STMT);
                    sim->timewheel()->append(sim->time(), a);
                }
                if (av->io_type() == IOnone) {
                    vl_warn("too many args to task %s", te_task->name());
                    done = true;
                    break;
                }
            }
            else {
                if (av->io_type() != IOnone)
                    vl_warn("too few args to task %s", te_task->name());
                done = true;
                break;
            }
        }
        if (done)
            break;
    }

    sim->push_context(te_task);
    sim->timewheel()->append(sim->time(),
        new vl_action_item(this, sim->context()));
    sim->pop_context();

    agen = lsGen<vl_expr*>(te_args);
    dgen = lsGen<vl_decl*>(te_task->decls());
    done = false;
    while (dgen.next(&decl)) {
        lsGen<vl_var*> vgen(decl->ids());
        vl_var *av;
        while (vgen.next(&av)) {
            if (agen.next(&ae)) {
                if (av->io_type() == IOoutput || av->io_type() == IOinout) {
                    vl_var *vo = ae->source();
                    if (!vo) {
                        vl_error("internal, variable not found in expr");
                        sim->abort();
                        sim->set_context(tcx);
                        return;
                    }
                    // set actual = formal
                    vl_bassign_stmt *bs =
                        new vl_bassign_stmt(BassignStmt, vo, 0, 0, av);
                    bs->or_flags(SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
                    bs->set_range(chk_copy(ae->source_range()));
                    vl_action_item *a = new vl_action_item(bs, sim->context());
                    a->or_flags(AI_DEL_STMT);
                    sim->timewheel()->append(sim->time(), a);
                }
                if (av->io_type() == IOnone) {
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
    sim->set_context(tcx);
}


EVtype
vl_task_enable_stmt::eval(vl_event*, vl_simulator *sim)
{
    if (te_task)
        vl_setup_list(sim, te_task->stmts());
    return (EVnone);
}
// End vl_task_enable_stmt functions


void
vl_disable_stmt::setup(vl_simulator *sim)
{
    sim->timewheel()->append(sim->time(),
        new vl_action_item(this, sim->context()));
}


EVtype
vl_disable_stmt::eval(vl_event*, vl_simulator *sim)
{
    if (!d_name) {
        if (sim->context()->block())
            d_target = sim->context()->block();
        else if (sim->context()->fjblk())
            d_target = sim->context()->fjblk();
        else if (sim->context()->task())
            d_target = sim->context()->task();
    }
    else {
        vl_stmt *s = sim->context()->lookup_block(d_name);
        if (s) {
            if (s->type() == BeginEndStmt || s->type() == ForkJoinStmt)
                d_target = s;
            return (EVnone);
        }
        vl_task *t = sim->context()->lookup_task(d_name);
        if (t) {
            d_target = t;
            return (EVnone);
        }
        vl_error("in disable, block/task %s not found", d_name);
        sim->abort();
    }
    return (EVnone);
}
// End vl_disable_stmt functions.


void
vl_deassign_stmt::setup(vl_simulator *sim)
{
    sim->timewheel()->append(sim->time(),
        new vl_action_item(this, sim->context()));
}


EVtype
vl_deassign_stmt::eval(vl_event*, vl_simulator*)
{
    if (st_type == DeassignStmt)
        d_lhs->set_assigned(0);
    else if (st_type == ReleaseStmt)
        d_lhs->set_forced(0);
    return (EVnone);
}
// End vl_deassign_stmt functions.


//---------------------------------------------------------------------------
//  Instances
//---------------------------------------------------------------------------

void
vl_gate_inst::setup(vl_simulator *sim)
{
    if (!gi_terms) {
        vl_error("too few terminals for gate %s", i_name);
        sim->abort();
        return;
    }
    if (!gi_setup) {
        vl_error("(internal) gate not initialized");
        sim->abort();
        return;
    }
    if (!(*gi_setup)(sim, this))
        sim->abort();
}


EVtype
vl_gate_inst::eval(vl_event*, vl_simulator *sim)
{
    if (!gi_eval) {
        vl_error("(internal) gate not initialized");
        sim->abort();
        return (EVnone);
    }
    if (!(*gi_eval)(sim, gi_set, this))
        // error
        sim->abort();
    return (EVnone);
}
// End vl_gate_inst functions.


void
vl_mp_inst::setup(vl_simulator *sim)
{
    if (pi_inst_list) {
        if (pi_inst_list->mptype() == MPundef) {
            vl_error("no master found for %s", pi_inst_list->name());
            sim->abort();
            return;
        }
        if (!pi_master) {
            vl_mp *mp;
            if (!sim->description()->mp_st()->lookup(pi_inst_list->name(),
                    &mp) || !mp) {
                vl_error("no master found for %s", pi_inst_list->name());
                sim->abort();
                return;
            }
            mp->inc_inst_count();
            mp = mp->copy();
            mp->set_instance(this);
            pi_master = mp;
            sim->abort();
            return;
        }
        pi_master->setup(sim);
        link_ports(sim);
    }
}


EVtype
vl_mp_inst::eval(vl_event*, vl_simulator *sim)
{
    if (pi_inst_list && pi_inst_list->mptype() == MPprim) {
        vl_primitive *prim = (vl_primitive*)pi_master;
        unsigned char s[MAXPRIMLEN];
        if (prim->type() == CombPrimDecl) {
            for (int i = 1; i < MAXPRIMLEN-1; i++) {
                if (prim->iodata(i))
                    s[i-1] = prim->iodata(i)->data_s()[0];
                else
                    break;
                // s = { in... }
            }

            if (!prim->seq_init()) {
                prim->set_seq_init(true);
                // sanity test for table entries
                unsigned char *row = prim->ptable();
                for (int i = 0; i < prim->rows(); i++) {
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

            unsigned char *row = prim->ptable();
            for (int i = 0; i < prim->rows(); i++) {
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
                    int x = prim->iodata(0)->data_s()[0];
                    prim->iodata(0)->data_s()[0] = row[0];
                    if (x != row[0])
                        prim->iodata(0)->trigger();
                    return (EVnone);
                }
                row += MAXPRIMLEN;
            }
            // set output to 'x'
            int x = prim->iodata(0)->data_s()[0];
            prim->iodata(0)->data_s()[0] = BitDC;
            if (x != BitDC)
                prim->iodata(0)->trigger();
        }
        else if (prim->type() == SeqPrimDecl) {
            for (int i = 0; i < MAXPRIMLEN; i++) {
                if (prim->iodata(i))
                    s[i] = prim->iodata(i)->data_s()[0];
                else
                    break;
                // s = { out, in... }
            }
            if (!prim->seq_init()) {
                prim->set_lastval(0, prim->iodata(0)->data_s()[0]);
                prim->set_seq_init(true);

                // sanity test for table entries
                unsigned char *row = prim->ptable();
                for (int i = 0; i < prim->rows(); i++) {
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

            unsigned char *row = prim->ptable();
            for (int i = 0; i < prim->rows(); i++) {
                unsigned char *col = row + 1;
                bool match = true;
                for (int j = 0; j < MAXPRIMLEN - 1; j++) {
                    if (col[j] == PrimNone)
                        break;
                    if (dcmpseq(s[j], prim->lastval(j), col[j]))
                        continue;
                    match = false;
                    break;
                }
                if (match) {
                    // set output to row[0];
                    if (row[0] != PrimM) {
                        int x = prim->iodata(0)->data_s()[0];
                        prim->iodata(0)->data_s()[0] = row[0];
                        if (x != row[0])
                            prim->iodata(0)->trigger();
                    }
                    prim->set_lastval(0, prim->iodata(0)->data_s()[0]);
                    for (int j = 1; j < MAXPRIMLEN; j++)
                        prim->set_lastval(j, s[j]);
                    return (EVnone);
                }
                row += MAXPRIMLEN;
            }
            // set output to 'x'
            int x = prim->iodata(0)->data_s()[0];
            prim->iodata(0)->data_s()[0] = BitDC;
            if (x != BitDC)
                prim->iodata(0)->trigger();
            prim->set_lastval(0, prim->iodata(0)->data_s()[0]);
            for (int i = 1; i < MAXPRIMLEN; i++)
                prim->set_lastval(i, s[i]);
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
    if (pi_inst_list->mptype() == MPmod) {
        vl_module *mod = (vl_module*)pi_master;
        if (!mod->ports() || !pi_ports)
            return;

        lsGen<vl_port_connect*> pcgen(pi_ports);
        // make sure ports are either all named, or none are named
        vl_port_connect *pc;
        bool named = false;
        if (pcgen.next(&pc)) {
            named = pc->name();
            while (pcgen.next(&pc)) {
                if ((named && !pc->name()) || (!named && pc->name())) {
                    vl_error("in module %s, mix of named and unnamed ports",
                        i_name);
                    sim->abort();
                    return;
                }
            }
        }
        if (named) {
            pcgen = lsGen<vl_port_connect*>(pi_ports);
            int argcnt = 1;
            while (pcgen.next(&pc)) {
                if (pc->expr()) {
                    vl_port *port = find_port(mod->ports(), pc->name());
                    if (port)
                        port_setup(sim, pc, port, argcnt);
                    else {
                        vl_error("can't find port %s in module %s", pc->name(),
                            i_name);
                        sim->abort();
                        return;
                    }
                    argcnt++;
                }
            }
        }
        else {
            pcgen = lsGen<vl_port_connect*>(pi_ports);
            lsGen<vl_port*> pgen(mod->ports());
            vl_port *port;
            int argcnt = 1;
            while (pcgen.next(&pc) && pgen.next(&port)) {
                if (pc->expr() && port->port_exp())
                    port_setup(sim, pc, port, argcnt);
                argcnt++;
            }
        }
    }
    else if (pi_inst_list->mptype() == MPprim) {
        vl_primitive *prm = (vl_primitive*)pi_master;
        if (!prm->ports() || !pi_ports)
            return;
        lsGen<vl_port_connect*> pcgen(pi_ports);
        lsGen<vl_port*> pgen(prm->ports());
        vl_port *port;
        vl_port_connect *pc;
        int argcnt = 1;
        while (pcgen.next(&pc) && pgen.next(&port)) {
            if (pc->expr() && port->port_exp())
                port_setup(sim, pc, port, argcnt);
            argcnt++;
        }
    }
}


// Establish linkage to a module/primitive instance port, argcnt is a
// *1 based* port index.  The context is in the caller, pc is the actual
// arg, the port contains the formal arg.
//
void
vl_mp_inst::port_setup(vl_simulator *sim, vl_port_connect *pc, vl_port *port,
    int argcnt)
{
    if (pc->i_assign() || pc->o_assign())
        // already set up
        return;
    char buf[32];
    const char *portname = pc->name();
    if (!portname) {
        portname = buf;
        sprintf(buf, "%d", argcnt);
    }
    bool isprim = (pi_inst_list->mptype() == MPprim ? true : false);
    const char *modpri = isprim ? "primitive" : "module";
    vl_primitive *primitive = isprim ? (vl_primitive*)pi_master : 0;

    if (port->port_exp()->length() != 1) {
        // the formal arg is a concatenation
        if (isprim) {
            vl_error("in %s instance %s, port %s, found concatenation, "
                "not allowed", modpri, i_name, portname);
            sim->abort();
            return;
        }
        vl_var *nvar = new vl_var;
        nvar->set_data_type(Dconcat);
        nvar->set_data_c(new lsList<vl_expr*>);
        lsGen<vl_var*> gen(port->port_exp());

        IOtype tt = IOnone;
        vl_var *var;
        while (gen.next(&var)) {
            vl_var *catvar;
            if (!pi_master->sig_st()->lookup(var->name(), &catvar)) {
                vl_error("in %s instance %s, port %s, actual arg %s "
                    "is undeclared", modpri, i_name, portname, var->name());
                sim->abort();
                return;
            }
            vl_range *range = var->range();
            var = catvar;
            vl_expr *xp = new vl_expr(var);
            if (range) {
                xp->set_etype(range->right() ? PartSelExpr : BitSelExpr);
                xp->edata().ide.range = chk_copy(range);
            }
            IOtype nt = var->io_type();
            if (tt == IOnone)
                tt = nt;
            else if (nt != tt) {
                vl_error("in %s instance %s, port %s, type mismatch in "
                    "concatenation", modpri, i_name, portname);
                sim->abort();
                return;
            }
            if (nt == IOinput || nt == IOinout) {
                if (var->net_type() < REGwire) {
                    if (var->net_type() == REGnone)
                        var->set_net_type(REGwire);
                    else {
                        vl_error("in %s instance %s, port %s, formal arg %s "
                            "is not a net", modpri, i_name, portname,
                            var->name());
                        sim->abort();
                        return;
                    }
                }
            }
            nvar->data_c()->newEnd(xp);
        }

        if (tt == IOinput || tt == IOinout) {
            // set formal = actual
            pc->set_i_assign(
                new vl_bassign_stmt(BassignStmt, nvar, 0, 0, pc->expr()));
            pc->i_assign()->or_flags(BAS_SAVE_RHS);
            pc->expr()->chain(pc->i_assign());
            // initialize
            sim->timewheel()->append(sim->time(),
                new vl_action_item(pc->i_assign(), sim->context()));
        }
        if (tt == IOoutput || tt == IOinout) {
            // set actual = formal
            vl_expr *assign_expr = new vl_expr;
            assign_expr->set_etype(ConcatExpr);
            assign_expr->edata().mcat.var = nvar;
            nvar = pc->expr()->source();
            // actual must be net
            if (!nvar || !nvar->check_net_type(REGwire)) {
                vl_error("in %s instance %s, formal connection "
                    "to port %s is not a net", modpri, i_name, portname);
                sim->abort();
                return;
            }
            pc->set_o_assign(new vl_bassign_stmt(BassignStmt, nvar, 0, 0,
                assign_expr));
            pc->o_assign()->or_flags(BAS_SAVE_LHS);
            vl_range *sr = pc->expr()->source_range();
            if (sr)
                pc->o_assign()->set_range(sr->reval());
            assign_expr->chain(pc->o_assign());
            sim->timewheel()->append(sim->time(),
                new vl_action_item(pc->o_assign(), sim->context()));
        }
        return;
    }

    lsGen<vl_var*> gen(port->port_exp());
    vl_var *var = 0;
    gen.next(&var);
    vl_var *nvar = 0;
    if (!pi_master->sig_st()->lookup(var->name(), &nvar)) {
        vl_error("in %s instance %s, port %s, actual arg %s "
            "is undeclared", modpri, i_name, portname, var->name());
        sim->abort();
        return;
    }
    port->port_exp()->replace(var, nvar);
    vl_range *range = var->range();
    var->set_range(0);
    delete var;
    var = nvar;

    if (isprim) {
        if (range)
            vl_warn("in %s instance %s, port %s, actual arg %s "
                "has range, ignored", modpri, i_name, portname, var->name());
        if (var->data_type() != Dbit || var->bits().size() != 1) {
            vl_error("in %s instance %s, port %s, actual arg %s "
                "is not unit width", modpri, i_name, portname, var->name());
            sim->abort();
            delete range;
            return;
        }
    }
    IOtype tt = var->io_type();
    if (isprim) {
        if (argcnt == 1 && tt != IOoutput) {
            vl_error("in %s instance %s, port %s, actual arg %s "
                "is not an output", modpri, i_name, portname, var->name());
            sim->abort();
            delete range;
            return;
        }
        if (argcnt > 1 && tt != IOinput) {
            vl_error("in %s instance %s, port %s, actual arg %s "
                "is not an input", modpri, i_name, portname, var->name());
            sim->abort();
            delete range;
            return;
        }
    }
    if (tt == IOnone) {
        vl_warn("formal arg %s in %s instance %s is not declared\n"
            "input/output/inout, assuming inout",
            portname, modpri, i_name);
        tt = IOinout;
    }
    if (tt == IOinput || tt == IOinout) {
        // set formal = actual
        // formal must be net
        if (!var->check_net_type(REGwire)) {
            vl_error("in %s instance %s, actual connection "
                "to port %s is not a net", modpri, i_name, portname);
            sim->abort();
            delete range;
            return;
        }
        pc->set_i_assign(new vl_bassign_stmt(BassignStmt, var, 0, 0,
            pc->expr()));
        pc->i_assign()->or_flags(BAS_SAVE_LHS | BAS_SAVE_RHS);
        if (range)
            pc->i_assign()->set_range(range->reval());
        pc->expr()->chain(pc->i_assign());
        if (tt == IOinout)
            // prevent driver loop
            pc->expr()->or_flags(VAR_PORT_DRIVER);
        sim->timewheel()->append(sim->time(),
            new vl_action_item(pc->i_assign(), sim->context()));
        // bs freed with pc
    }
    if (tt == IOoutput || tt == IOinout) {
        // set actual = formal
        vl_var *vnew = pc->expr()->source();
        // actual must be net
        if (!vnew || !vnew->check_net_type(REGwire)) {
            vl_error("in %s instance %s, formal connection "
                "to port %s is not a net", modpri, i_name, portname);
            sim->abort();
            delete range;
            return;
        }
        if (range) {
            vl_expr *xp = new vl_expr(var);
            xp->set_etype(range->right() ? PartSelExpr : BitSelExpr);
            xp->edata().ide.range = range;
            var = xp;
        }
        pc->set_o_assign(new vl_bassign_stmt(BassignStmt, vnew, 0, 0, var));
        pc->o_assign()->or_flags(BAS_SAVE_LHS);
        if (!range)
            pc->o_assign()->or_flags(BAS_SAVE_RHS);
        vl_range *sr = pc->expr()->source_range();
        if (sr)
            pc->o_assign()->set_range(sr->reval());
        var->chain(pc->o_assign());
        if (tt == IOinout) {
            // prevent driver loop
            var->or_flags(VAR_PORT_DRIVER);
        }
        sim->timewheel()->append(sim->time(),
            new vl_action_item(pc->o_assign(), sim->context()));
    }
    if (isprim) {
        primitive->set_iodata(argcnt-1, var);
        sim->push_context(primitive);
        if (argcnt > 1)
            var->chain(this);
        sim->pop_context();
    }
}
// End vl_mp_inst functions.

