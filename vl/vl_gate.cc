
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

//
// Built-in gate primitives.  All terminals are scalars.
//

namespace {
    const char *msg = "gate %s has too few terminals";
    const char *msg1 = "gate %s has null terminal %d";
    const char *msg2 = "(internal) gate %s not initialized";
    const char *msg3 = "gate %s has non-bitfield terminal %d";
    const char *msg4 =
        "arrayed gate %s, terminal %d has bit width less than array size";
    const char *msg5 = "gate %s has non-lvalue output terminal %d";
}

//---------------------------------------------------------------------------
//  Local
//---------------------------------------------------------------------------

namespace {
    inline int op_not(int i)
    {
        if (i == BitH)
            return (BitL);
        if (i == BitL)
            return (BitH);
        return (BitDC);
    }


    inline int set_and(int i1, int i2)
    {
        if (i1 == BitL || i2 == BitL)
            return (BitL);
        if (i1 == BitH && i2 == BitH)
            return (BitH);
        return (BitDC);
    }


    inline int set_nand(int i1, int i2)
    {
        if (i1 == BitL || i2 == BitL)
            return (BitH);
        if (i1 == BitH && i2 == BitH)
            return (BitL);
        return (BitDC);
    }


    inline int set_or(int i1, int i2)
    {
        if (i1 == BitH || i2 == BitH)
            return (BitH);
        if (i1 == BitL && i2 == BitL)
            return (BitL);
        return (BitDC);
    }


    inline int set_nor(int i1, int i2)
    {
        if (i1 == BitH || i2 == BitH)
            return (BitL);
        if (i1 == BitL && i2 == BitL)
            return (BitH);
        return (BitDC);
    }


    inline int set_xor(int i1, int i2)
    {
        if ((i1 == BitH && i2 == BitL) || (i1 == BitL && i2 == BitH))
            return (BitH);
        if ((i1 == BitH && i2 == BitH) || (i1 == BitL && i2 == BitL))
            return (BitL);
        return (BitDC);
    }


    inline int set_xnor(int i1, int i2)
    {
        if ((i1 == BitH && i2 == BitL) || (i1 == BitL && i2 == BitH))
            return (BitL);
        if ((i1 == BitH && i2 == BitH) || (i1 == BitL && i2 == BitL))
            return (BitH);
        return (BitDC);
    }


    inline int set_buf(int i1, int)
    {
        return (i1);
    }


    inline int set_bufif0(int i1, int c)
    {
        if (c == BitH)
            return (BitZ);
        if (c == BitL)
            return (i1 == BitZ ? BitDC : i1);
        if (i1 == BitL)
            return (BitL);
        if (i1 == BitH)
            return (BitH);
        return (BitDC);
    }


    inline int set_bufif1(int i1, int c)
    {
        if (c == BitL)
            return (BitZ);
        if (c == BitH)
            return (i1 == BitZ ? BitDC : i1);
        if (i1 == BitL)
            return (BitL);
        if (i1 == BitH)
            return (BitH);
        return (BitDC);
    }


    inline int set_not(int i1, int)
    {
        return (op_not(i1));
    }


    inline int set_notif0(int i1, int c)
    {
        if (c == BitH)
            return (BitZ);
        if (c == BitL)
            return (op_not(i1));
        if (i1 == BitL)
            return (BitH);
        if (i1 == BitH)
            return (BitL);
        return (BitDC);
    }


    inline int set_notif1(int i1, int c)
    {
        if (c == BitL)
            return (BitZ);
        if (c == BitH)
            return (op_not(i1));
        if (i1 == BitL)
            return (BitH);
        if (i1 == BitH)
            return (BitL);
        return (BitDC);
    }


    inline int set_nmos(int d, int c)
    {
        if (c == BitL)
            return (BitZ);
        return (d);
    }


    inline int set_pmos(int d, int c)
    {
        if (c == BitH)
            return (BitZ);
        return (d);
    }
}


//---------------------------------------------------------------------------
//  Instances
//---------------------------------------------------------------------------

// Set the delay statement according to the specified delays and the
// transition.
//
void
vl_gate_inst::set_delay(vl_simulator*, int bit)
{
    if (gi_inst_list && gi_inst_list->delays()) {
        if (!gi_delay)
            gi_delay = new vl_delay;
        vl_expr *rd = 0, *fd = 0, *td = 0;
        if (gi_inst_list->delays()->list) {
            vl_expr *e;
            lsGen<vl_expr*> gen(gi_inst_list->delays()->list);
            if (gen.next(&e)) {
                rd = e;
                if (gen.next(&e)) {
                    fd = e;
                    if (gen.next(&e))
                        td = e;
                }
            }
        }
        else
            rd = gi_inst_list->delays()->delay1;
        if (bit == BitH)
            gi_delay->delay1 = rd;
        else if (bit == BitL)
            gi_delay->delay1 = (fd ? fd : rd);
        else if (bit == BitZ) {
            if (td)
                gi_delay->delay1 = td;
            else {
                gi_delay->delay1 = rd;
                int m = rd->eval();
                if (fd) {
                    int tmp = fd->eval();
                    if (tmp < m)
                        gi_delay->delay1 = fd;
                }
            }
        }
        else if (bit == BitDC) {
            // minimum delay
            int m = rd->eval();
            gi_delay->delay1 = rd;
            if (fd) {
                int tmp = fd->eval();
                if (tmp < m) {
                    m = tmp;
                    gi_delay->delay1 = fd;
                }
            }
            if (td) {
                int tmp = td->eval();
                if (tmp < m) {
                    m = tmp;
                    gi_delay->delay1 = td;
                }
            }
        }
    }
}


