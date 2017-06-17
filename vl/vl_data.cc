
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 1996 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL STEPHEN R. WHITELEY BE LIABLE       *
 *   FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION      *
 *   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN           *
 *   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN         *
 *   THE SOFTWARE.                                                        *
 *                                                                        *
 * This software is available for non-commercial use under the terms of   *
 * the GNU General Public License as published by the Free Software       *
 * Foundation; either version 2 of the License, or (at your option) any   *
 * later version.                                                         *
 *                                                                        *
 * A commercial license is available to those wishing to use this         *
 * library or a derivative work for commercial purposes.  Contact         *
 * Whiteley Research Inc., 456 Flora Vista Ave., Sunnyvale, CA 94086.     *
 * This provision will be enforced to the fullest extent possible under   *
 * law.                                                                   *
 *                                                                        *
 * You should have received a copy of the GNU Library General Public      *
 * License along with this library; if not, write to the Free             *
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.     *
 *========================================================================*
 *                                                                        *
 * Verilog Support Files                                                  *
 *                                                                        *
 *========================================================================*
 $Id: vl_data.cc,v 1.7 2008/09/22 02:47:43 stevew Exp $
 *========================================================================*/

#include "vl_list.h"
#include "vl_st.h"
#include "vl_defs.h"
#include "vl_types.h"

// set this to machine bit width
#define DefBits (8*(int)sizeof(int))

vl_simulator *vl_var::simulator;

//---------------------------------------------------------------------------
//  Local
//---------------------------------------------------------------------------

inline int max(int x, int y) { return (x > y ? x : y); }
inline int min(int x, int y) { return (x < y ? x : y); }
inline int rsize(int m, int l) { return (abs(m - l) + 1); }

// Return status of i'th bit of integer
//
inline int
bit(int i, int pos)
{
    if ((i >> pos) & 1)  
        return (BitH);
    return (BitL);
}


// Return status of i'th bit of vl_time_t
//
inline int
bit(vl_time_t t, int pos)
{
    if ((t >> pos) & 1)  
        return (BitH);
    return (BitL);
}


inline int
rnd(double x)
{
    return ((int)(x >= 0.0 ? x + 0.5 : x - 0.5));
}


inline vl_time_t
trnd(double x)
{
    return ((vl_time_t)(x >= 0.0 ? x + 0.5 : x - 0.5));
}


static bool
is_indeterminate(char *s, int l, int m)
{
    for (int i = l; i <= m; i++) {
        if (s[i] != BitL && s[i] != BitH)
            return (true);
    }
    return (false);
}


// Return an int constructed from bit field
//
inline int
bits2int(char *s, int wid)
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


inline int
bits2time(char *s, int wid)
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


static double
bits2real(char *s, int wid)
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


// Return the effective bit width
//
static int
bit_size(vl_var *v)
{
    if (v->data_type == Dbit)
        return (v->bits.size);
    if (v->data_type == Dint)
        return (DefBits);
    if (v->data_type == Dtime)
        return (sizeof(vl_time_t)*8);
    return (0);
}


// Compute the actual number of bits set in the assignment, know that
// l_from is in range
//
static int
bits_set(vl_var *dst, vl_range *r, vl_var *src, int l_from)
{
    int m, l;
    if (r) {
        if (!r->eval(&m, &l))
            return (0);
        if (!dst->bits.check_range(&m, &l))
            return (0);
    }
    else {
        m = dst->bits.hi_index;
        l = dst->bits.lo_index;
    }
    int w = rsize(m, l);
    int w1 = bit_size(src) - dst->bits.Bstart(l_from, l_from);
    return (min(w, w1));
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
    name = 0;
    data_type = Dnone;
    net_type = REGnone;
    io_type = IOnone;
    flags = 0;
    u.r = 0;
    range = 0;
    events = 0;
    delay = 0;
    cassign = 0;
    drivers = 0;
}


vl_var::vl_var(const char *n, vl_range *r, lsList<vl_expr*> *c)
{
    name = n;
    data_type = c ? Dconcat : Dnone;
    net_type = REGnone;
    io_type = IOnone;
    flags = 0;
    u.r = 0;
    u.c = c;
    range = r;
    events = 0;
    delay = 0;
    cassign = 0;
    drivers = 0;
}


// Copy constructor.  This makes a duplicate, used when copying
// modules for instantiation.
//
vl_var::vl_var(vl_var &d)
{
    name = vl_strdup(d.name);
    data_type = d.data_type;
    net_type = d.net_type;
    io_type = d.io_type;
    flags = 0;
    array = d.array;
    bits = d.bits;
    range = d.range->copy();
    events = 0;
    strength = d.strength;
    delay = d.delay->copy();
    cassign = 0;
    drivers = 0;

    if (data_type == Dint) {
        if (array.size) {
            u.d = new int[array.size];
            memcpy(u.d, d.u.d, array.size*sizeof(int));
        }
        else
            u.i = d.u.i;
    }
    else if (data_type == Dreal) {
        if (array.size) {
            u.d = new double[array.size];
            memcpy(u.d, d.u.d, array.size*sizeof(double));
        }
        else
            u.r = d.u.r;
    }
    else if (data_type == Dtime) {
        if (array.size) {
            u.d = new vl_time_t[array.size];
            memcpy(u.d, d.u.d, array.size*sizeof(vl_time_t));
        }
        else
            u.t = d.u.t;
    }
    else if (data_type == Dstring) {
        if (array.size) {
            char **s = new char*[array.size];
            u.d = s;
            char **ss = (char**)d.u.d;
            for (int i = 0; i < array.size; i++)
                s[i] = vl_strdup(ss[i]);
        }
        else
            u.s = vl_strdup(d.u.s);
    }
    else if (data_type == Dbit) {
        if (array.size) {
            char **s = new char*[array.size];
            u.d = s;
            char **ss = (char**)d.u.d;
            for (int i = 0; i < array.size; i++) {
                s[i] = new char[bits.size];
                memcpy(s[i], ss[i], bits.size);
            }
        }
        else {
            u.s = new char[bits.size];
            memcpy(u.s, d.u.s, bits.size);
        }
    }
    else if (data_type == Dconcat) {
        u.c = new lsList<vl_expr*>;
        vl_expr *e;
        lsGen<vl_expr*> gen(d.u.c);
        while (gen.next(&e))
            u.c->newEnd(e->copy());
    }
}


