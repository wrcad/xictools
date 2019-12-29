
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

// Print format modifiers
//
enum { DSPall, DSPb, DSPh, DSPo };
typedef unsigned char DSPtype;

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
struct vl_range_or_type;
struct multi_concat;
struct bitexp_parse;

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
enum { STRnone, STRhiZ, STRsmall, STRmed, STRweak, STRlarge, STRpull,
    STRstrong, STRsupply };
typedef unsigned short STRength;

struct vl_strength
{
    vl_strength() { str0 = STRnone; str1 = STRnone; }

    STRength str0;
    STRength str1;
};

// Basic data item
//
struct vl_var
{
    vl_var();
    vl_var(const char*, vl_range*, lsList<vl_expr*>* = 0);
    vl_var(vl_var&);
    virtual ~vl_var();

    virtual vl_var *copy();
    virtual vl_var &eval();
    virtual vl_var *source() { return (this); }
    virtual void chain(vl_stmt*);
    virtual void unchain(vl_stmt*);
    virtual void unchain_disabled(vl_stmt*);
    virtual void print(ostream&);

    void print_value(ostream&, DSPtype = DSPh);

    void configure(vl_range*, int = RegDecl, vl_range* = 0);
    void operator=(vl_var&);
    void assign(vl_range*, vl_var*, vl_range*);
    void assign(vl_range*, vl_var*, int*, int*);
    void assign_to(double, double, double, int, int);

private:
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

public:
    void clear(vl_range*, int);
    void reset();
    void set(bitexp_parse*);
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

    operator int();
    operator unsigned();
    operator vl_time_t();
    operator double();
    operator char*();

    void addb(vl_var&, int);
    void addb(vl_var&, vl_time_t);
    void addb(vl_var&, vl_var&);
    void subb(vl_var&, int);
    void subb(int, vl_var&);
    void subb(vl_var&, vl_time_t);
    void subb(vl_time_t, vl_var&);
    void subb(vl_var&, vl_var&);

    // vl_print.cc
    const char *decl_type();
    int pwidth(char);
    char *bitstr();

    const char *name;             // data item's name

    Dtype data_type;              // declared type
      // If type == Dnone, object can be coerced into any type on assignment,
      // otherwise object will retain type.  Dnone looks like an int.

    REGtype net_type;             // type of net
      // If not REGnone, object must be a bit field, specifies type of net,
      // or reg

    IOtype io_type;               // type of port
      // Specifies whether net is input, output, inout, or none

    unsigned char flags;          // misc. indicators

    vl_array array;               // range if array
    vl_array bits;                // bit field range
    union {
        int i;                    // integer (Dint)
        double r;                 // real (Dreal)
        vl_time_t t;              // time (Dtime)
        char *s;                  // bit field or char string (Dbit, Dstring)
        void *d;                  // array data
        lsList<vl_expr*> *c;      // concatenation list (Dconcat)
    } u;

    vl_range *range;              // array range from parser
    vl_action_item *events;       // list of events to trigger
    vl_strength strength;         // net assign strength
    vl_delay *delay;              // net delay
    vl_bassign_stmt *cassign;     // continuous procedural assignment
    lsList<vl_driver*> *drivers;  // driver list for net

    static vl_simulator *simulator ; // access to simulator
};

// values for vl_var flags field
//
#define VAR_IN_TABLE        0x1
    // This vl_var has been placed in a symbol table

#define VAR_PORT_DRIVER     0x2
    // This vl_var drives across an inout module port

#define VAR_CP_ASSIGN       0x4
    // A continuous procedural assign is active

#define VAR_F_ASSIGN        0x8
    // A force statement is active


// Math and logical operations involving vl_var structs
//
extern vl_var &operator*(vl_var&, vl_var&);
extern vl_var &operator/(vl_var&, vl_var&);
extern vl_var &operator%(vl_var&, vl_var&);
extern vl_var &operator+(vl_var&, vl_var&);
extern vl_var &operator-(vl_var&, vl_var&);
extern vl_var &operator-(vl_var&);
extern vl_var &operator<<(vl_var&, vl_var&);
extern vl_var &operator<<(vl_var&, int);
extern vl_var &operator>>(vl_var&, vl_var&);
extern vl_var &operator>>(vl_var&, int);
extern vl_var &operator==(vl_var&, vl_var&);
extern vl_var &case_eq(vl_var&, vl_var&);
extern vl_var &casex_eq(vl_var&, vl_var&);
extern vl_var &casez_eq(vl_var&, vl_var&);
extern vl_var &case_neq(vl_var&, vl_var&);
extern vl_var &operator!=(vl_var&, vl_var&);
extern vl_var &operator&&(vl_var&, vl_var&);
extern vl_var &operator||(vl_var&, vl_var&);
extern vl_var &operator!(vl_var&);
extern vl_var &reduce(vl_var&, int);
extern vl_var &operator<(vl_var&, vl_var&);
extern vl_var &operator<=(vl_var&, vl_var&);
extern vl_var &operator>(vl_var&, vl_var&);
extern vl_var &operator>=(vl_var&, vl_var&);
extern vl_var &operator&(vl_var&, vl_var&);
extern vl_var &operator|(vl_var&, vl_var&);
extern vl_var &operator^(vl_var&, vl_var&);
extern vl_var &operator~(vl_var&);
extern vl_var &tcond(vl_var&, vl_expr*, vl_expr*);

#define CX_INCR 100

// A vl_var factory
//
struct vl_var_factory
{
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
    struct bl { vl_var block[CX_INCR]; bl *next; } *da_blocks;
};


