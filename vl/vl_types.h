 
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

#include <setjmp.h>
#include <iostream>

using std::ostream;
using std::cout;
using std::cerr;

typedef unsigned long long int vl_time_t;

// Print format modifiers.
//
enum DSPtype { DSPall, DSPb, DSPh, DSPo };

//---------------------------------------------------------------------------
//  Forward references
//---------------------------------------------------------------------------

// Data variables and expressions
struct vl_var;
struct vl_expr;
struct vl_strength;
struct vl_driver;
struct vl_range;
struct vl_delay;
struct vl_event_expr;

// Parser objects
struct vl_parser;
struct vl_bitexp_parse;
struct range_or_type;
struct multi_concat;

// Simulator objects
struct vl_simulator;
struct vl_context;
struct vl_stmt;
struct vl_action_item;
struct vl_sblk;
struct vl_stack;
struct vl_timeslot;
struct vl_top_mod_list;
struct vl_monitor;

// Verilog description objects
struct vl_desc;
struct vl_mp;
struct vl_module;
struct vl_primitive;
struct vl_prim_entry;
struct vl_port;
struct vl_port_connect;

// Module items
struct vl_decl;
struct vl_procstmt;
struct vl_cont_assign;
struct vl_specify_block;
struct vl_specify_item;
struct vl_spec_term_desc;
struct vl_path_desc;
struct vl_task;
struct vl_function;
struct vl_dlstr;
struct vl_gate_inst_list;
struct vl_mp_inst_list;

// Statements
struct vl_bassign_stmt;
struct vl_sys_task_stmt;
struct vl_begin_end_stmt;
struct vl_if_else_stmt;
struct vl_case_stmt;
struct vl_case_item;
struct vl_forever_stmt;
struct vl_repeat_stmt;
struct vl_while_stmt;
struct vl_for_stmt;
struct vl_delay_control_stmt;
struct vl_event_control_stmt;
struct vl_wait_stmt;
struct vl_send_event_stmt;
struct vl_fork_join_stmt;
struct vl_fj_break;
struct vl_task_enable_stmt;
struct vl_disable_stmt;
struct vl_deassign_stmt;

// Instances
struct vl_inst;
struct vl_gate_inst;
struct vl_gate_out;
struct vl_mp_inst;


//---------------------------------------------------------------------------
//  Exported globals
//---------------------------------------------------------------------------

namespace vl {

    // vl_print.cc
    void vl_error(const char*, ...);
    void vl_warn(const char*, ...);
    const char *vl_datestring();
    const char *vl_version();
    char *vl_fix_str(const char*);
    char *vl_strdup(const char*);

#define errout(x) cout << (x) << "\n---\n"

} // namespace vl
using namespace vl;


//---------------------------------------------------------------------------
//  Output stream overrides
//---------------------------------------------------------------------------

// Data variables and expressions
ostream &operator<<(ostream&, vl_var*);
ostream &operator<<(ostream&, vl_expr*);
ostream &operator<<(ostream&, vl_strength);
ostream &operator<<(ostream&, vl_range*);
ostream &operator<<(ostream&, vl_delay*);
ostream &operator<<(ostream&, vl_event_expr*);

// Verilog description objects
ostream &operator<<(ostream&, vl_desc*);
ostream &operator<<(ostream&, vl_module*);
ostream &operator<<(ostream&, vl_primitive*);
ostream &operator<<(ostream&, vl_port*);
ostream &operator<<(ostream&, vl_port_connect*);

// Module items
ostream &operator<<(ostream&, vl_stmt*);
ostream &operator<<(ostream&, vl_decl*);
ostream &operator<<(ostream&, vl_procstmt*);
ostream &operator<<(ostream&, vl_cont_assign*);
ostream &operator<<(ostream&, vl_specify_block*);
ostream &operator<<(ostream&, vl_specify_item*);
ostream &operator<<(ostream&, vl_spec_term_desc*);
ostream &operator<<(ostream&, vl_path_desc*);
ostream &operator<<(ostream&, vl_task*);
ostream &operator<<(ostream&, vl_function*);
ostream &operator<<(ostream&, vl_gate_inst_list*);
ostream &operator<<(ostream&, vl_mp_inst_list*);

// Statements
ostream &operator<<(ostream&, vl_bassign_stmt*);
ostream &operator<<(ostream&, vl_sys_task_stmt*);
ostream &operator<<(ostream&, vl_begin_end_stmt*);
ostream &operator<<(ostream&, vl_if_else_stmt*);
ostream &operator<<(ostream&, vl_case_stmt*);
ostream &operator<<(ostream&, vl_case_item*);
ostream &operator<<(ostream&, vl_forever_stmt*);
ostream &operator<<(ostream&, vl_repeat_stmt*);
ostream &operator<<(ostream&, vl_while_stmt*);
ostream &operator<<(ostream&, vl_for_stmt*);
ostream &operator<<(ostream&, vl_delay_control_stmt*);
ostream &operator<<(ostream&, vl_event_control_stmt*);
ostream &operator<<(ostream&, vl_wait_stmt*);
ostream &operator<<(ostream&, vl_send_event_stmt*);
ostream &operator<<(ostream&, vl_fork_join_stmt*);
ostream &operator<<(ostream&, vl_fj_break*);
ostream &operator<<(ostream&, vl_task_enable_stmt*);
ostream &operator<<(ostream&, vl_disable_stmt*);
ostream &operator<<(ostream&, vl_deassign_stmt*);

// Instances
ostream &operator<<(ostream&, vl_gate_inst*);
ostream &operator<<(ostream&, vl_mp_inst*);

// This wraps calls to copy functions to handle a null pointer. 
// Passing a null pointer directly to a method used to work ok if
// 'this' was tested before use, no longer true.
//
template <class T>
inline T *chk_copy(T*x)
{
    if (x)
        return (x->copy());
    return (0);
}

//---------------------------------------------------------------------------
//  Data variables and expressions
//---------------------------------------------------------------------------

// Name prefix for temporary variable
#define TEMP_VAR_PREF "@@_"

// Basic bit values
enum { BitL, BitH, BitDC, BitZ};

// Types of data objects, an object is Dnone until it is assigned a type
enum { Dnone, Dbit, Dint, Dtime, Dreal, Dstring, Dconcat };
typedef unsigned char Dtype;

// Types of connection, "net type" is >= REGwire
enum { REGnone, REGevent, REGparam, REGreg, REGwire, REGtri, REGtri0,
    REGtri1, REGsupply0, REGsupply1, REGwand, REGtriand, REGwor, REGtrior,
    REGtrireg };
typedef unsigned char REGtype;

// Port i/o types
enum { IOnone, IOinput, IOoutput, IOinout };
typedef unsigned char IOtype;

// Returns from bitset()
#define Lmask 0x1
#define Hmask 0x2
#define Xmask 0x4

// Struct to define a range of values
//
struct vl_array
{
    vl_array()
        {
            clear();
        }

    void clear()
        {
            a_lo_index = a_hi_index = 0;
            a_size = 0;
        }

    void set(int sz)
        {
            a_size = sz;
            a_lo_index = 0;
            a_hi_index = sz-1;
        }

    void set_lo_hi(int l, int h)
        {
            a_lo_index = l;
            a_hi_index = h;
        }

    int size()      const { return (a_size); }
    int lo_index()  const { return (a_lo_index); }
    int hi_index()  const { return (a_hi_index); }

    // vl_data.cc
    void set(vl_range*);
    bool check_range(int*, int*);

    // The following apply to a bit-field range.

    // Convert internal index to user index.
    int Btou(int i)
        {
            return (a_hi_index >= a_lo_index ? a_lo_index+i : a_lo_index-i);
        }

    // Move the range so that 0 is the end value.
    void Bnorm()
        {
            if (a_hi_index >= a_lo_index) {
                a_hi_index -= a_lo_index;
                a_lo_index = 0;
            }
            else {
                a_lo_index -= a_hi_index;
                a_hi_index = 0;
            }
        }

    // Return the internal index for the least significant value.
    int Bstart(int, int ls)
        {
            return (abs(ls - a_lo_index));
        }

    // Return the internal index for the most significant value.
    int Bend(int ms, int)
        {
            return (abs(ms - a_lo_index));
        }

    // The following apply to an array range.

    // Convert internal index to user index.
    int Atou(int i)
        {
            return (a_lo_index >= a_hi_index ? a_hi_index+i : a_hi_index-i);
        }

    // Move the range so that 0 is the end value.
    void Anorm()
        {
            if (a_lo_index >= a_hi_index)
                {
                    a_lo_index -= a_hi_index;
                    a_hi_index = 0;
                }
            else {
                a_hi_index -= a_lo_index;
                a_lo_index = 0;
            }
        }

    // Return the internal index for the least significant value.
    int Astart(int ms, int)
        {
            return (abs(ms - a_hi_index));
        }

    // Return the internal index for the most significant value.
    int Aend(int, int ls)
        {
            return (abs(ls - a_hi_index));
        }

private:
    int a_lo_index;
    int a_hi_index;
    int a_size;
};

// Drive strengths
//
enum STRength { STRnone, STRhiZ, STRsmall, STRmed, STRweak, STRlarge, STRpull,
    STRstrong, STRsupply };

struct vl_strength
{
    vl_strength()
        {
            s_str0 = STRnone;
            s_str1 = STRnone;
        }

    STRength str0()             const { return (s_str0); }
    STRength str1()             const { return (s_str1); }
    void set_str0(STRength s)   { s_str0 = s; }
    void set_str1(STRength s)   { s_str1 = s; }

    // vl_print.cc
    void print();

private:
    STRength s_str0;
    STRength s_str1;
};

// Set this to machine bit width.
#define DefBits (8*(int)sizeof(int))

// Basic data item
//
struct vl_var
{
    // vl_data.cc
    vl_var();
    vl_var(const char*, vl_range*, lsList<vl_expr*>* = 0);
    vl_var(vl_var&);
    virtual ~vl_var();

    virtual vl_var *copy();
    virtual vl_var &eval();
    virtual void chain(vl_stmt*);
    virtual void unchain(vl_stmt*);
    virtual void unchain_disabled(vl_stmt*);

    virtual vl_var *source()    { return (this); }

    void configure(vl_range*, int = RegDecl, vl_range* = 0);
    void operator=(vl_var&);
    void assign(vl_range*, vl_var*, vl_range*);
    void assign(vl_range*, vl_var*, int*, int*);

    void clear(vl_range*, int);
    void reset();
    void set(vl_bitexp_parse*);
    void set(int);
    void set(vl_time_t);
    void set(double);
    void set(char*);
    void setx(int);
    void setz(int);
    void setb(int);
    void sett(vl_time_t);
    void setbits(int);
    bool set_bit_of(int, int);
    bool set_bit_elt(int, const char*, int);
    bool set_int_elt(int, int);
    bool set_time_elt(int, vl_time_t);
    bool set_real_elt(int, double);
    bool set_str_elt(int, char*);

    void set_assigned(vl_bassign_stmt*);
    void set_forced(vl_bassign_stmt*);
    void freeze_concat();

    void default_range(int*, int*);
    bool check_net_type(REGtype);
    bool check_bit_range(int*, int*);
    void add_driver(vl_var*, int, int, int);
    int resolve_bit(int, vl_var*, int);
    void trigger();