vl_var::~vl_var()
{
    delete [] name;
    if (data_type == Dbit || data_type == Dstring) {
        if (array.size) {
            char **s = (char**)u.d;
            for (int i = 0; i < array.size; i++)
                delete [] s[i];
            delete [] s;
        }
        else
            delete [] u.s;
    }
    else if (data_type == Dint) {
        if (array.size)
            delete [] (int*)u.d;
    }
    else if (data_type == Dtime) {
        if (array.size)
            delete [] (vl_time_t*)u.d;
    }
    else if (data_type == Dreal) {
        if (array.size)
            delete [] (double*)u.d;
    }
    else if (data_type == Dconcat)
        delete_list(u.c);
    if (drivers) {
        lsGen<vl_driver*> gen(drivers);
        vl_driver *d;
        while (gen.next(&d))
            delete d;
        delete drivers;
    }
    events->free();
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


static bool
same(vl_stack *s1, vl_stack *s2)
{
    if (!s1 && !s2)
        return (true);
    if (!s1 || !s2)
        return (false);
    if (s1->num != s2->num)
        return (false);
    for (int i = 0; i < s1->num; i++)
        if (s1->acts[i].actions != s2->acts[i].actions)
            return (false);
    return (true);
}


// Set up an asynchronous action to perform when data changes,
// for events if the stmt is a vl_action_item, or for continuous
// assign and formal/actual association otherwise
//
void
vl_var::chain(vl_stmt *stmt)
{
    if (data_type == Dconcat) {
        lsGen<vl_expr*> gen(u.c, true);
        vl_expr *e;
        while (gen.prev(&e))
            e->chain(stmt);
        return;
    }
    if (stmt->type == ActionItem) {
        vl_action_item *a = (vl_action_item*)stmt;
        for (vl_action_item *aa = events; aa; aa = aa->next) {
            if (aa->event == a->event)
                return;
        }
        if (a->event)
            a->event->init();
        vl_action_item *an = new vl_action_item(a->stmt, simulator->context);
        an->stack = a->stack->copy();
        an->event = a->event;
        an->flags = a->flags;
        an->next = events;
        events = an;
    }
    else {
        vl_action_item *a;
        for (a = events; a; a = a->next) {
            if (a->stmt == stmt)
                return;
        }
        a = new vl_action_item(stmt, simulator->context);
        a->next = events;
        events = a;
    }
}


void
vl_var::unchain(vl_stmt *stmt)
{
    if (data_type == Dconcat) {
        lsGen<vl_expr*> gen(u.c, true);
        vl_expr *e;
        while (gen.prev(&e))
            e->unchain(stmt);
        return;
    }
    if (stmt->type == ActionItem) {
        vl_action_item *a = (vl_action_item*)stmt;
        vl_action_item *ap = 0, *an;
        for (vl_action_item *aa = events; aa; aa = an) {
            an = aa->next;
            if (aa->stmt == a->stmt && same(aa->stack, a->stack)) {
                if (ap)
                    ap->next = an;
                else
                    events = an;
                if (aa != a)
                    delete aa;
                return;
            }
            ap = aa;
        }
    }
    else {
        vl_action_item *ap = 0, *an;
        for (vl_action_item *a = events; a; a = an) {
            an = a->next;
            if (a->stmt == stmt) {
                if (ap)
                    ap->next = an;
                else
                    events = an;
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
    if (data_type == Dconcat) {
        lsGen<vl_expr*> gen(u.c, true);
        vl_expr *e;
        while (gen.prev(&e))
            e->unchain_disabled(stmt);
        return;
    }
    events = events->purge(stmt);
}


// Set the size and data_type of the data.  This is called from the
// setup function of declarations
//
void
vl_var::configure(vl_range *rng, int t, vl_range *ary)
{
    if (data_type != Dnone || net_type != REGnone) {
        // already configured, but allow situations like
        // output r;
        // reg [7:0] r;
        if (rng && data_type == Dbit && bits.size == 1 && !ary && !array.size)
            delete [] u.s;
        else
            return;
    }

    if (t == RealDecl) {
        data_type = Dreal;
        // Verilog doesn't support real arrays, allow them here anyway
        array.set(ary);
        if (array.size) {
            u.d = new double[array.size];
            memset(u.d, 0, array.size*sizeof(double));
        }
    }
    else if (t == IntDecl) {
        data_type = Dint;
        array.set(ary);
        if (array.size) {
            u.d = new int[array.size];
            memset(u.d, 0, array.size*sizeof(int));
        }
    }
    else if (t == TimeDecl) {
        data_type = Dtime;
        array.set(ary);
        if (array.size) {
            u.d = new vl_time_t[array.size];
            memset(u.d, 0, array.size*sizeof(vl_time_t));
        }
    }
    else if (t == ParamDecl) {
        if (rng) {
            data_type = Dbit;
            bits.set(rng);
            if (!bits.size)
                bits.size = 1;
            u.s = new char[bits.size];
            memset(u.s, BitL, bits.size);
        }
        else
            data_type = Dint;
    }
    else {
        data_type = Dbit;
        bits.set(rng);
        if (!bits.size)
            bits.size = 1;
        array.set(ary);
        if (array.size) {
            char **s = new char*[array.size];
            u.d = s;
            for (int i = 0; i < array.size; i++) {
                s[i] = new char[bits.size];
                memset(s[i], BitDC, bits.size);
            }
        }
        else {
            u.s = new char[bits.size];
            memset(u.s, BitDC, bits.size);
        }
    }
}

static void
print_strength(vl_strength s)
{
    cout << '(';
    switch (s.str0) {
    case STRnone:
        cout << "s0";
        break;
    case STRhiZ:
        cout << "z0";
        break;
    case STRsmall:
        cout << "s";
        break;
    case STRmed:
        cout << "m";
        break;
    case STRweak:
        cout << "w0";
        break;
    case STRlarge:
        cout << "l";
        break;
    case STRpull:
        cout << "p0";
        break;
    case STRstrong:
        cout << "s0";
        break;
    case STRsupply:
        cout << "sp0";
        break;
    }
    switch (s.str1) {
    case STRnone:
        cout << ",s1";
        break;
    case STRhiZ:
        cout << ",z1";
        break;
    case STRsmall:
        cout << "s";
        break;
    case STRmed:
        cout << "m";
        break;
    case STRweak:
        cout << ",w1";
        break;
    case STRlarge:
        cout << "l";
        break;
    case STRpull:
        cout << ",p1";
        break;
    case STRstrong:
        cout << ",s1";
        break;
    case STRsupply:
        cout << ",sp1";
        break;
    }
    cout << ')';
}


static void
print_drivers(vl_var *d)
{
    if (!d->drivers)
        return;
    lsGen<vl_driver*> gen(d->drivers);
    vl_driver *dr;
    cout << "Drivers: ";
    while (gen.next(&dr)) {
        if (dr->srcvar->flags & VAR_PORT_DRIVER)
            cout << "(p)";
        cout << dr->srcvar;
        if (dr->srcvar->net_type >= REGwire)
            print_strength(dr->srcvar->strength);
        else
            print_strength(d->strength);
        cout << '[' << dr->m_to << '|' << dr->l_to << '|' << dr->l_from << "] ";
        dr->srcvar->print_value(cout);
        cout << ' ';
    }
    cout << "\n";
}


static void
probe1(vl_var *vo, int md, int ld, vl_var *vin, int ms, int ls)
{
    cout << vo << '[' << md << '|' << ld << ']';
    cout << " assigning: " << vin << '[' << ms << '|' << ls << "] ";
    vin->print_value(cout);
    cout << '\n';
}


static void
probe2(vl_var *vo)
{
    cout << "New value: ";
    vo->print_value(cout);
    cout << '\n';
    print_drivers(vo);
}


// General assignment.  If the type of the recipient is already specified,
// coerce to that type.  Otherwise copy directly.
//
void
vl_var::operator=(vl_var &d)
{
    if (net_type == REGreg && (flags & VAR_CP_ASSIGN))
        return;
    if (flags & VAR_F_ASSIGN)
        return;
    bool arm_trigger = false;
    if (data_type == Dnone) {
        data_type = d.data_type;
        array = d.array;
        bits = d.bits;
        arm_trigger = true;
        if (data_type == Dint) {
            if (array.size) {
                u.d = new int[array.size];
                memcpy(u.d, d.u.d, array.size*sizeof(int));
            }
            else
                u.i = d.u.i;
        }
        else if (data_type == Dreal) {
            if (array.size) {
                u.d = new double[array.size];
                memcpy(u.d, d.u.d, array.size*sizeof(double));
            }
            else
                u.r = d.u.r;
        }
        else if (data_type == Dtime) {
            if (array.size) {
                u.d = new vl_time_t[array.size];
                memcpy(u.d, d.u.d, array.size*sizeof(vl_time_t));
            }
            else
                u.t = d.u.t;
        }
        else if (data_type == Dstring) {
            if (array.size) {
                char **s = new char*[array.size];
                u.d = s;
                char **ss = (char**)d.u.d;
                for (int i = 0; i < array.size; i++)
                    s[i] = vl_strdup(ss[i]);
            }
            else
                u.s = vl_strdup(d.u.s);
        }
        else if (data_type == Dbit) {
            if (array.size) {
                char **s = new char*[array.size];
                u.d = s;
                char **ss = (char**)d.u.d;
                for (int i = 0; i < array.size; i++) {
                    s[i] = new char[bits.size];
                    memcpy(s[i], ss[i], bits.size);
                }
            }
            else {
                u.d = new char[bits.size];
                memcpy(u.d, d.u.d, bits.size);
            }
        }
        else if (data_type == Dconcat) {
            u.c = new lsList<vl_expr*>;
            vl_expr *e;
            lsGen<vl_expr*> gen(d.u.c);
            while (gen.next(&e))
                u.c->newEnd(e->copy());
        }
    }
    else if (data_type == Dbit) {
        if (net_type >= REGwire && d.data_type == Dbit) {
            if (array.size != 0) {
                vl_error("in assignment, assigning object is an array");
                simulator->abort();
                return;
            }
            add_driver(&d, bits.size - 1, 0, 0);
            if (simulator->dbg_flags & DBG_assign)
                probe1(this, bits.size - 1, 0, &d, d.bits.size - 1, 0);
            int i = 0;
            for (int j = 0; i < bits.size && j < d.bits.size;
                    i++, j++) {
                int b = resolve_bit(i, &d, 0);
                if (u.s[i] != b) {
                    arm_trigger = true;
                    u.s[i] = b;
                }
            }
            for ( ; i < bits.size; i++) {
                if (u.s[i] != BitL) {
                    arm_trigger = true;
                    u.s[i] = BitL;
                }
            }

            if (simulator->dbg_flags & DBG_assign)
                probe2(this);
            if (arm_trigger && events)
                trigger();
            return;
        }
        if (array.size == 0) {
            if (net_type == REGsupply0 || net_type == REGsupply1)
                return;
            int wd;
            char *s = d.bit_elt(0, &wd);
            int mw = min(bits.size, wd);
            int i;
            for (i = 0; i < mw; i++) {
                if (u.s[i] != s[i]) {
                    arm_trigger = true;
                    u.s[i] = s[i];
                }
            }
            for ( ; i < bits.size; i++) {
                if (u.s[i] != BitL) {
                    arm_trigger = true;
                    u.s[i] = BitL;
                }
            }
            if (simulator->dbg_flags & DBG_assign) {
                cout << this << " = ";
                print_value(cout);
                cout << '\n';
            }
        }
        else {
            int ms = min(array.size, d.array.size);
            int wd;
            for (int j = 0; j < ms; j++) {
                char *s = d.bit_elt(j, &wd);
                char *b = ((char**)u.d)[j];
                int mw = min(bits.size, wd);
                int i;
                for (i = 0; i < mw; i++) {
                    if (b[i] != s[i]) {
                        arm_trigger = true;
                        b[i] = s[i];
                    }
                }
                for ( ; i < bits.size; i++) {
                    if (b[i] != BitL) {
                        arm_trigger = true;
                        b[i] = BitL;
                    }
                }
            }
        }
    }
    else if (data_type == Dint) {
        if (array.size == 0) {
            int b = d.int_elt(0);
            if (u.i != b) {
                arm_trigger = true;
                u.i = b;
            }
            if (simulator->dbg_flags & DBG_assign) {
                cout << this << " = ";
                print_value(cout);
                cout << '\n';
            }
        }
        else {
            int ms = min(array.size, d.array.size);
            int *dd = (int*)u.d;
            int i;
            for (i = 0; i < ms; i++) {
                int b = d.int_elt(i);
                if (dd[i] != b) {
                    arm_trigger = true;
                    dd[i] = b;
                }
            }
            for ( ; i < array.size; i++) {
                if (dd[i] != 0) {
                    arm_trigger = true;
                    dd[i] = 0;
                }
            }
        }
    }
    else if (data_type == Dtime) {
        if (array.size == 0) {
            vl_time_t t = d.time_elt(0);
            if (u.t != t) {
                arm_trigger = true;
                u.t = t;
            }
            if (simulator->dbg_flags & DBG_assign) {
                cout << this << " = ";
                print_value(cout);
                cout << '\n';
            }
        }
        else {
            int ms = min(array.size, d.array.size);
            vl_time_t *tt = (vl_time_t*)u.d;
            int i;
            for (i = 0; i < ms; i++) {
                vl_time_t t = d.time_elt(i);
                if (tt[i] != t) {
                    arm_trigger = true;
                    tt[i] = t;
                }
            }
            for ( ; i < array.size; i++) {
                if (tt[i] != 0) {
                    arm_trigger = true;
                    tt[i] = 0;
                }
            }
        }
    }
    else if (data_type == Dreal) {
        if (array.size == 0) {
            double r = d.real_elt(0);
            if (u.r != r) {
                arm_trigger = true;
                u.r = r;
            }
            if (simulator->dbg_flags & DBG_assign) {
                cout << this << " = ";
                print_value(cout);
                cout << '\n';
            }
        }
        else {
            int ms = min(array.size, d.array.size);
            double *dd = (double*)u.d;
            int i;
            for (i = 0; i < ms; i++) {
                double r = d.real_elt(i);
                if (dd[i] != r) {
                    arm_trigger = true;
                    dd[i] = r;
                }
            }
            for ( ; i < array.size; i++) {
                if (dd[i] != 0.0) {
                    arm_trigger = true;
                    dd[i] = 0.0;
                }
            }
        }
    }
    else if (data_type == Dstring) {
        if (array.size == 0) {
            delete [] u.s;
            u.s = vl_strdup(d.str_elt(0));
        }
        else {
            int ms = min(array.size, d.array.size);
            char **ss = (char**)u.d;
            int i;
            for (i = 0; i < ms; i++) {
                delete [] ss[i];
                ss[i] = vl_strdup(d.str_elt(i));
            }
            for ( ; i < array.size; i++) {
                delete [] ss[i];
                ss[i] = 0;
            }
        }
    }
    else if (data_type == Dconcat) {
        if (d.array.size != 0) {
            vl_error(
                "rhs of concatenation assignment is an array, not allowed");
            errout(this);
            simulator->abort();
            return;
        }
        lsGen<vl_expr*> gen(u.c, true);
        vl_expr *e;
        int bc = 0;
        while (gen.prev(&e)) {
            vl_var *v = e->source();
            if (!v) {
                vl_error("in variable assignment, bad expression type in "
                    "concatenation:");
                errout(this);
                simulator->abort();
                return;
            }
            if (v->data_type != Dbit) {
                vl_error("in variable assignment, concatenation contains "
                    "unsized value:");
                errout(this);
                simulator->abort();
                return;
            }
            vl_range *r = e->source_range();
            if (!v->array.size) {
                if (bc < bit_size(&d)) {
                    int l_from = d.bits.Btou(bc);
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
                    if (!v->array.check_range(&m, &l))
                        continue;
                }
                else {
                    m = v->array.hi_index;
                    l = v->array.lo_index;
                }
                bool atrigger = false;
                int w;
                char *t = d.bit_elt(0, &w);

                int i = v->array.Astart(m, l);
                int ie = v->array.Aend(m, l);
                for ( ; i <= ie; i++) {
                    char *s = ((char**)v->u.d)[i];
                    int cnt = 0;
                    while (bc < d.bits.size && cnt < v->bits.size) {
                        if (s[cnt] != t[bc]) {
                            atrigger = true;
                            s[cnt] = t[bc];
                        }
                        cnt++;
                        bc++;
                    }
                    if (bc == d.bits.size) {
                        for ( ; cnt < v->bits.size; cnt++) {
                            if (s[cnt] != BitL) {
                                atrigger = true;
                                s[cnt] = BitL;
                            }
                        }
                    }
                }
                if (atrigger && v->events) {
                    v->trigger();
                    arm_trigger = true;
                }
            }
        }
    }
    if (arm_trigger && events)
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
    if (net_type == REGreg && (flags & VAR_CP_ASSIGN))
        return;
    if (flags & VAR_F_ASSIGN)
        return;
    if (!rd && !rs) {
        *this = *src;
        return;
    }
    if (data_type == Dconcat) {
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
            if (src->array.size)
                ms = ls = src->array.Atou(src->array.size);
            else if (src->bits.size)
                ms = ls = src->bits.Btou(src->bits.size);
            else
                ms = ls = 1;
        }
    }
    else
        src->default_range(&ms, &ls);

    if (data_type == Dnone) {
        if (!rd)
            assign_init(src, ms, ls);
        else {
            vl_error("range given to uninitialized variable");
            errout(rd);
            simulator->abort();
            return;
        }
    }
    else if (data_type == Dbit)
        assign_bit_range(md, ld, src, ms, ls);
    else if (data_type == Dint)
        assign_int_range(md, ld, src, ms, ls);
    else if (data_type == Dtime)
        assign_time_range(md, ld, src, ms, ls);
    else if (data_type == Dreal)
        assign_real_range(md, ld, src, ms, ls);
    else if (data_type == Dstring)
        assign_string_range(md, ld, src, ms, ls);
    else {
        vl_error("bad data type in range assignment");
        simulator->abort();
    }
}


// A variation that takes integer pointers for the source range values
//
void
vl_var::assign(vl_range *rd, vl_var *src, int *m_from, int *l_from)
{
    const char *msg = "indeterminate range specification in assignment";
    if (net_type == REGreg && (flags & VAR_CP_ASSIGN))
        return;
    if (flags & VAR_F_ASSIGN)
        return;
    if (!rd && !m_from && !l_from) {
        *this = *src;
        return;
    }
    if (data_type == Dconcat) {
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

    if (data_type == Dnone) {
        if (!rd)
            assign_init(src, ms, ls);
        else {
            vl_error("range given to uninitialized variable");
            errout(rd);
            simulator->abort();
            return;
        }
    }
    else if (data_type == Dbit)
        assign_bit_range(md, ld, src, ms, ls);
    else if (data_type == Dint)
        assign_int_range(md, ld, src, ms, ls);
    else if (data_type == Dtime)
        assign_time_range(md, ld, src, ms, ls);
    else if (data_type == Dreal)
        assign_real_range(md, ld, src, ms, ls);
    else if (data_type == Dstring)
        assign_string_range(md, ld, src, ms, ls);
    else {
        vl_error("bad data type in range assignment");
        simulator->abort();
    }
}


// This is for WRspice interface.  Take val, subtract offs, and quantize
// with bitsize quan.  Use the resulting int to set data.  Note, if the
// data type is real, just save val.  If m and l are >= 0, set the
// indicated bits, if a bit field.
//
void
vl_var::assign_to(double val, double offs, double quan, int m, int l)
{
    if (data_type == Dnone) {
        data_type = Dreal;
        u.r = val;
        trigger();
        return;
    }
    if (data_type == Dreal) {
        if (u.r != val) {
            u.r = val;
            trigger();
        }
        return;
    }
    if (data_type == Dint) {
        val -= offs;
        int b;
        if (quan != 0.0)
            b = (int)((val + 0.5*(val > 0.0 ? quan : -quan))/quan);
        else
            b = (int)val;
        if (u.i != b) {
            u.i = b;
            trigger();
        }
        return;
    }
    if (data_type == Dtime) {
        val -= offs;
        vl_time_t t;
        if (quan != 0.0)
            t = (vl_time_t)((val + 0.5*(val > 0.0 ? quan : -quan))/quan);
        else
            t = (vl_time_t)val;
        if (u.t != t) {
            u.t = t;
            trigger();
        }
        return;
    }
    if (data_type == Dstring)
        return;
    if (data_type == Dbit) {
        int ival;
        val -= offs;
        if (quan != 0.0)
            ival = (int)((val + 0.5*(val > 0.0 ? quan : -quan))/quan);
        else
            ival = (int)val;
        bool arm_trigger = false;
        if (m >= 0 && l >= 0) {
            if (bits.check_range(&m, &l)) {
                if (m >= l) {
                    for (int i = l; i <= m; i++) {
                        int b = (ival & (1 << (i-l))) ? BitH : BitL;
                        if (u.s[i] != b) {
                            arm_trigger = true;
                            u.s[i] = b;
                        }
                    }
                }
                else {
                    for (int i = m; i <= l; i++) {
                        int b = (ival & (1 << (i-m))) ? BitH : BitL;
                        if (u.s[i] != b) {
                            arm_trigger = true;
                            u.s[i] = b;
                        }
                    }
                }
                if (arm_trigger && events)
                    trigger();
            }
            return;
        }
        for (int i = 0; i < bits.size; i++) {
            int b = (ival & (1 << i)) ? BitH : BitL;
            if (u.s[i] != b) {
                arm_trigger = true;
                u.s[i] = b;
            }
        }
        if (arm_trigger && events)
            trigger();
    }
}


//
// The following are private
//

void
vl_var::assign_init(vl_var *src, int ms, int ls)
{
    if (data_type != Dnone)
        return;
    if (src->array.size == 0) {
        if (src->check_bit_range(&ms, &ls)) {
            setx(abs(ms - ls) + 1);
            bits.lo_index = ls;
            bits.hi_index = ms;
            bits.Bnorm();
            int j = src->bits.Bstart(ms, ls);
            for (int i = 0; i < bits.size; i++, j++)
                u.s[i] = src->bit_of(j);
        }
        else
            setx(1);
    }
    else {
        if (src->array.check_range(&ms, &ls)) {
            data_type = src->data_type;
            int sr = rsize(ms, ls);
            array.size = sr;
            if (array.size == 1)
                array.size = 0;
            else {
                array.hi_index = ms;
                array.lo_index = ls;
                array.Anorm();
            }
            if (src->data_type == Dbit) {
                bits = src->bits;
                int bw;
                if (array.size == 0) {
                    u.s = new char[bits.size];
                    char *s = src->bit_elt(src->array.Astart(ms, ls), &bw);
                    memcpy(u.s, s, bw);
                    if (src->data_type == Dstring)
                        delete [] s;
                }
                else {
                    char *s, **ss = new char*[array.size];
                    u.d = ss;
                    int j = src->array.Astart(ms, ls);
                    for (int i = 0; i < sr; i++, j++) {
                        ss[i] = new char[bits.size];
                        s = src->bit_elt(j, &bw);
                        memcpy(ss[i], s, bw);
                        if (src->data_type == Dstring)
                            delete [] s;
                    }
                }
            }
            else if (src->data_type == Dint) {
                if (array.size)
                    u.d = new int[array.size];
                int j = src->array.Astart(ms, ls);
                for (int i = 0; i < sr; i++, j++)
                    set_int_elt(i, src->int_elt(j));
            }
            else if (src->data_type == Dtime) {
                if (array.size)
                    u.d = new vl_time_t[array.size];
                int j = src->array.Astart(ms, ls);
                for (int i = 0; i < sr; i++, j++)
                    set_time_elt(i, src->time_elt(j));
            }
            else if (src->data_type == Dreal) {
                if (array.size)
                    u.d = new double[array.size];
                int j = src->array.Astart(ms, ls);
                for (int i = 0; i < sr; i++, j++)
                    set_real_elt(i, src->real_elt(j));
            }
            else if (src->data_type == Dstring) {
                if (array.size)
                    u.d = new char*[array.size];
                int j = src->array.Astart(ms, ls);
                for (int i = 0; i < sr; i++, j++)
                    set_str_elt(i, vl_strdup(src->str_elt(j)));
            }
        }
        else {
            if (src->data_type == Dbit)
                setx(src->bits.size);
            else if (src->data_type == Dint)
                set((int)0);
            else if (src->data_type == Dtime)
                sett(0);
            else if (src->data_type == Dreal) {
                data_type = Dreal;
                u.r = 0.0;
            }
            else if (src->data_type == Dstring) {
                data_type = Dstring;
                u.s = vl_strdup("");
            }
        }
    }
    if (events)
        trigger();
}


void
vl_var::assign_bit_range(int md, int ld, vl_var *src, int ms, int ls)
{
    if (data_type != Dbit)
        return;
    if (net_type >= REGwire) {
        if (array.size != 0) {
            vl_error("in assignment, assigned-to net is an array!");
            simulator->abort();
            return;
        }
        if (src->array.size != 0) {
            vl_error("in assignment, assigning net is an array!");
            simulator->abort();
            return;
        }
        if (src->data_type != Dbit && src->data_type != Dint &&
                src->data_type != Dtime) {
            vl_error("in assignment, assigning net from wrong data type");
            simulator->abort();
            return;
        }
        if (bits.check_range(&md, &ld)) {
            bool arm_trigger = false;
            int i = bits.Bstart(md, ld);
            int ie = bits.Bend(md, ld);
            if (src->check_bit_range(&ms, &ls)) {
                int j = src->bits.Bstart(ms, ls);
                int je = src->bits.Bend(ms, ls);
                add_driver(src, ie, i, j);
                if (simulator->dbg_flags & DBG_assign)
                    probe1(this, ie, i, src, je, j);
                for ( ; i <= ie && j <= je; i++, j++) {
                    int b = resolve_bit(i, src, j);
                    if (u.s[i] != b) {
                        arm_trigger = true;
                        u.s[i] = b;
                    }
                }
                for ( ; i <= ie; i++) {
                    if (u.s[i] != BitL) {
                        arm_trigger = true;
                        u.s[i] = BitL;
                    }
                }
                if (simulator->dbg_flags & DBG_assign)
                    probe2(this);
            }
            else {
                for ( ; i <= ie; i++) {
                    if (u.s[i] != BitDC) {
                        arm_trigger = true;
                        u.s[i] = BitDC;
                    }
                }
            }
            if (arm_trigger && events)
                trigger();
        }
    }
    else if (array.size == 0) {
        if (net_type == REGsupply0 || net_type == REGsupply1)
            return;
        if (!bits.check_range(&md, &ld))
            return;
        if (src->array.size == 0)
            assign_bit_SS(md, ld, src, ms, ls);
        else
            assign_bit_SA(md, ld, src, ms, ls);
        if (simulator->dbg_flags & DBG_assign) {
            cout << this << " = ";
            print_value(cout);
            cout << '\n';
        }
    }
    else {
        if (!array.check_range(&md, &ld))
            return;
        if (src->array.size == 0)
            assign_bit_AS(md, ld, src, ms, ls);
        else
            assign_bit_AA(md, ld, src, ms, ls);
    }
}


// Assign bit field, scalar from scalar
//
void
vl_var::assign_bit_SS(int md, int ld, vl_var *src, int ms, int ls)
{
    bool arm_trigger = false;
    int i = bits.Bstart(md, ld);
    int ie = bits.Bend(md, ld);
    if (src->check_bit_range(&ms, &ls)) {
        int bw;
        char *s = src->bit_elt(0, &bw);
        int j = src->bits.Bstart(ms, ls);
        int je = src->bits.Bend(ms, ls);
        for ( ; i <= ie && j <= je; i++, j++) {
            int b = s[j];
            if (u.s[i] != b) {
                arm_trigger = true;
                u.s[i] = b;
            }
        }
        for ( ; i <= ie; i++) {
            if (u.s[i] != BitL) {
                arm_trigger = true;
                u.s[i] = BitL;
            }
        }
        if (src->data_type == Dstring)
            delete [] s;
    }
    else {
        for ( ; i <= ie; i++) {
            if (u.s[i] != BitDC) {
                arm_trigger = true;
                u.s[i] = BitDC;
            }
        }
    }
    if (arm_trigger && events)
        trigger();
}


// Assign bit field, scalar from array
//
void
vl_var::assign_bit_SA(int md, int ld, vl_var *src, int ms, int ls)
{
    bool arm_trigger = false;
    int i = bits.Bstart(md, ld);
    int ie = bits.Bend(md, ld);
    if (src->array.check_range(&ms, &ls)) {
        int bw;
        char *s = src->bit_elt(src->array.Astart(ms, ls), &bw);
        int j = src->bits.Bstart(ms, ls);
        int je = src->bits.Bend(ms, ls);
        for ( ; i <= ie && j <= je; i++, j++) {
            int b = s[j];
            if (u.s[i] != b) {
                arm_trigger = true;
                u.s[i] = b;
            }
        }
        for ( ; i <= ie; i++) {
            if (u.s[i] != BitL) {
                arm_trigger = true;
                u.s[i] = BitL;
            }
        }
        if (src->data_type == Dstring)
            delete [] s;
    }
    else {
        for ( ; i <= ie; i++) {
            if (u.s[i] != BitDC) {
                arm_trigger = true;
                u.s[i] = BitDC;
            }
        }
    }
    if (arm_trigger && events)
        trigger();
}


// Assign bit field, array from scalar
//
void
vl_var::assign_bit_AS(int md, int ld, vl_var *src, int ms, int ls)
{
    bool arm_trigger = false;
    if (src->check_bit_range(&ms, &ls)) {
        int bw;
        char *s = src->bit_elt(0, &bw);
        if (set_bit_elt(array.Astart(md, ld),
                &s[src->bits.Bstart(ms, ls)], rsize(ms, ls)))
            arm_trigger = true;
        if (src->data_type == Dstring)
            delete [] s;
    }
    else {
        if (set_bit_elt(array.Astart(md, ld), 0, BitDC))
            arm_trigger = true;
    }
    if (arm_trigger && events)
        trigger();
}


// Assign bit field, array from array
//
void
vl_var::assign_bit_AA(int md, int ld, vl_var *src, int ms, int ls)
{
    bool arm_trigger = false;
    int i = array.Astart(md, ld);
    int ie = array.Aend(md, ld);
    if (src->array.check_range(&ms, &ls)) {
        int j = src->array.Astart(ms, ls);
        int je = src->array.Aend(ms, ls);
        for ( ; i <= ie && j <= je; i++, j++) {
            int bw;
            char *s = src->bit_elt(j, &bw);
            if (set_bit_elt(i, s, bw))
                arm_trigger = true;
            if (src->data_type == Dstring)
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
    if (arm_trigger && events)
        trigger();
}


void
vl_var::assign_int_range(int md, int ld, vl_var *src, int ms, int ls)
{
    if (data_type != Dint)
        return;
    if (array.size == 0) {
        if (!check_bit_range(&md, &ld))
            return;
        if (src->array.size == 0)
            assign_int_SS(md, ld, src, ms, ls);
        else
            assign_int_SA(md, ld, src, ms, ls);
        if (simulator->dbg_flags & DBG_assign) {
            cout << this << " = ";
            print_value(cout);
            cout << '\n';
        }
    }
    else {
        if (!array.check_range(&md, &ld))
            return;
        if (src->array.size == 0)
            assign_int_AS(md, ld, src, ms, ls);
        else
            assign_int_AA(md, ld, src, ms, ls);
    }
}


// Assign integer, scalar from scalar
//
void
vl_var::assign_int_SS(int md, int ld, vl_var *src, int ms, int ls)
{
    bool arm_trigger = false;
    if (src->check_bit_range(&ms, &ls)) {
        int bw;
        char *s = src->bit_elt(0, &bw);
        int j = src->bits.Bstart(ms, ls);
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
            int je = src->bits.Bend(ms, ls);
            for ( ; i <= md && j <= je; i++, j++) {
                if (set_bit_of(i, s[j]))
                    arm_trigger = true;
            }
            for ( ; i <= md; i++)
                if (set_bit_of(i, BitL))
                    arm_trigger = true;
        }
        if (src->data_type == Dstring)
            delete [] s;
    }
    else {
        for (int i = ld; i <= md; i++) {
            if (set_bit_of(i, BitL))
                arm_trigger = true;
        }
    }
    if (arm_trigger && events)
        trigger();
}


// Assign integer, scalar from array
//
void
vl_var::assign_int_SA(int md, int ld, vl_var *src, int ms, int ls)
{
    bool arm_trigger = false;
    if (src->array.check_range(&ms, &ls)) {
        int i, j, bw;
        char *s = src->bit_elt(src->array.Astart(ms, ls), &bw);
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
        if (src->data_type == Dstring)
            delete [] s;
    }
    else {
        for (int i = ld; i <= md; i++) {
            if (set_bit_of(i, BitL))
                arm_trigger = true;
        }
    }
    if (arm_trigger && events)
        trigger();
}


// Assign integer, array from scalar
//
void
vl_var::assign_int_AS(int md, int ld, vl_var *src, int ms, int ls)
{
    bool arm_trigger = false;
    if (src->check_bit_range(&ms, &ls)) {
        int bw;
        char *s = src->bit_elt(0, &bw);
        int j = src->bits.Bstart(ms, ls);
        int d = bits2int(s + j, rsize(ms, ls));
        if (set_int_elt(array.Astart(md, ld), d))
            arm_trigger = true;
        if (src->data_type == Dstring)
            delete [] s;
    }
    else {
        if (set_int_elt(array.Astart(md, ld), 0))
            arm_trigger = true;
    }
    if (arm_trigger && events)
        trigger();
}


// Assign integer, array from array
//
void
vl_var::assign_int_AA(int md, int ld, vl_var *src, int ms, int ls)
{
    bool arm_trigger = false;
    int i = array.Astart(md, ld);
    int ie = array.Aend(md, ld);
    if (src->array.check_range(&ms, &ls)) {
        int j = src->array.Astart(ms, ls);
        int je = src->array.Aend(ms, ls);
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
    if (arm_trigger && events)
        trigger();
}


void
vl_var::assign_time_range(int md, int ld, vl_var *src, int ms, int ls)
{
    if (data_type != Dtime)
        return;
    if (array.size == 0) {
        if (!check_bit_range(&md, &ld))
            return;
        if (src->array.size == 0)
            assign_time_SS(md, ld, src, ms, ls);
        else
            assign_time_SA(md, ld, src, ms, ls);
        if (simulator->dbg_flags & DBG_assign) {
            cout << this << " = ";
            print_value(cout);
            cout << '\n';
        }
    }
    else {
        if (!array.check_range(&md, &ld))
            return;
        if (src->array.size == 0)
            assign_time_AS(md, ld, src, ms, ls);
        else
            assign_time_AA(md, ld, src, ms, ls);
    }
}


// Assign time range, scalar from scalar
//
void
vl_var::assign_time_SS(int md, int ld, vl_var *src, int ms, int ls)
{
    bool arm_trigger = false;
    if (src->check_bit_range(&ms, &ls)) {
        int bw;
        char *s = src->bit_elt(0, &bw);
        int j = src->bits.Bstart(ms, ls);
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
            int je = src->bits.Bend(ms, ls);
            for ( ; i <= md && j <= je; i++, j++) {
                if (set_bit_of(i, s[j]))
                    arm_trigger = true;
            }
            for ( ; i <= md; i++)
                if (set_bit_of(i, BitL))
                    arm_trigger = true;
        }
        if (src->data_type == Dstring)
            delete [] s;
    }
    else {
        for (int i = ld; i <= md; i++) {
            if (set_bit_of(i, BitL))
                arm_trigger = true;
        }
    }
    if (arm_trigger && events)
        trigger();
}


// Assign time range, scalar from array
//
void
vl_var::assign_time_SA(int md, int ld, vl_var *src, int ms, int ls)
{
    bool arm_trigger = false;
    if (src->array.check_range(&ms, &ls)) {
        int i, j, bw;
        char *s = src->bit_elt(src->array.Astart(ms, ls), &bw);
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
        if (src->data_type == Dstring)
            delete [] s;
    }
    else {
        for (int i = ld; i <= md; i++) {
            if (set_bit_of(i, BitL))
                arm_trigger = true;
        }
    }
    if (arm_trigger && events)
        trigger();
}


// Assign time range, array from scalar
//
void
vl_var::assign_time_AS(int md, int ld, vl_var *src, int ms, int ls)
{
    bool arm_trigger = false;
    if (src->check_bit_range(&ms, &ls)) {
        int bw;
        char *s = src->bit_elt(0, &bw);
        int j = src->bits.Bstart(ms, ls);
        vl_time_t d = bits2time(s + j, rsize(ms, ls));
        if (set_time_elt(array.Astart(md, ld), d))
            arm_trigger = true;
        if (src->data_type == Dstring)
            delete [] s;
    }
    else {
        if (set_int_elt(array.Astart(md, ld), 0))
            arm_trigger = true;
    }
    if (arm_trigger && events)
        trigger();
}


// Assign time range, array from array
//
void
vl_var::assign_time_AA(int md, int ld, vl_var *src, int ms, int ls)
{
    bool arm_trigger = false;
    int i = array.Astart(md, ld);
    int ie = array.Aend(md, ld);
    if (src->array.check_range(&ms, &ls)) {
        int j = src->array.Astart(ms, ls);
        int je = src->array.Aend(ms, ls);
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
    if (arm_trigger && events)
        trigger();
}


// Assign to real
//
void
vl_var::assign_real_range(int md, int ld, vl_var *src, int ms, int ls)
{
    if (data_type != Dreal)
        return;
    if (array.size == 0) {
        if (!check_bit_range(&md, &ld))
            return;
        if (src->array.size == 0)
            assign_real_SS(md, ld, src, ms, ls);
        else
            assign_real_SA(md, ld, src, ms, ls);
        if (simulator->dbg_flags & DBG_assign) {
            cout << this << " = ";
            print_value(cout);
            cout << '\n';
        }
    }
    else {
        if (!array.check_range(&md, &ld))
            return;
        if (src->array.size == 0)
            assign_real_AS(md, ld, src, ms, ls);
        else
            assign_real_AA(md, ld, src, ms, ls);
    }
}


// Assign to real, scalar from scalar
//
void
vl_var::assign_real_SS(int md, int ld, vl_var *src, int ms, int ls)
{
    (void)md;
    (void)ld;
    bool arm_trigger = false;
    // The lhs range [md:ld] is ignored here
    if (src->bits.check_range(&ms, &ls)) {
        if (set_real_elt(0, src->real_bit_sel(ms, ls)))
            arm_trigger = true;
    }
    else {
        if (real_elt(0) != 0.0)
            arm_trigger = true;
        set_real_elt(0, 0.0);
    }
    if (arm_trigger && events)
        trigger();
}


// Assign to real, scalar from array
//
void
vl_var::assign_real_SA(int md, int ld, vl_var *src, int ms, int ls)
{
    (void)md;
    (void)ld;
    bool arm_trigger = false;
    // The lhs range [md:ld] is ignored here
    if (src->array.check_range(&ms, &ls)) {
        if (set_real_elt(0, src->real_elt(src->array.Astart(ms, ls))))
            arm_trigger = true;
    }
    else {
        if (real_elt(0) != 0.0)
            arm_trigger = true;
        set_real_elt(0, 0.0);
    }
    if (arm_trigger && events)
        trigger();
}


// Assign to real, array from scalar
//
void
vl_var::assign_real_AS(int md, int ld, vl_var *src, int ms, int ls)
{
    bool arm_trigger = false;
    int ix = array.Astart(md, ld);
    if (src->bits.check_range(&ms, &ls)) {
        if (set_real_elt(ix, src->real_bit_sel(ms, ls)))
            arm_trigger = true;
    }
    else {
        if (real_elt(ix) != 0.0)
            arm_trigger = true;
        set_real_elt(ix, 0.0);
    }
    if (arm_trigger && events)
        trigger();
}


// Assign to real, array from array
//
void
vl_var::assign_real_AA(int md, int ld, vl_var *src, int ms, int ls)
{
    bool arm_trigger = false;
    int i = array.Astart(md, ld);
    int ie = array.Aend(md, ld);
    if (src->array.check_range(&ms, &ls)) {
        int j = src->array.Astart(ms, ls);
        int je = src->array.Aend(ms, ls);
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
    if (arm_trigger && events)
        trigger();
}


// Assign to string
//
void
vl_var::assign_string_range(int md, int ld, vl_var *src, int ms, int ls)
{
    if (data_type != Dstring)
        return;
    if (array.size == 0) {
        if (!check_bit_range(&md, &ld))
            return;
        if (src->array.size == 0)
            assign_string_SS(md, ld, src, ms, ls);
        else
            assign_string_SA(md, ld, src, ms, ls);
    }
    else {
        if (!array.check_range(&md, &ld))
            return;
        if (src->array.size == 0)
            assign_string_AS(md, ld, src, ms, ls);
        else
            assign_string_AA(md, ld, src, ms, ls);
    }
}


// Assign to string, scalar from scalar
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


// Assign to string, scalar from array
//
void
vl_var::assign_string_SA(int md, int ld, vl_var *src, int ms, int ls)
{
    (void)md;
    (void)ld;
    if (src->array.check_range(&ms, &ls))
        set_str_elt(0, vl_strdup(src->str_elt(src->array.Astart(ms, ls))));
    else
        set_str_elt(0, 0);
}


// Assign to string, array from scalar
//
void
vl_var::assign_string_AS(int md, int ld, vl_var *src, int ms, int ls)
{
    (void)ms;
    (void)ls;
    set_str_elt(array.Astart(md, ld), vl_strdup(src->str_elt(0)));
}


// Assign to string, array from array
//
void
vl_var::assign_string_AA(int md, int ld, vl_var *src, int ms, int ls)
{
    int i = array.Astart(md, ld);
    int ie = array.Aend(md, ld);
    if (src->array.check_range(&ms, &ls)) {
        int j = src->array.Astart(ms, ls);
        int je = src->array.Aend(ms, ls);
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


//
// The following are public
//

// Clear the indicated bits or elements, if of type Dbit, fill with bitd,
// otherwise zero
//
void
vl_var::clear(vl_range *rng, int bitd)
{
    int md = 0, ld = 0;
    if (rng) {
        if (!rng->eval(&md, &ld))
            return;
    }
    if (data_type == Dbit) {
        if (array.size) {
            if (rng) {
                if (array.check_range(&md, &ld)) {
                    int i = array.Astart(md, ld);
                    int ie = array.Aend(md, ld);
                    for ( ; i <= ie; i++)
                        memset(((char**)u.d)[i], bitd, bits.size);
                }
            }
            else {
                for (int i = 0; i < array.size; i++)
                    memset(((char**)u.d)[i], bitd, bits.size);
            }
        }
        else {
            if (rng) {
                if (bits.check_range(&md, &ld)) {
                    int i = bits.Bstart(md, ld);
                    int ie = bits.Bend(md, ld);
                    for ( ; i <= ie; i++)
                        u.s[i] = bitd;
                }
            }
            else {
                for (int i = 0; i < bits.size; i++)
                    u.s[i] = bitd;
            }
        }
    }
    else if (data_type == Dint) {
        if (array.size) {
            if (rng) {
                if (array.check_range(&md, &ld)) {
                    int i = array.Astart(md, ld);
                    int ie = array.Aend(md, ld);
                    for ( ; i <= ie; i++)
                        ((int*)u.d)[i] = 0;
                }
            }
            else {
                for (int i = 0; i < array.size; i++)
                    ((int*)u.d)[i] = 0;
            }
        }
        else {
            if (rng) {
                if (check_bit_range(&md, &ld)) {
                    int i = bits.Bstart(md, ld);
                    int ie = bits.Bend(md, ld);
                    for ( ; i <= ie; i++)
                        set_bit_of(i, BitL);
                }
            }
            else
                u.i = 0;
        }
    }
    else if (data_type == Dtime) {
        if (array.size) {
            if (rng) {
                if (array.check_range(&md, &ld)) {
                    int i = array.Astart(md, ld);
                    int ie = array.Aend(md, ld);
                    for ( ; i <= ie; i++)
                        ((vl_time_t*)u.d)[i] = 0;
                }
            }
            else {
                for (int i = 0; i < array.size; i++)
                    ((vl_time_t*)u.d)[i] = 0;
            }
        }
        else {
            if (rng) {
                if (check_bit_range(&md, &ld)) {
                    int i = bits.Bstart(md, ld);
                    int ie = bits.Bend(md, ld);
                    for ( ; i <= ie; i++)
                        set_bit_of(i, BitL);
                }
            }
            else
                u.t = 0;
        }
    }
    else if (data_type == Dreal) {
        if (array.size) {
            if (rng) {
                if (array.check_range(&md, &ld)) {
                    int i = array.Astart(md, ld);
                    int ie = array.Aend(md, ld);
                    for ( ; i <= ie; i++)
                        ((double*)u.d)[i] = 0.0;
                }
            }
            else {
                for (int i = 0; i < array.size; i++)
                    ((double*)u.d)[i] = 0.0;
            }
        }
        else
            u.r = 0.0;
    }
    else if (data_type == Dstring) {
        if (array.size) {
            if (rng) {
                if (array.check_range(&md, &ld)) {
                    int i = array.Astart(md, ld);
                    int ie = array.Aend(md, ld);
                    for ( ; i <= ie; i++) {
                        if (((char**)u.d)[i])
                            *((char**)u.d)[i] = 0;
                    }
                }
            }
            else {
                for (int i = 0; i < array.size; i++)
                    if (((char**)u.d)[i])
                        *((char**)u.d)[i] = 0;
            }
        }
        else
            *u.s = 0;
    }
    if (events)
        trigger();
}


// Clear all value and type information
//
void
vl_var::reset()
{
    if (array.size) {
        if (data_type == Dbit || data_type == Dstring) {
            char **s = (char**)u.d;
            for (int i = 0; i < array.size; i++)
                delete [] s[i];
        }
        else
            delete [] (char*)u.d;
    }
    else if (data_type == Dbit || data_type == Dstring)
        delete [] u.s;
    data_type = Dnone;
    array.clear();
    bits.clear();
    u.r = 0;
}


// Set bit field from bit expression parser
//
void
vl_var::set(bitexp_parse *p)
{
    if ((data_type == Dnone || data_type == Dbit) && !array.size) {
        if (data_type == Dbit)
            delete [] u.s;
        data_type = Dbit;
        bits = p->bits;
        u.r = 0;
        u.s = new char[bits.size];
        memcpy(u.s, p->u.s, bits.size);
    }
    else {
        vl_error("(internal) incorrect data type in set-bits");
        simulator->abort();
    }
}


// Set scalar int
//
void
vl_var::set(int i)
{
    if ((data_type == Dnone || data_type == Dint) && !array.size) {
        data_type = Dint;
        bits.clear();
        u.r = 0;
        u.i = i;
    }
    else {
        vl_error("(internal) incorrect data type in set-scalar_int");
        simulator->abort();
    }
}


// Set scalar time
//
void
vl_var::set(vl_time_t t)
{
    if ((data_type == Dnone || data_type == Dtime) && !array.size) {
        data_type = Dtime;
        bits.clear();
        u.r = 0;
        u.i = t;
    }
    else {
        vl_error("(internal) incorrect data type in set-scalar_time");
        simulator->abort();
    }
}


// Set scalar real
//
void
vl_var::set(double r)
{
    if ((data_type == Dnone || data_type == Dreal) && !array.size) {
        data_type = Dreal;
        bits.clear();
        u.r = r;
    }
    else {
        vl_error("(internal) incorrect data type in set-scalar_real");
        simulator->abort();
    }
}


// Set scalar string
//
void
vl_var::set(char *string)
{
    if ((data_type == Dnone || data_type == Dstring) && !array.size) {
        if (data_type == Dstring)
            delete [] u.s;
        else
            data_type = Dstring;
        bits.clear();
        u.r = 0;
        u.s = string;
    }
    else {
        vl_error("(internal) incorrect data type in set-scalar_string");
        simulator->abort();
    }
}


// Set scalar bit field to undefined with width w
//
void        
vl_var::setx(int w)
{
    if ((data_type == Dnone || data_type == Dbit) && !array.size) {
        if (data_type == Dbit)
            delete [] u.s;
        else
            data_type = Dbit;
        bits.size = w;
        bits.lo_index = 0;
        bits.hi_index = w-1;
        u.s = new char[w];
        for (int i = 0; i < w; i++)
            u.s[i] = BitDC;
    }
    else {
        vl_error("(internal) incorrect data type in set-dc");
        simulator->abort();
    }
}               


// Set scalar bit field to hi-z with width w
//
void        
vl_var::setz(int w)
{
    if ((data_type == Dnone || data_type == Dbit) && !array.size) {
        if (data_type == Dbit)
            delete [] u.s;
        else
            data_type = Dbit;
        bits.size = w;
        bits.lo_index = 0;
        bits.hi_index = w-1;
        u.s = new char[w];
        for (int i = 0; i < w; i++)
            u.s[i] = BitZ;
    }
    else {
        vl_error("(internal) incorrect data type in set-z");
        simulator->abort();
    }
}               


// Set scalar bit field to representation of ix
//
void
vl_var::setb(int ix)
{
    if ((data_type == Dnone || data_type == Dbit) && !array.size) {
        if (data_type == Dbit)
            delete [] u.s;
        else
            data_type = Dbit;
        bits.size = DefBits;  
        bits.lo_index = 0;
        bits.hi_index = bits.size - 1;
        u.s = new char[bits.size];
        int mask = 1;
        for (int i = 0; i < bits.size; i++) {
            if (ix & mask)
                u.s[i] = BitH;
            else
                u.s[i] = BitL;
            mask <<= 1;
        }
    }
    else {
        vl_error("(internal) incorrect data type in set-integer");
        simulator->abort();
    }
}


// Set scalar bit field to representation of t
//
void
vl_var::sett(vl_time_t t)
{
    if ((data_type == Dnone || data_type == Dbit) && !array.size) {
        if (data_type == Dbit)
            delete [] u.s;
        else
            data_type = Dbit;
        bits.size = (int)(8*sizeof(vl_time_t));
        bits.lo_index = 0;
        bits.hi_index = bits.size - 1;
        u.s = new char[bits.size];
        vl_time_t mask = 1;
        for (int i = 0; i < bits.size; i++) {
            if (t & mask)
                u.s[i] = BitH;
            else
                u.s[i] = BitL;
            mask <<= 1;
        }
    }
    else {
        vl_error("(internal) incorrect data type in set-time");
        simulator->abort();
    }
}


// Fill the bit field with b
//
void
vl_var::setbits(int b)
{
    if (data_type == Dbit) {
        if (!array.size)
            memset(u.s, b, bits.size);
        else {
            for (int i = 0; i < array.size; i++) {
                char *s = ((char**)u.d)[i];
                memset(s, b, bits.size);
            }
        }
    }
}


// Set a single bit at position pos. Return true if change
//
bool
vl_var::set_bit_of(int pos, int data)
{
    if (data_type == Dbit) {
        if (pos >= 0 && pos < bits.size) {
            char *s = (array.size ? ((char**)u.d)[0] : u.s);
            char oldc = s[pos];
            s[pos] = data;
            if (oldc != data)
                return (true);
        }
    }
    else if (data_type == Dint) {
        if (pos >= 0 && pos < DefBits) {
            int i = (array.size ? ((int*)u.d)[0] : u.i);
            int oldi = i;
            int mask = 1 << pos;
            if (data == BitH)
                i |= mask;
            else
                i &= ~mask;
            if (array.size)
                ((int*)u.d)[0] = i;
            else
                u.i = i;
            if (i != oldi)
                return (true);
        }
    }
    else if (data_type == Dtime) {
        if (pos >= 0 && pos < (int)sizeof(vl_time_t)*8) {
            vl_time_t i = (array.size ? ((vl_time_t*)u.d)[0] : u.t);
            vl_time_t oldi = i;
            vl_time_t mask = 1 << pos;
            if (data == BitH)
                i |= mask;
            else
                i &= ~mask;
            if (array.size)
                ((vl_time_t*)u.d)[0] = i;
            else
                u.t = i;
            if (i != oldi)
                return (true);
        }
    }
    return (false);
}


// Set bits of vector bit field entry indx from src.  Extra bits are
// cleared.  If src is 0, fill the row with the value passed as swid.
// Returns true if the value changes
//
bool
vl_var::set_bit_elt(int indx, const char *src, int swid)
{
    bool changed = false;
    if (data_type == Dbit) {
        char *s = 0;
        if (array.size == 0 && indx == 0)
            s = u.s;
        else if (indx >= 0 && indx < array.size)
            s = ((char**)u.d)[indx];
        if (s) {
            if (!src) {
                for (int i = 0; i < bits.size; i++) {
                    if (s[i] != swid) {
                        changed = true;
                        s[i] = swid;
                    }
                }
            }
            else {
                int mw = min(bits.size, swid);
                int i;
                for (i = 0; i < mw; i++) {
                    if (s[i] != src[i]) {
                        changed = true;
                        s[i] = src[i];
                    }
                }
                for ( ; i < bits.size; i++) {
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


// Set vector int entry indx, indx is raw, return true if changed
//
bool
vl_var::set_int_elt(int indx, int val)
{
    bool changed = false;
    if (data_type == Dint) {
        if (array.size == 0 && indx == 0) {
            if (u.i != val) {
                changed = true;
                u.i = val;
            }
        }
        else if (indx >= 0 && indx < array.size) {
            if (((int*)u.d)[indx] != val) {
                changed = true;
                ((int*)u.d)[indx] = val;
            }
        }
    }
    return (changed);
}


// Set vector time entry indx, indx is raw, return true if changed
//
bool
vl_var::set_time_elt(int indx, vl_time_t val)
{
    bool changed = false;
    if (data_type == Dtime) {
        if (array.size == 0 && indx == 0) {
            if (u.t != val) {
                changed = true;
                u.t = val;
            }
        }
        else if (indx >= 0 && indx < array.size) {
            if (((vl_time_t*)u.d)[indx] != val) {
                changed = true;
                ((vl_time_t*)u.d)[indx] = val;
            }
        }
    }
    return (changed);
}


// Set vector real entry indx, indx is raw, return true if changed
//
bool
vl_var::set_real_elt(int indx, double val)
{
    bool changed = false;
    if (data_type == Dreal) {
        if (array.size == 0 && indx == 0) {
            if (u.r != val) {
                changed = true;
                u.r = val;
            }
        }
        else if (indx >= 0 && indx < array.size) {
            if (((double*)u.d)[indx] != val) {
                changed = true;
                ((double*)u.d)[indx] = val;
            }
        }
    }
    return (changed);
}


// Set vector string entry indx (not copied), indx is raw.
// Don't bother with changes
//
bool
vl_var::set_str_elt(int indx, char *val)
{
    if (data_type == Dstring) {
        if (array.size == 0 && indx == 0) {
            delete [] u.s;
            u.s = val;
        }
        else if (indx >= 0 && indx < array.size) {
            char **ss = (char**)u.d;
            delete [] ss[indx];
            ss[indx] = val;
        }
    }
    return (false);
}


void
vl_var::set_assigned(vl_bassign_stmt *bs)
{
    if (flags & VAR_F_ASSIGN)
        return;
    if (bs && cassign == bs && (flags & VAR_CP_ASSIGN)) {
        vl_var z = case_eq(*cassign->lhs, cassign->rhs->eval());
        if (z.u.s[0] == BitH)
            return;
    }
    if (data_type == Dconcat) {
        lsGen<vl_expr*> gen(u.c);
        vl_expr *e;
        while (gen.next(&e)) {
            vl_var *v = e->source();
            if (v) {
                if (bs && !v->check_net_type(REGreg)) {
                    vl_error("in procedural continuous assign, concatenated "
                        "component is not a reg");
                    errout(this);
                    simulator->abort();
                    return;
                }
                v->flags &= ~VAR_CP_ASSIGN;
            }
            else {
                vl_error("in procedural continuous assign, concatenated "
                    "component has undefined width");
                errout(this);
                simulator->abort();
                return;
            }
        }
    }
    else if (bs && net_type != REGreg) {
        vl_error("in procedural continuous assign, %s is not a reg", name);
        errout(this);
        simulator->abort();
        return;
    }
    flags &= ~VAR_CP_ASSIGN;
    if (cassign) {
        cassign->rhs->unchain(cassign);
        cassign = 0;
    }
    if (bs) {
        cassign = bs;
        cassign->lhs->assign(0, &cassign->rhs->eval(), 0);
        cassign->rhs->chain(cassign);
        flags |= VAR_CP_ASSIGN;
        if (data_type == Dconcat) {
            lsGen<vl_expr*> gen(u.c);
            vl_expr *e;
            while (gen.next(&e)) {
                vl_var *v = e->source();
                v->flags |= VAR_CP_ASSIGN;
            }
        }
    }
}


void
vl_var::set_forced(vl_bassign_stmt *bs)
{
    if (bs) {
        if (cassign == bs && (flags & VAR_F_ASSIGN)) {
            vl_var z = case_eq(*cassign->lhs, cassign->rhs->eval());
            if (z.u.s[0] == BitH)
                return;
        }
        set_assigned(0);
    }
    if (data_type == Dconcat) {
        lsGen<vl_expr*> gen(u.c);
        vl_expr *e;
        while (gen.next(&e)) {
            vl_var *v = e->source();
            if (!v) {
                vl_error("in force, bad value in concatenation");
                errout(this);
                simulator->abort();
                return;
            }
            v->flags &= ~VAR_F_ASSIGN;
        }
    }
    flags &= ~VAR_F_ASSIGN;
    if (cassign) {
        cassign->rhs->unchain(cassign);
        cassign = 0;
    }
    if (bs) {
        cassign = bs;
        cassign->lhs->assign(0, &cassign->rhs->eval(), 0);
        cassign->rhs->chain(cassign);
        flags |= VAR_F_ASSIGN;
        if (data_type == Dconcat) {
            lsGen<vl_expr*> gen(u.c);
            vl_expr *e;
            while (gen.next(&e)) {
                vl_var *v = e->source();
                v->flags |= VAR_F_ASSIGN;
            }
        }
    }
}


// If the lhs is a concatenation in a delayed assignment, have to evaluate
// any ranges before the delay
//
void
vl_var::freeze_concat()
{
    if (data_type != Dconcat)
        return;
    lsGen<vl_expr*> gen(u.c);
    vl_expr *e;
    while (gen.next(&e)) {
        if (e->etype == BitSelExpr || e->etype == PartSelExpr) {
            vl_range *tr;
            e->ux.ide.range->eval(&tr);
            delete e->ux.ide.range;
            e->ux.ide.range = tr;
        }
    }
}


// Set the default effective bit field parameters
//
void
vl_var::default_range(int *m, int *l)
{
    if (array.size) {
        *m = array.hi_index;
        *l = array.lo_index;
        return;
    }
    *m = 0;
    *l = 0;
    if (data_type == Dbit) {
        *m = bits.hi_index;
        *l = bits.lo_index;
    }
    else if (data_type == Dint)
        *m = DefBits - 1;
    else if (data_type == Dtime)
        *m = sizeof(vl_time_t)*8 - 1;
    else if (data_type == Dreal)
        *m = DefBits - 1;
    else if (data_type == Dstring)
        *m = (u.s ? (strlen(u.s) + 1)*8 : 0);
}


// Return true if the variable is a reg and reg_or_net is REGreg, or
// type is net and reg_or_net is not REGreg.
//
bool
vl_var::check_net_type(REGtype reg_or_net)
{
    if (data_type == Dconcat) {
        lsGen<vl_expr*> gen(u.c);
        vl_expr *e;
        while (gen.next(&e)) {
            vl_var *v = e->source();
            if (!v) {
                vl_error("bad expression in concatenation:");
                errout(this);
                simulator->abort();
                return (false);
            }
            if (!v->check_net_type(reg_or_net))
                return (false);
        }
        return (true);
    }
    else if (reg_or_net == REGreg)
        return (net_type == REGreg ? true : false);
    else if (net_type < REGwire) {
        if (net_type == REGnone)
            net_type = REGwire;
        else
            return (false);
    }
    return (true);
}


// Check and readjust the range values if necessary to correspond to the
// effective bit width.  False is returned if out of range
//
bool
vl_var::check_bit_range(int *m, int *l)
{
    if (data_type == Dbit)
        return (bits.check_range(m, l));
    if (data_type == Dint || data_type == Dreal) {
        int mx = DefBits - 1;
        if (*m == *l) {
            if (*m < 0 || *m >= mx)
                return (false);
        }
        else {
            if (*m < *l) {
                vl_error("in check range, index direction mismatch found");
                simulator->abort();
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
    if (data_type == Dtime) {
        int mx = sizeof(vl_time_t)*8 - 1;
        if (*m == *l) {
            if (*m < 0 || *m >= mx)
                return (false);
        }
        else {
            if (*m < *l) {
                vl_error("in check range, index direction mismatch found");
                simulator->abort();
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
    if (data_type == Dstring) {
        char *s = (array.size ? ((char**)u.d)[0] : u.s);
        int mx = (s ? (strlen(s) + 1)*8 : 0);
        if (*m == *l) {
            if (*m < 0 || *m >= mx)
                return (false);
        }
        else {
            if (*m < *l) {
                vl_error("in check range, index direction mismatch found");
                simulator->abort();
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
//  lf:  lsb of source range, internal (0-based) offset
//
void
vl_var::add_driver(vl_var *d, int mt, int lt, int lf)
{
    if (d->array.size != 0) {
        vl_error("net driver is an array, not allowed");
        simulator->abort();
        return;
    }
    if (!drivers) {
        drivers = new lsList<vl_driver*>;
        vl_driver *drv = new vl_driver(d, mt, lt, lf);
        drivers->newEnd(drv);
        return;
    }
    lsGen<vl_driver*> gen(drivers);
    vl_driver *drv;
    while (gen.next(&drv)) {
        if (drv->srcvar == d && drv->l_from == lf) {
            if (lt == drv->l_to) {
                if (mt > drv->m_to)
                    drv->m_to = mt;
                return;
            }
        }
    }
    drv = new vl_driver(d, mt, lt, lf);
    drivers->newEnd(drv);
}


// Return the highest strength bit, for determining bit value of a net
// with competing drivers
//
static int
strength_bit(int b1, vl_strength s1, int b2, vl_strength s2)
{
    if (b1 == b2)
        return (b1);
    if (b1 == BitZ)
        return (b2);
    if (b2 == BitZ)
        return (b1);
    if (b1 == BitL && s1.str0 > s2.str1)
        return (BitL);
    if (b1 == BitH && s1.str1 > s2.str0)
        return (BitH);
    if (b2 == BitL && s2.str0 > s1.str1)
        return (BitL);
    if (b2 == BitH && s2.str1 > s1.str0)
        return (BitH);
    return (BitDC);
}


// Resolve contention, return the "winning" value, called for nets.
// The ix is in the bits range,  din should be in the drivers list
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
    lsGen<vl_driver*> gen(drivers);
    vl_driver *dr;
    vl_strength accum_str;
    bool skip_portdrv = false;
    if (din->bit_of(ixs) != BitZ)
        skip_portdrv = true;
    bool first = true;
    while (gen.next(&dr)) {
        vl_var *dt = dr->srcvar;
        if (skip_portdrv && (dt->flags & VAR_PORT_DRIVER) && dt != din)
            continue;

        if (ix > dr->m_to || ix < dr->l_to)
            continue;

        vl_strength st = dt->strength;
        if (st.str0 == STRnone)
            st = strength;
        if (st.str0 == STRnone)
            st.str0 = STRstrong;
        if (st.str1 == STRnone)
            st.str1 = STRstrong;

        int bx = dt->bit_of((ix - dr->l_to) + dr->l_from);
        if ((bx == BitH && st.str1 == STRhiZ) ||
                (bx == BitL && st.str0 == STRhiZ))
            continue;
        if (bx == BitZ)
            continue;

        if ((net_type == REGwand || net_type == REGtrior) && bx == BitL) {
            b = BitL;
            return (b);
        }
        if ((net_type == REGwor || net_type == REGtriand) && bx == BitH) {
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
            if (st.str1 > accum_str.str1)
                accum_str.str1 = st.str1;
        }
        else if (b == BitL && bx == BitL) {
            if (st.str0 > accum_str.str0)
                accum_str.str0 = st.str0;
        }
        else if (b == BitDC) {
            if (st.str1 > accum_str.str1)
                accum_str.str1 = st.str1;
            if (st.str0 > accum_str.str0)
                accum_str.str0 = st.str0;
        }
    }
    if (b == BitZ) {
        if (net_type == REGtri0)
            b = BitL;
        else if (net_type == REGtri1)
            b = BitH;
    }
    return (b);
}


// This is called when data changes,  perform the asynchronous actions
//
void
vl_var::trigger()
{
    vl_action_item *ap = 0, *an;
    for (vl_action_item *a = events; a; a = an) {
        an = a->next;
        if (a->event) {
            if ((int)a->event->eval(simulator)) {
                if (a->event->count) {
                    a->event->count--;
                    return;
                }
                a->next = 0;
                a->event->unchain(a);
                a->event = 0;
                simulator->timewheel->append_trig(simulator->time, a);
                if (ap)
                    ap->next = an;
                else
                    events = an;
                continue;
            }
            ap = a;
            continue;
        }
        // continuous assign
        simulator->timewheel->append_trig(simulator->time, a->copy());
        ap = a;
    }
}


// Return true if a bit is BitDC or BitZ
//
bool
vl_var::is_x()
{
    if (data_type == Dbit && !array.size) {
        for (int i = 0; i < bits.size; i++) {
            if (u.s[i] != BitL && u.s[i] != BitH)
                return (true);
        }
    }
    return (false);
}


// Return true if all bits are BitZ
//
bool
vl_var::is_z()
{
    if (data_type == Dbit && !array.size) {
        for (int i = 0; i < bits.size; i++) {
            if (u.s[i] != BitZ)
                return (false);
        }
        return (true);
    }
    return (false);
}


// Return the bit value at the given raw index position
//
int
vl_var::bit_of(int i)
{
    if (data_type == Dbit) {
        if (i < bits.size) {
            if (array.size)
                return (((char**)u.d)[0][i]);
            else
                return (u.s[i]);
        }
    }
    else if (data_type == Dint) {
        if (i < (int)sizeof(int)*8) {
            if (array.size)
                return (bit(((int*)u.d)[0], i));
            else
                return (bit(u.i, i));
        }
    }
    else if (data_type == Dtime) {
        if (i < (int)sizeof(vl_time_t)*8) {
            if (array.size)
                return (bit(((vl_time_t*)u.d)[0], i));
            else
                return (bit(u.t, i));
        }
    }
    return (BitZ);
}


// Return integer created from selected bits of bit field
//
int
vl_var::int_bit_sel(int m, int l)
{
    int ret = 0;
    if (data_type == Dbit) {
        char *s;
        if (array.size == 0)
            s = u.s;
        else
            s = *(char**)u.s;
        int mask = 1;
        int i = bits.Bstart(m, l);
        int ie = bits.Bend(m, l);
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


// Return vl_time_t created from selected bits of bit field
//
vl_time_t
vl_var::time_bit_sel(int m, int l)
{
    vl_time_t ret = 0;
    if (data_type == Dbit) {
        char *s;
        if (array.size == 0)
            s = u.s;
        else
            s = *(char**)u.s;
        vl_time_t mask = 1;
        int i = bits.Bstart(m, l);
        int ie = bits.Bend(m, l);
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


// Return real created from selected bits of bit field
//
double
vl_var::real_bit_sel(int m, int l)
{
    vl_time_t ret = 0;
    if (data_type == Dbit) {
        char *s;
        if (array.size == 0)
            s = u.s;
        else
            s = *(char**)u.s;
        vl_time_t mask = 1;
        int i = bits.Bstart(m, l);
        int ie = bits.Bend(m, l);
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
    if (data_type == Dbit) {
        int ret = 0;
        for (int i = 0; i < bits.size; i++) {
            if (u.s[i] == BitH)
                return (Hmask);
            else if (u.s[i] != BitL)
                ret |= Xmask;
        }
        if (ret & Xmask)
            return (Xmask);
        return (Lmask);
    }
    if (data_type == Dint)
        return (u.i ? Hmask : Lmask);
    if (data_type == Dtime)
        return (u.t ? Hmask : Lmask);
    return (Lmask);
}
 

// Return a pointer to the raw num'th data element, the type of data is
// returned in rt, works whether array or not
//
void *
vl_var::element(int num, int *rt)
{
    if (array.size == 0 && num == 0) {
        if (data_type == Dnone) {
            *rt = Dint;
            return (&u.i);
        }
        if (data_type == Dbit) {
            *rt = Dbit;
            return (u.s);
        }
        if (data_type == Dint) {
            *rt = Dint;
            return (&u.i);
        }
        if (data_type == Dtime) {
            *rt = Dtime;
            return (&u.t);
        }
        if (data_type == Dreal) {
            *rt = Dreal;
            return (&u.r);
        }
        if (data_type == Dstring) {
            *rt = Dstring;
            return (u.s);
        }
        return (0);
    }
    if (num < 0 || num >= array.size)
        return (0);

    if (data_type == Dbit) {
        *rt = Dbit;
        return (((char**)u.d)[num]);
    }
    if (data_type == Dint) {
        *rt = Dint;
        return (((int*)u.d) + num);
    }
    if (data_type == Dtime) {
        *rt = Dtime;
        return (((vl_time_t*)u.d) + num);
    }
    if (data_type == Dreal) {
        *rt = Dreal;
        return (((double*)u.d) + num);
    }
    if (data_type == Dstring) {
        *rt = Dstring;
        return (((char**)u.d)[num]);
    }
    return (0);
}


// Return the bits and field width from raw num'th element
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
        *bw = bits.size;
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


// Return integer corresponding to raw num'th data element
//
int
vl_var::int_elt(int num)
{
    int tp;
    void *v = element(num, &tp);
    if (!v)
        return (0);
    if (tp == Dbit)
        return (bits2int((char*)v, bits.size));
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


// Return vl_time_t corresponding to raw num'th data element
//
vl_time_t
vl_var::time_elt(int num)
{
    int tp;
    void *v = element(num, &tp);
    if (!v)
        return (0);
    if (tp == Dbit)
        return (bits2time((char*)v, bits.size));
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


// Return double corresponding to raw num'th data element
//
double
vl_var::real_elt(int num)
{
    int tp;
    void *v = element(num, &tp);
    if (!v)
        return (0);
    if (tp == Dbit)
        return (bits2real((char*)v, bits.size));
    if (tp == Dint)
        return ((double)*(int*)v);
    if (tp == Dtime)
        return ((double)*(vl_time_t*)v);
    if (tp == Dreal)
        return (*(double*)v);
    return (0);
}


// Return string for raw num'th element, if string data
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
        int sz = bits.size/8 + 2;
        char *ss = new char[sz];
        for (int i = 0; i < sz; i++)
            ss[i] = 0;
        char *s = ss;
        char *b = (char*)v;
        int mask = 1;
        for (int i = 0; i < bits.size; i++) {
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


// Conversion operator
//
vl_var::operator int()
{
    return (int_elt(0));
}


// Conversion operator
//
vl_var::operator unsigned()
{
    return ((unsigned)int_elt(0));
}


// Conversion operator
//
vl_var::operator vl_time_t()
{
    return (time_elt(0));
}


// Conversion operator
//
vl_var::operator double()
{
    return (real_elt(0));
}


// Conversion operator
//
vl_var::operator char*()
{
    return (str_elt(0));
}
// End of vl_var functions


// Initialize values from range specifier
//
void
vl_array::set(vl_range *range)
{
    if (!range || !range->left)
        return;
    if (!range->eval(&hi_index, &lo_index)) {
        vl_error("range initialization failed");
        errout(range);
        range->left->simulator->abort();
        hi_index = lo_index = 0;
    }
    size = rsize(hi_index, lo_index);
}


// Check/adjust range against source vector.  Return false if out of
// range.  The abort flag is set if a direction error is found
//
bool
vl_array::check_range(int *m, int *l)
{
    const char *err1 = "in check range, index direction mismatch found";
    if (*m == *l) {
        // bit or element select
        if (*m < min(hi_index, lo_index) || *m > max(hi_index, lo_index))
            return (false);
        return (true);
    }
    if (lo_index <= hi_index) {
        if (*m < *l) {
            vl_error(err1);
            vl_var::simulator->abort();
            return (false);
        }
        if (*l > hi_index || *m < lo_index)
            return (false);
        if (*m > hi_index)
            *m = hi_index;
        if (*l < lo_index)
            *l = lo_index;
    }
    else {
        if (*m > *l) {
            vl_error(err1);
            vl_var::simulator->abort();
            return (false);
        }
        if (*l < hi_index || *m > lo_index)
            return (false);
        if (*m < hi_index)
            *m = hi_index;
        if (*l > lo_index)
            *l = lo_index;
    }
    return (true);
}
// End of vl_array functions


vl_range *
vl_range::copy()
{
    if (!this || !left)
        return (0);
    return (new vl_range(left->copy(), right ? right->copy() : 0));
}


// Compute the integer range parameters.  If parameters are valid return
// true
//
bool
vl_range::eval(int *m, int *l)
{
    *m = 0;
    *l = 0;
    if (!left)
        return (true);
    vl_var &dl = left->eval();
    if (dl.is_x())
        return (false);
    *m = (int)dl;
    if (right) {
        vl_var &dr = right->eval();
        if (dr.is_x())
            return (false);
        *l = (int)dr;
    }
    else
        *l = *m;
    return (true);
}


// Create a new range struct by evaluating this
//
bool
vl_range::eval(vl_range **r)
{
    *r = 0;
    if (this) {
        int m, l;
        if (!eval(&m, &l))
            return (false);
        vl_expr *mx = new vl_expr;
        mx->etype = IntExpr;
        mx->data_type = Dint;
        mx->u.i = m;
        vl_expr *lx = 0;
        if (m != l) {
            lx = new vl_expr;
            lx->etype = IntExpr;
            lx->data_type = Dint;
            lx->u.i = l;
        }
        *r = new vl_range(mx, lx);
    }
    return (true);
}


// Compute the range width, return 0 if the range is bad
//
int
vl_range::width()
{
    int m, l;
    if (eval(&m, &l))
        return (abs(m - l) + 1); 
    return (0);
}
// End of vl_range functions


vl_delay::~vl_delay()
{
    delete delay1;
    delete_list(list);
}


vl_delay *
vl_delay::copy()
{
    if (!this)
        return (0);
    vl_delay *retval;
    if (delay1)
        retval = new vl_delay(delay1->copy());
    else
        retval = new vl_delay(copy_list(list));
    return (retval);
}


vl_time_t
vl_delay::eval()
{
    vl_module *cmod = vl_var::simulator->context->currentModule();
    if (!cmod) {
        vl_error("internal, no current module for delay evaluation");
        vl_var::simulator->abort();
        return (0);
    }
    double tstep = vl_var::simulator->description->tstep;
    double tunit = cmod->tunit;
    double tprec = cmod->tprec;
    if (list) {
        lsGen<vl_expr*> gen(list);
        vl_expr *e;
        if (gen.next(&e)) {
            double td = (double)e->eval();
            vl_time_t t = (vl_time_t)(td*tunit/tprec + 0.5);
            t *= (vl_time_t)(tprec/tstep);
            return (t);
        }
    }
    else if (delay1) {
        double td = (double)delay1->eval();
        vl_time_t t = (vl_time_t)(td*tunit/tprec + 0.5);
        t *= (vl_time_t)(tprec/tstep);
        return (t);
    }
    return (0);
}
// End of vl_delay functions


vl_event_expr::~vl_event_expr()
{
    delete expr;
    delete_list(list);
    delete repeat;
}


vl_event_expr *
vl_event_expr::copy()
{
    if (!this)
        return (0);
    vl_event_expr *retval = new vl_event_expr(type, 0);
    if (expr)
        retval->expr = expr->copy();
    retval->list = copy_list(list);
    return (retval);
}


void
vl_event_expr::init()
{
    if (expr)
        expr->eval();
    else if (list) {
        lsGen<vl_event_expr*> gen(list);
        vl_event_expr *e;
        while (gen.next(&e))
            e->init();
    }
}


bool
vl_event_expr::eval(vl_simulator *sim)
{
    if (type == OrEventExpr) {
        if (list) {
            vl_event_expr *e;
            lsGen<vl_event_expr*> gen(list);
            while (gen.next(&e)) {
                if (e->eval(sim))
                    return (true);
            }
        }
    }
    else if (type == EdgeEventExpr) {
        vl_var last = *expr;
        vl_var d = expr->eval();
        if (d.net_type == REGevent)
            // named event sent
            return (true);
        if (last.data_type == Dnone)
            return (false);
        vl_var &z = case_eq(last, d);
        if (z.u.s[0] == BitL)
            return (true);
    }
    else if (type == PosedgeEventExpr) {
        vl_var last = *expr;
        if (last.data_type == Dnone)
            return (false);
        vl_var &d = expr->eval();
        if (last.data_type != Dbit || d.data_type != Dbit) {
            vl_error("non-bitfield found as posedge event trigger");
            sim->abort();
            return (false);
        }
        if ((last.u.s[0] == BitL && d.u.s[0] != BitL) ||
                (last.u.s[0] != BitH && d.u.s[0] == BitH))
            return (true);
    }
    else if (type == NegedgeEventExpr) {
        vl_var last = *expr;
        if (last.data_type == Dnone)
            return (false);
        vl_var &d = expr->eval();
        if (last.data_type != Dbit || d.data_type != Dbit) {
            vl_error("non-bitfield found as negedge event trigger");
            sim->abort();
            return (false);
        }
        if ((last.u.s[0] == BitH && d.u.s[0] != BitH) ||
                (last.u.s[0] != BitL && d.u.s[0] == BitL))
            return (true);
    }
    else if (type == LevelEventExpr) {
        if ((int)expr->eval())
            return (true);
    }
    return (false);
}


// Chain the action to the expression(s)
//
void
vl_event_expr::chain(vl_action_item *a)
{
    if (expr)
        expr->chain(a);
    else if (list) {
        lsGen<vl_event_expr*> gen(list);
        vl_event_expr *e;
        while (gen.next(&e)) {
            if (e->expr)
                e->expr->chain(a);
        }
    }
}


// Remove actions equivalent to the passed action from the expression(s)
//
void
vl_event_expr::unchain(vl_action_item *a)
{
    if (expr)
        expr->unchain(a);
    else if (list) {
        lsGen<vl_event_expr*> gen(list);
        vl_event_expr *e;
        while (gen.next(&e)) {
            if (e->expr)
                e->expr->unchain(a);
        }
    }
}


// Remove the chained actions that have blk in the context hierarchy
//
void
vl_event_expr::unchain_disabled(vl_stmt *blk)
{
    if (expr)
        expr->unchain_disabled(blk);
    else if (list) {
        lsGen<vl_event_expr*> gen(list);
        vl_event_expr *e;
        while (gen.next(&e)) {
            if (e->expr)
                e->expr->unchain_disabled(blk);
        }
    }
}
// End of vl_event_expr functions

