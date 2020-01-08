
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

#include "vl_list.h"
#include "vl_st.h"
#include "vl_defs.h"
#include "vl_types.h"


//---------------------------------------------------------------------------
//  Local
//---------------------------------------------------------------------------

namespace {
    inline int max(int x, int y) { return (x > y ? x : y); }
    inline int min(int x, int y) { return (x < y ? x : y); }
    inline int rsize(int m, int l) { return (abs(m - l) + 1); }

    // Return status of i'th bit of integer
    //
    inline int bit(int i, int pos)
    {
        if ((i >> pos) & 1)  
            return (BitH);
        return (BitL);
    }


    // Return status of i'th bit of vl_time_t
    //
    inline int bit(vl_time_t t, int pos)
    {
        if ((t >> pos) & 1)  
            return (BitH);
        return (BitL);
    }


    inline int rnd(double x)
    {
        return ((int)(x >= 0.0 ? x + 0.5 : x - 0.5));
    }


    inline vl_time_t trnd(double x)
    {
        return ((vl_time_t)(x >= 0.0 ? x + 0.5 : x - 0.5));
    }


    inline bool is_indeterminate(char *s, int l, int m)
    {
        for (int i = l; i <= m; i++) {
            if (s[i] != BitL && s[i] != BitH)
                return (true);
        }
        return (false);
    }


    // Return an int constructed from bit field.
    //
    inline int bits2int(char *s, int wid)
    {
        int cnt = 0;
        int mask = 1;
        for (int i = 0; i < wid; i++) {
            if (s[i] == BitH)
                cnt |= mask;
            else if (s[i] != BitL)
                return (0);
            mask <<= 1;
        }
        return (cnt);
    }


    inline int bits2time(char *s, int wid)
    {
        vl_time_t cnt = 0;
        vl_time_t mask = 1;
        for (int i = 0; i < wid; i++) {
            if (s[i] == BitH)
                cnt |= mask;
            else if (s[i] != BitL)
                return (0);
            mask <<= 1;
        }
        return (cnt);
    }


    inline double bits2real(char *s, int wid)
    {
        double sum = 0;
        double a = 1.0;
        for (int i = 0; i < wid; i++) {
            if (s[i] == BitH)
                sum += a;
            a *= 2.0;
        }
        return (sum);
    }
}


//---------------------------------------------------------------------------
//  Data variables and expressions
//---------------------------------------------------------------------------

// The vl_var type represents a net, register, or numeric data item. 
// Named vl_vars are stored in a symbol table.  The parser creates a
// separate vl_var for declarations, lhs of assignments, and port
// references.  Only those that are declared are put in the symbol
// table.  The lhs of assignments (vl_bassign_stmt) is replaced by the
// declared vl_var from the symbol table in the setup call for the
// assignment.  In ports, there is no replacement, but the declared
// vl_var is chained to rather than the vl_var in the port structs.
//
// The range field is used by the parser to indicate a bit/part select
// in assignments, and the size in declarations.  This field is not
// used in the vl_var functions.
//
// See note for the derived vl_expr class in vl_expr.cc.

vl_var::vl_var()
{
    v_name = 0;
    v_data_type = Dnone;
    v_net_type = REGnone;
    v_io_type = IOnone;
    v_flags = 0;
    set_data_r(0);
    v_range = 0;
    v_events = 0;
    v_delay = 0;
    v_cassign = 0;
    v_drivers = 0;
}


vl_var::vl_var(const char *n, vl_range *r, lsList<vl_expr*> *c)
{
    v_name = n;
    v_data_type = c ? Dconcat : Dnone;
    v_net_type = REGnone;
    v_io_type = IOnone;
    v_flags = 0;
    set_data_c(c);
    v_range = r;
    v_events = 0;
    v_delay = 0;
    v_cassign = 0;
    v_drivers = 0;
}


// Copy constructor.  This makes a duplicate, used when copying
// modules for instantiation.
//
vl_var::vl_var(vl_var &d)
{
    v_name = vl_strdup(d.v_name);
    v_data_type = d.v_data_type;
    v_net_type = d.v_net_type;
    v_io_type = d.v_io_type;
    v_flags = 0;
    set_data_r(0);
    v_array = d.v_array;
    v_bits = d.v_bits;
    v_range = chk_copy(d.v_range);
    v_events = 0;
    v_strength = d.v_strength;
    v_delay = chk_copy(d.v_delay);
    v_cassign = 0;
    v_drivers = 0;

    if (v_data_type == Dint) {
        if (v_array.size()) {
            set_data_pi(new int[v_array.size()]);
            memcpy(data_pi(), d.data_pi(), v_array.size()*sizeof(int));
        }
        else
            set_data_i(d.data_i());
    }
    else if (v_data_type == Dreal) {
        if (v_array.size()) {
            set_data_pr(new double[v_array.size()]);
            memcpy(data_pr(), d.data_pr(), v_array.size()*sizeof(double));
        }
        else
            set_data_r(d.data_r());
    }
    else if (v_data_type == Dtime) {
        if (v_array.size()) {
            set_data_pt(new vl_time_t[v_array.size()]);
            memcpy(data_pt(), d.data_pt(), v_array.size()*sizeof(vl_time_t));
        }
        else
            set_data_t(d.data_t());
    }
    else if (v_data_type == Dstring) {
        if (v_array.size()) {
            char **s = new char*[v_array.size()];
            set_data_ps(s);
            char **ss = d.data_ps();
            for (int i = 0; i < v_array.size(); i++)
                s[i] = vl_strdup(ss[i]);
        }
        else
            set_data_s(vl_strdup(d.data_s()));
    }
    else if (v_data_type == Dbit) {
        if (v_array.size()) {
            char **s = new char*[v_array.size()];
            set_data_ps(s);
            char **ss = d.data_ps();
            for (int i = 0; i < v_array.size(); i++) {
                s[i] = new char[v_bits.size()];
                memcpy(s[i], ss[i], v_bits.size());
            }
        }
        else {
            set_data_s(new char[v_bits.size()]);
            memcpy(data_s(), d.data_s(), v_bits.size());
        }
    }
    else if (v_data_type == Dconcat) {
        set_data_c(new lsList<vl_expr*>);
        vl_expr *e;
        lsGen<vl_expr*> gen(d.data_c());
        while (gen.next(&e))
            data_c()->newEnd(chk_copy(e));
    }
}


vl_var::~vl_var()
{
    delete [] v_name;
    if (v_data_type == Dbit || v_data_type == Dstring) {
        if (v_array.size()) {
            char **s = data_ps();
            for (int i = 0; i < v_array.size(); i++)
                delete [] s[i];
            delete [] s;
        }
        else
            delete [] data_s();
    }
    else if (v_data_type == Dint) {
        if (v_array.size())
            delete [] data_pi();
    }
    else if (v_data_type == Dtime) {
        if (v_array.size())
            delete [] data_pt();
    }
    else if (v_data_type == Dreal) {
        if (v_array.size())
            delete [] data_pr();
    }
    else if (v_data_type == Dconcat)
        delete_list(data_c());
    if (v_drivers) {
        lsGen<vl_driver*> gen(v_drivers);
        vl_driver *d;
        while (gen.next(&d))
            delete d;
        delete v_drivers;
    }
    vl_action_item::destroy(v_events);
}


vl_var *
vl_var::copy()
{
    return (new vl_var(*this));
}


vl_var &
vl_var::eval()
{
    return (*this);
}


namespace {
    bool same(vl_stack *s1, vl_stack *s2)
    {
        if (!s1 && !s2)
            return (true);
        if (!s1 || !s2)
            return (false);
        if (s1->num() != s2->num())
            return (false);
        for (int i = 0; i < s1->num(); i++) {
            if (s1->act(i).actions() != s2->act(i).actions())
                return (false);
        }
        return (true);
    }
}


// Set up an asynchronous action to perform when data changes,
// for events if the stmt is a vl_action_item, or for continuous
// assign and formal/actual association otherwise.
//
void
vl_var::chain(vl_stmt *stmt)
{
    if (v_data_type == Dconcat) {
        lsGen<vl_expr*> gen(data_c(), true);
        vl_expr *e;
        while (gen.prev(&e))
            e->chain(stmt);
        return;
    }
    if (stmt->type() == ActionItem) {
        vl_action_item *a = (vl_action_item*)stmt;
        for (vl_action_item *aa = v_events; aa; aa = aa->next()) {
            if (aa->event() == a->event())
                return;
        }
        if (a->event())
            a->event()->init();
        vl_action_item *an = new vl_action_item(a->stmt(), VS()->context());
        an->set_stack(chk_copy(a->stack()));
        an->set_event(a->event());
        an->set_flags(a->flags());
        an->set_next(v_events);
        v_events = an;
    }
    else {
        vl_action_item *a;
        for (a = v_events; a; a = a->next()) {
            if (a->stmt() == stmt)
                return;
        }
        a = new vl_action_item(stmt, VS()->context());
        a->set_next(v_events);
        v_events = a;
    }
}


void
vl_var::unchain(vl_stmt *stmt)
{
    if (v_data_type == Dconcat) {
        lsGen<vl_expr*> gen(data_c(), true);
        vl_expr *e;
        while (gen.prev(&e))
            e->unchain(stmt);
        return;
    }
    if (stmt->type() == ActionItem) {
        vl_action_item *a = (vl_action_item*)stmt;
        vl_action_item *ap = 0, *an;
        for (vl_action_item *aa = v_events; aa; aa = an) {
            an = aa->next();
            if (aa->stmt() == a->stmt() && same(aa->stack(), a->stack())) {
                if (ap)
                    ap->set_next(an);
                else
                    v_events = an;
                if (aa != a)
                    delete aa;
                return;
            }
            ap = aa;
        }
    }
    else {
        vl_action_item *ap = 0, *an;
        for (vl_action_item *a = v_events; a; a = an) {
            an = a->next();
            if (a->stmt() == stmt) {
                if (ap)
                    ap->set_next(an);
                else
                    v_events = an;
                delete a;
                return;
            }
            ap = a;
        }
    }
}


void
vl_var::unchain_disabled(vl_stmt *stmt)
{
    if (v_data_type == Dconcat) {
        lsGen<vl_expr*> gen(data_c(), true);
        vl_expr *e;
        while (gen.prev(&e))
            e->unchain_disabled(stmt);
        return;
    }
    if (v_events)
        v_events = v_events->purge(stmt);
}


// Set the size and data_type of the data.  This is called from the
// setup function of declarations.
//
void
vl_var::configure(vl_range *rng, int t, vl_range *ary)
{
    if (v_data_type != Dnone || v_net_type != REGnone) {
        // already configured, but allow situations like
        // output r;
        // reg [7:0] r;
        if (rng && v_data_type == Dbit && v_bits.size() == 1 && !ary &&
                !v_array.size()) {
            delete [] data_s();
            set_data_s(0);
        }
        else
            return;
    }

    if (t == RealDecl) {
        v_data_type = Dreal;
        // Verilog doesn't support real arrays, allow them here anyway.
        v_array.set(ary);
        if (v_array.size()) {
            set_data_pr(new double[v_array.size()]);
            memset(data_pr(), 0, v_array.size()*sizeof(double));
        }
    }
    else if (t == IntDecl) {
        v_data_type = Dint;
        v_array.set(ary);
        if (v_array.size()) {
            set_data_pi(new int[v_array.size()]);
            memset(data_pi(), 0, v_array.size()*sizeof(int));
        }
    }
    else if (t == TimeDecl) {
        v_data_type = Dtime;
        v_array.set(ary);
        if (v_array.size()) {
            set_data_pt(new vl_time_t[v_array.size()]);
            memset(data_pt(), 0, v_array.size()*sizeof(vl_time_t));
        }
    }
    else if (t == ParamDecl) {
        if (rng) {
            v_data_type = Dbit;
            v_bits.set(rng);
            if (!v_bits.size())
                v_bits.set(1);
            set_data_s(new char[v_bits.size()]);
            memset(data_s(), BitL, v_bits.size());
        }
        else
            v_data_type = Dint;
    }
    else {
        v_data_type = Dbit;
        v_bits.set(rng);
        if (!v_bits.size())
            v_bits.set(1);
        v_array.set(ary);
        if (v_array.size()) {
            char **s = new char*[v_array.size()];
            set_data_ps(s);
            for (int i = 0; i < v_array.size(); i++) {
                s[i] = new char[v_bits.size()];
                memset(s[i], BitDC, v_bits.size());
            }
        }
        else {
            set_data_s(new char[v_bits.size()]);
            memset(data_s(), BitDC, v_bits.size());
        }
    }
}