// General expression description
//
struct vl_expr : public vl_var
{
    vl_expr();
    vl_expr(vl_var*);
    vl_expr(short, int, double, void*, void*, void*);
    virtual ~vl_expr();

    vl_expr *copy();
    vl_var &eval();
    void chain(vl_stmt *s) { chcore(s, 0); }
    void unchain(vl_stmt *s) { chcore(s, 1); }
    void unchain_disabled(vl_stmt *s) { chcore(s, 2); }
    void chcore(vl_stmt*, int);
    void print(ostream&);
    const char *symbol();
    vl_var *source();

    vl_range *source_range()
        { return  ((etype == BitSelExpr || etype == PartSelExpr) ?
        ux.ide.range : 0); }

    int etype;
    union {
        struct ide {
            const char *name;
            vl_range *range;
            vl_var *var;
        } ide;
        struct func_call {
            const char *name;
            lsList<vl_expr*> *args;
            vl_function *func;
        } func_call;
        struct exprs {
            vl_expr *e1;
            vl_expr *e2;
            vl_expr *e3;
        } exprs;
        struct mcat {
            vl_expr *rep;
            vl_var *var;
        } mcat;
        lsList<vl_expr*> *expr_list;
        vl_sys_task_stmt *systask;
    } ux;
};

// List element for a net driver
//
struct vl_driver
{
    vl_driver() { srcvar = 0; m_to = l_to = 0; l_from = 0; }
    vl_driver(vl_var *d, int mt, int lt, int lf)
        { srcvar = d; m_to = mt; l_to = lt; l_from = lf; }

    vl_var *srcvar;          // source vl_var
    int m_to;                // high index in target to assign
    int l_to;                // low index in target to assign
    int l_from;              // low index in source
};

// Range descriptor for bit field or array
//
struct vl_range
{
    vl_range(vl_expr *l, vl_expr *r) { left = l; right = r; }
    ~vl_range() { delete left; delete right; }
    vl_range *copy();
    bool eval(int*, int*);
    bool eval(vl_range**);
    int width();

    vl_expr *left;
    vl_expr *right;
};

// Delay specification
//
struct vl_delay
{
    vl_delay() { delay1 = 0; list = 0; }
    vl_delay(vl_expr *d) { delay1 = d; list = 0; }
    vl_delay(lsList<vl_expr*> *l) { delay1 = 0; list = l; }
    ~vl_delay();
    vl_delay *copy();
    vl_time_t eval();

    vl_expr *delay1;
    lsList<vl_expr*> *list;
};

// Event expression description
//
struct vl_event_expr
{
    vl_event_expr() { type = 0; expr = 0; list = 0; repeat = 0; count = 0; }
    vl_event_expr(short t, vl_expr *e)
        { type = t; expr = e; list = 0; repeat = 0; count = 0; }
    ~vl_event_expr();
    vl_event_expr *copy();
    void init();
    bool eval(vl_simulator*);
    void chain(vl_action_item*);
    void unchain(vl_action_item*);
    void unchain_disabled(vl_stmt*);

    short type;
    vl_expr *expr;
    lsList<vl_event_expr*> *list;
    vl_expr *repeat;
    int count;
};


//---------------------------------------------------------------------------
//  Parser objects
//---------------------------------------------------------------------------

enum vlERRtype { ERR_OK, ERR_WARN, ERR_COMPILE, ERR_INTERNAL }; 

// Main class for Verilog parser
//
struct vl_parser
{
    vl_parser();
    ~vl_parser();
    bool parse(int, char**);
    bool parse(FILE*);
    void clear();
    void error(vlERRtype, const char*, ...);
    bool parse_timescale(const char*);
    void print(ostream&);

    double tunit, tprec;        // current `timescale parameters
    vl_desc *description;
    vl_context *context;
    char *filename;
    vl_stack_t<vl_module*> *module_stack;
    vl_stack_t<FILE*> *file_stack;
    vl_stack_t<char*> *fname_stack;
    vl_stack_t<int> *lineno_stack;
    vl_stack_t<char*> *dir_stack;
    table<char*> *macros;
    int ifelseSP;
    char modName[MAXSTRLEN];
    char last_macro[MAXSTRLEN];
    char curr_macro[MAXSTRLEN];
    char ifelse_stack[MAXSTRLEN];
    jmp_buf jbuf;
    bool no_go;
    bool verbose;
};

// Range or type storage
//
struct vl_range_or_type
{
    vl_range_or_type() { type = 0; range = 0; }
    vl_range_or_type(short t, vl_range *r) { type = t; range = r; }
    ~vl_range_or_type() { delete range; }

    short type;
    vl_range *range;
};

// Multi-concatenation temporary container object
//
struct multi_concat
{
    multi_concat() { rep = 0; concat = 0; }
    multi_concat(vl_expr *r, lsList<vl_expr*> *c) { rep = r; concat = c; }

    vl_expr *rep;
    lsList<vl_expr*> *concat;
};

// Parser class for bit field
//
struct bitexp_parse : public vl_var
{
    bitexp_parse() { data_type = Dbit; u.s = brep = new char[MAXSTRLEN]; }
    void bin(char*);
    void dec(char*);
    void oct(char*);
    void hex(char*);

    char *brep;
};


//---------------------------------------------------------------------------
//  Simulator objects
//---------------------------------------------------------------------------

enum VLdelayType { DLYmin, DLYtyp, DLYmax };
enum VLstopType { VLrun, VLstop, VLabort };