// Configure the struct for a particular gate type.  This could be done
// more elegantly, but it minimizes indirection.
//
void
vl_gate_inst::set_type(int t)
{
    st_type = t;
    switch (st_type) {
    case AndGate:
        gi_setup = gsetup_gate;
        gi_eval = geval_gate;
        gi_set = set_and;
        gi_string = "and";
        break;
    case NandGate:
        gi_setup = gsetup_gate;
        gi_eval = geval_gate;
        gi_set = set_nand;
        gi_string = "nand";
        break;
    case OrGate:
        gi_setup = gsetup_gate;
        gi_eval = geval_gate;
        gi_set = set_or;
        gi_string = "or";
        break;
    case NorGate:
        gi_setup = gsetup_gate;
        gi_eval = geval_gate;
        gi_set = set_nor;
        gi_string = "nor";
        break;
    case XorGate:
        gi_setup = gsetup_gate;
        gi_eval = geval_gate;
        gi_set = set_xor;
        gi_string = "xor";
        break;
    case XnorGate:
        gi_setup = gsetup_gate;
        gi_eval = geval_gate;
        gi_set = set_xnor;
        gi_string = "xnor";
        break;
    case BufGate:
        gi_setup = gsetup_buf;
        gi_eval = geval_buf;
        gi_set = set_buf;
        gi_string = "buf";
        break;
    case Bufif0Gate:
        gi_setup = gsetup_cbuf;
        gi_eval = geval_cbuf;
        gi_set = set_bufif0;
        gi_string = "bufif0";
        break;
    case Bufif1Gate:
        gi_setup = gsetup_cbuf;
        gi_eval = geval_cbuf;
        gi_set = set_bufif1;
        gi_string = "bufif1";
        break;
    case NotGate:
        gi_setup = gsetup_buf;
        gi_eval = geval_buf;
        gi_set = set_not;
        gi_string = "not";
        break;
    case Notif0Gate:
        gi_setup = gsetup_cbuf;
        gi_eval = geval_cbuf;
        gi_set = set_notif0;
        gi_string = "notif0";
        break;
    case Notif1Gate:
        gi_setup = gsetup_cbuf;
        gi_eval = geval_cbuf;
        gi_set = set_notif1;
        gi_string = "notif1";
        break;
    case PulldownGate:
        gi_setup = 0;
        gi_eval = 0;
        gi_set = 0;
        gi_string = "pulldown";
        break;
    case PullupGate:
        gi_setup = 0;
        gi_eval = 0;
        gi_set = 0;
        gi_string = "pullup";
        break;
    case NmosGate:
        gi_setup = gsetup_mos;
        gi_eval = geval_mos;
        gi_set = set_nmos;
        gi_string = "nmos";
        break;
    case RnmosGate:
        gi_setup = gsetup_mos;
        gi_eval = geval_mos;
        gi_set = set_nmos;
        gi_string = "rnmos";
        break;
    case PmosGate:
        gi_setup = gsetup_mos;
        gi_eval = geval_mos;
        gi_set = set_pmos;
        gi_string = "pmos";
        break;
    case RpmosGate:
        gi_setup = gsetup_mos;
        gi_eval = geval_mos;
        gi_set = set_pmos;
        gi_string = "rpmos";
        break;
    case CmosGate:
        gi_setup = gsetup_cmos;
        gi_eval = geval_cmos;
        gi_set = 0;
        gi_string = "cmos";
        break;
    case RcmosGate:
        gi_setup = gsetup_cmos;
        gi_eval = geval_cmos;
        gi_set = 0;
        gi_string = "rcmos";
        break;
    case TranGate:
        gi_setup = gsetup_tran;
        gi_eval = geval_tran;
        gi_set = 0;
        gi_string = "tran";
        break;
    case RtranGate:
        gi_setup = gsetup_tran;
        gi_eval = geval_tran;
        gi_set = 0;
        gi_string = "rtran";
        break;
    case Tranif0Gate:
        gi_setup = gsetup_ctran;
        gi_eval = geval_ctran;
        gi_set = 0;
        gi_string = "tranif0";
        break;
    case Rtranif0Gate:
        gi_setup = gsetup_ctran;
        gi_eval = geval_ctran;
        gi_set = 0;
        gi_string = "rtranif0";
        break;
    case Tranif1Gate:
        gi_setup = gsetup_ctran;
        gi_eval = geval_ctran;
        gi_set = 0;
        gi_string = "tranif0";
        break;
    case Rtranif1Gate:
        gi_setup = gsetup_ctran;
        gi_eval = geval_ctran;
        gi_set = 0;
        gi_string = "rtranif0";
        break;
    default:
        VP()->error(ERR_INTERNAL, "Unexpected Gate Type");
        break;
    }
}


// Static function.
//
bool
vl_gate_inst::gsetup_gate(vl_simulator*, vl_gate_inst *gate)
{
    // gate type
    const char *nm = gate->name() ? gate->name() : "";
    if (gate->gi_terms->length() < 3) {
        vl_error(msg, nm);
        return (false);
    }
    lsGen<vl_expr*> gen(gate->gi_terms);

    vl_array a;
    if (gate->gi_array)
        a.set(gate->gi_array);
    if (!a.size())
        a.set(1);

    // output
    vl_expr *expr = 0;
    gen.next(&expr);
    if (!expr) {
        vl_error(msg1, nm, 1);
        return (false);
    }
    gate->gi_outputs = new lsList<vl_var*>;
    vl_var *vo = new vl_var;
    vo->set_net_type(REGreg);
    vo->set_strength(gate->gi_inst_list->strength());
    vo->setx(a.size());
    gate->gi_outputs->newEnd(vo);
    vl_var *vs = expr->source();
    vl_range *r = expr->source_range();
    if (!vs) {
        vl_error(msg5, nm, 1);
        return (false);
    }
    vs->assign(r, vo, 0);

    // inputs
    int n = 1;
    while (gen.next(&expr)) {
        if (!expr) {
            vl_error(msg1, nm, n);
            return (false);
        }
        expr->chain(gate);
        n++;
    }
    return (true);
}


// Static function.
//
bool
vl_gate_inst::gsetup_buf(vl_simulator*, vl_gate_inst *gate)
{
    // buffer type
    const char *nm = gate->name() ? gate->name() : "";
    int num = gate->gi_terms->length();
    if (num < 2) {
        vl_error(msg, nm);
        return (false);
    }
    lsGen<vl_expr*> gen(gate->gi_terms, true);  // start at end

    vl_array a;
    if (gate->gi_array)
        a.set(gate->gi_array);
    if (!a.size())
        a.set(1);

    // input
    vl_expr *expr = 0;
    gen.prev(&expr);
    if (!expr) {
        vl_error(msg1, nm, num);
        return (false);
    }
    expr->chain(gate);

    // outputs
    int n = 1;
    gate->gi_outputs = new lsList<vl_var*>;
    while (gen.prev(&expr)) {
        if (!expr) {
            vl_error(msg1, nm, num - n);
            return (false);
        }
        vl_var *vo = new vl_var;
        vo->set_net_type(REGreg);
        vo->set_strength(gate->gi_inst_list->strength());
        vo->setx(a.size());
        gate->gi_outputs->newEnd(vo);
        vl_var *vs = expr->source();
        if (!vs) {
            vl_error(msg5, nm, num - n);
            return (false);
        }
        vl_range *r = expr->source_range();
        vs->assign(r, vo, 0);
        n++;
    }
    return (true);
}