// General assignment.  If the type of the recipient is already specified,
// coerce to that type.  Otherwise copy directly.
//
void
vl_var::operator=(vl_var &d)
{
    if (v_net_type == REGreg && (v_flags & VAR_CP_ASSIGN))
        return;
    if (v_flags & VAR_F_ASSIGN)
        return;
    bool arm_trigger = false;
    if (v_data_type == Dnone) {
        v_data_type = d.v_data_type;
        v_array = d.v_array;
        v_bits = d.v_bits;
        arm_trigger = true;
        if (v_data_type == Dint) {
            if (v_array.size()) {
                set_data_pi(new int[v_array.size()]);
                memcpy(data_pi(), d.data_pi(), v_array.size()*sizeof(int));
            }
            else
                set_data_i(d.data_i());
        }
        else if (v_data_type == Dreal) {
            if (v_array.size()) {
                set_data_pr(new double[v_array.size()]);
                memcpy(data_pr(), d.data_pr(), v_array.size()*sizeof(double));
            }
            else
                set_data_r(d.data_r());
        }
        else if (v_data_type == Dtime) {
            if (v_array.size()) {
                set_data_pt(new vl_time_t[v_array.size()]);
                memcpy(data_pt(), d.data_pt(),
                    v_array.size()*sizeof(vl_time_t));
            }
            else
                set_data_t(d.data_t());
        }
        else if (v_data_type == Dstring) {
            if (v_array.size()) {
                char **s = new char*[v_array.size()];
                set_data_ps(s);
                char **ss = d.data_ps();
                for (int i = 0; i < v_array.size(); i++)
                    s[i] = vl_strdup(ss[i]);
            }
            else
                set_data_s(vl_strdup(d.data_s()));
        }
        else if (v_data_type == Dbit) {
            if (v_array.size()) {
                char **s = new char*[v_array.size()];
                set_data_ps(s);
                char **ss = d.data_ps();
                for (int i = 0; i < v_array.size(); i++) {
                    s[i] = new char[v_bits.size()];
                    memcpy(s[i], ss[i], v_bits.size());
                }
            }
            else {
                set_data_s(new char[v_bits.size()]);
                memcpy(data_s(), d.data_s(), v_bits.size());
            }
        }
        else if (v_data_type == Dconcat) {
            set_data_c(new lsList<vl_expr*>);
            vl_expr *e;
            lsGen<vl_expr*> gen(d.data_c());
            while (gen.next(&e))
                data_c()->newEnd(chk_copy(e));
        }
    }
    else if (v_data_type == Dbit) {
        if (v_net_type >= REGwire && d.v_data_type == Dbit) {
            if (v_array.size() != 0) {
                vl_error("in assignment, assigning object is an v_array");
                VS()->abort();
                return;
            }
            add_driver(&d, v_bits.size() - 1, 0, 0);
            if (VS()->dbg_flags() & DBG_assign)
                probe1(this, v_bits.size() - 1, 0, &d, d.v_bits.size() - 1, 0);
            int i = 0;
            for (int j = 0; i < v_bits.size() && j < d.v_bits.size();
                    i++, j++) {
                int b = resolve_bit(i, &d, 0);
                if (data_s()[i] != b) {
                    arm_trigger = true;
                    data_s()[i] = b;
                }
            }
            for ( ; i < v_bits.size(); i++) {
                if (data_s()[i] != BitL) {
                    arm_trigger = true;
                    data_s()[i] = BitL;
                }
            }

            if (VS()->dbg_flags() & DBG_assign)
                probe2();
            if (arm_trigger && v_events)
                trigger();
            return;
        }
        if (v_array.size() == 0) {
            if (v_net_type == REGsupply0 || v_net_type == REGsupply1)
                return;
            int wd;
            char *s = d.bit_elt(0, &wd);
            int mw = min(v_bits.size(), wd);
            int i;
            for (i = 0; i < mw; i++) {
                if (data_s()[i] != s[i]) {
                    arm_trigger = true;
                    data_s()[i] = s[i];
                }
            }
            for ( ; i < v_bits.size(); i++) {
                if (data_s()[i] != BitL) {
                    arm_trigger = true;
                    data_s()[i] = BitL;
                }
            }
            if (VS()->dbg_flags() & DBG_assign) {
                cout << this << " = ";
                print_value(cout);
                cout << '\n';
            }
        }
        else {
            int ms = min(v_array.size(), d.v_array.size());
            int wd;
            for (int j = 0; j < ms; j++) {
                char *s = d.bit_elt(j, &wd);
                char *b = data_ps()[j];
                int mw = min(v_bits.size(), wd);
                int i;
                for (i = 0; i < mw; i++) {
                    if (b[i] != s[i]) {
                        arm_trigger = true;
                        b[i] = s[i];
                    }
                }
                for ( ; i < v_bits.size(); i++) {
                    if (b[i] != BitL) {
                        arm_trigger = true;
                        b[i] = BitL;
                    }
                }
            }
        }
    }
    else if (v_data_type == Dint) {
        if (v_array.size() == 0) {
            int b = d.int_elt(0);
            if (data_i() != b) {
                arm_trigger = true;
                set_data_i(b);
            }
            if (VS()->dbg_flags() & DBG_assign) {
                cout << this << " = ";
                print_value(cout);
                cout << '\n';
            }
        }
        else {
            int ms = min(v_array.size(), d.v_array.size());
            int *dd = data_pi();
            int i;
            for (i = 0; i < ms; i++) {
                int b = d.int_elt(i);
                if (dd[i] != b) {
                    arm_trigger = true;
                    dd[i] = b;
                }
            }
            for ( ; i < v_array.size(); i++) {
                if (dd[i] != 0) {
                    arm_trigger = true;
                    dd[i] = 0;
                }
            }
        }
    }
    else if (v_data_type == Dtime) {
        if (v_array.size() == 0) {
            vl_time_t t = d.time_elt(0);
            if (data_t() != t) {
                arm_trigger = true;
                set_data_t(t);
            }
            if (VS()->dbg_flags() & DBG_assign) {
                cout << this << " = ";
                print_value(cout);
                cout << '\n';
            }
        }
        else {
            int ms = min(v_array.size(), d.v_array.size());
            vl_time_t *tt = data_pt();
            int i;
            for (i = 0; i < ms; i++) {
                vl_time_t t = d.time_elt(i);
                if (tt[i] != t) {
                    arm_trigger = true;
                    tt[i] = t;
                }
            }
            for ( ; i < v_array.size(); i++) {
                if (tt[i] != 0) {
                    arm_trigger = true;
                    tt[i] = 0;
                }
            }
        }
    }
    else if (v_data_type == Dreal) {
        if (v_array.size() == 0) {
            double r = d.real_elt(0);
            if (data_r() != r) {
                arm_trigger = true;
                set_data_r(r);
            }
            if (VS()->dbg_flags() & DBG_assign) {
                cout << this << " = ";
                print_value(cout);
                cout << '\n';
            }
        }
        else {
            int ms = min(v_array.size(), d.v_array.size());
            double *dd = data_pr();
            int i;
            for (i = 0; i < ms; i++) {
                double r = d.real_elt(i);
                if (dd[i] != r) {
                    arm_trigger = true;
                    dd[i] = r;
                }
            }
            for ( ; i < v_array.size(); i++) {
                if (dd[i] != 0.0) {
                    arm_trigger = true;
                    dd[i] = 0.0;
                }
            }
        }
    }
    else if (v_data_type == Dstring) {
        if (v_array.size() == 0) {
            delete [] data_s();
            set_data_s(vl_strdup(d.str_elt(0)));
        }
        else {
            int ms = min(v_array.size(), d.v_array.size());
            char **ss = data_ps();
            int i;
            for (i = 0; i < ms; i++) {
                delete [] ss[i];
                ss[i] = vl_strdup(d.str_elt(i));
            }
            for ( ; i < v_array.size(); i++) {
                delete [] ss[i];
                ss[i] = 0;
            }
        }
    }
    else if (v_data_type == Dconcat) {
        if (d.v_array.size() != 0) {
            vl_error(
                "rhs of concatenation assignment is an v_array, not allowed");
            errout(this);
            VS()->abort();
            return;
        }
        lsGen<vl_expr*> gen(data_c(), true);
        vl_expr *e;
        int bc = 0;
        while (gen.prev(&e)) {
            vl_var *v = e->source();
            if (!v) {
                vl_error("in variable assignment, bad expression type in "
                    "concatenation:");
                errout(this);
                VS()->abort();
                return;
            }
            if (v->v_data_type != Dbit) {
                vl_error("in variable assignment, concatenation contains "
                    "unsized value:");
                errout(this);
                VS()->abort();
                return;
            }
            vl_range *r = e->source_range();
            if (!v->v_array.size()) {
                if (bc < d.bit_size()) {
                    int l_from = d.v_bits.Btou(bc);
                    v->assign(r, &d, 0, &l_from);
                    bc += bits_set(v, r, &d, l_from);
                }
                else
                    v->clear(r, BitL);
            }
            else {
                int m, l;
                if (r) {
                    if (!r->eval(&m, &l))
                        continue;
                    if (!v->v_array.check_range(&m, &l))
                        continue;
                }
                else {
                    m = v->v_array.hi_index();
                    l = v->v_array.lo_index();
                }
                bool atrigger = false;
                int w;
                char *t = d.bit_elt(0, &w);

                int i = v->v_array.Astart(m, l);
                int ie = v->v_array.Aend(m, l);
                for ( ; i <= ie; i++) {
                    char *s = v->data_ps()[i];
                    int cnt = 0;
                    while (bc < d.v_bits.size() && cnt < v->v_bits.size()) {
                        if (s[cnt] != t[bc]) {
                            atrigger = true;
                            s[cnt] = t[bc];
                        }
                        cnt++;
                        bc++;
                    }
                    if (bc == d.v_bits.size()) {
                        for ( ; cnt < v->v_bits.size(); cnt++) {
                            if (s[cnt] != BitL) {
                                atrigger = true;
                                s[cnt] = BitL;
                            }
                        }
                    }
                }
                if (atrigger && v->v_events) {
                    v->trigger();
                    arm_trigger = true;
                }
            }
        }
    }
    if (arm_trigger && v_events)
        trigger();
}


// This function supports general assignments and data types, some of
// which are not supported in the Verilog language.  Coersions are
// performed where necessary and possible.  Most data types can be
// provided with a range, and/or can be an array.  The concatenation
// data type is an exception - it does not allow a range and can't
// be arrayed.
//
//    Type            Range Interpretation      In Verilog
//    Dnone
//      scalar        not allowed               -
//      array         not allowed               -
//    Dbit
//      scalar        bit field                 yes
//      array         array elements            yes (registers only)
//    Dint
//      scalar        bit field                 yes
//      array         array elements            yes
//    Dtime
//      scalar        bit field                 yes
//      array         array elements            yes
//    Dreal
//      scalar        not allowed               yes
//      array         array elements            no
//    Dstring
//      scalar        not allowed               yes (const expr only)
//      array         array elements            no
//
// Assignments
//
// In general, both the source and destination can have a range
// specification.  If the variable is an array, the range applies to
// the array indices, and assignments are made on a per-word basis. 
// If the variable is not an array, the range specifies the bits which
// are involved in assignment.  If an array is assigned to a non-array
// and vice-versa, the lowest-index word in the array is taken for the
// assignment.  Arrays can be assigned to other arrays, in which case
// the ranges are mapped word-for-word.
//
// Below, (*) indicates the assignments that are currently impelmented.
//
//          Source
//           Dnone  Dbit  Dint  Dtime  Dreal  Dstring
//   Dest         -  S  A  S  A  S  A   S  A   S  A
//  Dnone   -     *  *  *  *  *  *  *   *  *   *  *
//  Dbit    S        *  *  *  *  *  *   *  *   *  *
//          A        *  *  *  *  *  *   *  *   *  *
//  Dint    S        *  *  *  *  *  *   *  *   *  *
//  Dint    A        *  *  *  *  *  *   *  *   *  *
//  Dtime   S        *  *  *  *  *  *   *  *   *  *
//          A        *  *  *  *  *  *   *  *   *  *
//  Dreal   S        *  *  *  *  *  *   *  *   *  *
//          A        *  *  *  *  *  *   *  *   *  *
//  Dstring S                                  *  *
//          A                                  *  *
//
// Rules for assignment
//
// * If the destination is wider than the source, the detination is
//   BitL-filled.
// * If the destination range is entirely out of range, nothing happens.
// * If the source range is entirely out of range, the destination is
// * filled with BitDC.
// * If either range is partially out of range, the part that is in range
// * is completed normally, and the part that is out of range is ignored
//   (destination) or shrunk-to-fit (source).
// * A nondeterminate value is stored as 0 in Dint, Dtime, and Dreal
//   variables.
// * Reals are converted to integers for bit range functionality.
// * Strings are converted to ascii bit streams for bit range
//   functionality.
// 
void
vl_var::assign(vl_range *rd, vl_var *src, vl_range *rs)
{
    if (v_net_type == REGreg && (v_flags & VAR_CP_ASSIGN))
        return;
    if (v_flags & VAR_F_ASSIGN)
        return;
    if (!rd && !rs) {
        *this = *src;
        return;
    }
    if (v_data_type == Dconcat) {
        // can't get here
        vl_error("concatenation with range not allowed");
        return;
    }
    int md, ld;
    if (rd) {
        if (!rd->eval(&md, &ld))
            return;
    }
    else 
        default_range(&md, &ld);

    int ms, ls;
    if (rs) {
        if (!rs->eval(&ms, &ls)) {
            // pass a bogus range, the assign functions
            // should do the right thing
            if (src->v_array.size())
                ms = ls = src->v_array.Atou(src->v_array.size());
            else if (src->v_bits.size())
                ms = ls = src->v_bits.Btou(src->v_bits.size());
            else
                ms = ls = 1;
        }
    }
    else
        src->default_range(&ms, &ls);

    if (v_data_type == Dnone) {
        if (!rd)
            assign_init(src, ms, ls);
        else {
            vl_error("range given to uninitialized variable");
            errout(rd);
            VS()->abort();
            return;
        }
    }
    else if (v_data_type == Dbit)
        assign_bit_range(md, ld, src, ms, ls);
    else if (v_data_type == Dint)
        assign_int_range(md, ld, src, ms, ls);
    else if (v_data_type == Dtime)
        assign_time_range(md, ld, src, ms, ls);
    else if (v_data_type == Dreal)
        assign_real_range(md, ld, src, ms, ls);
    else if (v_data_type == Dstring)
        assign_string_range(md, ld, src, ms, ls);
    else {
        vl_error("bad data type in range assignment");
        VS()->abort();
    }
}