// Main class for simulator
//
struct vl_simulator
{
    vl_simulator();
    ~vl_simulator();
    bool initialize(vl_desc*, VLdelayType = DLYtyp, int = 0);
    bool simulate();
    VLstopType step();
    void close_files();
    void flush_files();
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
    void do_dump();
    char *dumpindex(vl_var*);
    bool monitor_change(lsList<vl_expr*>*);
    void display_print(lsList<vl_expr*>*, ostream&, DSPtype, unsigned short);
    void fdisplay_print(lsList<vl_expr*>*, DSPtype, unsigned short);

    void abort() { stop = VLabort; }
    void finish() { stop = VLstop; }

    vl_desc *description;             // the verilog deck to simulate
    VLdelayType dmode;                // min/typ/max delay mode
    VLstopType stop;                  // stop simulation code
    bool first_point;                 // set for initial time point only
    bool monitor_state;               // monitor enabled flag
    bool fmonitor_state;              // fmonitor enabled flag
    vl_monitor *monitors;             // list of monitors
    vl_monitor *fmonitors;            // list of fmonitors
    vl_time_t time;                   // accumulated delay for setup
    vl_time_t steptime;               // accumulating time when stepping
    vl_context *context;              // evaluation context
    vl_timeslot *timewheel;           // time sorted events for evaluation
	vl_action_item *next_actions;     // actions to do first at next time
    vl_action_item *fj_end;           // fork/join return context list
    vl_top_mod_list *top_modules;     // pointer to top module
    int dbg_flags;                    // debugging flags
    vl_var time_data;                 // for @($time) events
    ostream *channels[32];       // fhandles
    ostream *dmpfile;            // VCD dump file name
    vl_context *dmpcx;                // context of dump
    int dmpdepth;                     // depth of dump
    int dmpstatus;                    // status flags for dump;
#define DMP_ACTIVE 0x1
#define DMP_HEADER 0x2
#define DMP_ON     0x4
#define DMP_ALL    0x8
    vl_var **dmpdata;                 // stuff for $dumpvars
    vl_var *dmplast;
    int dmpsize;
    int dmpindx;
    int tfunit;                       // stuff for $timeformat
    int tfprec;
    const char *tfsuffix;
    int tfwidth;
    vl_var_factory var_factory;
};


// Context list for simulator
//
struct vl_context
{
    vl_context() { module = 0; primitive = 0; task = 0; function = 0;
        block = 0; fjblk = 0; parent = 0; }

    static void destroy(vl_context *c)
        {
            while (c) {
                vl_context *cx = c;
                c = c->parent;
                delete cx;
            }
        }

    vl_context *copy();
    vl_context *push();
    vl_context *push(vl_mp*);
    vl_context *push(vl_module*);
    vl_context *push(vl_primitive*);
    vl_context *push(vl_task*);
    vl_context *push(vl_function*);
    vl_context *push(vl_begin_end_stmt*);
    vl_context *push(vl_fork_join_stmt*);
    static vl_context *pop(vl_context*);
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
    void print(ostream&);
    char *hiername();

    vl_module *module;
    vl_primitive *primitive;
    vl_task *task;
    vl_function *function;
    vl_begin_end_stmt *block;
    vl_fork_join_stmt *fjblk;
    vl_context *parent;

    static vl_simulator *simulator;
};

// The flags field of a vl_stmt is used in different ways by the various
// derived objects.  All flags used are listed here

#define SIM_INTERNAL       0x1
    // Statement was created for internal use during simulation, free
    // after execution

#define BAS_SAVE_RHS       0x2
    // This applies to vl_bassign_stmt structs.  When set, the rhs field
    // is not deleted when the object is deleted.

#define BAS_SAVE_LHS       0x4
    // This applies to vl_bassign_stmt structs.  When set, the lhs field
    // is not deleted when the object is deleted.

#define AI_DEL_STMT        0x8
    // This applies to vl_action_item structs.  When set, the stmt is
    // deleted when the action item is deleted.  The stmt has the
    // SIM_INTERNAL flag set, but we can't check for this directly
    // since the stmt may already be free

#define DAS_DEL_VAR        0x8
    // Similar to above, for vl_deassign_stmt and the var field.

// This, and the enum below, provide return values for the eval()
// function of vl_stmt and derivatives
// 
union vl_event
{
    vl_event_expr *event;
    vl_time_t time;
};

enum EVtype { EVnone, EVdelay, EVevent };

// Base class for Verilog module items, statements, and action
//
struct vl_stmt {
    vl_stmt() {  type = 0; flags = 0; }
    virtual ~vl_stmt() { }
    virtual vl_stmt *copy() { return (0); }
    virtual void init() { }
    virtual void setup(vl_simulator*) { }
    virtual EVtype eval(vl_event*, vl_simulator*) { return (EVnone); }
    virtual void disable(vl_stmt*) { }
    virtual void dump(ostream&, int) { }
    virtual void print(ostream&) { }
    virtual const char *lterm() { return ("\n"); }
    virtual void dumpvars(ostream&, vl_simulator*) { }

    short type;
    unsigned short flags;
};

// Action node for time wheel and event queue
//
struct vl_action_item : public vl_stmt
{
    vl_action_item(vl_stmt*, vl_context*);
    virtual ~vl_action_item();

    static void destroy(vl_action_item *a)
        {
            while (a) {
                vl_action_item *ax = a;
                a = a->next;
                delete ax;
            }
        }

    vl_action_item *copy();
    vl_action_item *purge(vl_stmt*);
    EVtype eval(vl_event*, vl_simulator*);
    void print(ostream&);