    bool is_x();
    bool is_z();
    int bit_of(int);
    int int_bit_sel(int, int);
    vl_time_t time_bit_sel(int, int);
    double real_bit_sel(int, int);
    int bitset();
    void *element(int, int*);
    char *bit_elt(int, int*);
    int int_elt(int);
    vl_time_t time_elt(int);
    double real_elt(int);
    char *str_elt(int);

    // vl_expr.cc
    void addb(vl_var&, int);
    void addb(vl_var&, vl_time_t);
    void addb(vl_var&, vl_var&);
    void subb(vl_var&, int);
    void subb(int, vl_var&);
    void subb(vl_var&, vl_time_t);
    void subb(vl_time_t, vl_var&);
    void subb(vl_var&, vl_var&);

    // vl_print.cc
    virtual void print(ostream&);
    void print_value(ostream&, DSPtype = DSPh);
    const char *decl_type();
    int pwidth(char);
    char *bitstr();

    // Conversion operators.
    operator int()          { return (int_elt(0)); }
    operator unsigned()     { return ((unsigned int)int_elt(0)); }
    operator vl_time_t()    { return (time_elt(0)); }
    operator double()       { return (real_elt(0)); }
    operator char*()        { return (str_elt(0)); }

    const char *name()              const { return (v_name); }
    void set_name(const char *n)    { v_name = n; }

    Dtype data_type()               const { return (v_data_type); }
    void set_data_type(Dtype d)     { v_data_type = d; }

    REGtype net_type()              const { return (v_net_type); }
    void set_net_type(REGtype d)    { v_net_type = d; }

    IOtype io_type()                const { return (v_io_type); }
    void set_io_type(IOtype d)      { v_io_type = d; }

    unsigned int flags()            const { return (v_flags); }
    void set_flags(unsigned int f)  { v_flags = f; }
    void or_flags(unsigned int f)   { v_flags |= f; }
    void anot_flags(unsigned int f) { v_flags &= ~f; }

    vl_array &array()               { return (v_array); }
    vl_array &bits()                { return (v_bits); }

    union var_data
    {
        int i;                      // integer (Dint)
        int *pi;
        double r;                   // real (Dreal)
        double *pr;
        vl_time_t t;                // time (Dtime)
        vl_time_t *pt;
        char *s;                    // bit field or char string (Dbit, Dstring)
        char **ps;
        lsList<vl_expr*> *c;        // concatenation list (Dconcat)
    };

    int data_i()                    const { return (v_data.i); }
    int *data_pi()                  const { return (v_data.pi); }
    double data_r()                 const { return (v_data.r); }
    double *data_pr()               const { return (v_data.pr); }
    vl_time_t data_t()              const { return (v_data.t); }
    vl_time_t *data_pt()            const { return (v_data.pt); }
    char *data_s()                  const { return (v_data.s); }
    char **data_ps()                const { return (v_data.ps); }
    lsList<vl_expr*> *data_c()      const { return (v_data.c); }

    void set_data_i(int i)
        {
            memset(&v_data, 0, sizeof(var_data));
            v_data.i = i;
        }

    void set_data_pi(int *pi)
        {
            memset(&v_data, 0, sizeof(var_data));
            v_data.pi = pi;
        }

    void set_data_r(double r)
        {
            memset(&v_data, 0, sizeof(var_data));
            v_data.r = r;
        }

    void set_data_pr(double *pr)
        {
            memset(&v_data, 0, sizeof(var_data));
            v_data.pr = pr;
        }

    void set_data_t(vl_time_t t)
        {
            memset(&v_data, 0, sizeof(var_data));
            v_data.t = t;
        }

    void set_data_pt(vl_time_t *pt)
        {
            memset(&v_data, 0, sizeof(var_data));
            v_data.pt = pt;
        }

    void set_data_s(char *s)
        {
            memset(&v_data, 0, sizeof(var_data));
            v_data.s = s;
        }

    void set_data_ps(char **ps)
        {
            memset(&v_data, 0, sizeof(var_data));
            v_data.ps = ps;
        }

    void set_data_c(lsList<vl_expr*> *c)
        {
            memset(&v_data, 0, sizeof(var_data));
            v_data.c = c;
        }

    vl_range *range()               const { return (v_range); }
    void set_range(vl_range *r)     { v_range = r; }

    vl_strength strength()          const { return (v_strength); }
    void set_strength(vl_strength s) { v_strength = s; }
    void set_strength(STRength s0, STRength s1)
        {
            v_strength.set_str0(s0);;
            v_strength.set_str1(s1);;
        }

    vl_delay *delay()               const { return (v_delay); }
    void set_delay(vl_delay *d)     { v_delay = d; }
    vl_action_item *events()        const { return (v_events); }

private:
    static int bits_set(vl_var*, vl_range*, vl_var*, int);
    void print_drivers();
    static void probe1(vl_var*, int, int, vl_var*, int, int);
    void probe2();
    void assign_init(vl_var*, int, int);
    void assign_bit_range(int, int, vl_var*, int, int);
    void assign_bit_SS(int, int, vl_var*, int, int);
    void assign_bit_SA(int, int, vl_var*, int, int);
    void assign_bit_AS(int, int, vl_var*, int, int);
    void assign_bit_AA(int, int, vl_var*, int, int);
    void assign_int_range(int, int, vl_var*, int, int);
    void assign_int_SS(int, int, vl_var*, int, int);
    void assign_int_SA(int, int, vl_var*, int, int);
    void assign_int_AS(int, int, vl_var*, int, int);
    void assign_int_AA(int, int, vl_var*, int, int);
    void assign_time_range(int, int, vl_var*, int, int);
    void assign_time_SS(int, int, vl_var*, int, int);
    void assign_time_SA(int, int, vl_var*, int, int);
    void assign_time_AS(int, int, vl_var*, int, int);
    void assign_time_AA(int, int, vl_var*, int, int);
    void assign_real_range(int, int, vl_var*, int, int);
    void assign_real_SS(int, int, vl_var*, int, int);
    void assign_real_SA(int, int, vl_var*, int, int);
    void assign_real_AS(int, int, vl_var*, int, int);
    void assign_real_AA(int, int, vl_var*, int, int);
    void assign_string_range(int, int, vl_var*, int, int);
    void assign_string_SS(int, int, vl_var*, int, int);
    void assign_string_SA(int, int, vl_var*, int, int);
    void assign_string_AS(int, int, vl_var*, int, int);
    void assign_string_AA(int, int, vl_var*, int, int);

protected:
    // Return the effective bit width.
    //
    int bit_size()
        {
            if (v_data_type == Dbit)
                return (bits().size());
            if (v_data_type == Dint)
                return (DefBits);
            if (v_data_type == Dtime)
                return (sizeof(vl_time_t)*8);
            return (0);
        }

    const char *v_name;             // data item's name

    Dtype v_data_type;              // declared type
        // If type == Dnone, object can be coerced into any type on
        // assignment, otherwise object will retain type.  Dnone looks
        // like an int.

    REGtype v_net_type;             // type of net
        // If not REGnone, object must be a bit field, specifies type
        // of net, or reg.

    IOtype v_io_type;               // type of port
        // Specifies whether net is input, output, inout, or none.

    unsigned char v_flags;          // misc. indicators

    // values for vl_var flags field
    //
#define VAR_IN_TABLE                0x1
    // This vl_var has been placed in a symbol table.

#define VAR_PORT_DRIVER             0x2
    // This vl_var drives across an inout module port.

#define VAR_CP_ASSIGN               0x4
    // A continuous procedural assign is active.

#define VAR_F_ASSIGN                0x8
    // A force statement is active.

    vl_array v_array;               // range if array
    vl_array v_bits;                // bit field range
    var_data v_data;                // contents

    vl_range *v_range;              // array range from parser
    vl_strength v_strength;         // net assign strength
    vl_delay *v_delay;              // net delay
    vl_action_item *v_events;       // list of events to trigger
    vl_bassign_stmt *v_cassign;     // continuous procedural assignment
    lsList<vl_driver*> *v_drivers;  // driver list for net
};

#define CX_INCR 100

// A vl_var factory.
//
struct vl_var_factory
{
    struct bl {
        vl_var block[CX_INCR];
        bl *next;
    };

    vl_var_factory()
        {
            da_numused = 0;
            da_blocks = 0;
        }

    void clear()
        {
            da_numused = 0;
            while (da_blocks) {
                bl *b = da_blocks;
                da_blocks = b->next;
                delete b;
            }
        }

    vl_var & new_var()
        {
            if (da_numused == 0) {
                da_blocks = new bl;
                da_blocks->next = 0;
                da_numused = 1;
            }
            else if (da_numused == CX_INCR) {
                bl *b = new bl;
                b->next = da_blocks;
                da_blocks = b;
                da_numused = 1;
            }
            else
                da_numused++;
            return (da_blocks->block[da_numused - 1]);
        }

private:
    int da_numused;
    bl *da_blocks;
};

// Math and logical operations involving vl_var structs.
//
// vl_expr.cc
vl_var &operator*(vl_var&, vl_var&);
vl_var &operator/(vl_var&, vl_var&);
vl_var &operator%(vl_var&, vl_var&);
vl_var &operator+(vl_var&, vl_var&);
vl_var &operator-(vl_var&, vl_var&);
vl_var &operator-(vl_var&);
vl_var &operator<<(vl_var&, vl_var&);
vl_var &operator<<(vl_var&, int);
vl_var &operator>>(vl_var&, vl_var&);
vl_var &operator>>(vl_var&, int);
vl_var &operator==(vl_var&, vl_var&);
vl_var &case_eq(vl_var&, vl_var&);
vl_var &casex_eq(vl_var&, vl_var&);
vl_var &casez_eq(vl_var&, vl_var&);
vl_var &case_neq(vl_var&, vl_var&);
vl_var &operator!=(vl_var&, vl_var&);
vl_var &operator&&(vl_var&, vl_var&);
vl_var &operator||(vl_var&, vl_var&);
vl_var &operator!(vl_var&);
vl_var &reduce(vl_var&, int);
vl_var &operator<(vl_var&, vl_var&);
vl_var &operator<=(vl_var&, vl_var&);
vl_var &operator>(vl_var&, vl_var&);
vl_var &operator>=(vl_var&, vl_var&);
vl_var &operator&(vl_var&, vl_var&);
vl_var &operator|(vl_var&, vl_var&);
vl_var &operator^(vl_var&, vl_var&);
vl_var &operator~(vl_var&);
vl_var &tcond(vl_var&, vl_expr*, vl_expr*);

// General expression description.
//
struct vl_expr : public vl_var
{
    // vl_expr.cc
    vl_expr();
    vl_expr(vl_var*);
    vl_expr(int, int, double, void*, void*, void*);
    virtual ~vl_expr();

    vl_expr *copy();
    vl_var &eval();
    void chcore(vl_stmt*, int);
    vl_var *source();

    // vl_print.cc
    void print(ostream&);
    const char *symbol();

    void chain(vl_stmt *s)              { chcore(s, 0); }
    void unchain(vl_stmt *s)            { chcore(s, 1); }
    void unchain_disabled(vl_stmt *s)   { chcore(s, 2); }

    vl_range *source_range()
        {
            return  ((e_type == BitSelExpr || e_type == PartSelExpr) ?
                e_data.ide.range : 0);
        }