// A variation that takes integer pointers for the source range values.
//
void
vl_var::assign(vl_range *rd, vl_var *src, int *m_from, int *l_from)
{
    const char *msg = "indeterminate range specification in assignment";
    if (v_net_type == REGreg && (v_flags & VAR_CP_ASSIGN))
        return;
    if (v_flags & VAR_F_ASSIGN)
        return;
    if (!rd && !m_from && !l_from) {
        *this = *src;
        return;
    }
    if (v_data_type == Dconcat) {
        // can't get here
        vl_error("concatenation with range not allowed");
        return;
    }
    int md, ld;
    if (rd) {
        if (!rd->eval(&md, &ld)) {
            vl_warn(msg);
            errout(rd);
            return;
        }
    }
    else
        default_range(&md, &ld);

    int dfms, dfls;
    src->default_range(&dfms, &dfls);
    int ms, ls;
    if (m_from)
        ms = *m_from;
    else
        ms = dfms;
    if (l_from)
        ls = *l_from;
    else
        ls = dfls;

    if (v_data_type == Dnone) {
        if (!rd)
            assign_init(src, ms, ls);
        else {
            vl_error("range given to uninitialized variable");
            errout(rd);
            VS()->abort();
            return;
        }
    }
    else if (v_data_type == Dbit)
        assign_bit_range(md, ld, src, ms, ls);
    else if (v_data_type == Dint)
        assign_int_range(md, ld, src, ms, ls);
    else if (v_data_type == Dtime)
        assign_time_range(md, ld, src, ms, ls);
    else if (v_data_type == Dreal)
        assign_real_range(md, ld, src, ms, ls);
    else if (v_data_type == Dstring)
        assign_string_range(md, ld, src, ms, ls);
    else {
        vl_error("bad data type in range assignment");
        VS()->abort();
    }
}


// Clear the indicated bits or elements, if of type Dbit, fill with bitd,
// otherwise zero.
//
void
vl_var::clear(vl_range *rng, int bitd)
{
    int md = 0, ld = 0;
    if (rng) {
        if (!rng->eval(&md, &ld))
            return;
    }
    if (v_data_type == Dbit) {
        if (v_array.size()) {
            if (rng) {
                if (v_array.check_range(&md, &ld)) {
                    int i = v_array.Astart(md, ld);
                    int ie = v_array.Aend(md, ld);
                    for ( ; i <= ie; i++)
                        memset(data_ps()[i], bitd, v_bits.size());
                }
            }
            else {
                for (int i = 0; i < v_array.size(); i++)
                    memset(data_ps()[i], bitd, v_bits.size());
            }
        }
        else {
            if (rng) {
                if (v_bits.check_range(&md, &ld)) {
                    int i = v_bits.Bstart(md, ld);
                    int ie = v_bits.Bend(md, ld);
                    for ( ; i <= ie; i++)
                        data_s()[i] = bitd;
                }
            }
            else {
                for (int i = 0; i < v_bits.size(); i++)
                    data_s()[i] = bitd;
            }
        }
    }
    else if (v_data_type == Dint) {
        if (v_array.size()) {
            if (rng) {
                if (v_array.check_range(&md, &ld)) {
                    int i = v_array.Astart(md, ld);
                    int ie = v_array.Aend(md, ld);
                    for ( ; i <= ie; i++)
                        data_pi()[i] = 0;
                }
            }
            else {
                for (int i = 0; i < v_array.size(); i++)
                    data_pi()[i] = 0;
            }
        }
        else {
            if (rng) {
                if (check_bit_range(&md, &ld)) {
                    int i = v_bits.Bstart(md, ld);
                    int ie = v_bits.Bend(md, ld);
                    for ( ; i <= ie; i++)
                        set_bit_of(i, BitL);
                }
            }
            else
                set_data_i(0);
        }
    }
    else if (v_data_type == Dtime) {
        if (v_array.size()) {
            if (rng) {
                if (v_array.check_range(&md, &ld)) {
                    int i = v_array.Astart(md, ld);
                    int ie = v_array.Aend(md, ld);
                    for ( ; i <= ie; i++)
                        data_pt()[i] = 0;
                }
            }
            else {
                for (int i = 0; i < v_array.size(); i++)
                    data_pt()[i] = 0;
            }
        }
        else {
            if (rng) {
                if (check_bit_range(&md, &ld)) {
                    int i = v_bits.Bstart(md, ld);
                    int ie = v_bits.Bend(md, ld);
                    for ( ; i <= ie; i++)
                        set_bit_of(i, BitL);
                }
            }
            else
                set_data_t(0);
        }
    }
    else if (v_data_type == Dreal) {
        if (v_array.size()) {
            if (rng) {
                if (v_array.check_range(&md, &ld)) {
                    int i = v_array.Astart(md, ld);
                    int ie = v_array.Aend(md, ld);
                    for ( ; i <= ie; i++)
                        data_pr()[i] = 0.0;
                }
            }
            else {
                for (int i = 0; i < v_array.size(); i++)
                    data_pr()[i] = 0.0;
            }
        }
        else
            set_data_r(0.0);
    }
    else if (v_data_type == Dstring) {
        if (v_array.size()) {
            if (rng) {
                if (v_array.check_range(&md, &ld)) {
                    int i = v_array.Astart(md, ld);
                    int ie = v_array.Aend(md, ld);
                    for ( ; i <= ie; i++) {
                        if (data_ps()[i])
                            *data_ps()[i] = 0;
                    }
                }
            }
            else {
                for (int i = 0; i < v_array.size(); i++) {
                    if (data_ps()[i])
                        *data_ps()[i] = 0;
                }
            }
        }
        else
            *data_s() = 0;
    }
    if (v_events)
        trigger();
}


// Clear all value and type information.
//
void
vl_var::reset()
{
    if (v_array.size()) {
        if (v_data_type == Dbit || v_data_type == Dstring) {
            char **s = data_ps();
            for (int i = 0; i < v_array.size(); i++)
                delete [] s[i];
        }
        else
            delete [] data_ps();
    }
    else if (v_data_type == Dbit || v_data_type == Dstring)
        delete [] data_s();
    v_data_type = Dnone;
    v_array.clear();
    v_bits.clear();
    set_data_r(0);
}


// Set bit field from bit expression parser.
//
void
vl_var::set(vl_bitexp_parse *p)
{
    if ((v_data_type == Dnone || v_data_type == Dbit) && !v_array.size()) {
        if (v_data_type == Dbit)
            delete [] data_s();
        v_data_type = Dbit;
        v_bits = p->v_bits;
        set_data_s(new char[v_bits.size()]);
        memcpy(data_s(), p->data_s(), v_bits.size());
    }
    else {
        vl_error("(internal) incorrect data type in set-v_bits");
        VS()->abort();
    }
}


// Set scalar int.
//
void
vl_var::set(int i)
{
    if ((v_data_type == Dnone || v_data_type == Dint) && !v_array.size()) {
        v_data_type = Dint;
        v_bits.clear();
        set_data_i(i);
    }
    else {
        vl_error("(internal) incorrect data type in set-scalar_int");
        VS()->abort();
    }
}


// Set scalar time.
//
void
vl_var::set(vl_time_t t)
{
    if ((v_data_type == Dnone || v_data_type == Dtime) && !v_array.size()) {
        v_data_type = Dtime;
        v_bits.clear();
        set_data_t(t);
    }
    else {
        vl_error("(internal) incorrect data type in set-scalar_time");
        VS()->abort();
    }
}


// Set scalar real.
//
void
vl_var::set(double r)
{
    if ((v_data_type == Dnone || v_data_type == Dreal) && !v_array.size()) {
        v_data_type = Dreal;
        v_bits.clear();
        set_data_r(r);
    }
    else {
        vl_error("(internal) incorrect data type in set-scalar_real");
        VS()->abort();
    }
}


// Set scalar string.
//
void
vl_var::set(char *string)
{
    if ((v_data_type == Dnone || v_data_type == Dstring) && !v_array.size()) {
        if (v_data_type == Dstring)
            delete [] data_s();
        else
            v_data_type = Dstring;
        v_bits.clear();
        set_data_s(string);
    }
    else {
        vl_error("(internal) incorrect data type in set-scalar_string");
        VS()->abort();
    }
}


// Set scalar bit field to undefined with width w.
//
void        
vl_var::setx(int w)
{
    if ((v_data_type == Dnone || v_data_type == Dbit) && !v_array.size()) {
        if (v_data_type == Dbit)
            delete [] data_s();
        else
            v_data_type = Dbit;
        v_bits.set(w);
        set_data_s(new char[w]);
        for (int i = 0; i < w; i++)
            data_s()[i] = BitDC;
    }
    else {
        vl_error("(internal) incorrect data type in set-dc");
        VS()->abort();
    }
}               


// Set scalar bit field to hi-z with width w.
//
void        
vl_var::setz(int w)
{
    if ((v_data_type == Dnone || v_data_type == Dbit) && !v_array.size()) {
        if (v_data_type == Dbit)
            delete [] data_s();
        else
            v_data_type = Dbit;
        v_bits.set(w);
        set_data_s(new char[w]);
        for (int i = 0; i < w; i++)
            data_s()[i] = BitZ;
    }
    else {
        vl_error("(internal) incorrect data type in set-z");
        VS()->abort();
    }
}               


// Set scalar bit field to representation of ix.
//
void
vl_var::setb(int ix)
{
    if ((v_data_type == Dnone || v_data_type == Dbit) && !v_array.size()) {
        if (v_data_type == Dbit)
            delete [] data_s();
        else
            v_data_type = Dbit;
        v_bits.set(DefBits);
        set_data_s(new char[v_bits.size()]);
        int mask = 1;
        for (int i = 0; i < v_bits.size(); i++) {
            if (ix & mask)
                data_s()[i] = BitH;
            else
                data_s()[i] = BitL;
            mask <<= 1;
        }
    }
    else {
        vl_error("(internal) incorrect data type in set-integer");
        VS()->abort();
    }
}


// Set scalar bit field to representation of t.
//
void
vl_var::sett(vl_time_t t)
{
    if ((v_data_type == Dnone || v_data_type == Dbit) && !v_array.size()) {
        if (v_data_type == Dbit)
            delete [] data_s();
        else
            v_data_type = Dbit;
        v_bits.set((int)(8*sizeof(vl_time_t)));
        set_data_s(new char[v_bits.size()]);
        vl_time_t mask = 1;
        for (int i = 0; i < v_bits.size(); i++) {
            if (t & mask)
                data_s()[i] = BitH;
            else
                data_s()[i] = BitL;
            mask <<= 1;
        }
    }
    else {
        vl_error("(internal) incorrect data type in set-time");
        VS()->abort();
    }
}