// Static function.
//
bool
vl_gate_inst::gsetup_cbuf(vl_simulator*, vl_gate_inst *gate)
{
    // buffer type with control input
    const char *nm = gate->name() ? gate->name() : "";
    int num = gate->gi_terms->length();
    if (num < 3) {
        vl_error(msg, nm);
        return (false);
    }
    lsGen<vl_expr*> gen(gate->gi_terms, true);  // start at end

    vl_array a;
    if (gate->gi_array)
        a.set(gate->gi_array);
    if (!a.size())
        a.set(1);

    // control
    vl_expr *expr = 0;
    gen.prev(&expr);
    if (!expr) {
        vl_error(msg1, nm, num);
        return (false);
    }
    expr->chain(gate);

    // input
    gen.prev(&expr);
    if (!expr) {
        vl_error(msg1, nm, num-1);
        return (false);
    }
    expr->chain(gate);

    // outputs
    int n = 2;
    gate->gi_outputs = new lsList<vl_var*>;
    while (gen.prev(&expr)) {
        if (!expr) {
            vl_error(msg1, nm, num - n);
            return (false);
        }
        vl_var *vo = new vl_var;
        vo->set_net_type(REGreg);
        vo->set_strength(gate->gi_inst_list->strength());
        vo->setz(a.size());
        gate->gi_outputs->newEnd(vo);
        n++;
    }
    return (true);
}


// Static function.
//
bool
vl_gate_inst::gsetup_mos(vl_simulator*, vl_gate_inst *gate)
{
    // MOS gate
    const char *nm = gate->name() ? gate->name() : "";
    if (gate->gi_terms->length() != 3) {
        vl_error("mos gate %s does not have 3 terminals", nm);
        return (false);
    }
    lsGen<vl_expr*> gen(gate->gi_terms);

    vl_array a;
    if (gate->gi_array)
        a.set(gate->gi_array);
    if (!a.size())
        a.set(1);

    // output
    vl_expr *expr = 0;
    gen.next(&expr);
    if (!expr) {
        vl_error(msg1, nm, 1);
        return (false);
    }
    gate->gi_outputs = new lsList<vl_var*>;
    vl_var *vo = new vl_var;
    vo->set_net_type(REGreg);
    vo->set_strength(gate->gi_inst_list->strength());
    vo->setz(a.size());
    gate->gi_outputs->newEnd(vo);

    // input
    gen.next(&expr);
    if (!expr) {
        vl_error(msg1, nm, 2);
        return (false);
    }
    expr->chain(gate);

    // input
    gen.next(&expr);
    if (!expr) {
        vl_error(msg1, nm, 3);
        return (false);
    }
    expr->chain(gate);
    return (true);
}


// Static function.
//
bool
vl_gate_inst::gsetup_cmos(vl_simulator*, vl_gate_inst *gate)
{
    // CMOS gate
    const char *nm = gate->name() ? gate->name() : "";
    if (gate->gi_terms->length() != 4) {
        vl_error("cmos gate %s does not have 4 terminals", nm);
        return (false);
    }
    lsGen<vl_expr*> gen(gate->gi_terms);

    vl_array a;
    if (gate->gi_array)
        a.set(gate->gi_array);
    if (!a.size())
        a.set(1);

    // output
    vl_expr *expr = 0;
    gen.next(&expr);
    if (!expr) {
        vl_error(msg1, nm, 1);
        return (false);
    }
    gate->gi_outputs = new lsList<vl_var*>;
    vl_var *vo = new vl_var;
    vo->set_net_type(REGreg);
    vo->set_strength(gate->gi_inst_list->strength());
    vo->setz(a.size());
    gate->gi_outputs->newEnd(vo);

    // input
    gen.next(&expr);
    if (!expr) {
        vl_error(msg1, nm, 2);
        return (false);
    }
    expr->chain(gate);

    // input
    gen.next(&expr);
    if (!expr) {
        vl_error(msg1, nm, 3);
        return (false);
    }
    expr->chain(gate);

    // input
    gen.next(&expr);
    if (!expr) {
        vl_error(msg1, nm, 4);
        return (false);
    }
    expr->chain(gate);
    return (true);
}


// Static function.
//
bool
vl_gate_inst::gsetup_tran(vl_simulator*, vl_gate_inst *gate)
{
    const char *nm = gate->name() ? gate->name() : "";
    if (gate->gi_terms->length() != 2) {
        vl_error("tran gate %s does not have 2 terminals", nm);
        return (false);
    }
    lsGen<vl_expr*> gen(gate->gi_terms);

    vl_array a;
    if (gate->gi_array)
        a.set(gate->gi_array);
    if (!a.size())
        a.set(1);

    // inout
    vl_expr *expr1 = 0;
    gen.next(&expr1);
    if (!expr1) {
        vl_error(msg1, nm, 1);
        return (false);
    }
    expr1->eval();
    expr1->chain(gate);
    gate->gi_outputs = new lsList<vl_var*>;
    vl_var *vo = new vl_var;
    vo->set_net_type(REGreg);
    vo->set_strength(gate->gi_inst_list->strength());
    vo->setz(a.size());
    gate->gi_outputs->newEnd(vo);

    // inout
    vl_expr *expr2 = 0;
    gen.next(&expr2);
    if (!expr2) {
        vl_error(msg1, nm, 2);
        return (false);
    }
    expr2->eval();
    expr2->chain(gate);
    vo = new vl_var;
    vo->set_net_type(REGreg);
    vo->set_strength(gate->gi_inst_list->strength());
    vo->setz(a.size());
    gate->gi_outputs->newEnd(vo);

    return (true);
}


// Static function.
//
bool
vl_gate_inst::gsetup_ctran(vl_simulator*, vl_gate_inst *gate)
{
    const char *nm = gate->name() ? gate->name() : "";
    if (gate->gi_terms->length() != 2) {
        vl_error("tranif gate %s does not have 3 terminals", nm);
        return (false);
    }
    lsGen<vl_expr*> gen(gate->gi_terms);

    vl_array a;
    if (gate->gi_array)
        a.set(gate->gi_array);
    if (!a.size())
        a.set(1);

    // inout
    vl_expr *expr1 = 0;
    gen.next(&expr1);
    if (!expr1) {
        vl_error(msg1, nm, 1);
        return (false);
    }
    expr1->eval();
    expr1->chain(gate);
    gate->gi_outputs = new lsList<vl_var*>;
    vl_var *vo = new vl_var;
    vo->set_net_type(REGreg);
    vo->set_strength(gate->gi_inst_list->strength());
    vo->setz(a.size());
    gate->gi_outputs->newEnd(vo);

    // inout
    vl_expr *expr2 = 0;
    gen.next(&expr2);
    if (!expr2) {
        vl_error(msg1, nm, 2);
        return (false);
    }
    expr2->eval();
    expr2->chain(gate);
    vo = new vl_var;
    vo->set_net_type(REGreg);
    vo->set_strength(gate->gi_inst_list->strength());
    vo->setz(a.size());
    gate->gi_outputs->newEnd(vo);

    // control
    vl_expr *expr3 = 0;
    gen.next(&expr3);
    if (!expr3) {
        vl_error(msg1, nm, 3);
        return (false);
    }

    return (true);
}