    vl_stmt *stmt;          // statement to evaluate
    vl_stack *stack;        // stack, if no stmt
    vl_event_expr *event;   // used in event queue
    vl_context *context;    // context for evaluation
    vl_action_item *next;
};

// Types of block in stack
enum ABtype { Sequential, Fork, Fence };

// Structure to store a block of actions
//
struct vl_sblk
{
    vl_sblk() { type = Sequential; actions = 0; fjblk = 0; }

    ABtype type;
    vl_action_item *actions;
    vl_stmt *fjblk;     // parent block, if fork/join
};

// Stack object for actions
//
struct vl_stack
{
    vl_stack(vl_sblk *a, int n) {
        acts = new vl_sblk[n]; num = n;
        for (int i = 0; i < n; i++) acts[i] = a[i]; }
    ~vl_stack() { delete [] acts; }
    vl_stack *copy();
    void print(ostream&);

    vl_sblk *acts;
    int num;  // depth of stack
};

// List head for actions at a time point
//
struct vl_timeslot
{
    vl_timeslot(vl_time_t);
    ~vl_timeslot();

    static void destroy(vl_timeslot *t)
        {
            while (t) {
                vl_timeslot *tx = t;
                t = t->next;
                delete tx;
            }
        }

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

    vl_time_t time;
    vl_action_item *actions;		// "active" events
    vl_action_item *trig_actions;   // triggered events
    vl_action_item *zdly_actions;   // "inactive" events
    vl_action_item *nbau_actions;   // "non-blocking assign update" events
    vl_action_item *mon_actions;    // "monitor" events
    vl_timeslot *next;
private:
    vl_action_item *a_end;			// end of actions list
    vl_action_item *t_end;			// end of triggered events list
    vl_action_item *z_end;			// end of zdly_actions list
    vl_action_item *n_end;			// end of nbau_actions list
    vl_action_item *m_end;			// end of mon_actions list
};

// List multiple 'top' modules
//
struct vl_top_mod_list
{
    vl_top_mod_list() { num = 0; mods = 0; }
    ~vl_top_mod_list() { delete [] mods; }
    bool istop(vl_module *mod)
        { for (int i = 0; i < num; i++) if (mod == mods[i]) return (true);
        return (false); }

    int num;
    vl_module **mods;
};

// Monitor context list element
//
struct vl_monitor
{
    vl_monitor(vl_context *c, lsList<vl_expr*> *a, DSPtype t)
        { cx = c; args = a; dtype = t; next = 0; }
    ~vl_monitor();

    vl_context *cx;
    lsList<vl_expr*> *args;
    DSPtype dtype;
    vl_monitor *next;
};


//---------------------------------------------------------------------------
//  Verilog description objects
//---------------------------------------------------------------------------

// Main class for a Verilog description
//
struct vl_desc
{
    vl_desc();
    ~vl_desc();
    void dump(ostream&);
    vl_gate_inst_list *add_gate_inst(short, vl_dlstr*, lsList<vl_gate_inst*>*);
    vl_stmt *add_mp_inst(vl_desc*, char*, vl_dlstr*, lsList<vl_mp_inst*>*);

    double tstep;
    lsList<vl_module*> *modules;
    lsList<vl_primitive*> *primitives;
    table<vl_mp*> *mp_st;
    set_t mp_undefined;
    vl_simulator *simulator;
};

// Base for modules and primitives
//
struct vl_mp
{
    virtual ~vl_mp() { }
    virtual vl_mp *copy() { return (0); }
    virtual void init() { }
    virtual void dump(ostream&) { }
    virtual void dumpvars(ostream&, vl_simulator*) { }
    virtual void setup(vl_simulator*) { }

    short type;
    const char *name;
    lsList<vl_port*> *ports;
    table<vl_var*> *sig_st;
    int inst_count;                 // number of instances of this primitive
    vl_mp_inst *instance;           // copy of master
};

// Module description
//
struct vl_module : public vl_mp
{
    vl_module();
    vl_module(vl_desc *desc, char*, lsList<vl_port*>*, lsList<vl_stmt*>*);
    ~vl_module();
    vl_module *copy();
    void dump(ostream&);
    void dumpvars(ostream&, vl_simulator*);
    void sort_moditems();
    void init();
    void setup(vl_simulator*);

    double tunit, tprec;
    lsList<vl_stmt*> *mod_items;
    table<vl_inst*> *inst_st;
    table<vl_function*> *func_st;
    table<vl_task*> *task_st;
    table<vl_stmt*> *blk_st;
};

// User-defined primitive description
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

    lsList<vl_decl*> *decls;
    vl_bassign_stmt *initial;
    unsigned char *ptable;          // entry table
    int rows;                       // array depth
    bool seq_init;                  // set when initialized
    vl_var *iodata[MAXPRIMLEN];    // i/o data pointers
    unsigned char lastvals[MAXPRIMLEN];  // previous values
};

// Primitive table entry
//
struct vl_prim_entry
{
    vl_prim_entry() { memset(this, PrimNone, sizeof(vl_prim_entry)); }
    vl_prim_entry(lsList<int>*, unsigned char, unsigned char);

    unsigned char inputs[MAXPRIMLEN];
    unsigned char state;
    unsigned char next_state;
};

// Port description for modules and primitives
//
struct vl_port
{
    vl_port() { type = 0; name = 0; port_exp = 0; }
    vl_port(short, char*, lsList<vl_var*>*);
    ~vl_port();
    vl_port *copy();