// Fill the bit field with b.
//
void
vl_var::setbits(int b)
{
    if (v_data_type == Dbit) {
        if (!v_array.size())
            memset(data_s(), b, v_bits.size());
        else {
            for (int i = 0; i < v_array.size(); i++) {
                char *s = data_ps()[i];
                memset(s, b, v_bits.size());
            }
        }
    }
}


// Set a single bit at position pos. Return true if change.
//
bool
vl_var::set_bit_of(int pos, int bdata)
{
    if (v_data_type == Dbit) {
        if (pos >= 0 && pos < v_bits.size()) {
            char *s = (v_array.size() ? data_ps()[0] : data_s());
            char oldc = s[pos];
            s[pos] = bdata;
            if (oldc != bdata)
                return (true);
        }
    }
    else if (v_data_type == Dint) {
        if (pos >= 0 && pos < DefBits) {
            int i = (v_array.size() ? data_pi()[0] : data_i());
            int oldi = i;
            int mask = 1 << pos;
            if (bdata == BitH)
                i |= mask;
            else
                i &= ~mask;
            if (v_array.size())
                data_pi()[0] = i;
            else
                set_data_i(i);
            if (i != oldi)
                return (true);
        }
    }
    else if (v_data_type == Dtime) {
        if (pos >= 0 && pos < (int)sizeof(vl_time_t)*8) {
            vl_time_t i =
                (v_array.size() ? data_pt()[0] : data_t());
            vl_time_t oldi = i;
            vl_time_t mask = 1 << pos;
            if (bdata == BitH)
                i |= mask;
            else
                i &= ~mask;
            if (v_array.size())
                data_pt()[0] = i;
            else
                set_data_t(i);
            if (i != oldi)
                return (true);
        }
    }
    return (false);
}


// Set bits of vector bit field entry indx from src.  Extra bits are
// cleared.  If src is 0, fill the row with the value passed as swid.
// Returns true if the value changes.
//
bool
vl_var::set_bit_elt(int indx, const char *src, int swid)
{
    bool changed = false;
    if (v_data_type == Dbit) {
        char *s = 0;
        if (v_array.size() == 0 && indx == 0)
            s = data_s();
        else if (indx >= 0 && indx < v_array.size())
            s = data_ps()[indx];
        if (s) {
            if (!src) {
                for (int i = 0; i < v_bits.size(); i++) {
                    if (s[i] != swid) {
                        changed = true;
                        s[i] = swid;
                    }
                }
            }
            else {
                int mw = min(v_bits.size(), swid);
                int i;
                for (i = 0; i < mw; i++) {
                    if (s[i] != src[i]) {
                        changed = true;
                        s[i] = src[i];
                    }
                }
                for ( ; i < v_bits.size(); i++) {
                    if (s[i] != BitL) {
                        changed = true;
                        s[i] = BitL;
                    }
                }
            }
        }
    }
    return (changed);
}


// Set vector int entry indx, indx is raw, return true if changed.
//
bool
vl_var::set_int_elt(int indx, int val)
{
    bool changed = false;
    if (v_data_type == Dint) {
        if (v_array.size() == 0 && indx == 0) {
            if (data_i() != val) {
                changed = true;
                set_data_i(val);
            }
        }
        else if (indx >= 0 && indx < v_array.size()) {
            if (data_pi()[indx] != val) {
                changed = true;
                data_pi()[indx] = val;
            }
        }
    }
    return (changed);
}


// Set vector time entry indx, indx is raw, return true if changed.
//
bool
vl_var::set_time_elt(int indx, vl_time_t val)
{
    bool changed = false;
    if (v_data_type == Dtime) {
        if (v_array.size() == 0 && indx == 0) {
            if (data_t() != val) {
                changed = true;
                set_data_t(val);
            }
        }
        else if (indx >= 0 && indx < v_array.size()) {
            if (data_pt()[indx] != val) {
                changed = true;
                data_pt()[indx] = val;
            }
        }
    }
    return (changed);
}


// Set vector real entry indx, indx is raw, return true if changed.
//
bool
vl_var::set_real_elt(int indx, double val)
{
    bool changed = false;
    if (v_data_type == Dreal) {
        if (v_array.size() == 0 && indx == 0) {
            if (data_r() != val) {
                changed = true;
                set_data_r(val);
            }
        }
        else if (indx >= 0 && indx < v_array.size()) {
            if (data_pr()[indx] != val) {
                changed = true;
                data_pr()[indx] = val;
            }
        }
    }
    return (changed);
}


// Set vector string entry indx (not copied), indx is raw.
// Don't bother with changes.
//
bool
vl_var::set_str_elt(int indx, char *val)
{
    if (v_data_type == Dstring) {
        if (v_array.size() == 0 && indx == 0) {
            delete [] data_s();
            set_data_s(val);
        }
        else if (indx >= 0 && indx < v_array.size()) {
            char **ss = data_ps();
            delete [] ss[indx];
            ss[indx] = val;
        }
    }
    return (false);
}


void
vl_var::set_assigned(vl_bassign_stmt *bs)
{
    if (v_flags & VAR_F_ASSIGN)
        return;
    if (bs && v_cassign == bs && (v_flags & VAR_CP_ASSIGN)) {
        vl_var z = case_eq(*v_cassign->lhs(), v_cassign->rhs()->eval());
        if (z.data_s()[0] == BitH)
            return;
    }
    if (v_data_type == Dconcat) {
        lsGen<vl_expr*> gen(data_c());
        vl_expr *e;
        while (gen.next(&e)) {
            vl_var *v = e->source();
            if (v) {
                if (bs && !v->check_net_type(REGreg)) {
                    vl_error("in procedural continuous assign, concatenated "
                        "component is not a reg");
                    errout(this);
                    VS()->abort();
                    return;
                }
                v->anot_flags(VAR_CP_ASSIGN);
            }
            else {
                vl_error("in procedural continuous assign, concatenated "
                    "component has undefined width");
                errout(this);
                VS()->abort();
                return;
            }
        }
    }
    else if (bs && v_net_type != REGreg) {
        vl_error("in procedural continuous assign, %s is not a reg", v_name);
        errout(this);
        VS()->abort();
        return;
    }
    anot_flags(VAR_CP_ASSIGN);
    if (v_cassign) {
        v_cassign->rhs()->unchain(v_cassign);
        v_cassign = 0;
    }
    if (bs) {
        v_cassign = bs;
        v_cassign->lhs()->assign(0, &v_cassign->rhs()->eval(), 0);
        v_cassign->rhs()->chain(v_cassign);
        or_flags(VAR_CP_ASSIGN);
        if (v_data_type == Dconcat) {
            lsGen<vl_expr*> gen(data_c());
            vl_expr *e;
            while (gen.next(&e)) {
                vl_var *v = e->source();
                v->or_flags(VAR_CP_ASSIGN);
            }
        }
    }
}


void
vl_var::set_forced(vl_bassign_stmt *bs)
{
    if (bs) {
        if (v_cassign == bs && (v_flags & VAR_F_ASSIGN)) {
            vl_var z = case_eq(*v_cassign->lhs(), v_cassign->rhs()->eval());
            if (z.data_s()[0] == BitH)
                return;
        }
        set_assigned(0);
    }
    if (v_data_type == Dconcat) {
        lsGen<vl_expr*> gen(data_c());
        vl_expr *e;
        while (gen.next(&e)) {
            vl_var *v = e->source();
            if (!v) {
                vl_error("in force, bad value in concatenation");
                errout(this);
                VS()->abort();
                return;
            }
            v->anot_flags(VAR_F_ASSIGN);
        }
    }
    anot_flags(VAR_F_ASSIGN);
    if (v_cassign) {
        v_cassign->rhs()->unchain(v_cassign);
        v_cassign = 0;
    }
    if (bs) {
        v_cassign = bs;
        v_cassign->lhs()->assign(0, &v_cassign->rhs()->eval(), 0);
        v_cassign->rhs()->chain(v_cassign);
        or_flags(VAR_F_ASSIGN);
        if (v_data_type == Dconcat) {
            lsGen<vl_expr*> gen(data_c());
            vl_expr *e;
            while (gen.next(&e)) {
                vl_var *v = e->source();
                v->or_flags(VAR_F_ASSIGN);
            }
        }
    }
}


// If the lhs is a concatenation in a delayed assignment, have to evaluate
// any ranges before the delay.
//
void
vl_var::freeze_concat()
{
    if (v_data_type != Dconcat)
        return;
    lsGen<vl_expr*> gen(data_c());
    vl_expr *e;
    while (gen.next(&e)) {
        if (e->etype() == BitSelExpr || e->etype() == PartSelExpr) {
            vl_range *tr = 0;
            if (e->edata().ide.range) {
                tr = e->edata().ide.range->reval();
                delete e->edata().ide.range;
            }
            e->edata().ide.range = tr;
        }
    }
}


// Set the default effective bit field parameters.
//
void
vl_var::default_range(int *m, int *l)
{
    if (v_array.size()) {
        *m = v_array.hi_index();
        *l = v_array.lo_index();
        return;
    }
    *m = 0;
    *l = 0;
    if (v_data_type == Dbit) {
        *m = v_bits.hi_index();
        *l = v_bits.lo_index();
    }
    else if (v_data_type == Dint)
        *m = DefBits - 1;
    else if (v_data_type == Dtime)
        *m = sizeof(vl_time_t)*8 - 1;
    else if (v_data_type == Dreal)
        *m = DefBits - 1;
    else if (v_data_type == Dstring)
        *m = (data_s() ? (strlen(data_s()) + 1)*8 : 0);
}


// Return true if the variable is a reg and reg_or_net is REGreg, or
// type is net and reg_or_net is not REGreg.
//
bool
vl_var::check_net_type(REGtype reg_or_net)
{
    if (v_data_type == Dconcat) {
        lsGen<vl_expr*> gen(data_c());
        vl_expr *e;
        while (gen.next(&e)) {
            vl_var *v = e->source();
            if (!v) {
                vl_error("bad expression in concatenation:");
                errout(this);
                VS()->abort();
                return (false);
            }
            if (!v->check_net_type(reg_or_net))
                return (false);
        }
        return (true);
    }
    else if (reg_or_net == REGreg)
        return (v_net_type == REGreg ? true : false);
    else if (v_net_type < REGwire) {
        if (v_net_type == REGnone)
            v_net_type = REGwire;
        else
            return (false);
    }
    return (true);
}


// Check and readjust the range values if necessary to correspond to the
// effective bit width.  False is returned if out of range.
//
bool
vl_var::check_bit_range(int *m, int *l)
{
    if (v_data_type == Dbit)
        return (v_bits.check_range(m, l));
    if (v_data_type == Dint || v_data_type == Dreal) {
        int mx = DefBits - 1;
        if (*m == *l) {
            if (*m < 0 || *m >= mx)
                return (false);
        }
        else {
            if (*m < *l) {
                vl_error("in check range, index direction mismatch found");
                VS()->abort();
                return (false);
            }
            if (*l > mx || *m < 0)
                return (false);
            if (*m > mx)
                *m = mx;
            if (*l < 0)
                *l = 0;
        }
        return (true);
    }
    if (v_data_type == Dtime) {
        int mx = sizeof(vl_time_t)*8 - 1;
        if (*m == *l) {
            if (*m < 0 || *m >= mx)
                return (false);
        }
        else {
            if (*m < *l) {
                vl_error("in check range, index direction mismatch found");
                VS()->abort();
                return (false);
            }
            if (*l > mx || *m < 0)
                return (false);
            if (*m > mx)
                *m = mx;
            if (*l < 0)
                *l = 0;
        }
        return (true);
    }
    if (v_data_type == Dstring) {
        char *s = (v_array.size() ? data_ps()[0] : data_s());
        int mx = (s ? (strlen(s) + 1)*8 : 0);
        if (*m == *l) {
            if (*m < 0 || *m >= mx)
                return (false);
        }
        else {
            if (*m < *l) {
                vl_error("in check range, index direction mismatch found");
                VS()->abort();
                return (false);
            }
            if (*l > mx || *m < 0)
                return (false);
            if (*m > mx)
                *m = mx;
            if (*l < 0)
                *l = 0;
        }
        return (true);
    }
    return (false);
}