    union expr_data
    {
        struct ide
        {
            const char *name;
            vl_range *range;
            vl_var *var;
        } ide;
        struct func_call
        {
            const char *name;
            lsList<vl_expr*> *args;
            vl_function *func;
        } func_call;
        struct exprs
        {
            vl_expr *e1;
            vl_expr *e2;
            vl_expr *e3;
        } exprs;
        struct mcat
        {
            vl_expr *rep;
            vl_var *var;
        } mcat;
        lsList<vl_expr*> *expr_list;
        vl_sys_task_stmt *systask;
    };

    int etype()             const { return (e_type); }
    void set_etype(int t)   { e_type = t; }
    expr_data &edata()      { return (e_data); }

private:
    int         e_type;
    expr_data   e_data;
};

// List element for a net driver.
//
struct vl_driver
{
    vl_driver()
        {
            d_srcvar = 0;
            d_m_to = 0;
            d_l_to = 0;
            d_l_from = 0;
        }

    vl_driver(vl_var *d, int mt, int lt, int lf)
        {
            d_srcvar = d;
            d_m_to = mt;
            d_l_to = lt;
            d_l_from = lf;
        }

    void set_m_to(int i)    { d_m_to = i; }

    vl_var *srcvar()        { return (d_srcvar); }
    int m_to()              const { return (d_m_to); }
    int l_to()              const { return (d_l_to); }
    int l_from()            const { return (d_l_from); }

private:
    vl_var *d_srcvar;       // source vl_var
    int d_m_to;             // high index in target to assign
    int d_l_to;             // low index in target to assign
    int d_l_from;           // low index in source
};

// Range descriptor for bit field or array.
//
struct vl_range
{
    vl_range(vl_expr *l, vl_expr *r)
        {
            r_left = l;
            r_right = r;
        }

    ~vl_range()
        {
            delete r_left;
            delete r_right;
        }

    vl_expr *left()         { return (r_left); }
    vl_expr *right()        { return (r_right); }

    // vl_data.cc
    vl_range *copy();
    bool eval(int*, int*);
    vl_range *reval();
    int width();

private:
    vl_expr *r_left;
    vl_expr *r_right;
};

// Delay specification
//
struct vl_delay
{
    vl_delay()
        {
            d_delay1 = 0;
            d_list = 0;
        }

    vl_delay(vl_expr *d)
        {
            d_delay1 = d;
            d_list = 0;
        }

    vl_delay(lsList<vl_expr*> *l)
        {
            d_delay1 = 0;
            d_list = l;
        }

    ~vl_delay()
        {
            delete d_delay1;
            delete_list(d_list);
        }

    // vl_data.cc
    vl_delay *copy();
    vl_time_t eval();

    vl_expr *delay1()           const { return (d_delay1); }
    lsList<vl_expr*> *list()    const { return (d_list); }

    void set_delay1(vl_expr *e)         { d_delay1 = e; }
    void set_list(lsList<vl_expr*> *l)  { d_list = l; }

private:
    vl_expr             *d_delay1;
    lsList<vl_expr*>    *d_list;
};

// Event expression description
//
struct vl_event_expr
{
    vl_event_expr()
        {
            e_type = 0;
            e_count = 0;
            e_expr = 0;
            e_list = 0;
            e_repeat = 0;
        }

    vl_event_expr(int t, vl_expr *e)
        {
            e_type = t;
            e_count = 0;
            e_expr = e;
            e_list = 0;
            e_repeat = 0;
        }

    ~vl_event_expr()
        {
            delete e_expr;
            delete_list(e_list);
            delete e_repeat;
        }

    // vl_data.cc
    vl_event_expr *copy();
    void init();
    bool eval(vl_simulator*);
    void chain(vl_action_item*);
    void unchain(vl_action_item*);
    void unchain_disabled(vl_stmt*);

    int type()                      const { return (e_type); }
    int count()                     const { return (e_count); }
    vl_expr *expr()                 const { return (e_expr); }
    lsList<vl_event_expr*> *list()  const { return (e_list); }
    vl_expr *repeat()               const { return (e_repeat); }

    void set_count(int i)           { e_count = i; }
    void set_expr(vl_expr *e)       { e_expr = e; }
    void set_list(lsList<vl_event_expr*> *l) { e_list = l; }
    void set_repeat(vl_expr *e)     { e_repeat = e; }

private:
    int         e_type;
    int         e_count;
    vl_expr     *e_expr;
    lsList<vl_event_expr*> *e_list;
    vl_expr     *e_repeat;
};


//---------------------------------------------------------------------------
//  Parser objects
//---------------------------------------------------------------------------

enum vlERRtype { ERR_OK, ERR_WARN, ERR_COMPILE, ERR_INTERNAL }; 

inline struct vl_parser *VP();

// Main class for Verilog parser.
//
struct vl_parser
{
    friend inline vl_parser *VP() { return (vl_parser::ptr()); }
    friend int yylex();
    friend int yyparse();
    friend void yyerror(const char*);

    // vl_parse.cc
    vl_parser();
    ~vl_parser();

    bool parse(int, char**);
    bool parse(FILE*);
    void clear();
    void error(vlERRtype, const char*, ...);
    bool parse_timescale(const char*);
    void push_context(vl_module*);
    void push_context(vl_primitive*);
    void push_context(vl_function*);
    void pop_context();

    // vl_print.cc
    void print(ostream&);

    vl_desc *get_description()
        {
            vl_desc *d = p_description;
            p_description = 0;
            return d;
        }

    const char *filename()                  { return (p_filename); }
    void set_filename(const char *fn)       { p_filename = fn; }

    vl_stack_t<vl_module*> *module_stack()  { return (p_module_stack); }
    vl_stack_t<FILE*> *file_stack()         { return (p_file_stack); }
    vl_stack_t<const char*> *fname_stack()  { return (p_fname_stack); }
    vl_stack_t<int> *lineno_stack()         { return (p_lineno_stack); }
    vl_stack_t<const char*> *dir_stack()    { return (p_dir_stack); }

    table<const char*> *macros()            { return (p_macros); }

    jmp_buf *jbuf()                         { return (&p_jbuf); }

private:
    static vl_parser *ptr()
        {
            if (!p_parser)
                on_null_ptr();
            return (p_parser);
        }

    static void on_null_ptr();

    double                  p_tunit;
    double                  p_tprec;
    vl_desc                 *p_description;
    vl_context              *p_context;
    const char              *p_filename;
    vl_stack_t<vl_module*>  *p_module_stack;
    vl_stack_t<FILE*>       *p_file_stack;
    vl_stack_t<const char*> *p_fname_stack;
    vl_stack_t<int>         *p_lineno_stack;
    vl_stack_t<const char*> *p_dir_stack;
    table<const char*>      *p_macros;
    char                    p_ifelse_stack[MAXSTRLEN];
    int                     p_ifelseSP;
    jmp_buf                 p_jbuf;
    bool                    p_no_go;
    bool                    p_verbose;

    static vl_parser        *p_parser;
};

// Parser class for bit field.
//
struct vl_bitexp_parse : public vl_var
{
    vl_bitexp_parse()
        {
            v_data_type = Dbit;
            v_data.s = brep = new char[MAXSTRLEN];
        }

    // vl_parse.cc
    void bin(char*);
    void dec(char*);
    void oct(char*);
    void hex(char*);

private:
    char *brep;
};

// Range or type temporary storage.
//
struct range_or_type
{
    range_or_type()
        {
            type = 0;
            range = 0;
        }

    range_or_type(int t, vl_range *r)
        {
            type = t;
            range = r;
        }

    ~range_or_type()
        {
            delete range;
        }

    int         type;
    vl_range    *range;
};

// Multi-concatenation temporary container object.
//
struct multi_concat
{
    multi_concat()
        {
            rep = 0;
            concat = 0;
        }

    multi_concat(vl_expr *r, lsList<vl_expr*> *c)
        {
            rep = r;
            concat = c;
        }

    vl_expr *rep;
    lsList<vl_expr*> *concat;
};


//---------------------------------------------------------------------------
//  Simulator objects
//---------------------------------------------------------------------------

enum VLdelayType { DLYmin, DLYtyp, DLYmax };
enum VLstopType { VLrun, VLstop, VLabort };

inline struct vl_simulator *VS();

// Main class for simulator.
//
struct vl_simulator
{
    friend inline vl_simulator *VS() { return (vl_simulator::ptr()); }

    // vl_sim.cc
    vl_simulator();
    ~vl_simulator();

    bool initialize(vl_desc*, VLdelayType = DLYtyp, int = 0);
    bool simulate();
    VLstopType step();
    bool assign_to(vl_var*, double, double, double, int, int);
    void close_files();
    void flush_files();
    void push_context(vl_mp*);
    void push_context(vl_module*);
    void push_context(vl_primitive*);
    void push_context(vl_task*);
    void push_context(vl_function*);
    void push_context(vl_begin_end_stmt*);
    void push_context(vl_fork_join_stmt*);
    void pop_context();

    // vl_sys.cc
    char *dumpindex(vl_var*);
    vl_var &sys_time(vl_sys_task_stmt*, lsList<vl_expr*>*);
    vl_var &sys_printtimescale(vl_sys_task_stmt*, lsList<vl_expr*>*);
    vl_var &sys_timeformat(vl_sys_task_stmt*, lsList<vl_expr*>*);
    vl_var &sys_display(vl_sys_task_stmt*, lsList<vl_expr*>*);
    vl_var &sys_monitor(vl_sys_task_stmt*, lsList<vl_expr*>*);
    vl_var &sys_monitor_on(vl_sys_task_stmt*, lsList<vl_expr*>*);
    vl_var &sys_monitor_off(vl_sys_task_stmt*, lsList<vl_expr*>*);
    vl_var &sys_stop(vl_sys_task_stmt*, lsList<vl_expr*>*);
    vl_var &sys_finish(vl_sys_task_stmt*, lsList<vl_expr*>*);
    vl_var &sys_noop(vl_sys_task_stmt*, lsList<vl_expr*>*);
    vl_var &sys_fopen(vl_sys_task_stmt*, lsList<vl_expr*>*);
    vl_var &sys_fclose(vl_sys_task_stmt*, lsList<vl_expr*>*);
    vl_var &sys_fmonitor(vl_sys_task_stmt*, lsList<vl_expr*>*);
    vl_var &sys_fmonitor_on(vl_sys_task_stmt*, lsList<vl_expr*>*);
    vl_var &sys_fmonitor_off(vl_sys_task_stmt*, lsList<vl_expr*>*);
    vl_var &sys_fdisplay(vl_sys_task_stmt*, lsList<vl_expr*>*);
    vl_var &sys_random(vl_sys_task_stmt*, lsList<vl_expr*>*);
    vl_var &sys_dumpfile(vl_sys_task_stmt*, lsList<vl_expr*>*);
    vl_var &sys_dumpvars(vl_sys_task_stmt*, lsList<vl_expr*>*);
    vl_var &sys_dumpall(vl_sys_task_stmt*, lsList<vl_expr*>*);
    vl_var &sys_dumpon(vl_sys_task_stmt*, lsList<vl_expr*>*);
    vl_var &sys_dumpoff(vl_sys_task_stmt*, lsList<vl_expr*>*);
    vl_var &sys_readmemb(vl_sys_task_stmt*, lsList<vl_expr*>*);
    vl_var &sys_readmemh(vl_sys_task_stmt*, lsList<vl_expr*>*);

    void abort()                        { s_stop = VLabort; }
    void finish()                       { s_stop = VLstop; }