    short type;
    const char *name;
    lsList<vl_var*> *port_exp;
};

// Port connection description
//
struct vl_port_connect
{
    vl_port_connect() { type = 0; name = 0; expr = 0;
        i_assign = 0; o_assign = 0; }
    vl_port_connect(short, char*, vl_expr*);
    ~vl_port_connect();
    vl_port_connect *copy();

    short type;
    const char *name;
    vl_expr *expr;
    vl_bassign_stmt *i_assign;   // used in arg passing
    vl_bassign_stmt *o_assign;   // used in arg passing
};


//---------------------------------------------------------------------------
//  Module items
//---------------------------------------------------------------------------

// Basic declaration
//
struct vl_decl : public vl_stmt
{
    vl_decl() { type = 0; range = 0; ids = 0; list = 0; delay = 0; }
    vl_decl(short, vl_strength, vl_range*, vl_delay*,
        lsList<vl_bassign_stmt*>*, lsList<vl_var*>*);
    vl_decl(short, vl_range*, lsList<vl_var*>*);
    vl_decl(short, lsList<char*>*);
    vl_decl(short, vl_range*, lsList<vl_bassign_stmt*>*);
    ~vl_decl();
    vl_decl *copy();
    table<vl_var*> *symtab(vl_var*);
    void var_setup(vl_var*, int);
    void init();
    void setup(vl_simulator*);
    void print(ostream&);
    const char *decl_type();
    const char *lterm() { return (";\n"); }

    vl_range *range;
    lsList<vl_var*> *ids;
    lsList<vl_bassign_stmt*> *list;
    vl_delay *delay;
    vl_strength strength;
};

// Process statememt (always/initial) description
//
struct vl_procstmt : public vl_stmt
{
    vl_procstmt() { stmt = 0; lasttime = (vl_time_t)-1; }
    vl_procstmt(short, vl_stmt*);
    ~vl_procstmt();
    vl_procstmt *copy();
    void init();
    void setup(vl_simulator*);
    EVtype eval(vl_event*, vl_simulator*);
    void print(ostream&);
    void dumpvars(ostream&, vl_simulator*);
    const char *lterm() { return (""); }

    vl_stmt *stmt;
    vl_time_t lasttime;  // used for 'always' stmt
};

// Continuous assignment
//
struct vl_cont_assign : public vl_stmt
{
    vl_cont_assign() { delay = 0; assigns = 0; }
    vl_cont_assign(vl_strength, vl_delay*, lsList<vl_bassign_stmt*>*);
    ~vl_cont_assign();
    vl_cont_assign *copy();
    void setup(vl_simulator*);
    void print(ostream&);
    const char *lterm() { return (";\n"); }

    vl_strength strength;
    vl_delay *delay;
    lsList<vl_bassign_stmt*> *assigns;
};

// Specify block description
//
struct vl_specify_block : public vl_stmt
{
    vl_specify_block(lsList<vl_specify_item*>*);
    ~vl_specify_block();
    vl_specify_block *copy();
    void print(ostream&);

    lsList<vl_specify_item*> *items;
};

// Specify block item description
//
struct vl_specify_item
{
    vl_specify_item(short);
    vl_specify_item(short, lsList<vl_bassign_stmt*>*);
    vl_specify_item(short, vl_path_desc*, lsList<vl_expr*>*);
    vl_specify_item(short, vl_expr*, lsList<vl_spec_term_desc*>*, int,
        lsList<vl_spec_term_desc*>*, lsList<vl_expr*>*);
    vl_specify_item(short, vl_expr*, int, lsList<vl_spec_term_desc*>*,
        lsList<vl_spec_term_desc*>*, int, vl_expr*, lsList<vl_expr*>*);
    ~vl_specify_item();
    vl_specify_item *copy();
    void print(ostream&);
    const char *lterm() { return (";\n"); }

    short type;
    lsList<vl_bassign_stmt*> *params;
    vl_path_desc *lhs;
    lsList<vl_expr*> *rhs;
    vl_expr *expr;
    int pol;
    lsList<vl_spec_term_desc*> *list1;
    lsList<vl_spec_term_desc*> *list2;
    int edge_id;
    vl_expr *ifex;
};

// Specify item terminal description
//
struct vl_spec_term_desc
{
    vl_spec_term_desc(char*, vl_expr*, vl_expr*);
    vl_spec_term_desc(int, vl_expr*);
    ~vl_spec_term_desc();
    vl_spec_term_desc *copy();
    void print(ostream&);
    const char *lterm() { return (""); }

    const char *name;
    vl_expr *exp1;
    vl_expr *exp2;
    int pol;
};

// Specify item path description
//
struct vl_path_desc
{
    vl_path_desc(vl_spec_term_desc*, vl_spec_term_desc*);
    vl_path_desc(lsList<vl_spec_term_desc*>*, lsList<vl_spec_term_desc*>*);
    ~vl_path_desc();
    vl_path_desc *copy();
    void print(ostream&);

    int type;
    lsList<vl_spec_term_desc*> *list1;
    lsList<vl_spec_term_desc*> *list2;
};

// Task description
//
struct vl_task : public vl_stmt
{
    vl_task() { name = 0; decls = 0; stmts = 0; sig_st = 0; blk_st = 0; }
    vl_task(char*, lsList<vl_decl*>*, lsList<vl_stmt*>*);
    ~vl_task();
    vl_task *copy();
    void init();
    void disable(vl_stmt*);
    void dump(ostream&, int);
    void print(ostream&);
    const char *lterm() { return (";\n"); }