// Add d to driver list, if not already there.  Call this for nets.
// Args:
//  mt:  msb of destination range, internal (0-based) offset
//  lt:  lsb of destination range, internal (0-based) offset, <= mt
//  lf:  lsb of source range, internal (0-based) offset.
//
void
vl_var::add_driver(vl_var *d, int mt, int lt, int lf)
{
    if (d->v_array.size() != 0) {
        vl_error("net driver is an array, not allowed");
        VS()->abort();
        return;
    }
    if (!v_drivers) {
        v_drivers = new lsList<vl_driver*>;
        vl_driver *drv = new vl_driver(d, mt, lt, lf);
        v_drivers->newEnd(drv);
        return;
    }
    lsGen<vl_driver*> gen(v_drivers);
    vl_driver *drv;
    while (gen.next(&drv)) {
        if (drv->srcvar() == d && drv->l_from() == lf) {
            if (lt == drv->l_to()) {
                if (mt > drv->m_to())
                    drv->set_m_to(mt);
                return;
            }
        }
    }
    drv = new vl_driver(d, mt, lt, lf);
    v_drivers->newEnd(drv);
}


namespace {
    // Return the highest strength bit, for determining bit value of a net
    // with competing drivers.
    //
    int strength_bit(int b1, vl_strength s1, int b2, vl_strength s2)
    {
        if (b1 == b2)
            return (b1);
        if (b1 == BitZ)
            return (b2);
        if (b2 == BitZ)
            return (b1);
        if (b1 == BitL && s1.str0() > s2.str1())
            return (BitL);
        if (b1 == BitH && s1.str1() > s2.str0())
            return (BitH);
        if (b2 == BitL && s2.str0() > s1.str1())
            return (BitL);
        if (b2 == BitH && s2.str1() > s1.str0())
            return (BitH);
        return (BitDC);
    }
}


// Resolve contention, return the "winning" value, called for nets.
// The ix is in the bits range,  din should be in the drivers list.
//
// With inout ports, there is a potential problem due to the loop
// topology, since each node is continuously driven by the other node,
// which prevents setting a new value in general.  In addition, an
// actual arg may be a node for more than one inout loop.  The problem
// is resolved by setting a flag in each var/expr that drives across
// an inout port.  Here, when resolving a value, the following logic
// is used:
//    if (input is BitZ)
//        { no problem }
//    else
//        { skip all drivers with flag set, unless it == input }
//
// The value ixs is the bit offset of the driver.  Both ix and ixs are
// internal indices.
//
int
vl_var::resolve_bit(int ix, vl_var *din, int ixs)
{
    int b = BitZ;
    lsGen<vl_driver*> gen(v_drivers);
    vl_driver *dr;
    vl_strength accum_str;
    bool skip_portdrv = false;
    if (din->bit_of(ixs) != BitZ)
        skip_portdrv = true;
    bool first = true;
    while (gen.next(&dr)) {
        vl_var *dt = dr->srcvar();
        if (skip_portdrv && (dt->flags() & VAR_PORT_DRIVER) && dt != din)
            continue;

        if (ix > dr->m_to() || ix < dr->l_to())
            continue;

        vl_strength st = dt->v_strength;
        if (st.str0() == STRnone)
            st = v_strength;
        if (st.str0() == STRnone)
            st.set_str0(STRstrong);
        if (st.str1() == STRnone)
            st.set_str1(STRstrong);

        int bx = dt->bit_of((ix - dr->l_to()) + dr->l_from());
        if ((bx == BitH && st.str1() == STRhiZ) ||
                (bx == BitL && st.str0() == STRhiZ))
            continue;
        if (bx == BitZ)
            continue;

        if ((v_net_type == REGwand || v_net_type == REGtrior) && bx == BitL) {
            b = BitL;
            return (b);
        }
        if ((v_net_type == REGwor || v_net_type == REGtriand) && bx == BitH) {
            b = BitH;
            return (b);
        }
        if (first) {
            first = false;
            b = bx;
            accum_str = st;
        }
        else
            b = strength_bit(b, accum_str, bx, st);

        if (b == BitH && bx == BitH) {
            if (st.str1() > accum_str.str1())
                accum_str.set_str1(st.str1());
        }
        else if (b == BitL && bx == BitL) {
            if (st.str0() > accum_str.str0())
                accum_str.set_str0(st.str0());
        }
        else if (b == BitDC) {
            if (st.str1() > accum_str.str1())
                accum_str.set_str1(st.str1());
            if (st.str0() > accum_str.str0())
                accum_str.set_str0(st.str0());
        }
    }
    if (b == BitZ) {
        if (v_net_type == REGtri0)
            b = BitL;
        else if (v_net_type == REGtri1)
            b = BitH;
    }
    return (b);
}


// This is called when data changes,  perform the asynchronous actions.
//
void
vl_var::trigger()
{
    vl_action_item *ap = 0, *an;
    for (vl_action_item *a = v_events; a; a = an) {
        an = a->next();
        if (a->event()) {
            if (a->event()->eval(VS())) {
                if (a->event()->count()) {
                    a->event()->set_count(a->event()->count() - 1);
                    return;
                }
                a->set_next(0);
                a->event()->unchain(a);
                a->set_event(0);
                VS()->timewheel()->append_trig(VS()->time(), a);
                if (ap)
                    ap->set_next(an);
                else
                    v_events = an;
                continue;
            }
            ap = a;
            continue;
        }
        // continuous assign
        VS()->timewheel()->append_trig(VS()->time(), chk_copy(a));
        ap = a;
    }
}


// Return true if a bit is BitDC or BitZ.
//
bool
vl_var::is_x()
{
    if (v_data_type == Dbit && !v_array.size()) {
        for (int i = 0; i < v_bits.size(); i++) {
            if (data_s()[i] != BitL && data_s()[i] != BitH)
                return (true);
        }
    }
    return (false);
}


// Return true if all bits are BitZ.
//
bool
vl_var::is_z()
{
    if (v_data_type == Dbit && !v_array.size()) {
        for (int i = 0; i < v_bits.size(); i++) {
            if (data_s()[i] != BitZ)
                return (false);
        }
        return (true);
    }
    return (false);
}


// Return the bit value at the given raw index position.
//
int
vl_var::bit_of(int i)
{
    if (v_data_type == Dbit) {
        if (i < v_bits.size()) {
            if (v_array.size())
                return (data_ps()[0][i]);
            else
                return (data_s()[i]);
        }
    }
    else if (v_data_type == Dint) {
        if (i < (int)sizeof(int)*8) {
            if (v_array.size())
                return (bit(data_pi()[0], i));
            else
                return (bit(data_i(), i));
        }
    }
    else if (v_data_type == Dtime) {
        if (i < (int)sizeof(vl_time_t)*8) {
            if (v_array.size())
                return (bit(data_pt()[0], i));
            else
                return (bit(data_t(), i));
        }
    }
    return (BitZ);
}


// Return integer created from selected bits of bit field.
//
int
vl_var::int_bit_sel(int m, int l)
{
    int ret = 0;
    if (v_data_type == Dbit) {
        char *s;
        if (v_array.size() == 0)
            s = data_s();
        else
            s = *(char**)data_s();
        int mask = 1;
        int i = v_bits.Bstart(m, l);
        int ie = v_bits.Bend(m, l);
        for ( ; i <= ie; i++) {
            if (s[i] == BitH)
                ret |= mask;
            else if (s[i] != BitL)
                return (0);
            mask <<= 1;
        }
    }
    return (ret);
}


// Return vl_time_t created from selected bits of bit field.
//
vl_time_t
vl_var::time_bit_sel(int m, int l)
{
    vl_time_t ret = 0;
    if (v_data_type == Dbit) {
        char *s;
        if (v_array.size() == 0)
            s = data_s();
        else
            s = *(char**)data_s();
        vl_time_t mask = 1;
        int i = v_bits.Bstart(m, l);
        int ie = v_bits.Bend(m, l);
        for ( ; i <= ie; i++) {
            if (s[i] == BitH)
                ret |= mask;
            else if (s[i] != BitL)
                return (0);
            mask <<= 1;
        }
    }
    return (ret);
}


// Return real created from selected bits of bit field.
//
double
vl_var::real_bit_sel(int m, int l)
{
    vl_time_t ret = 0;
    if (v_data_type == Dbit) {
        char *s;
        if (v_array.size() == 0)
            s = data_s();
        else
            s = *(char**)data_s();
        vl_time_t mask = 1;
        int i = v_bits.Bstart(m, l);
        int ie = v_bits.Bend(m, l);
        for ( ; i <= ie; i++) {
            if (s[i] == BitH)
                ret |= mask;
            else if (s[i] != BitL)
                return (0.0);
            mask <<= 1;
        }
    }
    return ((double)ret);
}


// Return a code indicating bits set: Hmask if there is a high bit,
// else Xmask if there is an X bit, or Lmask otherwise.
//
int
vl_var::bitset()
{
    if (v_data_type == Dbit) {
        int ret = 0;
        for (int i = 0; i < v_bits.size(); i++) {
            if (data_s()[i] == BitH)
                return (Hmask);
            else if (data_s()[i] != BitL)
                ret |= Xmask;
        }
        if (ret & Xmask)
            return (Xmask);
        return (Lmask);
    }
    if (v_data_type == Dint)
        return (data_i() ? Hmask : Lmask);
    if (v_data_type == Dtime)
        return (data_t() ? Hmask : Lmask);
    return (Lmask);
}
 

// Return a pointer to the raw num'th data element, the type of data is
// returned in rt, works whether array or not.
//
void *
vl_var::element(int num, int *rt)
{
    if (v_array.size() == 0 && num == 0) {
        if (v_data_type == Dnone) {
            *rt = Dint;
            return (&v_data.i);
        }
        if (v_data_type == Dbit) {
            *rt = Dbit;
            return (data_s());
        }
        if (v_data_type == Dint) {
            *rt = Dint;
            return (&v_data.i);
        }
        if (v_data_type == Dtime) {
            *rt = Dtime;
            return (&v_data.t);
        }
        if (v_data_type == Dreal) {
            *rt = Dreal;
            return (&v_data.r);
        }
        if (v_data_type == Dstring) {
            *rt = Dstring;
            return (data_s());
        }
        return (0);
    }
    if (num < 0 || num >= v_array.size())
        return (0);

    if (v_data_type == Dbit) {
        *rt = Dbit;
        return (data_ps()[num]);
    }
    if (v_data_type == Dint) {
        *rt = Dint;
        return (data_pi() + num);
    }
    if (v_data_type == Dtime) {
        *rt = Dtime;
        return (data_pt() + num);
    }
    if (v_data_type == Dreal) {
        *rt = Dreal;
        return (data_pr() + num);
    }
    if (v_data_type == Dstring) {
        *rt = Dstring;
        return (data_ps()[num]);
    }
    return (0);
}


// Return the bits and field width from raw num'th element.
//
char *
vl_var::bit_elt(int num, int *bw)
{
    int tp;
    static char buf[8*sizeof(vl_time_t)];
    void *v = element(num, &tp);
    if (!v)
        return (0);
    if (tp == Dbit) {
        *bw = v_bits.size();
        return ((char*)v);
    }
    if (tp == Dint) {
        int sz = DefBits;
        char *s = buf;
        int ii = *(int*)v;
        for (int i = 0; i < sz; i++) {
            if (ii & 1)
                *s = BitH;
            else
                *s = BitL;
            s++;
            ii >>= 1;
        }
        *bw = sz;
        return (buf);
    }
    if (tp == Dtime) {
        int sz = 8*(int)sizeof(vl_time_t);
        char *s = buf;
        vl_time_t tt = *(vl_time_t*)v;
        for (int i = 0; i < sz; i++) {
            if (tt & 1)
                *s = BitH;
            else
                *s = BitL;
            s++;
            tt >>= 1;
        }
        *bw = sz;
        return (buf);
    }
    if (tp == Dreal) {
        int sz = DefBits;
        char *s = buf;
        double d = *(double*)v;
        int ii = (int)d;
        for (int i = 0; i < sz; i++) {
            if (ii & 1)
                *s = BitH;
            else
                *s = BitL;
            s++;
            ii >>= 1;
        }
        *bw = sz;
        return (buf);
    }
    if (tp == Dstring) {
        char *str = vl_fix_str((char*)v);
        int sz = 8*(strlen(str) + 1);
        *bw = sz;
        char *ss = new char[sz];
        char *s0 = ss;
        for (char *c = str; *c; c++) {
            for (int i = 0; i < 8; i++) {
                if ((*c >> i) & 1)
                    *ss = BitH;
                else
                    *ss = BitL;
                ss++;
            }
        }
        delete [] str;
        for (int i = 0; i < 8; i++)
            *ss++ = BitL;
        return (s0);
    }
    return (0);
}