    void set_description(vl_desc *d)    { s_description = d; }
    void set_time(vl_time_t t)          { s_time = t; }
    void set_steptime(vl_time_t t)      { s_steptime = t; }
    void set_context(vl_context *c)     { s_context = c; }
    void set_next_actions(vl_action_item *a) { s_next_actions = a; }
    void set_fj_end(vl_action_item *a)  { s_fj_end = a; }

    vl_desc *description()              const { return (s_description); }
    VLdelayType dmode()                 const { return (s_dmode); }
    VLstopType stop()                   const { return (s_stop); }
    vl_time_t time()                    const { return (s_time); }
    vl_context *context()               const { return (s_context); }
    vl_timeslot *timewheel()            const { return (s_timewheel); }
    vl_action_item *next_actions()      const { return (s_next_actions); }
    vl_action_item *fj_end()            const { return (s_fj_end); }
    int dbg_flags()                     const { return (s_dbg_flags); }
    vl_var &time_data()                 { return (s_time_data); }
    vl_top_mod_list *top_modules()      const { return (s_top_modules); }
    int dmpstatus()                     const { return (s_dmpstatus); }

private:
    static vl_simulator *ptr()
        {
            if (!s_simulator)
                on_null_ptr();
            return (s_simulator);
        }

    // vl_sim.cc
    static void on_null_ptr();

    // vl_sys,cc
    void do_dump();

    // vl_print.cc
    bool monitor_change(lsList<vl_expr*>*);
    void display_print(lsList<vl_expr*>*, ostream&, DSPtype, unsigned int);
    void fdisplay_print(lsList<vl_expr*>*, DSPtype, unsigned int);

    vl_desc         *s_description;     // the verilog deck to simulate
    VLdelayType     s_dmode;            // min/typ/max delay mode
    VLstopType      s_stop;             // stop simulation code
    bool            s_first_point;      // set for initial time point only
    bool            s_monitor_state;    // monitor enabled flag
    bool            s_fmonitor_state;   // fmonitor enabled flag
    vl_monitor      *s_monitors;        // list of monitors
    vl_monitor      *s_fmonitors;       // list of fmonitors
    vl_time_t       s_time;             // accumulated delay for setup
    vl_time_t       s_steptime;         // accumulating time when stepping
    vl_context      *s_context;         // evaluation context
    vl_timeslot     *s_timewheel;       // time sorted events for evaluation
	vl_action_item  *s_next_actions;    // actions to do first at next time
    vl_action_item  *s_fj_end;          // fork/join return context list
    vl_top_mod_list *s_top_modules;     // pointer to top module
    int             s_dbg_flags;        // debugging flags
    vl_var          s_time_data;        // for @($time) events
    ostream         *s_channels[32];    // fhandles
    ostream         *s_dmpfile;         // VCD dump file name
    vl_context      *s_dmpcx;           // context of dump
    int             s_dmpdepth;         // depth of dump
    int             s_dmpstatus;        // status flags for dump;
// s_dmpstatus flags
#define DMP_ACTIVE 0x1
#define DMP_HEADER 0x2
#define DMP_ON     0x4
#define DMP_ALL    0x8
    vl_var          **s_dmpdata;        // stuff for $dumpvars
    vl_var          *s_dmplast;
    int             s_dmpsize;
    int             s_dmpindx;
    int             s_tfunit;           // stuff for $timeformat
    int             s_tfprec;
    const char      *s_tfsuffix;
    int             s_tfwidth;

    static vl_simulator *s_simulator;

public:
    vl_var_factory  var_factory;
};

// Context list for simulator.
//
struct vl_context
{
    vl_context(vl_context *p=0)
        {
            c_module = 0;
            c_primitive = 0;
            c_task = 0;
            c_function = 0;
            c_block = 0;
            c_fjblk = 0;
            c_parent = p;
        }

    static void destroy(vl_context *c)
        {
            while (c) {
                vl_context *cx = c;
                c = c->c_parent;
                delete cx;
            }
        }

    void set_module(vl_module *m)           { c_module = m; }
    void set_primitive(vl_primitive *p)     { c_primitive = p; }
    void set_task(vl_task *t)               { c_task = t; }
    void set_function(vl_function *f)       { c_function = f; }
    void set_block(vl_begin_end_stmt *b)    { c_block = b; }
    void set_fjblk(vl_fork_join_stmt *f)    { c_fjblk = f; }

    vl_module *module()         const { return (c_module); }
    vl_primitive *primitive()   const { return (c_primitive); }
    vl_task *task()             const { return (c_task); }
    vl_function *function()     const { return (c_function); }
    vl_begin_end_stmt *block()  const { return (c_block); }
    vl_fork_join_stmt *fjblk()  const { return (c_fjblk); }
    vl_context *parent()        const { return (c_parent); }

    // vl_sim.cc
    vl_context *copy();
    bool in_context(vl_stmt*);
    vl_var *lookup_var(const char*, bool);
    vl_stmt *lookup_block(const char*);
    vl_task *lookup_task(const char*);
    vl_function *lookup_func(const char*);
    vl_inst *lookup_mp(const char*);
    table<vl_var*> *resolve_st(const char**, bool);
    table<vl_task*> *resolve_task(const char**);
    table<vl_function*> *resolve_func(const char**);
    table<vl_inst*> *resolve_mp(const char**);
    bool resolve_cx(const char**, vl_context&, bool);
    bool resolve_path(const char*, bool);
    vl_module *currentModule();
    vl_primitive *currentPrimitive();
    vl_function *currentFunction();
    vl_task *currentTask();

    // vl_print.cc
    void print(ostream&);
    char *hiername();

private:
    vl_module           *c_module;
    vl_primitive        *c_primitive;
    vl_task             *c_task;
    vl_function         *c_function;
    vl_begin_end_stmt   *c_block;
    vl_fork_join_stmt   *c_fjblk;
    vl_context          *c_parent;
};

// This, and the enum below, provide return values for the eval()
// function of vl_stmt and derivatives.
// 
union vl_event
{
    vl_event_expr   *event;
    vl_time_t       time;
};

enum EVtype { EVnone, EVdelay, EVevent };

// Base class for Verilog module items, statements, and action.
//
struct vl_stmt
{
    vl_stmt() : st_type(0), st_flags(0) { }

    virtual ~vl_stmt()                  { }
    virtual vl_stmt *copy()             { return (0); }
    virtual void init()                 { }
    virtual void setup(vl_simulator*)   { }
    virtual EVtype eval(vl_event*, vl_simulator*) { return (EVnone); }
    virtual void disable(vl_stmt*)      { }
    virtual void dump(ostream&, int)    { }
    virtual void print(ostream&)        { }
    virtual const char *lterm()         const { return ("\n"); }
    virtual void dumpvars(ostream&, vl_simulator*) { }

    int type()                          const { return (st_type); }
    void set_type(int t)                { st_type = t; }
    unsigned int flags()                const { return (st_flags); }
    void set_flags(unsigned int f)      { st_flags = f; }
    void or_flags(unsigned int f)       { st_flags |= f; }
    void anot_flags(unsigned int f)     { st_flags &= ~f; }

protected:
    int             st_type;
    unsigned int    st_flags;

    // The flags field of a vl_stmt is used in different ways by the
    // various derived objects.  All flags used are listed here

#define SIM_INTERNAL       0x1
    // Statement was created for internal use during simulation, free
    // after execution.

#define BAS_SAVE_RHS       0x2
    // This applies to vl_bassign_stmt structs.  When set, the rhs
    // field is not deleted when the object is deleted.

#define BAS_SAVE_LHS       0x4
    // This applies to vl_bassign_stmt structs.  When set, the lhs
    // field is not deleted when the object is deleted.

#define AI_DEL_STMT        0x8
    // This applies to vl_action_item structs.  When set, the stmt is
    // deleted when the action item is deleted.  The stmt has the
    // SIM_INTERNAL flag set, but we can't check for this directly
    // since the stmt may already be free.

#define DAS_DEL_VAR        0x8
    // Similar to above, for vl_deassign_stmt and the var field.
};

// Action node for time wheel and event queue.
//
struct vl_action_item : public vl_stmt
{
    // vl_sim.cc
    vl_action_item(vl_stmt*, vl_context*);
    virtual ~vl_action_item();

    vl_action_item *copy();
    vl_action_item *purge(vl_stmt*);
    EVtype eval(vl_event*, vl_simulator*);
    void print(ostream&);

    static void destroy(vl_action_item *a)
        {
            while (a) {
                vl_action_item *ax = a;
                a = a->ai_next;
                delete ax;
            }
        }

    void set_next(vl_action_item *a)    { ai_next = a; }
    void set_stmt(vl_stmt *s)           { ai_stmt = s; }
    void set_stack(vl_stack *s)         { ai_stack = s; }
    void set_event(vl_event_expr *e)    { ai_event = e; }

    vl_action_item *next()      const { return (ai_next); }
    vl_stmt *stmt()             const { return (ai_stmt); }
    vl_stack *stack()           const { return (ai_stack); }
    vl_event_expr *event()      const { return (ai_event); }
    vl_context *context()       const { return (ai_context); }

private:
    vl_action_item  *ai_next;
    vl_stmt         *ai_stmt;       // statement to evaluate
    vl_stack        *ai_stack;      // stack, if no stmt
    vl_event_expr   *ai_event;      // used in event queue
    vl_context      *ai_context;    // context for evaluation
};

// Types of block in stack.
enum ABtype { Sequential, Fork, Fence };

// Structure to store a block of actions.
//
struct vl_sblk
{
    vl_sblk()
        {
            sb_type = Sequential;
            sb_actions = 0;
            sb_fjblk = 0;
        }

    void set_type(ABtype t)                 { sb_type = t; }
    void set_actions(vl_action_item *a)     { sb_actions = a; }
    void set_fjblk(vl_stmt *f)              { sb_fjblk = f; }

    ABtype type()               const { return (sb_type); }
    vl_action_item *actions()   const { return (sb_actions); }
    vl_stmt *fjblk()            const { return (sb_fjblk); }

private:
    ABtype          sb_type;
    vl_action_item  *sb_actions;
    vl_stmt         *sb_fjblk;      // parent block, if fork/join
};

// Stack object for actions.
//
struct vl_stack
{
    vl_stack(vl_sblk *a, int n)
        {
            st_acts = new vl_sblk[n];
            st_num = n;
            for (int i = 0; i < n; i++)
                st_acts[i] = a[i];
        }

    ~vl_stack()
        {
            delete [] st_acts;
        }

    // vl_sim.cc
    vl_stack *copy();
    void print(ostream&);

    vl_sblk &act(int i)     const { return (st_acts[i]); }
    int num()               const { return (st_num); }

private:
    vl_sblk     *st_acts;
    int         st_num;     // depth of stack
};

// List head for actions at a time point.
//
struct vl_timeslot
{
    // vl_sim.cc
    vl_timeslot(vl_time_t);
    ~vl_timeslot();

    vl_timeslot *find_slot(vl_time_t);
    void append(vl_time_t, vl_action_item*);
    void append_trig(vl_time_t, vl_action_item*);
    void append_zdly(vl_time_t, vl_action_item*);
    void append_nbau(vl_time_t, vl_action_item*);
    void append_mon(vl_time_t, vl_action_item*);
    void eval_slot(vl_simulator*);
    void add_next_actions(vl_simulator*);
    void do_actions(vl_simulator*);
    void purge(vl_stmt*);
    void print(ostream&);