    const char *name;
    lsList<vl_decl*> *decls;
    lsList<vl_stmt*> *stmts;
    table<vl_var*> *sig_st;
    table<vl_stmt*> *blk_st;
};

// Function description
//
struct vl_function : public vl_stmt
{
    vl_function(short, vl_range*, char*, lsList<vl_decl*>*, lsList<vl_stmt*>*);
    ~vl_function();
    vl_function *copy();
    void init();
    void eval_func(vl_var*, lsList<vl_expr*>*);
    void print(ostream&);
    void dump(ostream&, int);
    const char *lterm() { return (";\n"); }

    const char *name;
    vl_range *range;
    lsList<vl_decl*> *decls;
    lsList<vl_stmt*> *stmts;
    table<vl_var*> *sig_st;
    table<vl_stmt*> *blk_st;
};

// Struct to store a drive strength and delay
//
struct vl_dlstr
{
    vl_dlstr() { delay = 0; strength.str0 = strength.str1 = STRnone; }

    vl_delay *delay;
    vl_strength strength;
};

// Description of a gate type
//
struct vl_gate_inst_list : public vl_stmt
{
    vl_gate_inst_list() { delays = 0; gates = 0; }
    vl_gate_inst_list(short, vl_dlstr*, lsList<vl_gate_inst*>*);
    ~vl_gate_inst_list();
    vl_gate_inst_list *copy();
    void setup(vl_simulator*);
    void print(ostream&);
    const char *lterm() { return (";\n"); }

    vl_strength strength;
    vl_delay *delays;
    lsList<vl_gate_inst*> *gates;
};

// Description of a module or primitive  instance type
//
struct vl_mp_inst_list : public vl_stmt
{
    vl_mp_inst_list()
        { name = 0; mptype = MPundef; mps = 0; params_or_delays = 0; }
    vl_mp_inst_list(MPtype, char*, vl_dlstr*, lsList<vl_mp_inst*>*);
    ~vl_mp_inst_list();
    vl_mp_inst_list *copy();
    void init();
    void setup(vl_simulator*);
    void print(ostream&);
    void dumpvars(ostream&, vl_simulator*);
    const char *lterm() { return (";\n"); }

    const char *name;             // name of master
    MPtype mptype;                // type of object in list
    lsList<vl_mp_inst*> *mps;     // list of instances
    vl_strength strength;
    vl_delay *params_or_delays;   // param overrides (in a vl_delay struct)
};


//---------------------------------------------------------------------------
//  Statements
//---------------------------------------------------------------------------

// Basic assignment description
//
struct vl_bassign_stmt : public vl_stmt
{
    vl_bassign_stmt(short, vl_var*, vl_event_expr*, vl_delay*, vl_var*);
    ~vl_bassign_stmt();
    vl_bassign_stmt *copy();
    void init();
    void setup(vl_simulator*);
    EVtype eval(vl_event*, vl_simulator*);
    void freeze_indices(vl_range*);
    void print(ostream&);
    const char *lterm() { return (";\n"); }

    vl_var *lhs;              // receiving variable
    vl_range *range;          // bit/part select for receiver
    vl_var *rhs;              // source expr
    vl_delay *delay;          // #wait a = #delay b
    vl_delay *wait;
    vl_event_expr *event;     // only one of event, delay is not 0
    vl_var *tmpvar;           // temporary transfer variable
};

// System task statememt description
//
struct vl_sys_task_stmt : public vl_stmt
{
    vl_sys_task_stmt() { name = 0; args = 0; dtype = DSPall; flags = 0; }
    vl_sys_task_stmt(char*, lsList<vl_expr*>*);
    ~vl_sys_task_stmt();
    vl_sys_task_stmt *copy();
    void setup(vl_simulator*);
    EVtype eval(vl_event*, vl_simulator*);
    void print(ostream&);
    const char *lterm() { return (";\n"); }

    const char *name;
    vl_var& (vl_simulator::*action)(vl_sys_task_stmt*, lsList<vl_expr*>*);
    lsList<vl_expr*> *args;                
    DSPtype dtype;
    unsigned short flags;
};

// Flags for vl_sys_task_stmt;
#define SYSafter     0x1
#define SYSno_nl     0x2

// Begin/end statement description
//
struct vl_begin_end_stmt : public vl_stmt
{
    vl_begin_end_stmt() { name = 0; decls = 0; stmts = 0; sig_st = 0;
        blk_st = 0; }
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

    const char *name;
    lsList<vl_decl*> *decls;
    lsList<vl_stmt*> *stmts;
    table<vl_var*> *sig_st;
    table<vl_stmt*> *blk_st;
};

// If/else statement description
//
struct vl_if_else_stmt : public vl_stmt
{
    vl_if_else_stmt() { cond = 0; if_stmt = else_stmt = 0; }
    vl_if_else_stmt(vl_expr*, vl_stmt*, vl_stmt*);
    ~vl_if_else_stmt() { delete cond; delete if_stmt; delete else_stmt; }
    vl_if_else_stmt *copy();
    void init();
    void setup(vl_simulator*);
    EVtype eval(vl_event*, vl_simulator*);
    void disable(vl_stmt*);
    void print(ostream&);
    void dumpvars(ostream&, vl_simulator*);
    const char *lterm() { return (""); }

    vl_expr *cond;
    vl_stmt *if_stmt;
    vl_stmt *else_stmt;
};