// Return integer corresponding to raw num'th data element.
//
int
vl_var::int_elt(int num)
{
    int tp;
    void *v = element(num, &tp);
    if (!v)
        return (0);
    if (tp == Dbit)
        return (bits2int((char*)v, v_bits.size()));
    if (tp == Dint)
        return (*(int*)v);
    if (tp == Dtime)
        return ((int)*(vl_time_t*)v);
    if (tp == Dreal)
        return (rnd(*(double*)v));
    if (tp == Dstring) {
        char *s = vl_fix_str((char*)v);
        int i = 0;
        for (int j = 0; j < (int)sizeof(int); j++) {
            if (s[j])
                i |= s[j] << j;
            else
                break;
        }
        delete [] s;
        return (i);
    }
    return (0);
}


// Return vl_time_t corresponding to raw num'th data element.
//
vl_time_t
vl_var::time_elt(int num)
{
    int tp;
    void *v = element(num, &tp);
    if (!v)
        return (0);
    if (tp == Dbit)
        return (bits2time((char*)v, v_bits.size()));
    if (tp == Dint)
        return (*(int*)v);
    if (tp == Dtime)
        return (*(vl_time_t*)v);
    if (tp == Dreal)
        return (trnd(*(double*)v));
    if (tp == Dstring) {
        char *s = vl_fix_str((char*)v);
        vl_time_t i = 0;
        for (int j = 0; j < (int)sizeof(vl_time_t); j++) {
            if (s[j])
                i |= s[j] << j;
            else
                break;
        }
        delete [] s;
        return (i);
    }
    return (0);
}


// Return double corresponding to raw num'th data element.
//
double
vl_var::real_elt(int num)
{
    int tp;
    void *v = element(num, &tp);
    if (!v)
        return (0);
    if (tp == Dbit)
        return (bits2real((char*)v, v_bits.size()));
    if (tp == Dint)
        return ((double)*(int*)v);
    if (tp == Dtime)
        return ((double)*(vl_time_t*)v);
    if (tp == Dreal)
        return (*(double*)v);
    return (0);
}


// Return string for raw num'th element, if string data.
//
char *
vl_var::str_elt(int num)
{
    int tp;
    void *v = element(num, &tp);
    if (!v)
        return (0);
    if (tp == Dstring)
        return ((char*)v);
    if (tp == Dbit) {
        int sz = v_bits.size()/8 + 2;
        char *ss = new char[sz];
        for (int i = 0; i < sz; i++)
            ss[i] = 0;
        char *s = ss;
        char *b = (char*)v;
        int mask = 1;
        for (int i = 0; i < v_bits.size(); i++) {
            if (b[i] == BitH)
                *s |= mask;
            mask <<= 1;
            if (mask == 0x100) {
                mask = 1;
                s++;
            }
        }
        return (ss);
    }
    return (0);
}


//
// The following are private
//

// Compute the actual number of bits set in the assignment, know that
// l_from is in range.
// Static private function.
//
int
vl_var::bits_set(vl_var *dst, vl_range *r, vl_var *src, int l_from)
{
    int m, l;
    if (r) {
        if (!r->eval(&m, &l))
            return (0);
        if (!dst->v_bits.check_range(&m, &l))
            return (0);
    }
    else {
        m = dst->v_bits.hi_index();
        l = dst->v_bits.lo_index();
    }
    int w = rsize(m, l);
    int w1 = src->bit_size() - dst->v_bits.Bstart(l_from, l_from);
    return (min(w, w1));
}


// Private function.
//
void
vl_var::print_drivers()
{
    if (!v_drivers)
        return;
    lsGen<vl_driver*> gen(v_drivers);
    vl_driver *dr;
    cout << "Drivers: ";
    while (gen.next(&dr)) {
        if (dr->srcvar()->flags() & VAR_PORT_DRIVER)
            cout << "(p)";
        cout << dr->srcvar();
        if (dr->srcvar()->net_type() >= REGwire)
            dr->srcvar()->v_strength.print();
        else
            v_strength.print();
        cout << '[' << dr->m_to() << '|' << dr->l_to() << '|' <<
            dr->l_from() << "] ";
        dr->srcvar()->print_value(cout);
        cout << ' ';
    }
    cout << "\n";
}


// Static private function.
//
void
vl_var::probe1(vl_var *vo, int md, int ld, vl_var *vin, int ms, int ls)
{
    cout << vo << '[' << md << '|' << ld << ']';
    cout << " assigning: " << vin << '[' << ms << '|' << ls << "] ";
    vin->print_value(cout);
    cout << '\n';
}


// Private function.
//
void
vl_var::probe2()
{
    cout << "New value: ";
    print_value(cout);
    cout << '\n';
    print_drivers();
}


void
vl_var::assign_init(vl_var *src, int ms, int ls)
{
    if (v_data_type != Dnone)
        return;
    if (src->v_array.size() == 0) {
        if (src->check_bit_range(&ms, &ls)) {
            setx(abs(ms - ls) + 1);
            v_bits.set_lo_hi(ls, ms);
            v_bits.Bnorm();
            int j = src->v_bits.Bstart(ms, ls);
            for (int i = 0; i < v_bits.size(); i++, j++)
                data_s()[i] = src->bit_of(j);
        }
        else
            setx(1);
    }
    else {
        if (src->v_array.check_range(&ms, &ls)) {
            v_data_type = src->v_data_type;
            int sr = rsize(ms, ls);
            v_array.set(sr);
            if (v_array.size() <= 1) {
                v_array.set((int)0);
                v_array.set_lo_hi(0, 0);
            }
            else {
                v_array.set_lo_hi(ls, ms);
                v_array.Anorm();
            }
            if (src->v_data_type == Dbit) {
                v_bits = src->v_bits;
                int bw;
                if (v_array.size() == 0) {
                    set_data_s(new char[v_bits.size()]);
                    char *s = src->bit_elt(src->v_array.Astart(ms, ls), &bw);
                    memcpy(data_s(), s, bw);
                    if (src->v_data_type == Dstring)
                        delete [] s;
                }
                else {
                    char *s, **ss = new char*[v_array.size()];
                    set_data_ps(ss);
                    int j = src->v_array.Astart(ms, ls);
                    for (int i = 0; i < sr; i++, j++) {
                        ss[i] = new char[v_bits.size()];
                        s = src->bit_elt(j, &bw);
                        memcpy(ss[i], s, bw);
                        if (src->v_data_type == Dstring)
                            delete [] s;
                    }
                }
            }
            else if (src->v_data_type == Dint) {
                if (v_array.size())
                    set_data_pi(new int[v_array.size()]);
                int j = src->v_array.Astart(ms, ls);
                for (int i = 0; i < sr; i++, j++)
                    set_int_elt(i, src->int_elt(j));
            }
            else if (src->v_data_type == Dtime) {
                if (v_array.size())
                    set_data_pt(new vl_time_t[v_array.size()]);
                int j = src->v_array.Astart(ms, ls);
                for (int i = 0; i < sr; i++, j++)
                    set_time_elt(i, src->time_elt(j));
            }
            else if (src->v_data_type == Dreal) {
                if (v_array.size())
                    set_data_pr(new double[v_array.size()]);
                int j = src->v_array.Astart(ms, ls);
                for (int i = 0; i < sr; i++, j++)
                    set_real_elt(i, src->real_elt(j));
            }
            else if (src->v_data_type == Dstring) {
                if (v_array.size())
                    set_data_ps(new char*[v_array.size()]);
                int j = src->v_array.Astart(ms, ls);
                for (int i = 0; i < sr; i++, j++)
                    set_str_elt(i, vl_strdup(src->str_elt(j)));
            }
        }
        else {
            if (src->v_data_type == Dbit)
                setx(src->v_bits.size());
            else if (src->v_data_type == Dint)
                set((int)0);
            else if (src->v_data_type == Dtime)
                sett(0);
            else if (src->v_data_type == Dreal) {
                v_data_type = Dreal;
                set_data_r(0.0);
            }
            else if (src->v_data_type == Dstring) {
                v_data_type = Dstring;
                set_data_s(vl_strdup(""));
            }
        }
    }
    if (v_events)
        trigger();
}


void
vl_var::assign_bit_range(int md, int ld, vl_var *src, int ms, int ls)
{
    if (v_data_type != Dbit)
        return;
    if (v_net_type >= REGwire) {
        if (v_array.size() != 0) {
            vl_error("in assignment, assigned-to net is an array!");
            VS()->abort();
            return;
        }
        if (src->v_array.size() != 0) {
            vl_error("in assignment, assigning net is an array!");
            VS()->abort();
            return;
        }
        if (src->v_data_type != Dbit && src->v_data_type != Dint &&
                src->v_data_type != Dtime) {
            vl_error("in assignment, assigning net from wrong data type");
            VS()->abort();
            return;
        }
        if (v_bits.check_range(&md, &ld)) {
            bool arm_trigger = false;
            int i = v_bits.Bstart(md, ld);
            int ie = v_bits.Bend(md, ld);
            if (src->check_bit_range(&ms, &ls)) {
                int j = src->v_bits.Bstart(ms, ls);
                int je = src->v_bits.Bend(ms, ls);
                add_driver(src, ie, i, j);
                if (VS()->dbg_flags() & DBG_assign)
                    probe1(this, ie, i, src, je, j);
                for ( ; i <= ie && j <= je; i++, j++) {
                    int b = resolve_bit(i, src, j);
                    if (data_s()[i] != b) {
                        arm_trigger = true;
                        data_s()[i] = b;
                    }
                }
                for ( ; i <= ie; i++) {
                    if (data_s()[i] != BitL) {
                        arm_trigger = true;
                        data_s()[i] = BitL;
                    }
                }
                if (VS()->dbg_flags() & DBG_assign)
                    probe2();
            }
            else {
                for ( ; i <= ie; i++) {
                    if (data_s()[i] != BitDC) {
                        arm_trigger = true;
                        data_s()[i] = BitDC;
                    }
                }
            }
            if (arm_trigger && v_events)
                trigger();
        }
    }
    else if (v_array.size() == 0) {
        if (v_net_type == REGsupply0 || v_net_type == REGsupply1)
            return;
        if (!v_bits.check_range(&md, &ld))
            return;
        if (src->v_array.size() == 0)
            assign_bit_SS(md, ld, src, ms, ls);
        else
            assign_bit_SA(md, ld, src, ms, ls);
        if (VS()->dbg_flags() & DBG_assign) {
            cout << this << " = ";
            print_value(cout);
            cout << '\n';
        }
    }
    else {
        if (!v_array.check_range(&md, &ld))
            return;
        if (src->v_array.size() == 0)
            assign_bit_AS(md, ld, src, ms, ls);
        else
            assign_bit_AA(md, ld, src, ms, ls);
    }
}


// Assign bit field, scalar from scalar.
//
void
vl_var::assign_bit_SS(int md, int ld, vl_var *src, int ms, int ls)
{
    bool arm_trigger = false;
    int i = v_bits.Bstart(md, ld);
    int ie = v_bits.Bend(md, ld);
    if (src->check_bit_range(&ms, &ls)) {
        int bw;
        char *s = src->bit_elt(0, &bw);
        int j = src->v_bits.Bstart(ms, ls);
        int je = src->v_bits.Bend(ms, ls);
        for ( ; i <= ie && j <= je; i++, j++) {
            int b = s[j];
            if (data_s()[i] != b) {
                arm_trigger = true;
                data_s()[i] = b;
            }
        }
        for ( ; i <= ie; i++) {
            if (data_s()[i] != BitL) {
                arm_trigger = true;
                data_s()[i] = BitL;
            }
        }
        if (src->v_data_type == Dstring)
            delete [] s;
    }
    else {
        for ( ; i <= ie; i++) {
            if (data_s()[i] != BitDC) {
                arm_trigger = true;
                data_s()[i] = BitDC;
            }
        }
    }
    if (arm_trigger && v_events)
        trigger();
}


// Assign bit field, scalar from array.
//
void
vl_var::assign_bit_SA(int md, int ld, vl_var *src, int ms, int ls)
{
    bool arm_trigger = false;
    int i = v_bits.Bstart(md, ld);
    int ie = v_bits.Bend(md, ld);
    if (src->v_array.check_range(&ms, &ls)) {
        int bw;
        char *s = src->bit_elt(src->v_array.Astart(ms, ls), &bw);
        int j = src->v_bits.Bstart(ms, ls);
        int je = src->v_bits.Bend(ms, ls);
        for ( ; i <= ie && j <= je; i++, j++) {
            int b = s[j];
            if (data_s()[i] != b) {
                arm_trigger = true;
                data_s()[i] = b;
            }
        }
        for ( ; i <= ie; i++) {
            if (data_s()[i] != BitL) {
                arm_trigger = true;
                data_s()[i] = BitL;
            }
        }
        if (src->v_data_type == Dstring)
            delete [] s;
    }
    else {
        for ( ; i <= ie; i++) {
            if (data_s()[i] != BitDC) {
                arm_trigger = true;
                data_s()[i] = BitDC;
            }
        }
    }
    if (arm_trigger && v_events)
        trigger();
}


