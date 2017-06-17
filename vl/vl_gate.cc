
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
 * You should have received a copy of the GNU General Public License      *
 * along with this program; if not, write to the Free Software            *
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.              *
 *========================================================================*
 *                                                                        *
 * Verilog Support Files                                                  *
 *                                                                        *
 *========================================================================*
 $Id: vl_gate.cc,v 1.6 2008/03/01 20:09:56 stevew Exp $
 *========================================================================*/

#include "vl_list.h"
#include "vl_st.h"
#include "vl_defs.h"
#include "vl_types.h"

//
// Built-in gate primitives.  All terminals are scalars.
//

static const char *msg = "gate %s has too few terminals";
static const char *msg1 = "gate %s has null terminal %d";
static const char *msg2 = "(internal) gate %s not initialized";
static const char *msg3 = "gate %s has non-bitfield terminal %d";
static const char *msg4 =
    "arrayed gate %s, terminal %d has bit width less than array size";
static const char *msg5 = "gate %s has non-lvalue output terminal %d";

//---------------------------------------------------------------------------
//  Local
//---------------------------------------------------------------------------

inline int
op_not(int i)
{
    if (i == BitH)
        return (BitL);
    if (i == BitL)
        return (BitH);
    return (BitDC);
}


static bool
gsetup_gate(vl_simulator*, vl_gate_inst *gate)
{
    // gate type
    const char *nm = gate->name ? gate->name : "";
    if (gate->terms->length() < 3) {
        vl_error(msg, nm);
        return (false);
    }
    lsGen<vl_expr*> gen(gate->terms);

    vl_array a;
    if (gate->array)
        a.set(gate->array);
    if (!a.size)
        a.size = 1;

    // output
    vl_expr *expr = 0;
    gen.next(&expr);
    if (!expr) {
        vl_error(msg1, nm, 1);
        return (false);
    }
    gate->outputs = new lsList<vl_var*>;
    vl_var *vo = new vl_var;
    vo->net_type = REGreg;
    vo->strength = gate->inst_list->strength;
    vo->setx(a.size);
    gate->outputs->newEnd(vo);
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


static bool
gsetup_buf(vl_simulator*, vl_gate_inst *gate)
{
    // buffer type
    const char *nm = gate->name ? gate->name : "";
    int num = gate->terms->length();
    if (num < 2) {
        vl_error(msg, nm);
        return (false);
    }
    lsGen<vl_expr*> gen(gate->terms, true);  // start at end

    vl_array a;
    if (gate->array)
        a.set(gate->array);
    if (!a.size)
        a.size = 1;

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
    gate->outputs = new lsList<vl_var*>;
    while (gen.prev(&expr)) {
        if (!expr) {
            vl_error(msg1, nm, num - n);
            return (false);
        }
        vl_var *vo = new vl_var;
        vo->net_type = REGreg;
        vo->strength = gate->inst_list->strength;
        vo->setx(a.size);
        gate->outputs->newEnd(vo);
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


static bool
gsetup_cbuf(vl_simulator*, vl_gate_inst *gate)
{
    // buffer type with control input
    const char *nm = gate->name ? gate->name : "";
    int num = gate->terms->length();
    if (num < 3) {
        vl_error(msg, nm);
        return (false);
    }
    lsGen<vl_expr*> gen(gate->terms, true);  // start at end

    vl_array a;
    if (gate->array)
        a.set(gate->array);
    if (!a.size)
        a.size = 1;

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
    gate->outputs = new lsList<vl_var*>;
    while (gen.prev(&expr)) {
        if (!expr) {
            vl_error(msg1, nm, num - n);
            return (false);
        }
        vl_var *vo = new vl_var;
        vo->net_type = REGreg;
        vo->strength = gate->inst_list->strength;
        vo->setz(a.size);
        gate->outputs->newEnd(vo);
        n++;
    }
    return (true);
}


static bool
gsetup_mos(vl_simulator*, vl_gate_inst *gate)
{
    // MOS gate
    const char *nm = gate->name ? gate->name : "";
    if (gate->terms->length() != 3) {
        vl_error("mos gate %s does not have 3 terminals", nm);
        return (false);
    }
    lsGen<vl_expr*> gen(gate->terms);

    vl_array a;
    if (gate->array)
        a.set(gate->array);
    if (!a.size)
        a.size = 1;

    // output
    vl_expr *expr = 0;
    gen.next(&expr);
    if (!expr) {
        vl_error(msg1, nm, 1);
        return (false);
    }
    gate->outputs = new lsList<vl_var*>;
    vl_var *vo = new vl_var;
    vo->net_type = REGreg;
    vo->strength = gate->inst_list->strength;
    vo->setz(a.size);
    gate->outputs->newEnd(vo);

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


static bool
gsetup_cmos(vl_simulator*, vl_gate_inst *gate)
{
    // CMOS gate
    const char *nm = gate->name ? gate->name : "";
    if (gate->terms->length() != 4) {
        vl_error("cmos gate %s does not have 4 terminals", nm);
        return (false);
    }
    lsGen<vl_expr*> gen(gate->terms);

    vl_array a;
    if (gate->array)
        a.set(gate->array);
    if (!a.size)
        a.size = 1;

    // output
    vl_expr *expr = 0;
    gen.next(&expr);
    if (!expr) {
        vl_error(msg1, nm, 1);
        return (false);
    }
    gate->outputs = new lsList<vl_var*>;
    vl_var *vo = new vl_var;
    vo->net_type = REGreg;
    vo->strength = gate->inst_list->strength;
    vo->setz(a.size);
    gate->outputs->newEnd(vo);

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


static bool
gsetup_tran(vl_simulator*, vl_gate_inst *gate)
{
    const char *nm = gate->name ? gate->name : "";
    if (gate->terms->length() != 2) {
        vl_error("tran gate %s does not have 2 terminals", nm);
        return (false);
    }
    lsGen<vl_expr*> gen(gate->terms);

    vl_array a;
    if (gate->array)
        a.set(gate->array);
    if (!a.size)
        a.size = 1;

    // inout
    vl_expr *expr1 = 0;
    gen.next(&expr1);
    if (!expr1) {
        vl_error(msg1, nm, 1);
        return (false);
    }
    expr1->eval();
    expr1->chain(gate);
    gate->outputs = new lsList<vl_var*>;
    vl_var *vo = new vl_var;
    vo->net_type = REGreg;
    vo->strength = gate->inst_list->strength;
    vo->setz(a.size);
    gate->outputs->newEnd(vo);

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
    vo->net_type = REGreg;
    vo->strength = gate->inst_list->strength;
    vo->setz(a.size);
    gate->outputs->newEnd(vo);

    return (true);
}


static bool
gsetup_ctran(vl_simulator*, vl_gate_inst *gate)
{
    const char *nm = gate->name ? gate->name : "";
    if (gate->terms->length() != 2) {
        vl_error("tranif gate %s does not have 3 terminals", nm);
        return (false);
    }
    lsGen<vl_expr*> gen(gate->terms);

    vl_array a;
    if (gate->array)
        a.set(gate->array);
    if (!a.size)
        a.size = 1;

    // inout
    vl_expr *expr1 = 0;
    gen.next(&expr1);
    if (!expr1) {
        vl_error(msg1, nm, 1);
        return (false);
    }
    expr1->eval();
    expr1->chain(gate);
    gate->outputs = new lsList<vl_var*>;
    vl_var *vo = new vl_var;
    vo->net_type = REGreg;
    vo->strength = gate->inst_list->strength;
    vo->setz(a.size);
    gate->outputs->newEnd(vo);

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
    vo->net_type = REGreg;
    vo->strength = gate->inst_list->strength;
    vo->setz(a.size);
    gate->outputs->newEnd(vo);

    // control
    vl_expr *expr3 = 0;
    gen.next(&expr3);
    if (!expr3) {
        vl_error(msg1, nm, 3);
        return (false);
    }

    return (true);
}


static bool
geval_gate(vl_simulator *sim, int(*set)(int, int), vl_gate_inst *gate)
{
    // gate type: 1 output, 2 or more inputs
    const char *nm = gate->name ? gate->name : "";
    if (!set) {
        vl_error(msg2, nm);
        return (false);
    }
    lsGen<vl_expr*> gen(gate->terms);

    vl_array a;
    if (gate->array)
        a.set(gate->array);
    if (!a.size)
        a.size = 1;

    // output
    vl_expr *oexp = 0;
    gen.next(&oexp);

    // inputs
    int n = gate->terms->length() - 1;
    vl_var **iv = new vl_var*[n];
    vl_expr *expr;
    int cnt = 0;
    while (gen.next(&expr)) {
        iv[cnt] = &expr->eval();
        if (iv[cnt]->data_type != Dbit) {
            vl_error(msg3, nm, cnt+2);
            return (false);
        }
        if (iv[cnt]->bits.size != 1 && iv[cnt]->bits.size < a.size) {
            vl_error(msg4, nm, cnt+2);
            return (false);
        }
        cnt++;
    }

    bool rfdly = false;
    if (gate->inst_list && gate->inst_list->delays &&
            gate->inst_list->delays->list &&
            gate->inst_list->delays->list->length() > 1)
        // use separate rise/fall delays
        rfdly = true;

    lsGen<vl_var*> ogen(gate->outputs);
    vl_var *v;
    if (ogen.next(&v)) {
        vl_var *vs = oexp->source();
        if (!vs) {
            vl_error(msg5, nm, 1);
            return (false);
        }
        bool changed = false;
        for (int i = 0; i < a.size; i++) {
            int obit = (iv[0]->bits.size > 1 ? iv[0]->u.s[i] : iv[0]->u.s[0]);
            for (int j = 1; j < n; j++) {
                int o = (iv[j]->bits.size > 1 ? iv[j]->u.s[i] : iv[j]->u.s[0]);
                obit = (*set)(obit, o);
            }

            int io = (v->bits.size > 1 ? i : 0);
            if (v->u.s[io] != obit) {
                v->u.s[io] = obit;
                changed = true;
                if (rfdly) {
                    // have to assign each val separately, delays may differ
                    gate->set_delay(sim, obit);
                    vl_time_t td = gate->delay->eval();
                    vl_bassign_stmt *bs =
                        new vl_bassign_stmt(BassignStmt, vs, 0, 0, v);
                    bs->flags |= (SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
                    bs->range = oexp->source_range()->copy();
                    vl_action_item *ai = new vl_action_item(bs, sim->context);
                    ai->flags |= AI_DEL_STMT;
                    sim->timewheel->append(sim->time + td, ai);
                }
            }
        }
        if (changed && !rfdly) {
            if (gate->inst_list && gate->inst_list->delays) {
                gate->set_delay(sim, BitH);
                vl_time_t td = gate->delay->eval();
                vl_bassign_stmt *bs =
                    new vl_bassign_stmt(BassignStmt, vs, 0, 0, v);
                bs->flags |= (SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
                bs->range = oexp->source_range()->copy();
                vl_action_item *ai = new vl_action_item(bs, sim->context);
                ai->flags |= AI_DEL_STMT;
                sim->timewheel->append(sim->time + td, ai);
            }
            else {
                vl_range *r = oexp->source_range();
                vs->assign(r, v, 0);
            }
        }
    }
    return (true);
}


static bool
geval_buf(vl_simulator *sim, int(*set)(int, int), vl_gate_inst *gate)
{
    // buffer type: 1 or more outputs, 1 input
    const char *nm = gate->name ? gate->name : "";
    if (!set) {
        vl_error(msg2, nm);
        return (false);
    }
    int num = gate->terms->length();
    lsGen<vl_expr*> gen(gate->terms, true);  // start at end

    vl_array a;
    if (gate->array)
        a.set(gate->array);
    if (!a.size)
        a.size = 1;

    // input
    vl_expr *expr = 0;
    gen.prev(&expr);
    vl_var &ip = expr->eval();
    if (ip.data_type != Dbit) {
        vl_error(msg3, nm, num);
        return (false);
    }
    if (ip.bits.size != 1 && ip.bits.size < a.size) {
        vl_error(msg4, nm, num);
        return (false);
    }

    bool rfdly = false;
    if (gate->inst_list && gate->inst_list->delays &&
            gate->inst_list->delays->list &&
            gate->inst_list->delays->list->length() > 1)
        // use separate rise/fall delays
        rfdly = true;

    lsGen<vl_var*> ogen(gate->outputs);
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
        for (int i = 0; i < a.size; i++) {
            int ii = (ip.bits.size > 1 ? ip.u.s[i] : ip.u.s[0]);
            int obit = (*set)(ii, BitL);

            int io = (v->bits.size > 1 ? i : 0);
            if (v->u.s[io] != obit) {
                v->u.s[io] = obit;
                changed = true;
                if (rfdly) {
                    // have to assign each val separately, delays may differ
                    gate->set_delay(sim, obit);
                    vl_time_t td = gate->delay->eval();
                    vl_bassign_stmt *bs =
                        new vl_bassign_stmt(BassignStmt, vs, 0, 0, v);
                    bs->flags |= (SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
                    bs->range = expr->source_range()->copy();
                    vl_action_item *ai = new vl_action_item(bs, sim->context);
                    ai->flags |= AI_DEL_STMT;
                    sim->timewheel->append(sim->time + td, ai);
                }
            }
        }
        if (changed && !rfdly) {
            if (gate->inst_list && gate->inst_list->delays) {
                gate->set_delay(sim, BitH);
                vl_time_t td = gate->delay->eval();
                vl_bassign_stmt *bs =
                    new vl_bassign_stmt(BassignStmt, vs, 0, 0, v);
                bs->flags |= (SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
                bs->range = expr->source_range()->copy();
                vl_action_item *ai = new vl_action_item(bs, sim->context);
                ai->flags |= AI_DEL_STMT;
                sim->timewheel->append(sim->time + td, ai);
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


static bool
geval_cbuf(vl_simulator *sim, int(*set)(int, int), vl_gate_inst *gate)
{
    // buffer type with control input: 1 or more outputs, 2 inputs
    const char *nm = gate->name ? gate->name : "";
    if (!set) {
        vl_error(msg2, nm);
        return (false);
    }
    int num = gate->terms->length();
    lsGen<vl_expr*> gen(gate->terms, true);  // start at end

    vl_array a;
    if (gate->array)
        a.set(gate->array);
    if (!a.size)
        a.size = 1;

    // control
    vl_expr *expr = 0;
    gen.prev(&expr);
    vl_var &c = expr->eval();
    if (c.data_type != Dbit) {
        vl_error(msg3, nm, num);
        return (false);
    }
    if (c.bits.size != 1 && c.bits.size < a.size) {
        vl_error(msg4, nm, num);
        return (false);
    }

    // input
    gen.prev(&expr);
    vl_var &ip = expr->eval();
    if (ip.data_type != Dbit) {
        vl_error(msg3, nm, num-1);
        return (false);
    }
    if (ip.bits.size != 1 && ip.bits.size < a.size) {
        vl_error(msg4, nm, num-1);
        return (false);
    }

    bool rfdly = false;
    if (gate->inst_list && gate->inst_list->delays &&
            gate->inst_list->delays->list &&
            gate->inst_list->delays->list->length() > 1)
        // use separate rise/fall delays
        rfdly = true;

    lsGen<vl_var*> ogen(gate->outputs);
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
        for (int i = 0; i < a.size; i++) {
            int ii = (ip.bits.size > 1 ? ip.u.s[i] : ip.u.s[0]);
            int ic = (c.bits.size > 1 ? c.u.s[i] : c.u.s[0]);
            int obit = (*set)(ii, ic);

            int io = (v->bits.size > 1 ? i : 0);
            if (v->u.s[io] != obit) {
                v->u.s[io] = obit;
                changed = true;
                if (rfdly) {
                    // have to assign each val separately, delays may differ
                    gate->set_delay(sim, obit);
                    vl_time_t td = gate->delay->eval();
                    vl_bassign_stmt *bs =
                        new vl_bassign_stmt(BassignStmt, vs, 0, 0, v);
                    bs->flags |= (SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
                    bs->range = expr->source_range()->copy();
                    vl_action_item *ai = new vl_action_item(bs, sim->context);
                    ai->flags |= AI_DEL_STMT;
                    sim->timewheel->append(sim->time + td, ai);
                }
            }
        }
        if (changed && !rfdly) {
            if (gate->inst_list && gate->inst_list->delays) {
                gate->set_delay(sim, BitH);
                vl_time_t td = gate->delay->eval();
                vl_bassign_stmt *bs =
                    new vl_bassign_stmt(BassignStmt, vs, 0, 0, v);
                bs->flags |= (SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
                bs->range = expr->source_range()->copy();
                vl_action_item *ai = new vl_action_item(bs, sim->context);
                ai->flags |= AI_DEL_STMT;
                sim->timewheel->append(sim->time + td, ai);
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


static bool
geval_mos(vl_simulator *sim, int(*set)(int, int), vl_gate_inst *gate)
{
    // MOS gate: 1 output, 2 inputs
    const char *nm = gate->name ? gate->name : "";
    if (!set) {
        vl_error(msg2, nm);
        return (false);
    }
    lsGen<vl_expr*> gen(gate->terms);

    vl_array a;
    if (gate->array)
        a.set(gate->array);
    if (!a.size)
        a.size = 1;

    // output
    vl_expr *oexp = 0;
    gen.next(&oexp);

    // input
    vl_expr *expr = 0;
    gen.next(&expr);
    vl_var &d = expr->eval();
    if (d.data_type != Dbit) {
        vl_error(msg3, nm, 2);
        return (false);
    }
    if (d.bits.size != 1 && d.bits.size < a.size) {
        vl_error(msg4, nm, 2);
        return (false);
    }

    // input
    gen.next(&expr);
    vl_var &c = expr->eval();
    if (c.data_type != Dbit) {
        vl_error(msg3, nm, 3);
        return (false);
    }
    if (c.bits.size != 1 && c.bits.size < a.size) {
        vl_error(msg4, nm, 3);
        return (false);
    }

    bool rfdly = false;
    if (gate->inst_list && gate->inst_list->delays &&
            gate->inst_list->delays->list &&
            gate->inst_list->delays->list->length() > 1)
        // use separate rise/fall delays
        rfdly = true;

    lsGen<vl_var*> ogen(gate->outputs);
    vl_var *v;
    if (ogen.next(&v)) {
        vl_var *vs = oexp->source();
        if (!vs) {
            vl_error(msg5, nm, 1);
            return (false);
        }
        *v = oexp->eval();
        bool changed = false;
        for (int i = 0; i < a.size; i++) {
            int ii = (d.bits.size > 1 ? d.u.s[i] : d.u.s[0]);
            int ic = (c.bits.size > 1 ? c.u.s[i] : c.u.s[0]);
            int obit = (*set)(ii, ic);

            int io = (v->bits.size > 1 ? i : 0);
            if (v->u.s[io] != obit) {
                v->u.s[io] = obit;
                changed = true;
                if (rfdly) {
                    // have to assign each val separately, delays may differ
                    gate->set_delay(sim, obit);
                    vl_time_t td = gate->delay->eval();
                    vl_bassign_stmt *bs =
                        new vl_bassign_stmt(BassignStmt, vs, 0, 0, v);
                    bs->flags |= (SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
                    bs->range = oexp->source_range()->copy();
                    vl_action_item *ai = new vl_action_item(bs, sim->context);
                    ai->flags |= AI_DEL_STMT;
                    sim->timewheel->append(sim->time + td, ai);
                }
            }
        }
        if (changed && !rfdly) {
            if (gate->inst_list && gate->inst_list->delays) {
                gate->set_delay(sim, BitH);
                vl_time_t td = gate->delay->eval();
                vl_bassign_stmt *bs =
                    new vl_bassign_stmt(BassignStmt, vs, 0, 0, v);
                bs->flags |= (SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
                bs->range = oexp->source_range()->copy();
                vl_action_item *ai = new vl_action_item(bs, sim->context);
                ai->flags |= AI_DEL_STMT;
                sim->timewheel->append(sim->time + td, ai);
            }
            else {
                vl_range *r = oexp->source_range();
                vs->assign(r, v, 0);
            }
        }
    }
    return (true);
}


static bool
geval_cmos(vl_simulator *sim, int(*)(int, int), vl_gate_inst *gate)
{
    // CMOS gate: 1 output, 3 inputs
    const char *nm = gate->name ? gate->name : "";
    lsGen<vl_expr*> gen(gate->terms);

    vl_array a;
    if (gate->array)
        a.set(gate->array);
    if (!a.size)
        a.size = 1;

    // output
    vl_expr *oexp = 0;
    gen.next(&oexp);

    // input
    vl_expr *expr = 0;
    gen.next(&expr);
    vl_var &d = expr->eval();
    if (d.data_type != Dbit) {
        vl_error(msg3, nm, 2);
        return (false);
    }
    if (d.bits.size != 1 && d.bits.size < a.size) {
        vl_error(msg4, nm, 2);
        return (false);
    }

    // input
    gen.next(&expr);
    vl_var &cn = expr->eval();
    if (cn.data_type != Dbit) {
        vl_error(msg3, nm, 3);
        return (false);
    }
    if (cn.bits.size != 1 && cn.bits.size < a.size) {
        vl_error(msg4, nm, 3);
        return (false);
    }

    // input
    gen.next(&expr);
    vl_var &cp = expr->eval();
    if (cp.data_type != Dbit) {
        vl_error(msg3, nm, 4);
        return (false);
    }
    if (cp.bits.size != 1 && cp.bits.size < a.size) {
        vl_error(msg4, nm, 4);
        return (false);
    }

    bool rfdly = false;
    if (gate->inst_list && gate->inst_list->delays &&
            gate->inst_list->delays->list &&
            gate->inst_list->delays->list->length() > 1)
        // use separate rise/fall delays
        rfdly = true;

    lsGen<vl_var*> ogen(gate->outputs);
    vl_var *v;
    if (ogen.next(&v)) {
        vl_var *vs = oexp->source();
        if (!vs) {
            vl_error(msg5, nm, 1);
            return (false);
        }
        *v = oexp->eval();
        bool changed = false;
        for (int i = 0; i < a.size; i++) {
            int ii = (d.bits.size > 1 ? d.u.s[i] : d.u.s[0]);
            int in = (cn.bits.size > 1 ? cn.u.s[i] : cn.u.s[0]);
            int ip = (cp.bits.size > 1 ? cp.u.s[i] : cp.u.s[0]);
            int obit;
            if (in == BitH || ip == BitL)
                obit = ii;
            else if (in == BitL && ip == BitH)
                obit = BitZ;
            else
                obit = ii;
            int io = (v->bits.size > 1 ? i : 0);
            if (v->u.s[io] != obit) {
                v->u.s[io] = obit;
                changed = true;
                if (rfdly) {
                    // have to assign each val separately, delays may differ
                    gate->set_delay(sim, obit);
                    vl_time_t td = gate->delay->eval();
                    vl_bassign_stmt *bs =
                        new vl_bassign_stmt(BassignStmt, vs, 0, 0, v);
                    bs->flags |= (SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
                    bs->range = oexp->source_range()->copy();
                    vl_action_item *ai = new vl_action_item(bs, sim->context);
                    ai->flags |= AI_DEL_STMT;
                    sim->timewheel->append(sim->time + td, ai);
                }
            }
        }
        if (changed && !rfdly) {
            if (gate->inst_list && gate->inst_list->delays) {
                gate->set_delay(sim, BitH);
                vl_time_t td = gate->delay->eval();
                vl_bassign_stmt *bs =
                    new vl_bassign_stmt(BassignStmt, vs, 0, 0, v);
                bs->flags |= (SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
                bs->range = oexp->source_range()->copy();
                vl_action_item *ai = new vl_action_item(bs, sim->context);
                ai->flags |= AI_DEL_STMT;
                sim->timewheel->append(sim->time + td, ai);
            }
            else {
                vl_range *r = oexp->source_range();
                vs->assign(r, v, 0);
            }
        }
    }
    return (true);
}


static bool
geval_tran(vl_simulator *sim, int(*)(int, int), vl_gate_inst *gate)
{
    const char *nm = gate->name ? gate->name : "";
    lsGen<vl_expr*> gen(gate->terms);
    lsGen<vl_var*> ogen(gate->outputs);

    vl_array a;
    if (gate->array)
        a.set(gate->array);
    if (!a.size)
        a.size = 1;

    vl_expr *expr1 = 0;
    gen.next(&expr1);
    vl_var v1 = *expr1;
    vl_var *vx1 = 0;
    ogen.next(&vx1);
    *vx1 = expr1->eval();
    if (vx1->data_type != Dbit) {
        vl_error(msg3, nm, 1);
        return (false);
    }
    if (vx1->bits.size != 1 && vx1->bits.size < a.size) {
        vl_error(msg4, nm, 1);
        return (false);
    }

    vl_expr *expr2 = 0;
    gen.next(&expr2);
    vl_var v2 = *expr2;
    vl_var *vx2 = 0;
    ogen.next(&vx2);
    *vx2 = expr2->eval();
    if (vx2->data_type != Dbit) {
        vl_error(msg3, nm, 2);
        return (false);
    }
    if (vx2->bits.size != 1 && vx2->bits.size < a.size) {
        vl_error(msg4, nm, 2);
        return (false);
    }

    bool ch1 = false;
    for (int i = 0; i < a.size; i++) {
        int i1 = (vx1->bits.size > 1 ? vx1->u.s[i] : vx1->u.s[0]);
        if (vx1->u.s[i1] != v1.u.s[i1]) {
            ch1 = true;
            break;
        }
    }
    bool ch2 = false;
    for (int i = 0; i < a.size; i++) {
        int i2 = (vx2->bits.size > 1 ? vx2->u.s[i] : vx2->u.s[0]);
        if (vx2->u.s[i2] != v2.u.s[i2]) {
            ch2 = true;
            break;
        }
    }
    if (ch1 && ch2) {
        vl_error("internal, tran gate bug detected");
        return (false);
    }

    bool rfdly = false;
    if (gate->inst_list && gate->inst_list->delays &&
            gate->inst_list->delays->list &&
            gate->inst_list->delays->list->length() > 1)
        // use separate rise/fall delays
        rfdly = true;

    if (ch1 && !ch2) {
        vl_var *vs = expr2->source();
        if (!vs) {
            vl_error(msg5, nm, 2);
            return (false);
        }
        for (int i = 0; i < a.size; i++) {
            int i1 = (vx1->bits.size > 1 ? vx1->u.s[i] : vx1->u.s[0]);
            int i2 = (vx2->bits.size > 1 ? vx2->u.s[i] : vx2->u.s[0]);
            if (vx2->u.s[i2] != vx1->u.s[i1]) {
                vx2->u.s[i2] = vx1->u.s[i1];
                if (rfdly) {
                    // have to assign each val separately, delays may differ
                    gate->set_delay(sim, vx2->u.s[i2]);
                    vl_time_t td = gate->delay->eval();
                    vl_bassign_stmt *bs =
                        new vl_bassign_stmt(BassignStmt, vs, 0, 0, vx2);
                    bs->flags |= (SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
                    bs->range = expr2->source_range()->copy();
                    vl_action_item *ai = new vl_action_item(bs, sim->context);
                    ai->flags |= AI_DEL_STMT;
                    sim->timewheel->append(sim->time + td, ai);
                }
            }
        }
        if (!rfdly) {
            if (gate->inst_list && gate->inst_list->delays) {
                gate->set_delay(sim, BitH);
                vl_time_t td = gate->delay->eval();
                vl_bassign_stmt *bs =
                    new vl_bassign_stmt(BassignStmt, vs, 0, 0, vx2);
                bs->flags |= (SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
                bs->range = expr2->source_range()->copy();
                vl_action_item *ai = new vl_action_item(bs, sim->context);
                ai->flags |= AI_DEL_STMT;
                sim->timewheel->append(sim->time + td, ai);
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
        for (int i = 0; i < a.size; i++) {
            int i1 = (vx1->bits.size > 1 ? vx1->u.s[i] : vx1->u.s[0]);
            int i2 = (vx2->bits.size > 1 ? vx2->u.s[i] : vx2->u.s[0]);
            if (vx1->u.s[i1] != vx2->u.s[i2]) {
                vx1->u.s[i1] = vx2->u.s[i2];
                if (rfdly) {
                    // have to assign each val separately, delays may differ
                    gate->set_delay(sim, vx1->u.s[i1]);
                    vl_time_t td = gate->delay->eval();
                    vl_bassign_stmt *bs =
                        new vl_bassign_stmt(BassignStmt, vs, 0, 0, vx1);
                    bs->flags |= (SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
                    bs->range = expr1->source_range()->copy();
                    vl_action_item *ai = new vl_action_item(bs, sim->context);
                    ai->flags |= AI_DEL_STMT;
                    sim->timewheel->append(sim->time + td, ai);
                }
            }
        }
        if (!rfdly) {
            if (gate->inst_list && gate->inst_list->delays) {
                gate->set_delay(sim, BitH);
                vl_time_t td = gate->delay->eval();
                vl_bassign_stmt *bs =
                    new vl_bassign_stmt(BassignStmt, vs, 0, 0, vx1);
                bs->flags |= (SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
                bs->range = expr1->source_range()->copy();
                vl_action_item *ai = new vl_action_item(bs, sim->context);
                ai->flags |= AI_DEL_STMT;
                sim->timewheel->append(sim->time + td, ai);
            }
            else {
                vl_range *r = expr1->source_range();
                vs->assign(r, vx1, 0);
            }
        }
    }
    return (true);
}


static bool
geval_ctran(vl_simulator *sim, int(*)(int, int), vl_gate_inst *gate)
{
    const char *nm = gate->name ? gate->name : "";
    lsGen<vl_expr*> gen(gate->terms);
    lsGen<vl_var*> ogen(gate->outputs);

    vl_array a;
    if (gate->array)
        a.set(gate->array);
    if (!a.size)
        a.size = 1;

    vl_expr *expr1 = 0;
    gen.next(&expr1);
    vl_var v1 = *expr1;
    vl_var *vx1 = 0;
    ogen.next(&vx1);
    *vx1 = expr1->eval();
    if (vx1->data_type != Dbit) {
        vl_error(msg3, nm, 1);
        return (false);
    }
    if (vx1->bits.size != 1 && vx1->bits.size < a.size) {
        vl_error(msg4, nm, 1);
        return (false);
    }

    vl_expr *expr2 = 0;
    gen.next(&expr2);
    vl_var v2 = *expr2;
    vl_var *vx2 = 0;
    ogen.next(&vx2);
    *vx2 = expr2->eval();
    if (vx2->data_type != Dbit) {
        vl_error(msg3, nm, 2);
        return (false);
    }
    if (vx2->bits.size != 1 && vx2->bits.size < a.size) {
        vl_error(msg4, nm, 2);
        return (false);
    }

    vl_expr *expr3 = 0;
    gen.next(&expr3);
    vl_var v3 = expr3->eval();
    if (v3.data_type != Dbit) {
        vl_error(msg3, nm, 3);
        return (false);
    }
    if (v3.bits.size != 1 && v3.bits.size < a.size) {
        vl_error(msg4, nm, 3);
        return (false);
    }

    bool ch1 = false;
    for (int i = 0; i < a.size; i++) {
        int i3 = (v3.bits.size > 1 ? v3.u.s[i] : v3.u.s[0]);
        if (((gate->type == Tranif1Gate || gate->type == Rtranif1Gate)
                && v3.u.s[i3] != BitH) ||
            ((gate->type == Tranif0Gate || gate->type == Rtranif0Gate)
                && v3.u.s[i3] != BitL))
            // off
            continue;
        int i1 = (vx1->bits.size > 1 ? vx1->u.s[i] : vx1->u.s[0]);
        if (vx1->u.s[i1] != v1.u.s[i1]) {
            ch1 = true;
            break;
        }
    }
    bool ch2 = false;
    for (int i = 0; i < a.size; i++) {
        int i3 = (v3.bits.size > 1 ? v3.u.s[i] : v3.u.s[0]);
        if (((gate->type == Tranif1Gate || gate->type == Rtranif1Gate)
                && v3.u.s[i3] != BitH) ||
            ((gate->type == Tranif0Gate || gate->type == Rtranif0Gate)
                && v3.u.s[i3] != BitL))
            // off
            continue;
        int i2 = (vx2->bits.size > 1 ? vx2->u.s[i] : vx2->u.s[0]);
        if (vx2->u.s[i2] != v2.u.s[i2]) {
            ch2 = true;
            break;
        }
    }
    if (ch1 && ch2) {
        vl_error("internal, tranif gate bug detected");
        return (false);
    }

    bool rfdly = false;
    if (gate->inst_list && gate->inst_list->delays &&
            gate->inst_list->delays->list &&
            gate->inst_list->delays->list->length() > 1)
        // use separate rise/fall delays
        rfdly = true;

    if (ch1 && !ch2) {
        vl_var *vs = expr2->source();
        if (!vs) {
            vl_error(msg5, nm, 2);
            return (false);
        }
        for (int i = 0; i < a.size; i++) {
            int i3 = (v3.bits.size > 1 ? v3.u.s[i] : v3.u.s[0]);
            if (((gate->type == Tranif1Gate || gate->type == Rtranif1Gate)
                    && v3.u.s[i3] != BitH) ||
                ((gate->type == Tranif0Gate || gate->type == Rtranif0Gate)
                    && v3.u.s[i3] != BitL))
                // off
                continue;
            int i1 = (vx1->bits.size > 1 ? vx1->u.s[i] : vx1->u.s[0]);
            int i2 = (vx2->bits.size > 1 ? vx2->u.s[i] : vx2->u.s[0]);
            if (vx2->u.s[i2] != vx1->u.s[i1]) {
                vx2->u.s[i2] = vx1->u.s[i1];
                if (rfdly) {
                    // have to assign each val separately, delays may differ
                    gate->set_delay(sim, vx2->u.s[i2]);
                    vl_time_t td = gate->delay->eval();
                    vl_bassign_stmt *bs =
                        new vl_bassign_stmt(BassignStmt, vs, 0, 0, vx2);
                    bs->flags |= (SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
                    bs->range = expr2->source_range()->copy();
                    vl_action_item *ai = new vl_action_item(bs, sim->context);
                    ai->flags |= AI_DEL_STMT;
                    sim->timewheel->append(sim->time + td, ai);
                }
            }
        }
        if (!rfdly) {
            if (gate->inst_list && gate->inst_list->delays) {
                gate->set_delay(sim, BitH);
                vl_time_t td = gate->delay->eval();
                vl_bassign_stmt *bs =
                    new vl_bassign_stmt(BassignStmt, vs, 0, 0, vx2);
                bs->flags |= (SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
                bs->range = expr2->source_range()->copy();
                vl_action_item *ai = new vl_action_item(bs, sim->context);
                ai->flags |= AI_DEL_STMT;
                sim->timewheel->append(sim->time + td, ai);
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
        for (int i = 0; i < a.size; i++) {
            int i3 = (v3.bits.size > 1 ? v3.u.s[i] : v3.u.s[0]);
            if (((gate->type == Tranif1Gate || gate->type == Rtranif1Gate)
                    && v3.u.s[i3] != BitH) ||
                ((gate->type == Tranif0Gate || gate->type == Rtranif0Gate)
                    && v3.u.s[i3] != BitL))
                // off
                continue;
            int i1 = (vx1->bits.size > 1 ? vx1->u.s[i] : vx1->u.s[0]);
            int i2 = (vx2->bits.size > 1 ? vx2->u.s[i] : vx2->u.s[0]);
            if (vx1->u.s[i1] != vx2->u.s[i2]) {
                vx1->u.s[i1] = vx2->u.s[i2];
                if (rfdly) {
                    // have to assign each val separately, delays may differ
                    gate->set_delay(sim, vx1->u.s[i1]);
                    vl_time_t td = gate->delay->eval();
                    vl_bassign_stmt *bs =
                        new vl_bassign_stmt(BassignStmt, vs, 0, 0, vx1);
                    bs->flags |= (SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
                    bs->range = expr1->source_range()->copy();
                    vl_action_item *ai = new vl_action_item(bs, sim->context);
                    ai->flags |= AI_DEL_STMT;
                    sim->timewheel->append(sim->time + td, ai);
                }
            }
        }
        if (!rfdly) {
            if (gate->inst_list && gate->inst_list->delays) {
                gate->set_delay(sim, BitH);
                vl_time_t td = gate->delay->eval();
                vl_bassign_stmt *bs =
                    new vl_bassign_stmt(BassignStmt, vs, 0, 0, vx1);
                bs->flags |= (SIM_INTERNAL | BAS_SAVE_LHS | BAS_SAVE_RHS);
                bs->range = expr1->source_range()->copy();
                vl_action_item *ai = new vl_action_item(bs, sim->context);
                ai->flags |= AI_DEL_STMT;
                sim->timewheel->append(sim->time + td, ai);
            }
            else {
                vl_range *r = expr1->source_range();
                vs->assign(r, vx1, 0);
            }
        }
    }
    return (true);
}


static int
set_and(int i1, int i2)
{
    if (i1 == BitL || i2 == BitL)
        return (BitL);
    if (i1 == BitH && i2 == BitH)
        return (BitH);
    return (BitDC);
}


static int
set_nand(int i1, int i2)
{
    if (i1 == BitL || i2 == BitL)
        return (BitH);
    if (i1 == BitH && i2 == BitH)
        return (BitL);
    return (BitDC);
}


static int
set_or(int i1, int i2)
{
    if (i1 == BitH || i2 == BitH)
        return (BitH);
    if (i1 == BitL && i2 == BitL)
        return (BitL);
    return (BitDC);
}


static int
set_nor(int i1, int i2)
{
    if (i1 == BitH || i2 == BitH)
        return (BitL);
    if (i1 == BitL && i2 == BitL)
        return (BitH);
    return (BitDC);
}


static int
set_xor(int i1, int i2)
{
    if ((i1 == BitH && i2 == BitL) || (i1 == BitL && i2 == BitH))
        return (BitH);
    if ((i1 == BitH && i2 == BitH) || (i1 == BitL && i2 == BitL))
        return (BitL);
    return (BitDC);
}


static int
set_xnor(int i1, int i2)
{
    if ((i1 == BitH && i2 == BitL) || (i1 == BitL && i2 == BitH))
        return (BitL);
    if ((i1 == BitH && i2 == BitH) || (i1 == BitL && i2 == BitL))
        return (BitH);
    return (BitDC);
}


static int
set_buf(int i1, int)
{
    return (i1);
}


static int
set_bufif0(int i1, int c)
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


static int
set_bufif1(int i1, int c)
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


static int
set_not(int i1, int)
{
    return (op_not(i1));
}


static int
set_notif0(int i1, int c)
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


static int
set_notif1(int i1, int c)
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


static int
set_nmos(int d, int c)
{
    if (c == BitL)
        return (BitZ);
    return (d);
}


static int
set_pmos(int d, int c)
{
    if (c == BitH)
        return (BitZ);
    return (d);
}


//---------------------------------------------------------------------------
//  Instances
//---------------------------------------------------------------------------

// Set the delay statement according to the specified delays and the
// transition
//
void
vl_gate_inst::set_delay(vl_simulator*, int bit)
{
    if (inst_list && inst_list->delays) {
        if (!delay)
            delay = new vl_delay;
        vl_expr *rd = 0, *fd = 0, *td = 0;
        if (inst_list->delays->list) {
            vl_expr *e;
            lsGen<vl_expr*> gen(inst_list->delays->list);
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
            rd = inst_list->delays->delay1;
        if (bit == BitH)
            delay->delay1 = rd;
        else if (bit == BitL)
            delay->delay1 = (fd ? fd : rd);
        else if (bit == BitZ) {
            if (td)
                delay->delay1 = td;
            else {
                delay->delay1 = rd;
                int m = rd->eval();
                if (fd) {
                    int tmp = fd->eval();
                    if (tmp < m)
                        delay->delay1 = fd;
                }
            }
        }
        else if (bit == BitDC) {
            // minimum delay
            int m = rd->eval();
            delay->delay1 = rd;
            if (fd) {
                int tmp = fd->eval();
                if (tmp < m) {
                    m = tmp;
                    delay->delay1 = fd;
                }
            }
            if (td) {
                int tmp = td->eval();
                if (tmp < m) {
                    m = tmp;
                    delay->delay1 = td;
                }
            }
        }
    }
}


// Configure the struct for a particular gate type.  This could be done
// more elegantly, but it minimizes indirection
//
void
vl_gate_inst::set_type(int t)
{
    type = t;
    switch (type) {
    case AndGate:
        gsetup = gsetup_gate;
        geval = geval_gate;
        gset = set_and;
        string = "and";
        break;
    case NandGate:
        gsetup = gsetup_gate;
        geval = geval_gate;
        gset = set_nand;
        string = "nand";
        break;
    case OrGate:
        gsetup = gsetup_gate;
        geval = geval_gate;
        gset = set_or;
        string = "or";
        break;
    case NorGate:
        gsetup = gsetup_gate;
        geval = geval_gate;
        gset = set_nor;
        string = "nor";
        break;
    case XorGate:
        gsetup = gsetup_gate;
        geval = geval_gate;
        gset = set_xor;
        string = "xor";
        break;
    case XnorGate:
        gsetup = gsetup_gate;
        geval = geval_gate;
        gset = set_xnor;
        string = "xnor";
        break;
    case BufGate:
        gsetup = gsetup_buf;
        geval = geval_buf;
        gset = set_buf;
        string = "buf";
        break;
    case Bufif0Gate:
        gsetup = gsetup_cbuf;
        geval = geval_cbuf;
        gset = set_bufif0;
        string = "bufif0";
        break;
    case Bufif1Gate:
        gsetup = gsetup_cbuf;
        geval = geval_cbuf;
        gset = set_bufif1;
        string = "bufif1";
        break;
    case NotGate:
        gsetup = gsetup_buf;
        geval = geval_buf;
        gset = set_not;
        string = "not";
        break;
    case Notif0Gate:
        gsetup = gsetup_cbuf;
        geval = geval_cbuf;
        gset = set_notif0;
        string = "notif0";
        break;
    case Notif1Gate:
        gsetup = gsetup_cbuf;
        geval = geval_cbuf;
        gset = set_notif1;
        string = "notif1";
        break;
    case PulldownGate:
        gsetup = 0;
        geval = 0;
        gset = 0;
        string = "pulldown";
        break;
    case PullupGate:
        gsetup = 0;
        geval = 0;
        gset = 0;
        string = "pullup";
        break;
    case NmosGate:
        gsetup = gsetup_mos;
        geval = geval_mos;
        gset = set_nmos;
        string = "nmos";
        break;
    case RnmosGate:
        gsetup = gsetup_mos;
        geval = geval_mos;
        gset = set_nmos;
        string = "rnmos";
        break;
    case PmosGate:
        gsetup = gsetup_mos;
        geval = geval_mos;
        gset = set_pmos;
        string = "pmos";
        break;
    case RpmosGate:
        gsetup = gsetup_mos;
        geval = geval_mos;
        gset = set_pmos;
        string = "rpmos";
        break;
    case CmosGate:
        gsetup = gsetup_cmos;
        geval = geval_cmos;
        gset = 0;
        string = "cmos";
        break;
    case RcmosGate:
        gsetup = gsetup_cmos;
        geval = geval_cmos;
        gset = 0;
        string = "rcmos";
        break;
    case TranGate:
        gsetup = gsetup_tran;
        geval = geval_tran;
        gset = 0;
        string = "tran";
        break;
    case RtranGate:
        gsetup = gsetup_tran;
        geval = geval_tran;
        gset = 0;
        string = "rtran";
        break;
    case Tranif0Gate:
        gsetup = gsetup_ctran;
        geval = geval_ctran;
        gset = 0;
        string = "tranif0";
        break;
    case Rtranif0Gate:
        gsetup = gsetup_ctran;
        geval = geval_ctran;
        gset = 0;
        string = "rtranif0";
        break;
    case Tranif1Gate:
        gsetup = gsetup_ctran;
        geval = geval_ctran;
        gset = 0;
        string = "tranif0";
        break;
    case Rtranif1Gate:
        gsetup = gsetup_ctran;
        geval = geval_ctran;
        gset = 0;
        string = "rtranif0";
        break;
    default:
        VP.error(ERR_INTERNAL, "Unexpected Gate Type");
        break;
    }
}