// Case statement description
//
struct vl_case_stmt : public vl_stmt
{
    vl_case_stmt() { cond = 0; case_items = 0; }
    vl_case_stmt(short, vl_expr*, lsList<vl_case_item*>*);
    ~vl_case_stmt();
    vl_case_stmt *copy();
    void init();
    void setup(vl_simulator*);
    EVtype eval(vl_event*, vl_simulator*);
    void disable(vl_stmt*);
    void print(ostream&);
    void dumpvars(ostream&, vl_simulator*);

    vl_expr *cond;
    lsList<vl_case_item*> *case_items;
};

// Case item description
//
struct vl_case_item : public vl_stmt
{
    vl_case_item() { exprs = 0; stmt = 0; }
    vl_case_item(short, lsList<vl_expr*>*, vl_stmt*);
    ~vl_case_item();
    vl_case_item *copy();
    void init();
    void disable(vl_stmt*);
    void print(ostream&);
    void dumpvars(ostream&, vl_simulator*);

    lsList<vl_expr*> *exprs;
    vl_stmt *stmt;
};

// Forever statement description
//
struct vl_forever_stmt : public vl_stmt
{
    vl_forever_stmt() { stmt = 0; }
    vl_forever_stmt(vl_stmt*);
    ~vl_forever_stmt() { delete stmt; }
    vl_forever_stmt *copy();
    void init();
    void setup(vl_simulator*);
    EVtype eval(vl_event*, vl_simulator*);
    void disable(vl_stmt*);
    void print(ostream&);
    void dumpvars(ostream&, vl_simulator*);
    const char *lterm() { return (""); }

    vl_stmt *stmt;
};

// Repeat statement description
//
struct vl_repeat_stmt : public vl_stmt
{
    vl_repeat_stmt() { count = 0; stmt = 0; cur_count = 0; }
    vl_repeat_stmt(vl_expr*, vl_stmt*);
    ~vl_repeat_stmt() { delete count; delete stmt; }
    vl_repeat_stmt *copy();
    void init();
    void setup(vl_simulator*);
    EVtype eval(vl_event*, vl_simulator*);
    void disable(vl_stmt*);
    void print(ostream&);
    void dumpvars(ostream&, vl_simulator*);
    const char *lterm() { return (""); }

    vl_expr *count;
    vl_stmt *stmt;
    int cur_count;
};

// While statement description
//
struct vl_while_stmt : public vl_stmt
{
    vl_while_stmt() { cond = 0; stmt = 0; }
    vl_while_stmt(vl_expr*, vl_stmt*);
    ~vl_while_stmt() { delete cond; delete stmt; }
    vl_while_stmt *copy();
    void init();
    void setup(vl_simulator*);
    EVtype eval(vl_event*, vl_simulator*);
    void disable(vl_stmt*);
    void print(ostream&);
    void dumpvars(ostream&, vl_simulator*);
    const char *lterm() { return (""); }

    vl_expr *cond;
    vl_stmt *stmt;
};

// For statement description
//
struct vl_for_stmt : public vl_stmt
{
    vl_for_stmt() { initial = end = 0; cond = 0; stmt = 0; }
    vl_for_stmt(vl_bassign_stmt*, vl_expr*, vl_bassign_stmt*, vl_stmt*);
    ~vl_for_stmt() { delete initial; delete end; delete cond; delete stmt; }
    vl_for_stmt *copy();
    void init();
    void setup(vl_simulator*);
    EVtype eval(vl_event*, vl_simulator*);
    void disable(vl_stmt*);
    void print(ostream&);
    void dumpvars(ostream&, vl_simulator*);
    const char *lterm() { return (""); }

    vl_bassign_stmt *initial;
    vl_bassign_stmt *end;
    vl_expr *cond;
    vl_stmt *stmt;
};

// Delay control statement description
//
struct vl_delay_control_stmt : public vl_stmt
{
    vl_delay_control_stmt() { delay = 0; stmt = 0; }
    vl_delay_control_stmt(vl_delay*, vl_stmt*);
    ~vl_delay_control_stmt() { delete delay; delete stmt; }
    vl_delay_control_stmt *copy();
    void init();
    void setup(vl_simulator*);
    EVtype eval(vl_event*, vl_simulator*);
    void disable(vl_stmt*);
    void print(ostream&);
    void dumpvars(ostream&, vl_simulator*);
    const char *lterm() { return (""); }

    vl_delay *delay;
    vl_stmt *stmt;
};

// Event control statement description
//
struct vl_event_control_stmt : public vl_stmt
{
    vl_event_control_stmt() { event = 0; stmt = 0;}
    vl_event_control_stmt(vl_event_expr*, vl_stmt*);
    ~vl_event_control_stmt() { delete event; delete stmt; }
    vl_event_control_stmt *copy();
    void init();
    void setup(vl_simulator*);
    EVtype eval(vl_event*, vl_simulator*);
    void disable(vl_stmt*);
    void print(ostream&);
    void dumpvars(ostream&, vl_simulator*);
    const char *lterm() { return (""); }

    vl_event_expr *event;
    vl_stmt *stmt;
};

// Wait statement description
//
struct vl_wait_stmt : public vl_stmt
{
    vl_wait_stmt() { cond = 0; stmt = 0; event = 0; }
    vl_wait_stmt(vl_expr*, vl_stmt*);
    ~vl_wait_stmt() { delete cond; delete stmt;
        if (event) { event->expr = 0; delete event; } }
    vl_wait_stmt *copy();
    void init();
    void setup(vl_simulator*);
    EVtype eval(vl_event*, vl_simulator*);
    void disable(vl_stmt*);
    void print(ostream&);
    void dumpvars(ostream&, vl_simulator*);
    const char *lterm() { return (""); }