// Assign bit field, array from scalar.
//
void
vl_var::assign_bit_AS(int md, int ld, vl_var *src, int ms, int ls)
{
    bool arm_trigger = false;
    if (src->check_bit_range(&ms, &ls)) {
        int bw;
        char *s = src->bit_elt(0, &bw);
        if (set_bit_elt(v_array.Astart(md, ld),
                &s[src->v_bits.Bstart(ms, ls)], rsize(ms, ls)))
            arm_trigger = true;
        if (src->v_data_type == Dstring)
            delete [] s;
    }
    else {
        if (set_bit_elt(v_array.Astart(md, ld), 0, BitDC))
            arm_trigger = true;
    }
    if (arm_trigger && v_events)
        trigger();
}


// Assign bit field, array from array.
//
void
vl_var::assign_bit_AA(int md, int ld, vl_var *src, int ms, int ls)
{
    bool arm_trigger = false;
    int i = v_array.Astart(md, ld);
    int ie = v_array.Aend(md, ld);
    if (src->v_array.check_range(&ms, &ls)) {
        int j = src->v_array.Astart(ms, ls);
        int je = src->v_array.Aend(ms, ls);
        for ( ; i <= ie && j <= je; i++, j++) {
            int bw;
            char *s = src->bit_elt(j, &bw);
            if (set_bit_elt(i, s, bw))
                arm_trigger = true;
            if (src->v_data_type == Dstring)
                delete [] s;
        }
        for ( ; i <= ie; i++) {
            if (set_bit_elt(i, 0, BitL))
                arm_trigger = true;
        }
    }
    else {
        for ( ; i <= ie; i++) {
            if (set_bit_elt(i, 0, BitDC))
                arm_trigger = true;
        }
    }
    if (arm_trigger && v_events)
        trigger();
}


void
vl_var::assign_int_range(int md, int ld, vl_var *src, int ms, int ls)
{
    if (v_data_type != Dint)
        return;
    if (v_array.size() == 0) {
        if (!check_bit_range(&md, &ld))
            return;
        if (src->v_array.size() == 0)
            assign_int_SS(md, ld, src, ms, ls);
        else
            assign_int_SA(md, ld, src, ms, ls);
        if (VS()->dbg_flags() & DBG_assign) {
            cout << this << " = ";
            print_value(cout);
            cout << '\n';
        }
    }
    else {
        if (!v_array.check_range(&md, &ld))
            return;
        if (src->v_array.size() == 0)
            assign_int_AS(md, ld, src, ms, ls);
        else
            assign_int_AA(md, ld, src, ms, ls);
    }
}


// Assign integer, scalar from scalar.
//
void
vl_var::assign_int_SS(int md, int ld, vl_var *src, int ms, int ls)
{
    bool arm_trigger = false;
    if (src->check_bit_range(&ms, &ls)) {
        int bw;
        char *s = src->bit_elt(0, &bw);
        int j = src->v_bits.Bstart(ms, ls);
        int sr = rsize(ms, ls);
        int w = min(sr, rsize(md, ld)) - 1;
        if (is_indeterminate(s, j, j + w)) {
            for (int i = ld; i <= md; i++) {
                if (set_bit_of(i, BitL))
                    arm_trigger = true;
            }
        }
        else {
            int i = 0;
            int je = src->v_bits.Bend(ms, ls);
            for ( ; i <= md && j <= je; i++, j++) {
                if (set_bit_of(i, s[j]))
                    arm_trigger = true;
            }
            for ( ; i <= md; i++)
                if (set_bit_of(i, BitL))
                    arm_trigger = true;
        }
        if (src->v_data_type == Dstring)
            delete [] s;
    }
    else {
        for (int i = ld; i <= md; i++) {
            if (set_bit_of(i, BitL))
                arm_trigger = true;
        }
    }
    if (arm_trigger && v_events)
        trigger();
}


// Assign integer, scalar from array.
//
void
vl_var::assign_int_SA(int md, int ld, vl_var *src, int ms, int ls)
{
    bool arm_trigger = false;
    if (src->v_array.check_range(&ms, &ls)) {
        int i, j, bw;
        char *s = src->bit_elt(src->v_array.Astart(ms, ls), &bw);
        if (is_indeterminate(s, 0, min(md - ld, bw-1))) {
            for (i = ld; i <= md; i++) {
                if (set_bit_of(i, BitL))
                    arm_trigger = true;
            }
        }
        else {
            for (i = ld, j = 0; i <= md && j < bw; i++, j++) {
                if (set_bit_of(i, s[j]))
                    arm_trigger = true;
            }
            for ( ; i <= md; i++) {
                if (set_bit_of(i, 0))
                    arm_trigger = true;
            }
        }
        if (src->v_data_type == Dstring)
            delete [] s;
    }
    else {
        for (int i = ld; i <= md; i++) {
            if (set_bit_of(i, BitL))
                arm_trigger = true;
        }
    }
    if (arm_trigger && v_events)
        trigger();
}


// Assign integer, array from scalar.
//
void
vl_var::assign_int_AS(int md, int ld, vl_var *src, int ms, int ls)
{
    bool arm_trigger = false;
    if (src->check_bit_range(&ms, &ls)) {
        int bw;
        char *s = src->bit_elt(0, &bw);
        int j = src->v_bits.Bstart(ms, ls);
        int d = bits2int(s + j, rsize(ms, ls));
        if (set_int_elt(v_array.Astart(md, ld), d))
            arm_trigger = true;
        if (src->v_data_type == Dstring)
            delete [] s;
    }
    else {
        if (set_int_elt(v_array.Astart(md, ld), 0))
            arm_trigger = true;
    }
    if (arm_trigger && v_events)
        trigger();
}


// Assign integer, array from array.
//
void
vl_var::assign_int_AA(int md, int ld, vl_var *src, int ms, int ls)
{
    bool arm_trigger = false;
    int i = v_array.Astart(md, ld);
    int ie = v_array.Aend(md, ld);
    if (src->v_array.check_range(&ms, &ls)) {
        int j = src->v_array.Astart(ms, ls);
        int je = src->v_array.Aend(ms, ls);
        for ( ; i <= ie && j <= je; i++, j++) {
            if (set_int_elt(i, src->int_elt(j)))
                arm_trigger = true;
        }
        for ( ; i <= ie; i++) {
            if (set_int_elt(i, 0))
                arm_trigger = true;
        }
    }
    else {
        for ( ; i <= ie; i++) {
            if (set_int_elt(i, 0))
                arm_trigger = true;
        }
    }
    if (arm_trigger && v_events)
        trigger();
}


void
vl_var::assign_time_range(int md, int ld, vl_var *src, int ms, int ls)
{
    if (v_data_type != Dtime)
        return;
    if (v_array.size() == 0) {
        if (!check_bit_range(&md, &ld))
            return;
        if (src->v_array.size() == 0)
            assign_time_SS(md, ld, src, ms, ls);
        else
            assign_time_SA(md, ld, src, ms, ls);
        if (VS()->dbg_flags() & DBG_assign) {
            cout << this << " = ";
            print_value(cout);
            cout << '\n';
        }
    }
    else {
        if (!v_array.check_range(&md, &ld))
            return;
        if (src->v_array.size() == 0)
            assign_time_AS(md, ld, src, ms, ls);
        else
            assign_time_AA(md, ld, src, ms, ls);
    }
}


// Assign time range, scalar from scalar.
//
void
vl_var::assign_time_SS(int md, int ld, vl_var *src, int ms, int ls)
{
    bool arm_trigger = false;
    if (src->check_bit_range(&ms, &ls)) {
        int bw;
        char *s = src->bit_elt(0, &bw);
        int j = src->v_bits.Bstart(ms, ls);
        int sr = rsize(ms, ls);
        int w = min(sr, rsize(md, ld)) - 1;
        if (is_indeterminate(s, j, j + w)) {
            for (int i = ld; i <= md; i++) {
                if (set_bit_of(i, BitL))
                    arm_trigger = true;
            }
        }
        else {
            int i = 0;
            int je = src->v_bits.Bend(ms, ls);
            for ( ; i <= md && j <= je; i++, j++) {
                if (set_bit_of(i, s[j]))
                    arm_trigger = true;
            }
            for ( ; i <= md; i++)
                if (set_bit_of(i, BitL))
                    arm_trigger = true;
        }
        if (src->v_data_type == Dstring)
            delete [] s;
    }
    else {
        for (int i = ld; i <= md; i++) {
            if (set_bit_of(i, BitL))
                arm_trigger = true;
        }
    }
    if (arm_trigger && v_events)
        trigger();
}


// Assign time range, scalar from array.
//
void
vl_var::assign_time_SA(int md, int ld, vl_var *src, int ms, int ls)
{
    bool arm_trigger = false;
    if (src->v_array.check_range(&ms, &ls)) {
        int i, j, bw;
        char *s = src->bit_elt(src->v_array.Astart(ms, ls), &bw);
        if (is_indeterminate(s, 0, min(md - ld, bw-1))) {
            for (i = ld; i <= md; i++) {
                if (set_bit_of(i, BitL))
                    arm_trigger = true;
            }
        }
        else {
            for (i = ld, j = 0; i <= md && j < bw; i++, j++) {
                if (set_bit_of(i, s[j]))
                    arm_trigger = true;
            }
            for ( ; i <= md; i++) {
                if (set_bit_of(i, 0))
                    arm_trigger = true;
            }
        }
        if (src->v_data_type == Dstring)
            delete [] s;
    }
    else {
        for (int i = ld; i <= md; i++) {
            if (set_bit_of(i, BitL))
                arm_trigger = true;
        }
    }
    if (arm_trigger && v_events)
        trigger();
}


// Assign time range, array from scalar.
//
void
vl_var::assign_time_AS(int md, int ld, vl_var *src, int ms, int ls)
{
    bool arm_trigger = false;
    if (src->check_bit_range(&ms, &ls)) {
        int bw;
        char *s = src->bit_elt(0, &bw);
        int j = src->v_bits.Bstart(ms, ls);
        vl_time_t d = bits2time(s + j, rsize(ms, ls));
        if (set_time_elt(v_array.Astart(md, ld), d))
            arm_trigger = true;
        if (src->v_data_type == Dstring)
            delete [] s;
    }
    else {
        if (set_int_elt(v_array.Astart(md, ld), 0))
            arm_trigger = true;
    }
    if (arm_trigger && v_events)
        trigger();
}


// Assign time range, array from array.
//
void
vl_var::assign_time_AA(int md, int ld, vl_var *src, int ms, int ls)
{
    bool arm_trigger = false;
    int i = v_array.Astart(md, ld);
    int ie = v_array.Aend(md, ld);
    if (src->v_array.check_range(&ms, &ls)) {
        int j = src->v_array.Astart(ms, ls);
        int je = src->v_array.Aend(ms, ls);
        for ( ; i <= ie && j <= je; i++, j++) {
            if (set_time_elt(i, src->time_elt(j)))
                arm_trigger = true;
        }
        for ( ; i <= ie; i++) {
            if (set_time_elt(i, 0))
                arm_trigger = true;
        }
    }
    else {
        for ( ; i <= ie; i++) {
            if (set_time_elt(i, 0))
                arm_trigger = true;
        }
    }
    if (arm_trigger && v_events)
        trigger();
}


// Assign to real.
//
void
vl_var::assign_real_range(int md, int ld, vl_var *src, int ms, int ls)
{
    if (v_data_type != Dreal)
        return;
    if (v_array.size() == 0) {
        if (!check_bit_range(&md, &ld))
            return;
        if (src->v_array.size() == 0)
            assign_real_SS(md, ld, src, ms, ls);
        else
            assign_real_SA(md, ld, src, ms, ls);
        if (VS()->dbg_flags() & DBG_assign) {
            cout << this << " = ";
            print_value(cout);
            cout << '\n';
        }
    }
    else {
        if (!v_array.check_range(&md, &ld))
            return;
        if (src->v_array.size() == 0)
            assign_real_AS(md, ld, src, ms, ls);
        else
            assign_real_AA(md, ld, src, ms, ls);
    }
}