    static void destroy(vl_timeslot *t)
        {
            while (t) {
                vl_timeslot *tx = t;
                t = t->next();
                delete tx;
            }
        }

    void set_next(vl_timeslot *ts)      { ts_next = ts; }
    void set_actions(vl_action_item *a) { ts_actions = a; }

    vl_timeslot *next()                 const { return (ts_next); }
    vl_time_t time()                    const { return (ts_time); }
    vl_action_item *actions()           const { return (ts_actions); }
    vl_action_item *trig_actions()      const { return (ts_trig_actions); }
    vl_action_item *zdly_actions()      const { return (ts_zdly_actions); }
    vl_action_item *nbau_actions()      const { return (ts_nbau_actions); }
    vl_action_item *mon_actions()       const { return (ts_mon_actions); }

private:
    vl_timeslot     *ts_next;
    vl_time_t       ts_time;
    vl_action_item  *ts_actions;        // "active" events
    vl_action_item  *ts_trig_actions;   // triggered events
    vl_action_item  *ts_zdly_actions;   // "inactive" events
    vl_action_item  *ts_nbau_actions;   // "non-blocking assign update" events
    vl_action_item  *ts_mon_actions;    // "monitor" events
    vl_action_item  *ts_a_end;      // end of actions list
    vl_action_item  *ts_t_end;      // end of triggered events list
    vl_action_item  *ts_z_end;      // end of zdly_actions list
    vl_action_item  *ts_n_end;      // end of nbau_actions list
    vl_action_item  *ts_m_end;      // end of mon_actions list
};

// List multiple 'top' modules.
//
struct vl_top_mod_list
{
    vl_top_mod_list(int n, vl_module **m)
        {
            tm_num = n;
            tm_mods = m;
        }

    ~vl_top_mod_list()
        {
            delete [] tm_mods;
        }

    void set_mod(int i, vl_module *m)   { tm_mods[i] = m; }

    int num()               const { return (tm_num); }
    vl_module *mod(int i)   const { return (tm_mods[i]); }

private:
    int         tm_num;
    vl_module   **tm_mods;
};

// Monitor context list element.
//
struct vl_monitor
{
    vl_monitor(vl_context *c, lsList<vl_expr*> *a, DSPtype t)
        {
            m_next = 0;
            m_cx = c;
            m_args = a;
            m_dtype = t;
        }

    ~vl_monitor()
        {
            // delete args ?
            vl_context::destroy(m_cx);
        }

    void set_next(vl_monitor *n)    { m_next = n; }

    vl_monitor *next()      const { return (m_next); }
    vl_context *cx()        const { return (m_cx); }
    lsList<vl_expr*> *args() const { return (m_args); }
    DSPtype dtype()         const { return (m_dtype); }

private:
    vl_monitor          *m_next;
    vl_context          *m_cx;
    lsList<vl_expr*>    *m_args;
    DSPtype             m_dtype;
};


//---------------------------------------------------------------------------
//  Verilog description objects
//---------------------------------------------------------------------------

// Main class for a Verilog description.
//
struct vl_desc
{
    // vl_sim.cc
    vl_desc();
    ~vl_desc();

    void dump(ostream&);
    static vl_gate_inst_list *add_gate_inst(int, vl_dlstr*,
        lsList<vl_gate_inst*>*);
    vl_stmt *add_mp_inst(char*, vl_dlstr*, lsList<vl_mp_inst*>*);

    void set_tstep(double t)        { d_tstep = t; }

    double tstep()                  const { return (d_tstep); }
    lsList<vl_module*> *modules()   const { return (d_modules); }
    lsList<vl_primitive*> *primitives() const { return (d_primitives); }
    table<vl_mp*> *mp_st()          const { return (d_mp_st); }
    set_t &mp_undefined()           { return (d_mp_undefined); }

private:
    double                  d_tstep;
    lsList<vl_module*>      *d_modules;
    lsList<vl_primitive*>   *d_primitives;
    table<vl_mp*>           *d_mp_st;
    set_t                   d_mp_undefined;
};

// Base for modules and primitives.
//
struct vl_mp
{
    vl_mp() : mp_type(0), mp_inst_count(0), mp_name(0), mp_ports(0),
        mp_sig_st(0), mp_instance(0) { }

    virtual ~vl_mp()
        {
            delete [] mp_name;
            delete_list(mp_ports);
            delete_table(mp_sig_st);
        }

    virtual vl_mp *copy()               { return (0); }
    virtual void init()                 { }
    virtual void dump(ostream&)         { }
    virtual void dumpvars(ostream&, vl_simulator*) { }
    virtual void setup(vl_simulator*)   { }

    void set_type(int t)                { mp_type = t; }
    void set_ports(lsList<vl_port*> *p) { mp_ports = p; }
    void set_sig_st(table<vl_var*> *t)  { mp_sig_st = t; }
    void set_instance(vl_mp_inst *i)    { mp_instance = i; }

    void inc_inst_count()               { mp_inst_count++; }

    int type()                  const { return (mp_type); }
    int inst_count()            const { return (mp_inst_count); }
    const char *name()          const { return (mp_name); }
    lsList<vl_port*> *ports()   const { return (mp_ports); }
    table<vl_var*> *sig_st()    const { return (mp_sig_st); }
    vl_mp_inst *instance()      const { return (mp_instance); }

protected:
    int                 mp_type;
    int                 mp_inst_count; // number of instances of this primitive
    const char          *mp_name;
    lsList<vl_port*>    *mp_ports;
    table<vl_var*>      *mp_sig_st;
    vl_mp_inst          *mp_instance;  // copy of master
};

// Module description.
//
struct vl_module : public vl_mp
{
    // vl_obj.cc
    vl_module();
    vl_module(vl_desc *desc, char*, lsList<vl_port*>*, lsList<vl_stmt*>*);
    ~vl_module();

    vl_module *copy();
    void init();

    // vl_sim.cc
    void dump(ostream&);
    void sort_moditems();
    void setup(vl_simulator*);

    // vl_sys.cc
    void dumpvars(ostream&, vl_simulator*);

    void set_tunit(double t)                    { m_tunit = t; }
    void set_tprec(double t)                    { m_tprec = t; }
    void set_mod_items(lsList<vl_stmt*> *i)     { m_mod_items = i; }
    void set_inst_st(table<vl_inst*> *t)        { m_inst_st = t; }
    void set_func_st(table<vl_function*> *t)    { m_func_st = t; }
    void set_task_st(table<vl_task*> *t)        { m_task_st = t; }
    void set_blk_st(table<vl_stmt*> *t)         { m_blk_st = t; }

    double              tunit()         const { return (m_tunit); }
    double              tprec()         const { return (m_tprec); }
    lsList<vl_stmt*>    *mod_items()    const { return (m_mod_items); }
    table<vl_inst*>     *inst_st()      const { return (m_inst_st); }
    table<vl_function*> *func_st()      const { return (m_func_st); }
    table<vl_task*>     *task_st()      const { return (m_task_st); }
    table<vl_stmt*>     *blk_st()       const { return (m_blk_st); }

private:
    double              m_tunit;
    double              m_tprec;
    lsList<vl_stmt*>    *m_mod_items;
    table<vl_inst*>     *m_inst_st;
    table<vl_function*> *m_func_st;
    table<vl_task*>     *m_task_st;
    table<vl_stmt*>     *m_blk_st;
};

// User-defined primitive description.
//
struct vl_primitive : public vl_mp
{
    vl_primitive();
    vl_primitive(vl_desc*, char*);
    ~vl_primitive();

    void init_table(lsList<vl_port*>*, lsList<vl_decl*>*, vl_bassign_stmt*,
        lsList<vl_prim_entry*>*);
    vl_primitive *copy();
    void init();
    void dump(ostream&);
    void dumpvars(ostream&, vl_simulator*);
    void setup(vl_simulator*);
    bool primtest(unsigned char, bool);
    const char *symbol(unsigned char);

    void set_seq_init(bool b)                   { p_seq_init = b; }
    void set_iodata(int i, vl_var *v)           { p_iodata[i] = v; }
    void set_lastval(int i, unsigned char c)    { p_lastvals[i] = c; }

    lsList<vl_decl*>    *decls()            const { return (p_decls); }
    vl_bassign_stmt     *initial()          const { return (p_initial); }
    unsigned char       *ptable()           const { return (p_ptable); }
    int                 rows()              const { return (p_rows); }
    bool                seq_init()          const { return (p_seq_init); }
    vl_var              *iodata(int i)      { return (p_iodata[i]); }
    unsigned char       lastval(int i)      const { return (p_lastvals[i]);}

private:
    lsList<vl_decl*>    *p_decls;
    vl_bassign_stmt     *p_initial;
    unsigned char       *p_ptable;                  // entry table
    int                 p_rows;                     // array depth
    bool                p_seq_init;                 // set when initialized
    vl_var              *p_iodata[MAXPRIMLEN];      // i/o data pointers
    unsigned char       p_lastvals[MAXPRIMLEN];     // previous values
};

// Primitive table entry.
//
struct vl_prim_entry
{
    vl_prim_entry()
        {
            memset(inputs, PrimNone, MAXPRIMLEN);
            state = 0;
            next_state = 0;
        }

    vl_prim_entry(lsList<int>*, unsigned char, unsigned char);

    unsigned char inputs[MAXPRIMLEN];
    unsigned char state;
    unsigned char next_state;
};

// Port description for modules and primitives.
//
struct vl_port
{
    vl_port()
        {
            p_type = 0;
            p_name = 0;
            p_port_exp = 0;
        }

    vl_port(int, char*, lsList<vl_var*>*);
    ~vl_port();
    vl_port *copy();

    int             type()              const { return (p_type); }
    const char      *name()             const { return (p_name); }
    lsList<vl_var*> *port_exp()         const { return (p_port_exp); }

private:
    int             p_type;
    const char      *p_name;
    lsList<vl_var*> *p_port_exp;
};

// Port connection description.
//
struct vl_port_connect
{
    vl_port_connect()
        {
            pc_type = 0;
            pc_name = 0;
            pc_expr = 0;
            pc_i_assign = 0;
            pc_o_assign = 0;
        }

    vl_port_connect(int, char*, vl_expr*);
    ~vl_port_connect();
    vl_port_connect *copy();

    void set_i_assign(vl_bassign_stmt *b)       { pc_i_assign = b; }
    void set_o_assign(vl_bassign_stmt *b)       { pc_o_assign = b; }

    int             type()                  const { return (pc_type); }
    const char      *name()                 const { return (pc_name); }
    vl_expr         *expr()                 const { return (pc_expr); }
    vl_bassign_stmt *i_assign()             const { return (pc_i_assign); }
    vl_bassign_stmt *o_assign()             const { return (pc_o_assign); }

private:
    int             pc_type;
    const char      *pc_name;
    vl_expr         *pc_expr;
    vl_bassign_stmt *pc_i_assign;       // used in arg passing
    vl_bassign_stmt *pc_o_assign;       // used in arg passing
};


//---------------------------------------------------------------------------
//  Module items
//---------------------------------------------------------------------------

// Basic declaration.
//
struct vl_decl : public vl_stmt
{
    vl_decl()
        {
            d_range = 0;
            d_ids = 0;
            d_list = 0;
            d_delay = 0;
        }