    vl_expr *cond;
    vl_stmt *stmt;
    vl_event_expr *event;
};

// Send event statement description
//
struct vl_send_event_stmt : public vl_stmt
{
    vl_send_event_stmt() { name = 0; }
    vl_send_event_stmt(char*);
    ~vl_send_event_stmt() { delete [] name; }
    vl_send_event_stmt *copy();
    void setup(vl_simulator*);
    EVtype eval(vl_event*, vl_simulator*);
    void print(ostream&);
    const char *lterm() { return (";\n"); }

    const char *name;
};

// Fork/join statement description
//
struct vl_fork_join_stmt : public vl_stmt
{
    vl_fork_join_stmt() { name = 0; decls = 0; stmts = 0; sig_st = 0;
        blk_st = 0; endcnt = 0; }
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

    const char *name;
    lsList<vl_decl*> *decls;
    lsList<vl_stmt*> *stmts;
    table<vl_var*> *sig_st;
    table<vl_stmt*> *blk_st;
    int endcnt;
};

// This is used to indicate the end of a thread in a fork/join block
//
struct vl_fj_break : public vl_stmt
{
    vl_fj_break(vl_fork_join_stmt *f, vl_begin_end_stmt *e)
        { type = ForkJoinBreak; fjblock = f; begin_end = e; }

    void setup(vl_simulator*);
    EVtype eval(vl_event*, vl_simulator*);
    void print(ostream&);

    vl_fork_join_stmt *fjblock;         // originating block
    vl_begin_end_stmt *begin_end;       // created thread container
};

// Task enable statement description
//
struct vl_task_enable_stmt : public vl_stmt
{
    vl_task_enable_stmt() { name = 0; args = 0; task = 0; }
    vl_task_enable_stmt(short, char*, lsList<vl_expr*>*);
    ~vl_task_enable_stmt();
    vl_task_enable_stmt *copy();
    void setup(vl_simulator*);
    EVtype eval(vl_event*, vl_simulator*);
    void print(ostream&);

    const char *name;
    lsList<vl_expr*> *args;
    vl_task *task;
};

// Disable statement description
//
struct vl_disable_stmt : public vl_stmt
{
    vl_disable_stmt() { name = 0; target = 0; }
    vl_disable_stmt(char*);
    ~vl_disable_stmt() { delete [] name; }
    vl_disable_stmt *copy();
    void setup(vl_simulator*);
    EVtype eval(vl_event*, vl_simulator*);
    void print(ostream&);
    const char *lterm() { return (";\n"); }

    const char *name;
    vl_stmt *target;
};

// Deassign statement description
//
struct vl_deassign_stmt : public vl_stmt
{
    vl_deassign_stmt() { lhs = 0; }
    vl_deassign_stmt(short, vl_var*);
    ~vl_deassign_stmt();
    vl_deassign_stmt *copy();
    void init();
    void setup(vl_simulator*);
    EVtype eval(vl_event*, vl_simulator*);
    void print(ostream&);
    const char *lterm() { return (";\n"); }

    vl_var *lhs;
};


//---------------------------------------------------------------------------
//  Instances
//---------------------------------------------------------------------------

// Base class for instances
//
struct vl_inst : public vl_stmt
{
    const char *name;
};

// Gate instance
//
struct vl_gate_inst : public vl_inst
{
    vl_gate_inst();
    vl_gate_inst(char*, lsList<vl_expr*>*, vl_range*);
    ~vl_gate_inst();
    vl_gate_inst *copy();
    void print(ostream &outs) { outs << string; }
    const char *lterm() { return (";\n"); }
    void setup(vl_simulator*);
    EVtype eval(vl_event*, vl_simulator*);
    void set_type(int);
    void set_delay(vl_simulator*, int);

    lsList<vl_expr*> *terms;
    lsList<vl_var*> *outputs;
    bool (*gsetup)(vl_simulator*, vl_gate_inst*);
    bool (*geval)(vl_simulator*, int(*)(int, int), vl_gate_inst*);
    int (*gset)(int, int);
    const char *string;
    vl_gate_inst_list *inst_list;
    vl_delay *delay;    // rise/fall/turn-off
    vl_range *array;    // arrayed size
};

// Module or primitive instance
//
struct vl_mp_inst : public vl_inst
{
    vl_mp_inst() { name = 0; ports = 0; inst_list = 0; master = 0; }
    vl_mp_inst(char*, lsList<vl_port_connect*>*);
    ~vl_mp_inst();
    vl_mp_inst *copy();
    void setup(vl_simulator*);
    EVtype eval(vl_event*, vl_simulator*);
    void link_ports(vl_simulator*);
    void port_setup(vl_simulator*, vl_port_connect*, vl_port*, int);
    void dumpvars(ostream&, vl_simulator*);

    lsList<vl_port_connect*> *ports;
    vl_mp_inst_list *inst_list;   // m/p inst_list
    vl_mp *master;                // module or primitive copy
};


//---------------------------------------------------------------------------
//  Exported globals
//---------------------------------------------------------------------------

// vl_parse.cc
extern vl_parser VP;

// vl_print.cc
extern void vl_error(const char*, ...);
extern void vl_warn(const char*, ...);
extern const char *vl_datestring();
extern const char *vl_version();
extern char *vl_fix_str(const char*);
extern char *vl_strdup(const char*);

#define errout(x) cout << (x) << "\n---\n"