// Assign to real, scalar from scalar.
//
void
vl_var::assign_real_SS(int md, int ld, vl_var *src, int ms, int ls)
{
    (void)md;
    (void)ld;
    bool arm_trigger = false;
    // The lhs range [md:ld] is ignored here
    if (src->v_bits.check_range(&ms, &ls)) {
        if (set_real_elt(0, src->real_bit_sel(ms, ls)))
            arm_trigger = true;
    }
    else {
        if (real_elt(0) != 0.0)
            arm_trigger = true;
        set_real_elt(0, 0.0);
    }
    if (arm_trigger && v_events)
        trigger();
}


// Assign to real, scalar from array.
//
void
vl_var::assign_real_SA(int md, int ld, vl_var *src, int ms, int ls)
{
    (void)md;
    (void)ld;
    bool arm_trigger = false;
    // The lhs range [md:ld] is ignored here
    if (src->v_array.check_range(&ms, &ls)) {
        if (set_real_elt(0, src->real_elt(src->v_array.Astart(ms, ls))))
            arm_trigger = true;
    }
    else {
        if (real_elt(0) != 0.0)
            arm_trigger = true;
        set_real_elt(0, 0.0);
    }
    if (arm_trigger && v_events)
        trigger();
}


// Assign to real, array from scalar.
//
void
vl_var::assign_real_AS(int md, int ld, vl_var *src, int ms, int ls)
{
    bool arm_trigger = false;
    int ix = v_array.Astart(md, ld);
    if (src->v_bits.check_range(&ms, &ls)) {
        if (set_real_elt(ix, src->real_bit_sel(ms, ls)))
            arm_trigger = true;
    }
    else {
        if (real_elt(ix) != 0.0)
            arm_trigger = true;
        set_real_elt(ix, 0.0);
    }
    if (arm_trigger && v_events)
        trigger();
}


// Assign to real, array from array.
//
void
vl_var::assign_real_AA(int md, int ld, vl_var *src, int ms, int ls)
{
    bool arm_trigger = false;
    int i = v_array.Astart(md, ld);
    int ie = v_array.Aend(md, ld);
    if (src->v_array.check_range(&ms, &ls)) {
        int j = src->v_array.Astart(ms, ls);
        int je = src->v_array.Aend(ms, ls);
        for ( ; i <= ie && j <= je; i++, j++) {
            if (set_real_elt(i, src->real_elt(j)))
                arm_trigger = true;
        }
        for ( ; i <= ie; i++) {
            if (set_real_elt(i, 0))
                arm_trigger = true;
        }
    }
    else {
        for ( ; i <= ie; i++) {
            if (set_real_elt(i, 0))
                arm_trigger = true;
        }
    }
    if (arm_trigger && v_events)
        trigger();
}


// Assign to string.
//
void
vl_var::assign_string_range(int md, int ld, vl_var *src, int ms, int ls)
{
    if (v_data_type != Dstring)
        return;
    if (v_array.size() == 0) {
        if (!check_bit_range(&md, &ld))
            return;
        if (src->v_array.size() == 0)
            assign_string_SS(md, ld, src, ms, ls);
        else
            assign_string_SA(md, ld, src, ms, ls);
    }
    else {
        if (!v_array.check_range(&md, &ld))
            return;
        if (src->v_array.size() == 0)
            assign_string_AS(md, ld, src, ms, ls);
        else
            assign_string_AA(md, ld, src, ms, ls);
    }
}


// Assign to string, scalar from scalar.
//
void
vl_var::assign_string_SS(int md, int ld, vl_var *src, int ms, int ls)
{
    (void)md;
    (void)ld;
    (void)ms;
    (void)ls;
    set_str_elt(0, vl_strdup(src->str_elt(0)));
}


// Assign to string, scalar from array.
//
void
vl_var::assign_string_SA(int md, int ld, vl_var *src, int ms, int ls)
{
    (void)md;
    (void)ld;
    if (src->v_array.check_range(&ms, &ls))
        set_str_elt(0, vl_strdup(src->str_elt(src->v_array.Astart(ms, ls))));
    else
        set_str_elt(0, 0);
}


// Assign to string, array from scalar.
//
void
vl_var::assign_string_AS(int md, int ld, vl_var *src, int ms, int ls)
{
    (void)ms;
    (void)ls;
    set_str_elt(v_array.Astart(md, ld), vl_strdup(src->str_elt(0)));
}


// Assign to string, array from array.
//
void
vl_var::assign_string_AA(int md, int ld, vl_var *src, int ms, int ls)
{
    int i = v_array.Astart(md, ld);
    int ie = v_array.Aend(md, ld);
    if (src->v_array.check_range(&ms, &ls)) {
        int j = src->v_array.Astart(ms, ls);
        int je = src->v_array.Aend(ms, ls);
        for ( ; i <= ie && j <= je; i++, j++)
            set_str_elt(i, vl_strdup(src->str_elt(j)));
        for ( ; i <= ie; i++)
            set_str_elt(i, 0);
    }
    else {
        for ( ; i <= ie; i++)
            set_str_elt(i, 0);
    }
}
// End of vl_var functions.


// Initialize values from range specifier.
//
void
vl_array::set(vl_range *range)
{
    if (!range || !range->left())
        return;
    if (!range->eval(&a_hi_index, &a_lo_index)) {
        vl_error("range initialization failed");
        errout(range);
        VS()->abort();
        a_hi_index = a_lo_index = 0;
    }
    a_size = rsize(a_hi_index, a_lo_index);
}


// Check/adjust range against source vector.  Return false if out of
// range.  The abort flag is set if a direction error is found.
//
bool
vl_array::check_range(int *m, int *l)
{
    const char *err1 = "in check range, index direction mismatch found";
    if (*m == *l) {
        // bit or element select
        if (*m < min(a_hi_index, a_lo_index) ||
                *m > max(a_hi_index, a_lo_index))
            return (false);
        return (true);
    }
    if (a_lo_index <= a_hi_index) {
        if (*m < *l) {
            vl_error(err1);
            VS()->abort();
            return (false);
        }
        if (*l > a_hi_index || *m < a_lo_index)
            return (false);
        if (*m > a_hi_index)
            *m = a_hi_index;
        if (*l < a_lo_index)
            *l = a_lo_index;
    }
    else {
        if (*m > *l) {
            vl_error(err1);
            VS()->abort();
            return (false);
        }
        if (*l < a_hi_index || *m > a_lo_index)
            return (false);
        if (*m < a_hi_index)
            *m = a_hi_index;
        if (*l > a_lo_index)
            *l = a_lo_index;
    }
    return (true);
}
// End of vl_array functions.


vl_range *
vl_range::copy()
{
    if (!r_left)
        return (0);
    return (new vl_range(r_left->copy(), chk_copy(r_right)));
}


// Compute the integer range parameters.  If parameters are valid return
// true.
//
bool
vl_range::eval(int *m, int *l)
{
    *m = 0;
    *l = 0;
    if (!r_left)
        return (true);
    vl_var &dl = r_left->eval();
    if (dl.is_x())
        return (false);
    *m = (int)dl;
    if (r_right) {
        vl_var &dr = r_right->eval();
        if (dr.is_x())
            return (false);
        *l = (int)dr;
    }
    else
        *l = *m;
    return (true);
}


// Create a new range struct by evaluating this.
//
vl_range *
vl_range::reval()
{
    int m, l;
    if (!eval(&m, &l))
        return (0);
    vl_expr *mx = new vl_expr;
    mx->set_etype(IntExpr);
    mx->set_data_type(Dint);
    mx->set_data_i(m);
    vl_expr *lx = 0;
    if (m != l) {
        lx = new vl_expr;
        lx->set_etype(IntExpr);
        lx->set_data_type(Dint);
        lx->set_data_i(l);
    }
    return (new vl_range(mx, lx));
}


// Compute the range width, return 0 if the range is bad.
//
int
vl_range::width()
{
    int m, l;
    if (eval(&m, &l))
        return (abs(m - l) + 1); 
    return (0);
}
// End of vl_range functions.


vl_delay *
vl_delay::copy()
{
    vl_delay *retval;
    if (d_delay1)
        retval = new vl_delay(d_delay1->copy());
    else
        retval = new vl_delay(copy_list(d_list));
    return (retval);
}


vl_time_t
vl_delay::eval()
{
    vl_module *cmod = VS()->context()->currentModule();
    if (!cmod) {
        vl_error("internal, no current module for delay evaluation");
        VS()->abort();
        return (0);
    }
    double tstep = VS()->description()->tstep();
    double tunit = cmod->tunit();
    double tprec = cmod->tprec();
    if (d_list) {
        lsGen<vl_expr*> gen(d_list);
        vl_expr *e;
        if (gen.next(&e)) {
            double td = (double)e->eval();
            vl_time_t t = (vl_time_t)(td*tunit/tprec + 0.5);
            t *= (vl_time_t)(tprec/tstep);
            return (t);
        }
    }
    else if (d_delay1) {
        double td = (double)d_delay1->eval();
        vl_time_t t = (vl_time_t)(td*tunit/tprec + 0.5);
        t *= (vl_time_t)(tprec/tstep);
        return (t);
    }
    return (0);
}
// End of vl_delay functions.


vl_event_expr *
vl_event_expr::copy()
{
    vl_event_expr *retval = new vl_event_expr(e_type, 0);
    retval->e_expr = chk_copy(e_expr);
    retval->e_list = copy_list(e_list);
    return (retval);
}


void
vl_event_expr::init()
{
    if (e_expr)
        e_expr->eval();
    else if (e_list) {
        lsGen<vl_event_expr*> gen(e_list);
        vl_event_expr *e;
        while (gen.next(&e))
            e->init();
    }
}


bool
vl_event_expr::eval(vl_simulator *sim)
{
    if (e_type == OrEventExpr) {
        if (e_list) {
            vl_event_expr *e;
            lsGen<vl_event_expr*> gen(e_list);
            while (gen.next(&e)) {
                if (e->eval(sim))
                    return (true);
            }
        }
    }
    else if (e_type == EdgeEventExpr) {
        vl_var last = *e_expr;
        vl_var d = e_expr->eval();
        if (d.net_type() == REGevent)
            // named event sent
            return (true);
        if (last.data_type() == Dnone)
            return (false);
        vl_var &z = case_eq(last, d);
        if (z.data_s()[0] == BitL)
            return (true);
    }
    else if (e_type == PosedgeEventExpr) {
        vl_var last = *e_expr;
        if (last.data_type() == Dnone)
            return (false);
        vl_var &d = e_expr->eval();
        if (last.data_type() != Dbit || d.data_type() != Dbit) {
            vl_error("non-bitfield found as posedge event trigger");
            sim->abort();
            return (false);
        }
        if ((last.data_s()[0] == BitL && d.data_s()[0] != BitL) ||
                (last.data_s()[0] != BitH && d.data_s()[0] == BitH))
            return (true);
    }
    else if (e_type == NegedgeEventExpr) {
        vl_var last = *e_expr;
        if (last.data_type() == Dnone)
            return (false);
        vl_var &d = e_expr->eval();
        if (last.data_type() != Dbit || d.data_type() != Dbit) {
            vl_error("non-bitfield found as negedge event trigger");
            sim->abort();
            return (false);
        }
        if ((last.data_s()[0] == BitH && d.data_s()[0] != BitH) ||
                (last.data_s()[0] != BitL && d.data_s()[0] == BitL))
            return (true);
    }
    else if (e_type == LevelEventExpr) {
        if ((int)e_expr->eval())
            return (true);
    }
    return (false);
}


// Chain the action to the expression(s).
//
void
vl_event_expr::chain(vl_action_item *a)
{
    if (e_expr)
        e_expr->chain(a);
    else if (e_list) {
        lsGen<vl_event_expr*> gen(e_list);
        vl_event_expr *e;
        while (gen.next(&e)) {
            if (e->e_expr)
                e->e_expr->chain(a);
        }
    }
}


// Remove actions equivalent to the passed action from the
// expression(s).
//
void
vl_event_expr::unchain(vl_action_item *a)
{
    if (e_expr)
        e_expr->unchain(a);
    else if (e_list) {
        lsGen<vl_event_expr*> gen(e_list);
        vl_event_expr *e;
        while (gen.next(&e)) {
            if (e->e_expr)
                e->e_expr->unchain(a);
        }
    }
}


// Remove the chained actions that have blk in the context hierarchy.
//
void
vl_event_expr::unchain_disabled(vl_stmt *blk)
{
    if (e_expr)
        e_expr->unchain_disabled(blk);
    else if (e_list) {
        lsGen<vl_event_expr*> gen(e_list);
        vl_event_expr *e;
        while (gen.next(&e)) {
            if (e->e_expr)
                e->e_expr->unchain_disabled(blk);
        }
    }
}
// End of vl_event_expr functions.