// Static function.
//
bool
vl_gate_inst::geval_gate(vl_simulator *sim, int(*set)(int, int),
    vl_gate_inst *gate)
{
    // gate type: 1 output, 2 or more inputs
    const char *nm = gate->name() ? gate->name() : "";
    if (!set) {
        vl_error(msg2, nm);
        return (false);
    }
    lsGen<vl_expr*> gen(gate->gi_terms);

    vl_array a;
    if (gate->gi_array)
        a.set(gate->gi_array);
    if (!a.size())
        a.set(1);

    // output
    vl_expr *oexp = 0;
    gen.next(&oexp);

    // inputs
    int n = gate->gi_terms->length() - 1;
    vl_var **iv = new vl_var*[n];
    vl_expr *expr;
    int cnt = 0;
    while (gen.next(&expr)) {
        iv[cnt] = &expr->eval();
        if (iv[cnt]->data_type() != Dbit) {
            vl_error(msg3, nm, cnt+2);
            return (false);
        }
        if (iv[cnt]->bits().size() != 1 && iv[cnt]->bits().size() < a.size()) {
            vl_error(msg4, nm, cnt+2);
            return (false);
        }
        cnt++;
    }

    bool rfdly = false;
    if (gate->gi_inst_list && gate->gi_inst_list->delays() &&
            gate->gi_inst_list->delays()->list &&
            gate->gi_inst_list->delays()->list->length() > 1)
        // use separate rise/fall delays
        rfdly = true;

    lsGen<vl_var*> ogen(gate->gi_outputs);
    vl_var *v;
    if (ogen.next(&v)) {
        vl_var *vs = oexp->source();
        if (!vs) {
            vl_error(msg5, nm, 1);
            return (false);
        }
        bool changed = false;
        for (int i = 0; i < a.size(); i++) {
            int obit = (iv[0]->bits().size() > 1 ?
                iv[0]->data_s()[i] : iv[0]->data_s()[0]);
            for (int j = 1; j < n; j++) {
                int o = (iv[j]->bits().size() > 1 ?
                    iv[j]->data_s()[i] : iv[j]->data_s()[0]);
                obit = (*set)(obit, o);
            }

            int io = (v->bits().size() > 1 ? i : 0);
            if (v->data_s()[io] != obit) {
                v->data_s()[io] = obit;
                changed = true;
                if (rfdly) {
                    // have to assign each val separately, delays may differ
                    gate->set_delay(sim, obit);
                    vl_time_t td = gate->gi_delay->eval();
                    vl_bassign_stmt *bs =
                        new vl_bassign_stmt(BassignStmt, vs, 0, 0, v);
                    bs->or_flags(SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
                    bs->set_range(chk_copy(oexp->source_range()));
                    vl_action_item *ai = new vl_action_item(bs, sim->context());
                    ai->or_flags(AI_DEL_STMT);
                    sim->timewheel()->append(sim->time() + td, ai);
                }
            }
        }
        if (changed && !rfdly) {
            if (gate->gi_inst_list && gate->gi_inst_list->delays()) {
                gate->set_delay(sim, BitH);
                vl_time_t td = gate->gi_delay->eval();
                vl_bassign_stmt *bs =
                    new vl_bassign_stmt(BassignStmt, vs, 0, 0, v);
                bs->or_flags(SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
                bs->set_range(chk_copy(oexp->source_range()));
                vl_action_item *ai = new vl_action_item(bs, sim->context());
                ai->or_flags(AI_DEL_STMT);
                sim->timewheel()->append(sim->time() + td, ai);
            }
            else {
                vl_range *r = oexp->source_range();
                vs->assign(r, v, 0);
            }
        }
    }
    return (true);
}


// Static function.
//
bool
vl_gate_inst::geval_buf(vl_simulator *sim, int(*set)(int, int),
    vl_gate_inst *gate)
{
    // buffer type: 1 or more outputs, 1 input
    const char *nm = gate->name() ? gate->name() : "";
    if (!set) {
        vl_error(msg2, nm);
        return (false);
    }
    int num = gate->gi_terms->length();
    lsGen<vl_expr*> gen(gate->gi_terms, true);  // start at end

    vl_array a;
    if (gate->gi_array)
        a.set(gate->gi_array);
    if (!a.size())
        a.set(1);

    // input
    vl_expr *expr = 0;
    gen.prev(&expr);
    vl_var &ip = expr->eval();
    if (ip.data_type() != Dbit) {
        vl_error(msg3, nm, num);
        return (false);
    }
    if (ip.bits().size() != 1 && ip.bits().size() < a.size()) {
        vl_error(msg4, nm, num);
        return (false);
    }

    bool rfdly = false;
    if (gate->gi_inst_list && gate->gi_inst_list->delays() &&
            gate->gi_inst_list->delays()->list &&
            gate->gi_inst_list->delays()->list->length() > 1)
        // use separate rise/fall delays
        rfdly = true;

    lsGen<vl_var*> ogen(gate->gi_outputs);
    vl_var *v;
    int n = 1;
    while (ogen.next(&v) && gen.prev(&expr)) {
        vl_var *vs = expr->source();
        if (!vs) {
            vl_error(msg5, nm, num - n);
            return (false);
        }
        *v = expr->eval();
        bool changed = false;
        for (int i = 0; i < a.size(); i++) {
            int ii = (ip.bits().size() > 1 ? ip.data_s()[i] : ip.data_s()[0]);
            int obit = (*set)(ii, BitL);

            int io = (v->bits().size() > 1 ? i : 0);
            if (v->data_s()[io] != obit) {
                v->data_s()[io] = obit;
                changed = true;
                if (rfdly) {
                    // have to assign each val separately, delays may differ
                    gate->set_delay(sim, obit);
                    vl_time_t td = gate->gi_delay->eval();
                    vl_bassign_stmt *bs =
                        new vl_bassign_stmt(BassignStmt, vs, 0, 0, v);
                    bs->or_flags(SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
                    bs->set_range(chk_copy(expr->source_range()));
                    vl_action_item *ai = new vl_action_item(bs, sim->context());
                    ai->or_flags(AI_DEL_STMT);
                    sim->timewheel()->append(sim->time() + td, ai);
                }
            }
        }
        if (changed && !rfdly) {
            if (gate->gi_inst_list && gate->gi_inst_list->delays()) {
                gate->set_delay(sim, BitH);
                vl_time_t td = gate->gi_delay->eval();
                vl_bassign_stmt *bs =
                    new vl_bassign_stmt(BassignStmt, vs, 0, 0, v);
                bs->or_flags(SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
                bs->set_range(chk_copy(expr->source_range()));
                vl_action_item *ai = new vl_action_item(bs, sim->context());
                ai->or_flags(AI_DEL_STMT);
                sim->timewheel()->append(sim->time() + td, ai);
            }
            else {
                vl_range *r = expr->source_range();
                vs->assign(r, v, 0);
            }
        }
        n++;
    }
    return (true);
}


// Static function.
//
bool
vl_gate_inst::geval_cbuf(vl_simulator *sim, int(*set)(int, int),
    vl_gate_inst *gate)
{
    // buffer type with control input: 1 or more outputs, 2 inputs
    const char *nm = gate->name() ? gate->name() : "";
    if (!set) {
        vl_error(msg2, nm);
        return (false);
    }
    int num = gate->gi_terms->length();
    lsGen<vl_expr*> gen(gate->gi_terms, true);  // start at end

    vl_array a;
    if (gate->gi_array)
        a.set(gate->gi_array);
    if (!a.size())
        a.set(1);

    // control
    vl_expr *expr = 0;
    gen.prev(&expr);
    vl_var &c = expr->eval();
    if (c.data_type() != Dbit) {
        vl_error(msg3, nm, num);
        return (false);
    }
    if (c.bits().size() != 1 && c.bits().size() < a.size()) {
        vl_error(msg4, nm, num);
        return (false);
    }

    // input
    gen.prev(&expr);
    vl_var &ip = expr->eval();
    if (ip.data_type() != Dbit) {
        vl_error(msg3, nm, num-1);
        return (false);
    }
    if (ip.bits().size() != 1 && ip.bits().size() < a.size()) {
        vl_error(msg4, nm, num-1);
        return (false);
    }

    bool rfdly = false;
    if (gate->gi_inst_list && gate->gi_inst_list->delays() &&
            gate->gi_inst_list->delays()->list &&
            gate->gi_inst_list->delays()->list->length() > 1)
        // use separate rise/fall delays
        rfdly = true;

    lsGen<vl_var*> ogen(gate->gi_outputs);
    vl_var *v;
    int n = 2;
    while (ogen.next(&v) && gen.prev(&expr)) {
        vl_var *vs = expr->source();
        if (!vs) {
            vl_error(msg5, nm, num - n);
            return (false);
        }
        *v = expr->eval();
        bool changed = false;
        for (int i = 0; i < a.size(); i++) {
            int ii = (ip.bits().size() > 1 ? ip.data_s()[i] : ip.data_s()[0]);
            int ic = (c.bits().size() > 1 ? c.data_s()[i] : c.data_s()[0]);
            int obit = (*set)(ii, ic);

            int io = (v->bits().size() > 1 ? i : 0);
            if (v->data_s()[io] != obit) {
                v->data_s()[io] = obit;
                changed = true;
                if (rfdly) {
                    // have to assign each val separately, delays may differ
                    gate->set_delay(sim, obit);
                    vl_time_t td = gate->gi_delay->eval();
                    vl_bassign_stmt *bs =
                        new vl_bassign_stmt(BassignStmt, vs, 0, 0, v);
                    bs->or_flags(SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
                    bs->set_range(chk_copy(expr->source_range()));
                    vl_action_item *ai = new vl_action_item(bs, sim->context());
                    ai->or_flags(AI_DEL_STMT);
                    sim->timewheel()->append(sim->time() + td, ai);
                }
            }
        }
        if (changed && !rfdly) {
            if (gate->gi_inst_list && gate->gi_inst_list->delays()) {
                gate->set_delay(sim, BitH);
                vl_time_t td = gate->gi_delay->eval();
                vl_bassign_stmt *bs =
                    new vl_bassign_stmt(BassignStmt, vs, 0, 0, v);
                bs->or_flags(SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
                bs->set_range(chk_copy(expr->source_range()));
                vl_action_item *ai = new vl_action_item(bs, sim->context());
                ai->or_flags(AI_DEL_STMT);
                sim->timewheel()->append(sim->time() + td, ai);
            }
            else {
                vl_range *r = expr->source_range();
                vs->assign(r, v, 0);
            }
        }
        n++;
    }
    return (true);
}


// Static function.
//
bool
vl_gate_inst::geval_mos(vl_simulator *sim, int(*set)(int, int),
    vl_gate_inst *gate)
{
    // MOS gate: 1 output, 2 inputs
    const char *nm = gate->name() ? gate->name() : "";
    if (!set) {
        vl_error(msg2, nm);
        return (false);
    }
    lsGen<vl_expr*> gen(gate->gi_terms);

    vl_array a;
    if (gate->gi_array)
        a.set(gate->gi_array);
    if (!a.size())
        a.set(1);

    // output
    vl_expr *oexp = 0;
    gen.next(&oexp);

    // input
    vl_expr *expr = 0;
    gen.next(&expr);
    vl_var &d = expr->eval();
    if (d.data_type() != Dbit) {
        vl_error(msg3, nm, 2);
        return (false);
    }
    if (d.bits().size() != 1 && d.bits().size() < a.size()) {
        vl_error(msg4, nm, 2);
        return (false);
    }

    // input
    gen.next(&expr);
    vl_var &c = expr->eval();
    if (c.data_type() != Dbit) {
        vl_error(msg3, nm, 3);
        return (false);
    }
    if (c.bits().size() != 1 && c.bits().size() < a.size()) {
        vl_error(msg4, nm, 3);
        return (false);
    }

    bool rfdly = false;
    if (gate->gi_inst_list && gate->gi_inst_list->delays() &&
            gate->gi_inst_list->delays()->list &&
            gate->gi_inst_list->delays()->list->length() > 1)
        // use separate rise/fall delays
        rfdly = true;

    lsGen<vl_var*> ogen(gate->gi_outputs);
    vl_var *v;
    if (ogen.next(&v)) {
        vl_var *vs = oexp->source();
        if (!vs) {
            vl_error(msg5, nm, 1);
            return (false);
        }
        *v = oexp->eval();
        bool changed = false;
        for (int i = 0; i < a.size(); i++) {
            int ii = (d.bits().size() > 1 ? d.data_s()[i] : d.data_s()[0]);
            int ic = (c.bits().size() > 1 ? c.data_s()[i] : c.data_s()[0]);
            int obit = (*set)(ii, ic);

            int io = (v->bits().size() > 1 ? i : 0);
            if (v->data_s()[io] != obit) {
                v->data_s()[io] = obit;
                changed = true;
                if (rfdly) {
                    // have to assign each val separately, delays may differ
                    gate->set_delay(sim, obit);
                    vl_time_t td = gate->gi_delay->eval();
                    vl_bassign_stmt *bs =
                        new vl_bassign_stmt(BassignStmt, vs, 0, 0, v);
                    bs->or_flags(SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
                    bs->set_range(chk_copy(oexp->source_range()));
                    vl_action_item *ai = new vl_action_item(bs, sim->context());
                    ai->or_flags(AI_DEL_STMT);
                    sim->timewheel()->append(sim->time() + td, ai);
                }
            }
        }
        if (changed && !rfdly) {
            if (gate->gi_inst_list && gate->gi_inst_list->delays()) {
                gate->set_delay(sim, BitH);
                vl_time_t td = gate->gi_delay->eval();
                vl_bassign_stmt *bs =
                    new vl_bassign_stmt(BassignStmt, vs, 0, 0, v);
                bs->or_flags(SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
                bs->set_range(chk_copy(oexp->source_range()));
                vl_action_item *ai = new vl_action_item(bs, sim->context());
                ai->or_flags(AI_DEL_STMT);
                sim->timewheel()->append(sim->time() + td, ai);
            }
            else {
                vl_range *r = oexp->source_range();
                vs->assign(r, v, 0);
            }
        }
    }
    return (true);
}


// Static function.
//
bool
vl_gate_inst::geval_cmos(vl_simulator *sim, int(*)(int, int),
    vl_gate_inst *gate)
{
    // CMOS gate: 1 output, 3 inputs
    const char *nm = gate->name() ? gate->name() : "";
    lsGen<vl_expr*> gen(gate->gi_terms);

    vl_array a;
    if (gate->gi_array)
        a.set(gate->gi_array);
    if (!a.size())
        a.set(1);

    // output
    vl_expr *oexp = 0;
    gen.next(&oexp);

    // input
    vl_expr *expr = 0;
    gen.next(&expr);
    vl_var &d = expr->eval();
    if (d.data_type() != Dbit) {
        vl_error(msg3, nm, 2);
        return (false);
    }
    if (d.bits().size() != 1 && d.bits().size() < a.size()) {
        vl_error(msg4, nm, 2);
        return (false);
    }

    // input
    gen.next(&expr);
    vl_var &cn = expr->eval();
    if (cn.data_type() != Dbit) {
        vl_error(msg3, nm, 3);
        return (false);
    }
    if (cn.bits().size() != 1 && cn.bits().size() < a.size()) {
        vl_error(msg4, nm, 3);
        return (false);
    }

    // input
    gen.next(&expr);
    vl_var &cp = expr->eval();
    if (cp.data_type() != Dbit) {
        vl_error(msg3, nm, 4);
        return (false);
    }
    if (cp.bits().size() != 1 && cp.bits().size() < a.size()) {
        vl_error(msg4, nm, 4);
        return (false);
    }

    bool rfdly = false;
    if (gate->gi_inst_list && gate->gi_inst_list->delays() &&
            gate->gi_inst_list->delays()->list &&
            gate->gi_inst_list->delays()->list->length() > 1)
        // use separate rise/fall delays
        rfdly = true;

    lsGen<vl_var*> ogen(gate->gi_outputs);
    vl_var *v;
    if (ogen.next(&v)) {
        vl_var *vs = oexp->source();
        if (!vs) {
            vl_error(msg5, nm, 1);
            return (false);
        }
        *v = oexp->eval();
        bool changed = false;
        for (int i = 0; i < a.size(); i++) {
            int ii = (d.bits().size() > 1 ? d.data_s()[i] : d.data_s()[0]);
            int in = (cn.bits().size() > 1 ? cn.data_s()[i] : cn.data_s()[0]);
            int ip = (cp.bits().size() > 1 ? cp.data_s()[i] : cp.data_s()[0]);
            int obit;
            if (in == BitH || ip == BitL)
                obit = ii;
            else if (in == BitL && ip == BitH)
                obit = BitZ;
            else
                obit = ii;
            int io = (v->bits().size() > 1 ? i : 0);
            if (v->data_s()[io] != obit) {
                v->data_s()[io] = obit;
                changed = true;
                if (rfdly) {
                    // have to assign each val separately, delays may differ
                    gate->set_delay(sim, obit);
                    vl_time_t td = gate->gi_delay->eval();
                    vl_bassign_stmt *bs =
                        new vl_bassign_stmt(BassignStmt, vs, 0, 0, v);
                    bs->or_flags(SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
                    bs->set_range(chk_copy(oexp->source_range()));
                    vl_action_item *ai = new vl_action_item(bs, sim->context());
                    ai->or_flags(AI_DEL_STMT);
                    sim->timewheel()->append(sim->time() + td, ai);
                }
            }
        }
        if (changed && !rfdly) {
            if (gate->gi_inst_list && gate->gi_inst_list->delays()) {
                gate->set_delay(sim, BitH);
                vl_time_t td = gate->gi_delay->eval();
                vl_bassign_stmt *bs =
                    new vl_bassign_stmt(BassignStmt, vs, 0, 0, v);
                bs->or_flags(SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
                bs->set_range(chk_copy(oexp->source_range()));
                vl_action_item *ai = new vl_action_item(bs, sim->context());
                ai->or_flags(AI_DEL_STMT);
                sim->timewheel()->append(sim->time() + td, ai);
            }
            else {
                vl_range *r = oexp->source_range();
                vs->assign(r, v, 0);
            }
        }
    }
    return (true);
}


// Static function.
//
bool
vl_gate_inst::geval_tran(vl_simulator *sim, int(*)(int, int),
    vl_gate_inst *gate)
{
    const char *nm = gate->name() ? gate->name() : "";
    lsGen<vl_expr*> gen(gate->gi_terms);
    lsGen<vl_var*> ogen(gate->gi_outputs);

    vl_array a;
    if (gate->gi_array)
        a.set(gate->gi_array);
    if (!a.size())
        a.set(1);

    vl_expr *expr1 = 0;
    gen.next(&expr1);
    vl_var v1 = *expr1;
    vl_var *vx1 = 0;
    ogen.next(&vx1);
    *vx1 = expr1->eval();
    if (vx1->data_type() != Dbit) {
        vl_error(msg3, nm, 1);
        return (false);
    }
    if (vx1->bits().size() != 1 && vx1->bits().size() < a.size()) {
        vl_error(msg4, nm, 1);
        return (false);
    }

    vl_expr *expr2 = 0;
    gen.next(&expr2);
    vl_var v2 = *expr2;
    vl_var *vx2 = 0;
    ogen.next(&vx2);
    *vx2 = expr2->eval();
    if (vx2->data_type() != Dbit) {
        vl_error(msg3, nm, 2);
        return (false);
    }
    if (vx2->bits().size() != 1 && vx2->bits().size() < a.size()) {
        vl_error(msg4, nm, 2);
        return (false);
    }

    bool ch1 = false;
    for (int i = 0; i < a.size(); i++) {
        int i1 = (vx1->bits().size() > 1 ? vx1->data_s()[i] : vx1->data_s()[0]);
        if (vx1->data_s()[i1] != v1.data_s()[i1]) {
            ch1 = true;
            break;
        }
    }
    bool ch2 = false;
    for (int i = 0; i < a.size(); i++) {
        int i2 = (vx2->bits().size() > 1 ? vx2->data_s()[i] : vx2->data_s()[0]);
        if (vx2->data_s()[i2] != v2.data_s()[i2]) {
            ch2 = true;
            break;
        }
    }
    if (ch1 && ch2) {
        vl_error("internal, tran gate bug detected");
        return (false);
    }

    bool rfdly = false;
    if (gate->gi_inst_list && gate->gi_inst_list->delays() &&
            gate->gi_inst_list->delays()->list &&
            gate->gi_inst_list->delays()->list->length() > 1)
        // use separate rise/fall delays
        rfdly = true;

    if (ch1 && !ch2) {
        vl_var *vs = expr2->source();
        if (!vs) {
            vl_error(msg5, nm, 2);
            return (false);
        }
        for (int i = 0; i < a.size(); i++) {
            int i1 = (vx1->bits().size() > 1 ?
                vx1->data_s()[i] : vx1->data_s()[0]);
            int i2 =
                (vx2->bits().size() > 1 ? vx2->data_s()[i] : vx2->data_s()[0]);
            if (vx2->data_s()[i2] != vx1->data_s()[i1]) {
                vx2->data_s()[i2] = vx1->data_s()[i1];
                if (rfdly) {
                    // have to assign each val separately, delays may differ
                    gate->set_delay(sim, vx2->data_s()[i2]);
                    vl_time_t td = gate->gi_delay->eval();
                    vl_bassign_stmt *bs =
                        new vl_bassign_stmt(BassignStmt, vs, 0, 0, vx2);
                    bs->or_flags(SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
                    bs->set_range(chk_copy(expr2->source_range()));
                    vl_action_item *ai = new vl_action_item(bs, sim->context());
                    ai->or_flags(AI_DEL_STMT);
                    sim->timewheel()->append(sim->time() + td, ai);
                }
            }
        }
        if (!rfdly) {
            if (gate->gi_inst_list && gate->gi_inst_list->delays()) {
                gate->set_delay(sim, BitH);
                vl_time_t td = gate->gi_delay->eval();
                vl_bassign_stmt *bs =
                    new vl_bassign_stmt(BassignStmt, vs, 0, 0, vx2);
                bs->or_flags(SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
                bs->set_range(chk_copy(expr2->source_range()));
                vl_action_item *ai = new vl_action_item(bs, sim->context());
                ai->or_flags(AI_DEL_STMT);
                sim->timewheel()->append(sim->time() + td, ai);
            }
            else {
                vl_range *r = expr2->source_range();
                vs->assign(r, vx2, 0);
            }
        }
    }
    else if (ch2 && !ch1) {
        vl_var *vs = expr1->source();
        if (!vs) {
            vl_error(msg5, nm, 1);
            return (false);
        }
        for (int i = 0; i < a.size(); i++) {
            int i1 = (vx1->bits().size() > 1 ?
                vx1->data_s()[i] : vx1->data_s()[0]);
            int i2 = (vx2->bits().size() > 1 ?
                vx2->data_s()[i] : vx2->data_s()[0]);
            if (vx1->data_s()[i1] != vx2->data_s()[i2]) {
                vx1->data_s()[i1] = vx2->data_s()[i2];
                if (rfdly) {
                    // have to assign each val separately, delays may differ
                    gate->set_delay(sim, vx1->data_s()[i1]);
                    vl_time_t td = gate->gi_delay->eval();
                    vl_bassign_stmt *bs =
                        new vl_bassign_stmt(BassignStmt, vs, 0, 0, vx1);
                    bs->or_flags(SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
                    bs->set_range(chk_copy(expr1->source_range()));
                    vl_action_item *ai = new vl_action_item(bs, sim->context());
                    ai->or_flags(AI_DEL_STMT);
                    sim->timewheel()->append(sim->time() + td, ai);
                }
            }
        }
        if (!rfdly) {
            if (gate->gi_inst_list && gate->gi_inst_list->delays()) {
                gate->set_delay(sim, BitH);
                vl_time_t td = gate->gi_delay->eval();
                vl_bassign_stmt *bs =
                    new vl_bassign_stmt(BassignStmt, vs, 0, 0, vx1);
                bs->or_flags(SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
                bs->set_range(chk_copy(expr1->source_range()));
                vl_action_item *ai = new vl_action_item(bs, sim->context());
                ai->or_flags(AI_DEL_STMT);
                sim->timewheel()->append(sim->time() + td, ai);
            }
            else {
                vl_range *r = expr1->source_range();
                vs->assign(r, vx1, 0);
            }
        }
    }
    return (true);
}


// Static function.
//
bool
vl_gate_inst::geval_ctran(vl_simulator *sim, int(*)(int, int),
    vl_gate_inst *gate)
{
    const char *nm = gate->name() ? gate->name() : "";
    lsGen<vl_expr*> gen(gate->gi_terms);
    lsGen<vl_var*> ogen(gate->gi_outputs);

    vl_array a;
    if (gate->gi_array)
        a.set(gate->gi_array);
    if (!a.size())
        a.set(1);

    vl_expr *expr1 = 0;
    gen.next(&expr1);
    vl_var v1 = *expr1;
    vl_var *vx1 = 0;
    ogen.next(&vx1);
    *vx1 = expr1->eval();
    if (vx1->data_type() != Dbit) {
        vl_error(msg3, nm, 1);
        return (false);
    }
    if (vx1->bits().size() != 1 && vx1->bits().size() < a.size()) {
        vl_error(msg4, nm, 1);
        return (false);
    }

    vl_expr *expr2 = 0;
    gen.next(&expr2);
    vl_var v2 = *expr2;
    vl_var *vx2 = 0;
    ogen.next(&vx2);
    *vx2 = expr2->eval();
    if (vx2->data_type() != Dbit) {
        vl_error(msg3, nm, 2);
        return (false);
    }
    if (vx2->bits().size() != 1 && vx2->bits().size() < a.size()) {
        vl_error(msg4, nm, 2);
        return (false);
    }

    vl_expr *expr3 = 0;
    gen.next(&expr3);
    vl_var v3 = expr3->eval();
    if (v3.data_type() != Dbit) {
        vl_error(msg3, nm, 3);
        return (false);
    }
    if (v3.bits().size() != 1 && v3.bits().size() < a.size()) {
        vl_error(msg4, nm, 3);
        return (false);
    }

    bool ch1 = false;
    for (int i = 0; i < a.size(); i++) {
        int i3 = (v3.bits().size() > 1 ? v3.data_s()[i] : v3.data_s()[0]);
        if (((gate->type() == Tranif1Gate || gate->type() == Rtranif1Gate)
                && v3.data_s()[i3] != BitH) ||
            ((gate->type() == Tranif0Gate || gate->type() == Rtranif0Gate)
                && v3.data_s()[i3] != BitL))
            // off
            continue;
        int i1 = (vx1->bits().size() > 1 ? vx1->data_s()[i] : vx1->data_s()[0]);
        if (vx1->data_s()[i1] != v1.data_s()[i1]) {
            ch1 = true;
            break;
        }
    }
    bool ch2 = false;
    for (int i = 0; i < a.size(); i++) {
        int i3 = (v3.bits().size() > 1 ? v3.data_s()[i] : v3.data_s()[0]);
        if (((gate->type() == Tranif1Gate || gate->type() == Rtranif1Gate)
                && v3.data_s()[i3] != BitH) ||
            ((gate->type() == Tranif0Gate || gate->type() == Rtranif0Gate)
                && v3.data_s()[i3] != BitL))
            // off
            continue;
        int i2 = (vx2->bits().size() > 1 ? vx2->data_s()[i] : vx2->data_s()[0]);
        if (vx2->data_s()[i2] != v2.data_s()[i2]) {
            ch2 = true;
            break;
        }
    }
    if (ch1 && ch2) {
        vl_error("internal, tranif gate bug detected");
        return (false);
    }

    bool rfdly = false;
    if (gate->gi_inst_list && gate->gi_inst_list->delays() &&
            gate->gi_inst_list->delays()->list &&
            gate->gi_inst_list->delays()->list->length() > 1)
        // use separate rise/fall delays
        rfdly = true;

    if (ch1 && !ch2) {
        vl_var *vs = expr2->source();
        if (!vs) {
            vl_error(msg5, nm, 2);
            return (false);
        }
        for (int i = 0; i < a.size(); i++) {
            int i3 = (v3.bits().size() > 1 ? v3.data_s()[i] : v3.data_s()[0]);
            if (((gate->type() == Tranif1Gate || gate->type() == Rtranif1Gate)
                    && v3.data_s()[i3] != BitH) ||
                ((gate->type() == Tranif0Gate || gate->type() == Rtranif0Gate)
                    && v3.data_s()[i3] != BitL))
                // off
                continue;
            int i1 = (vx1->bits().size() > 1 ?
                vx1->data_s()[i] : vx1->data_s()[0]);
            int i2 = (vx2->bits().size() > 1 ?
                vx2->data_s()[i] : vx2->data_s()[0]);
            if (vx2->data_s()[i2] != vx1->data_s()[i1]) {
                vx2->data_s()[i2] = vx1->data_s()[i1];
                if (rfdly) {
                    // have to assign each val separately, delays may differ
                    gate->set_delay(sim, vx2->data_s()[i2]);
                    vl_time_t td = gate->gi_delay->eval();
                    vl_bassign_stmt *bs =
                        new vl_bassign_stmt(BassignStmt, vs, 0, 0, vx2);
                    bs->or_flags(SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
                    bs->set_range(chk_copy(expr2->source_range()));
                    vl_action_item *ai = new vl_action_item(bs, sim->context());
                    ai->or_flags(AI_DEL_STMT);
                    sim->timewheel()->append(sim->time() + td, ai);
                }
            }
        }
        if (!rfdly) {
            if (gate->gi_inst_list && gate->gi_inst_list->delays()) {
                gate->set_delay(sim, BitH);
                vl_time_t td = gate->gi_delay->eval();
                vl_bassign_stmt *bs =
                    new vl_bassign_stmt(BassignStmt, vs, 0, 0, vx2);
                bs->or_flags(SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
                bs->set_range(chk_copy(expr2->source_range()));
                vl_action_item *ai = new vl_action_item(bs, sim->context());
                ai->or_flags(AI_DEL_STMT);
                sim->timewheel()->append(sim->time() + td, ai);
            }
            else {
                vl_range *r = expr2->source_range();
                vs->assign(r, vx2, 0);
            }
        }
    }
    else if (ch2 && !ch1) {
        vl_var *vs = expr1->source();
        if (!vs) {
            vl_error(msg5, nm, 1);
            return (false);
        }
        for (int i = 0; i < a.size(); i++) {
            int i3 = (v3.bits().size() > 1 ? v3.data_s()[i] : v3.data_s()[0]);
            if (((gate->type() == Tranif1Gate || gate->type() == Rtranif1Gate)
                    && v3.data_s()[i3] != BitH) ||
                ((gate->type() == Tranif0Gate || gate->type() == Rtranif0Gate)
                    && v3.data_s()[i3] != BitL))
                // off
                continue;
            int i1 = (vx1->bits().size() > 1 ?
                vx1->data_s()[i] : vx1->data_s()[0]);
            int i2 = (vx2->bits().size() > 1 ?
                vx2->data_s()[i] : vx2->data_s()[0]);
            if (vx1->data_s()[i1] != vx2->data_s()[i2]) {
                vx1->data_s()[i1] = vx2->data_s()[i2];
                if (rfdly) {
                    // have to assign each val separately, delays may differ
                    gate->set_delay(sim, vx1->data_s()[i1]);
                    vl_time_t td = gate->gi_delay->eval();
                    vl_bassign_stmt *bs =
                        new vl_bassign_stmt(BassignStmt, vs, 0, 0, vx1);
                    bs->or_flags(SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
                    bs->set_range(chk_copy(expr1->source_range()));
                    vl_action_item *ai = new vl_action_item(bs, sim->context());
                    ai->or_flags(AI_DEL_STMT);
                    sim->timewheel()->append(sim->time() + td, ai);
                }
            }
        }
        if (!rfdly) {
            if (gate->gi_inst_list && gate->gi_inst_list->delays()) {
                gate->set_delay(sim, BitH);
                vl_time_t td = gate->gi_delay->eval();
                vl_bassign_stmt *bs =
                    new vl_bassign_stmt(BassignStmt, vs, 0, 0, vx1);
                bs->or_flags(SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
                bs->set_range(chk_copy(expr1->source_range()));
                vl_action_item *ai = new vl_action_item(bs, sim->context());
                ai->or_flags(AI_DEL_STMT);
                sim->timewheel()->append(sim->time() + td, ai);
            }
            else {
                vl_range *r = expr1->source_range();
                vs->assign(r, vx1, 0);
            }
        }
    }
    return (true);
}