    vl_decl(int, vl_strength, vl_range*, vl_delay*,
        lsList<vl_bassign_stmt*>*, lsList<vl_var*>*);
    vl_decl(int, vl_range*, lsList<vl_var*>*);
    vl_decl(int, lsList<char*>*);
    vl_decl(int, vl_range*, lsList<vl_bassign_stmt*>*);
    ~vl_decl();

    vl_decl *copy();
    table<vl_var*> *symtab(vl_var*);
    void var_setup(vl_var*, int);
    void init();
    void setup(vl_simulator*);
    void print(ostream&);
    const char *decl_type();

    const char              *lterm()        const { return (";\n"); }

    vl_range                *range()        const { return (d_range); }
    lsList<vl_var*>         *ids()          const { return (d_ids); }
    lsList<vl_bassign_stmt*> *list()        const { return (d_list); }
    vl_delay                *delay()        const { return (d_delay); }
    const vl_strength       strength()      const { return (d_strength); }

private:
    vl_range                *d_range;
    lsList<vl_var*>         *d_ids;
    lsList<vl_bassign_stmt*> *d_list;
    vl_delay                *d_delay;
    vl_strength             d_strength;
};

// Process statememt (always/initial) description.
//
struct vl_procstmt : public vl_stmt
{
    vl_procstmt()
        {
            pr_stmt = 0;
            pr_lasttime = (vl_time_t)-1;
        }

    vl_procstmt(int, vl_stmt*);
    ~vl_procstmt();

    vl_procstmt *copy();
    void init();
    void setup(vl_simulator*);
    EVtype eval(vl_event*, vl_simulator*);
    void print(ostream&);
    void dumpvars(ostream&, vl_simulator*);

    const char  *lterm()            const { return (""); }

    vl_stmt     *stmt()             const { return (pr_stmt); }
    vl_time_t   lasttime()          const { return (pr_lasttime); }

private:
    vl_stmt     *pr_stmt;
    vl_time_t   pr_lasttime;    // used for 'always' stmt
};

// Continuous assignment.
//
struct vl_cont_assign : public vl_stmt
{
    vl_cont_assign()
        {
            c_delay = 0;
            c_assigns = 0;
        }

    vl_cont_assign(vl_strength, vl_delay*, lsList<vl_bassign_stmt*>*);
    ~vl_cont_assign();

    vl_cont_assign *copy();
    void setup(vl_simulator*);
    void print(ostream&);

    const char                  *lterm()        const { return (";\n"); }

    vl_delay                    *delay()        const { return (c_delay); }
    lsList<vl_bassign_stmt*>    *assigns()      const { return (c_assigns); }
    const vl_strength           strength()      const { return (c_strength); }

private:
    vl_delay                    *c_delay;
    lsList<vl_bassign_stmt*>    *c_assigns;
    vl_strength                 c_strength;
};

// Specify block description.
//
struct vl_specify_block : public vl_stmt
{
    vl_specify_block(lsList<vl_specify_item*>*);
    ~vl_specify_block();

    vl_specify_block *copy();
    void print(ostream&);

    lsList<vl_specify_item*> *items()   const { return (sb_items); }

private:
    lsList<vl_specify_item*> *sb_items;
};

// Specify block item description.
//
struct vl_specify_item
{
    vl_specify_item(int);
    vl_specify_item(int, lsList<vl_bassign_stmt*>*);
    vl_specify_item(int, vl_path_desc*, lsList<vl_expr*>*);
    vl_specify_item(int, vl_expr*, lsList<vl_spec_term_desc*>*, int,
        lsList<vl_spec_term_desc*>*, lsList<vl_expr*>*);
    vl_specify_item(int, vl_expr*, int, lsList<vl_spec_term_desc*>*,
        lsList<vl_spec_term_desc*>*, int, vl_expr*, lsList<vl_expr*>*);
    ~vl_specify_item();

    vl_specify_item *copy();
    void print(ostream&);

    const char                  *lterm()        const { return (";\n"); }

    int                         type()          const { return (si_type); }
    lsList<vl_bassign_stmt*>    *params()       const { return (si_params); }
    vl_path_desc                *lhs()          const { return (si_lhs); }
    lsList<vl_expr*>            *rhs()          const { return (si_rhs); }
    vl_expr                     *expr()         const { return (si_expr); }
    int                         pol()           const { return (si_pol); }
    int                         edge_id()       const { return (si_edge_id); }
    lsList<vl_spec_term_desc*>  *list1()        const { return (si_list1); }
    lsList<vl_spec_term_desc*>  *list2()        const { return (si_list2); }
    vl_expr                     *ifex()         const { return (si_ifex); }

private:
    int                         si_type;
    lsList<vl_bassign_stmt*>    *si_params;
    vl_path_desc                *si_lhs;
    lsList<vl_expr*>            *si_rhs;
    vl_expr                     *si_expr;
    int                         si_pol;
    int                         si_edge_id;
    lsList<vl_spec_term_desc*>  *si_list1;
    lsList<vl_spec_term_desc*>  *si_list2;
    vl_expr                     *si_ifex;
};

// Specify item terminal description.
//
struct vl_spec_term_desc
{
    vl_spec_term_desc(char*, vl_expr*, vl_expr*);
    vl_spec_term_desc(int, vl_expr*);
    ~vl_spec_term_desc();

    vl_spec_term_desc *copy();
    void print(ostream&);

    const char  *lterm()        const { return (""); }

    const char  *name()         const { return (st_name); }
    vl_expr     *exp1()         const { return (st_exp1); }
    vl_expr     *exp2()         const { return (st_exp2); }
    int         pol()           const { return (st_pol); }

private:
    const char  *st_name;
    vl_expr     *st_exp1;
    vl_expr     *st_exp2;
    int         st_pol;
};

// Specify item path description.
//
struct vl_path_desc
{
    vl_path_desc(vl_spec_term_desc*, vl_spec_term_desc*);
    vl_path_desc(lsList<vl_spec_term_desc*>*, lsList<vl_spec_term_desc*>*);
    ~vl_path_desc();

    vl_path_desc *copy();
    void print(ostream&);

    int                         type()      const { return (pa_type); }
    lsList<vl_spec_term_desc*>  *list1()    const { return (pa_list1); }
    lsList<vl_spec_term_desc*>  *list2()    const { return (pa_list2); }

private:
    int                         pa_type;
    lsList<vl_spec_term_desc*>  *pa_list1;
    lsList<vl_spec_term_desc*>  *pa_list2;
};

// Task description.
//
struct vl_task : public vl_stmt
{
    vl_task()
        {
            t_name = 0;
            t_decls = 0;
            t_stmts = 0;
            t_sig_st = 0;
            t_blk_st = 0;
        }

    vl_task(char*, lsList<vl_decl*>*, lsList<vl_stmt*>*);
    ~vl_task();

    vl_task *copy();
    void init();
    void disable(vl_stmt*);
    void dump(ostream&, int);
    void print(ostream&);

    const char          *lterm()        const { return (";\n"); }

    const char          *name()         const { return (t_name); }
    lsList<vl_decl*>    *decls()        const { return (t_decls); }
    lsList<vl_stmt*>    *stmts()        const { return (t_stmts); }
    table<vl_var*>      *sig_st()       const { return (t_sig_st); }
    table<vl_stmt*>     *blk_st()       const { return (t_blk_st); }

    void set_sig_st(table<vl_var*> *t)  { t_sig_st = t; }
    void set_blk_st(table<vl_stmt*> *t) { t_blk_st = t; }

private:
    const char          *t_name;
    lsList<vl_decl*>    *t_decls;
    lsList<vl_stmt*>    *t_stmts;
    table<vl_var*>      *t_sig_st;
    table<vl_stmt*>     *t_blk_st;
};

// Function description.
//
struct vl_function : public vl_stmt
{
    vl_function(int, vl_range*, char*, lsList<vl_decl*>*, lsList<vl_stmt*>*);
    ~vl_function();

    vl_function *copy();
    void init();
    void eval_func(vl_var*, lsList<vl_expr*>*);
    void print(ostream&);
    void dump(ostream&, int);

    const char          *lterm()        const { return (";\n"); }

    const char          *name()         const { return (f_name); }
    vl_range            *range()        const { return (f_range); }
    lsList<vl_decl*>    *decls()        const { return (f_decls); }
    lsList<vl_stmt*>    *stmts()        const { return (f_stmts); }
    table<vl_var*>      *sig_st()       const { return (f_sig_st); }
    table<vl_stmt*>     *blk_st()       const { return (f_blk_st); }

    void set_decls(lsList<vl_decl*> *d) { f_decls = d; }
    void set_stmts(lsList<vl_stmt*> *s) { f_stmts = s; }
    void set_sig_st(table<vl_var*> *t)  { f_sig_st = t; }
    void set_blk_st(table<vl_stmt*> *t) { f_blk_st = t; }

private:
    const char          *f_name;
    vl_range            *f_range;
    lsList<vl_decl*>    *f_decls;
    lsList<vl_stmt*>    *f_stmts;
    table<vl_var*>      *f_sig_st;
    table<vl_stmt*>     *f_blk_st;
};

// Struct to store a drive strength and delay.
//
struct vl_dlstr
{
    vl_dlstr()
        {
            d_delay = 0;
        }

    vl_delay            *delay()        const { return (d_delay); }
    const vl_strength   strength()      const { return (d_strength); }

    void set_delay(vl_delay *d)             { d_delay = d; }
    void set_strength(const vl_strength s)  { d_strength = s; }

private:
    vl_delay        *d_delay;
    vl_strength     d_strength;
};

// Description of a gate type.
//
struct vl_gate_inst_list : public vl_stmt
{
    vl_gate_inst_list()
        {
            g_delays = 0;
            g_gates = 0;
        }

    vl_gate_inst_list(int, vl_dlstr*, lsList<vl_gate_inst*>*);
    ~vl_gate_inst_list();

    vl_gate_inst_list *copy();
    void setup(vl_simulator*);
    void print(ostream&);

    const char              *lterm()        const { return (";\n"); }

    const vl_strength       strength()      const { return (g_strength); }
    vl_delay                *delays()       const { return (g_delays); }
    lsList<vl_gate_inst*>   *gates()        const { return (g_gates); }

private:
    vl_strength             g_strength;
    vl_delay                *g_delays;
    lsList<vl_gate_inst*>   *g_gates;
};

// Description of a module or primitive  instance type.
//
struct vl_mp_inst_list : public vl_stmt
{
    vl_mp_inst_list()
        {
            mp_name = 0;
            mp_mptype = MPundef;
            mp_mps = 0;
            mp_prms_or_dlys = 0;
        }

    vl_mp_inst_list(MPtype, char*, vl_dlstr*, lsList<vl_mp_inst*>*);
    ~vl_mp_inst_list();

    vl_mp_inst_list *copy();
    void init();
    void setup(vl_simulator*);
    void print(ostream&);
    void dumpvars(ostream&, vl_simulator*);

    const char          *lterm()            const { return (";\n"); }

    const char          *name()             const { return (mp_name); }
    MPtype              mptype()            const { return (mp_mptype); }
    lsList<vl_mp_inst*> *mps()              const { return (mp_mps); }
    const vl_strength   strength()          const { return (mp_strength); }
    vl_delay            *prms_or_dlys()     const { return (mp_prms_or_dlys); }

    void set_mptype(MPtype t)               { mp_mptype = t; }

private:
    const char          *mp_name;           // name of master
    MPtype              mp_mptype;          // type of object in list
    lsList<vl_mp_inst*> *mp_mps;            // list of instances
    vl_strength         mp_strength;
    vl_delay            *mp_prms_or_dlys;   // param overrides (in a vl_delay)
};


//---------------------------------------------------------------------------
//  Statements
//---------------------------------------------------------------------------

// Basic assignment description.
//
struct vl_bassign_stmt : public vl_stmt
{
    vl_bassign_stmt(int, vl_var*, vl_event_expr*, vl_delay*, vl_var*);
    ~vl_bassign_stmt();

    vl_bassign_stmt *copy();
    void init();
    void setup(vl_simulator*);
    EVtype eval(vl_event*, vl_simulator*);
    void freeze_indices(vl_range*);
    void print(ostream&);

    const char      *lterm()        const { return (";\n"); }

    vl_var          *lhs()          const { return (ba_lhs); }
    vl_range        *range()        const { return (ba_range); }
    vl_var          *rhs()          const { return (ba_rhs); }
    vl_delay        *delay()        const { return (ba_delay); }
    vl_delay        *wait()         const { return (ba_wait); }
    vl_event_expr   *event()        const { return (ba_event); }
    vl_var          *tmpvar()       const { return (ba_tmpvar); }

    void set_lhs(vl_var *v)         { ba_lhs = v; }
    void set_range(vl_range *r)     { ba_range = r; }
    void set_wait(vl_delay *d)      { ba_wait = d; }

private:
    vl_var          *ba_lhs;        // receiving variable
    vl_range        *ba_range;      // bit/part select for receiver
    vl_var          *ba_rhs;        // source expr
    vl_delay        *ba_delay;      // #wait a = #delay b
    vl_delay        *ba_wait;
    vl_event_expr   *ba_event;      // only one of event, delay is not 0
    vl_var          *ba_tmpvar;     // temporary transfer variable
};

// System task statememt description.
//
struct vl_sys_task_stmt : public vl_stmt
{
    vl_sys_task_stmt()
        {
            action = 0;
            sy_name = 0;
            sy_args = 0;
            sy_dtype = DSPall;
            sy_flags = 0;
        }

    vl_sys_task_stmt(char*, lsList<vl_expr*>*);
    ~vl_sys_task_stmt();

    vl_sys_task_stmt *copy();
    void setup(vl_simulator*);
    EVtype eval(vl_event*, vl_simulator*);
    void print(ostream&);

    vl_var& (vl_simulator::*action)(vl_sys_task_stmt*, lsList<vl_expr*>*);

    const char          *lterm()        const { return (";\n"); }

    const char          *name()         const { return (sy_name); }
    lsList<vl_expr*>    *args()         const { return (sy_args); }
    DSPtype             dtype()         const { return (sy_dtype); }
    unsigned int        flags()         const { return (sy_flags); }

private:
    const char          *sy_name;
    lsList<vl_expr*>    *sy_args;                
    DSPtype             sy_dtype;
    unsigned int        sy_flags;

    // Flags for vl_sys_task_stmt.
#define SYSafter     0x1
#define SYSno_nl     0x2
};

// Begin/end statement description.
//
struct vl_begin_end_stmt : public vl_stmt
{
    vl_begin_end_stmt()
        {
            be_name = 0;
            be_decls = 0;
            be_stmts = 0;
            be_sig_st = 0;
            be_blk_st = 0;
        }

    vl_begin_end_stmt(char*, lsList<vl_decl*>*, lsList<vl_stmt*>*);
    ~vl_begin_end_stmt();

    vl_begin_end_stmt *copy();
    void init();
    void setup(vl_simulator*);
    EVtype eval(vl_event*, vl_simulator*);
    void disable(vl_stmt*);
    void dump(ostream&, int);
    void print(ostream&);
    void dumpvars(ostream&, vl_simulator*);

    const char          *name()         const { return (be_name); }
    lsList<vl_decl*>    *decls()        const { return (be_decls); }
    lsList<vl_stmt*>    *stmts()        const { return (be_stmts); }
    table<vl_var*>      *sig_st()       const { return (be_sig_st); }
    table<vl_stmt*>     *blk_st()       const { return (be_blk_st); }

    void set_sig_st(table<vl_var*> *t)  { be_sig_st = t; }
    void set_blk_st(table<vl_stmt*> *t) { be_blk_st = t; }

private:
    const char          *be_name;
    lsList<vl_decl*>    *be_decls;
    lsList<vl_stmt*>    *be_stmts;
    table<vl_var*>      *be_sig_st;
    table<vl_stmt*>     *be_blk_st;
};

// If/else statement description.
//
struct vl_if_else_stmt : public vl_stmt
{
    vl_if_else_stmt()
        {
            ie_cond = 0;
            ie_if_stmt = 0;
            ie_else_stmt = 0;
        }

    vl_if_else_stmt(vl_expr*, vl_stmt*, vl_stmt*);

    ~vl_if_else_stmt()
        {
            delete ie_cond;
            delete ie_if_stmt;
            delete ie_else_stmt;
        }

    vl_if_else_stmt *copy();
    void init();
    void setup(vl_simulator*);
    EVtype eval(vl_event*, vl_simulator*);
    void disable(vl_stmt*);
    void print(ostream&);
    void dumpvars(ostream&, vl_simulator*);

    const char  *lterm()        const { return (""); }

    vl_expr     *cond()         const { return (ie_cond); }
    vl_stmt     *if_stmt()      const { return (ie_if_stmt); }
    vl_stmt     *else_stmt()    const { return (ie_else_stmt); }

private:
    vl_expr     *ie_cond;
    vl_stmt     *ie_if_stmt;
    vl_stmt     *ie_else_stmt;
};

// Case statement description.
//
struct vl_case_stmt : public vl_stmt
{
    vl_case_stmt()
        {
            c_cond = 0;
            c_case_items = 0;
        }

    vl_case_stmt(int, vl_expr*, lsList<vl_case_item*>*);
    ~vl_case_stmt();

    vl_case_stmt *copy();
    void init();
    void setup(vl_simulator*);
    EVtype eval(vl_event*, vl_simulator*);
    void disable(vl_stmt*);
    void print(ostream&);
    void dumpvars(ostream&, vl_simulator*);

    vl_expr                 *cond()         const { return (c_cond); }
    lsList<vl_case_item*>   *case_items()   const { return (c_case_items); }

private:
    vl_expr                 *c_cond;
    lsList<vl_case_item*>   *c_case_items;
};

// Case item description.
//
struct vl_case_item : public vl_stmt
{
    vl_case_item()
        {
            ci_exprs = 0;
            ci_stmt = 0;
        }

    vl_case_item(int, lsList<vl_expr*>*, vl_stmt*);
    ~vl_case_item();

    vl_case_item *copy();
    void init();
    void disable(vl_stmt*);
    void print(ostream&);
    void dumpvars(ostream&, vl_simulator*);

    lsList<vl_expr*>    *exprs()        const { return (ci_exprs); }
    vl_stmt             *stmt()         const { return (ci_stmt); }

private:
    lsList<vl_expr*>    *ci_exprs;
    vl_stmt             *ci_stmt;
};

// Forever statement description.
//
struct vl_forever_stmt : public vl_stmt
{
    vl_forever_stmt()
        {
            f_stmt = 0;
        }

    vl_forever_stmt(vl_stmt*);

    ~vl_forever_stmt()
        {
            delete f_stmt;
        }

    vl_forever_stmt *copy();
    void init();
    void setup(vl_simulator*);
    EVtype eval(vl_event*, vl_simulator*);
    void disable(vl_stmt*);
    void print(ostream&);
    void dumpvars(ostream&, vl_simulator*);

    const char  *lterm()    const { return (""); }

    vl_stmt     *stmt()     const { return (f_stmt); }

private:
    vl_stmt     *f_stmt;
};

// Repeat statement description.
//
struct vl_repeat_stmt : public vl_stmt
{
    vl_repeat_stmt()
        {
            r_count = 0;
            r_stmt = 0;
            r_cur_count = 0;
        }

    vl_repeat_stmt(vl_expr*, vl_stmt*);

    ~vl_repeat_stmt()
        {
            delete r_count;
            delete r_stmt;
        }

    vl_repeat_stmt *copy();
    void init();
    void setup(vl_simulator*);
    EVtype eval(vl_event*, vl_simulator*);
    void disable(vl_stmt*);
    void print(ostream&);
    void dumpvars(ostream&, vl_simulator*);

    const char  *lterm()        const { return (""); }

    vl_expr     *count()        const { return (r_count); }
    vl_stmt     *stmt()         const { return (r_stmt); }
    int         cur_count()     const { return (r_cur_count); }

private:
    vl_expr     *r_count;
    vl_stmt     *r_stmt;
    int         r_cur_count;
};

// While statement description.
//
struct vl_while_stmt : public vl_stmt
{
    vl_while_stmt()
        {
            w_cond = 0;
            w_stmt = 0;
        }

    vl_while_stmt(vl_expr*, vl_stmt*);

    ~vl_while_stmt()
        {
            delete w_cond;
            delete w_stmt;
        }

    vl_while_stmt *copy();
    void init();
    void setup(vl_simulator*);
    EVtype eval(vl_event*, vl_simulator*);
    void disable(vl_stmt*);
    void print(ostream&);
    void dumpvars(ostream&, vl_simulator*);

    const char  *lterm()    const { return (""); }

    vl_expr     *cond()     const { return (w_cond); }
    vl_stmt     *stmt()     const { return (w_stmt); }

private:
    vl_expr     *w_cond;
    vl_stmt     *w_stmt;
};

// For statement description.
//
struct vl_for_stmt : public vl_stmt
{
    vl_for_stmt()
        {
            f_initial = 0;
            f_end = 0;
            f_cond = 0;
            f_stmt = 0;
        }

    vl_for_stmt(vl_bassign_stmt*, vl_expr*, vl_bassign_stmt*, vl_stmt*);

    ~vl_for_stmt()
        {
            delete f_initial;
            delete f_end;
            delete f_cond;
            delete f_stmt;
        }

    vl_for_stmt *copy();
    void init();
    void setup(vl_simulator*);
    EVtype eval(vl_event*, vl_simulator*);
    void disable(vl_stmt*);
    void print(ostream&);
    void dumpvars(ostream&, vl_simulator*);

    const char          *lterm()        const { return (""); }

    vl_bassign_stmt     *initial()      const { return (f_initial); }
    vl_bassign_stmt     *end()          const { return (f_end); }
    vl_expr             *cond()         const { return (f_cond); }
    vl_stmt             *stmt()         const { return (f_stmt); }

private:
    vl_bassign_stmt     *f_initial;
    vl_bassign_stmt     *f_end;
    vl_expr             *f_cond;
    vl_stmt             *f_stmt;
};

// Delay control statement description.
//
struct vl_delay_control_stmt : public vl_stmt
{
    vl_delay_control_stmt()
        {
            d_delay = 0;
            d_stmt = 0;
        }

    vl_delay_control_stmt(vl_delay*, vl_stmt*);

    ~vl_delay_control_stmt()
        {
            delete d_delay;
            delete d_stmt;
        }

    vl_delay_control_stmt *copy();
    void init();
    void setup(vl_simulator*);
    EVtype eval(vl_event*, vl_simulator*);
    void disable(vl_stmt*);
    void print(ostream&);
    void dumpvars(ostream&, vl_simulator*);

    const char  *lterm()        const { return (""); }

    vl_delay    *delay()        const { return (d_delay); }
    vl_stmt     *stmt()         const { return (d_stmt); }

private:
    vl_delay    *d_delay;
    vl_stmt     *d_stmt;
};

// Event control statement description.
//
struct vl_event_control_stmt : public vl_stmt
{
    vl_event_control_stmt()
        {
            ec_event = 0;
            ec_stmt = 0;
        }

    vl_event_control_stmt(vl_event_expr*, vl_stmt*);

    ~vl_event_control_stmt()
        {
            delete ec_event;
            delete ec_stmt;
        }

    vl_event_control_stmt *copy();
    void init();
    void setup(vl_simulator*);
    EVtype eval(vl_event*, vl_simulator*);
    void disable(vl_stmt*);
    void print(ostream&);
    void dumpvars(ostream&, vl_simulator*);

    const char      *lterm()        const { return (""); }

    vl_event_expr   *event()        const { return (ec_event); }
    vl_stmt         *stmt()         const { return (ec_stmt); }

private:
    vl_event_expr   *ec_event;
    vl_stmt         *ec_stmt;
};

// Wait statement description.
//
struct vl_wait_stmt : public vl_stmt
{
    vl_wait_stmt()
        {
            w_cond = 0;
            w_stmt = 0;
            w_event = 0;
        }

    vl_wait_stmt(vl_expr*, vl_stmt*);

    ~vl_wait_stmt()
        {
            delete w_cond;
            delete w_stmt;
            if (w_event) {
                w_event->set_expr(0);
                delete w_event;
            }
        }

    vl_wait_stmt *copy();
    void init();
    void setup(vl_simulator*);
    EVtype eval(vl_event*, vl_simulator*);
    void disable(vl_stmt*);
    void print(ostream&);
    void dumpvars(ostream&, vl_simulator*);

    const char      *lterm()    const { return (""); }

    vl_expr         *cond()     const { return (w_cond); }
    vl_stmt         *stmt()     const { return (w_stmt); }
    vl_event_expr   *event()    const { return (w_event); }

private:
    vl_expr         *w_cond;
    vl_stmt         *w_stmt;
    vl_event_expr   *w_event;
};

// Send event statement description.
//
struct vl_send_event_stmt : public vl_stmt
{
    vl_send_event_stmt()
        {
            se_name = 0;
        }

    vl_send_event_stmt(char*);

    ~vl_send_event_stmt()
        {
            delete [] se_name;
        }

    vl_send_event_stmt *copy();
    void setup(vl_simulator*);
    EVtype eval(vl_event*, vl_simulator*);
    void print(ostream&);

    const char *lterm()     const { return (";\n"); }

    const char *name()      const { return (se_name); }

private:
    const char *se_name;
};

// Fork/join statement description.
//
struct vl_fork_join_stmt : public vl_stmt
{
    vl_fork_join_stmt()
        {
            fj_name = 0;
            fj_decls = 0;
            fj_stmts = 0;
            fj_sig_st = 0;
            fj_blk_st = 0;
            fj_endcnt = 0;
        }

    vl_fork_join_stmt(char*, lsList<vl_decl*>*, lsList<vl_stmt*>*);
    ~vl_fork_join_stmt();

    vl_fork_join_stmt *copy();
    void init();
    void setup(vl_simulator*);
    EVtype eval(vl_event*, vl_simulator*);
    void disable(vl_stmt*);
    void dump(ostream&, int);
    void print(ostream&);
    void dumpvars(ostream&, vl_simulator*);

    const char          *name()         const { return (fj_name); }
    lsList<vl_decl*>    *decls()        const { return (fj_decls); }
    lsList<vl_stmt*>    *stmts()        const { return (fj_stmts); }
    table<vl_var*>      *sig_st()       const { return (fj_sig_st); }
    table<vl_stmt*>     *blk_st()       const { return (fj_blk_st); }
    int                 endcnt()        const { return (fj_endcnt); }

    void set_sig_st(table<vl_var*> *t)  { fj_sig_st = t; }
    void set_blk_st(table<vl_stmt*> *t) { fj_blk_st = t; }
    void dec_endcnt()                   { fj_endcnt--; }

private:
    const char          *fj_name;
    lsList<vl_decl*>    *fj_decls;
    lsList<vl_stmt*>    *fj_stmts;
    table<vl_var*>      *fj_sig_st;
    table<vl_stmt*>     *fj_blk_st;
    int                 fj_endcnt;
};

// This is used to indicate the end of a thread in a fork/join block.
//
struct vl_fj_break : public vl_stmt
{
    vl_fj_break(vl_fork_join_stmt *f, vl_begin_end_stmt *e)
        {
            st_type = ForkJoinBreak;
            fjb_fjblock = f;
            fjb_begin_end = e;
        }

    void setup(vl_simulator*);
    EVtype eval(vl_event*, vl_simulator*);
    void print(ostream&);

    vl_fork_join_stmt *fjblock()    const { return (fjb_fjblock); }
    vl_begin_end_stmt *begin_end()  const { return (fjb_begin_end); }

private:
    vl_fork_join_stmt *fjb_fjblock;     // originating block
    vl_begin_end_stmt *fjb_begin_end;   // created thread container
};

// Task enable statement description.
//
struct vl_task_enable_stmt : public vl_stmt
{
    vl_task_enable_stmt()
        {
            te_name = 0;
            te_args = 0;
            te_task = 0;
        }

    vl_task_enable_stmt(int, char*, lsList<vl_expr*>*);
    ~vl_task_enable_stmt();

    vl_task_enable_stmt *copy();
    void setup(vl_simulator*);
    EVtype eval(vl_event*, vl_simulator*);
    void print(ostream&);

    const char          *name()     const { return (te_name); }
    lsList<vl_expr*>    *args()     const { return (te_args); }
    vl_task             *task()     const { return (te_task); }

private:
    const char          *te_name;
    lsList<vl_expr*>    *te_args;
    vl_task             *te_task;
};

// Disable statement description.
//
struct vl_disable_stmt : public vl_stmt
{
    vl_disable_stmt()
        {
            d_name = 0;
            d_target = 0;
        }

    vl_disable_stmt(char*);

    ~vl_disable_stmt()
        {
            delete [] d_name;
        }

    vl_disable_stmt *copy();
    void setup(vl_simulator*);
    EVtype eval(vl_event*, vl_simulator*);
    void print(ostream&);

    const char  *lterm()    const { return (";\n"); }

    const char  *name()     const { return (d_name); }
    vl_stmt     *target()   const { return (d_target); }

private:
    const char  *d_name;
    vl_stmt     *d_target;
};

// Deassign statement description.
//
struct vl_deassign_stmt : public vl_stmt
{
    vl_deassign_stmt()
        {
            d_lhs = 0;
        }

    vl_deassign_stmt(int, vl_var*);
    ~vl_deassign_stmt();

    vl_deassign_stmt *copy();
    void init();
    void setup(vl_simulator*);
    EVtype eval(vl_event*, vl_simulator*);
    void print(ostream&);

    const char  *lterm()    const { return (";\n"); }

    vl_var      *lhs()      const { return (d_lhs); }

private:
    vl_var      *d_lhs;
};


//---------------------------------------------------------------------------
//  Instances
//---------------------------------------------------------------------------

// Base class for instances.
//
struct vl_inst : public vl_stmt
{
    vl_inst() : i_name(0)   { }

    virtual ~vl_inst()
        {
            delete [] i_name;
        }

    const char *name()      const { return (i_name); }

protected:
    const char *i_name;
};

// Gate instance.
//
struct vl_gate_inst : public vl_inst
{
    vl_gate_inst();
    vl_gate_inst(char*, lsList<vl_expr*>*, vl_range*);
    ~vl_gate_inst();

    vl_gate_inst *copy();
    void setup(vl_simulator*);
    EVtype eval(vl_event*, vl_simulator*);
    void set_type(int);
    void set_delay(vl_simulator*, int);

    void                print(ostream &outs)    { outs << gi_string; }
    const char          *lterm()        const { return (";\n"); }

    lsList<vl_expr*>    *terms()        const { return (gi_terms); }
    lsList<vl_var*>     *outputs()      const { return (gi_outputs); }
    const char          *string()       const { return (gi_string); }
    vl_gate_inst_list   *inst_list()    const { return (gi_inst_list); }
    vl_delay            *delay()        const { return (gi_delay); }
    vl_range            *array()        const { return (gi_array); }

    void set_inst_list(vl_gate_inst_list *l)        { gi_inst_list = l; }

private:
    static bool gsetup_gate(vl_simulator*, vl_gate_inst*);
    static bool gsetup_buf(vl_simulator*, vl_gate_inst*);
    static bool gsetup_cbuf(vl_simulator*, vl_gate_inst*);
    static bool gsetup_mos(vl_simulator*, vl_gate_inst*);
    static bool gsetup_cmos(vl_simulator*, vl_gate_inst*);
    static bool gsetup_tran(vl_simulator*, vl_gate_inst*);
    static bool gsetup_ctran(vl_simulator*, vl_gate_inst*);
    static bool geval_gate(vl_simulator*, int(*)(int, int), vl_gate_inst*);
    static bool geval_buf(vl_simulator*, int(*)(int, int), vl_gate_inst*);
    static bool geval_cbuf(vl_simulator*, int(*)(int, int), vl_gate_inst*);
    static bool geval_mos(vl_simulator*, int(*)(int, int), vl_gate_inst*);
    static bool geval_cmos(vl_simulator*, int(*)(int, int), vl_gate_inst*);
    static bool geval_tran(vl_simulator*, int(*)(int, int), vl_gate_inst*);
    static bool geval_ctran(vl_simulator*, int(*)(int, int), vl_gate_inst*);

    bool (*gi_setup)(vl_simulator*, vl_gate_inst*);
    bool (*gi_eval)(vl_simulator*, int(*)(int, int), vl_gate_inst*);
    int  (*gi_set)(int, int);

    lsList<vl_expr*>    *gi_terms;
    lsList<vl_var*>     *gi_outputs;
    const char          *gi_string;
    vl_gate_inst_list   *gi_inst_list;
    vl_delay            *gi_delay;      // rise/fall/turn-off
    vl_range            *gi_array;      // arrayed size
};

// Module or primitive instance.
//
struct vl_mp_inst : public vl_inst
{
    vl_mp_inst()
        {
            pi_ports = 0;
            pi_inst_list = 0;
            pi_master = 0;
        }

    vl_mp_inst(char*, lsList<vl_port_connect*>*);
    ~vl_mp_inst();

    vl_mp_inst *copy();
    void setup(vl_simulator*);
    EVtype eval(vl_event*, vl_simulator*);
    void link_ports(vl_simulator*);
    void port_setup(vl_simulator*, vl_port_connect*, vl_port*, int);
    void dumpvars(ostream&, vl_simulator*);

    lsList<vl_port_connect*>    *ports()        const { return (pi_ports); }
    vl_mp_inst_list             *inst_list()    const { return (pi_inst_list); }
    vl_mp                       *master()       const { return (pi_master); }

    void set_inst_list(vl_mp_inst_list *l)      { pi_inst_list = l; }
    void set_master(vl_mp *m)                   { pi_master = m; }

private:
    lsList<vl_port_connect*>    *pi_ports;
    vl_mp_inst_list             *pi_inst_list;  // m/p inst_list
    vl_mp                       *pi_master;     // module or primitive copy
};

// End of file.

