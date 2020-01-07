
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
#include "miscutil/randval.h"
#include <unistd.h>
#include <fstream>
#include <fcntl.h>

using std::ios;
using std::ofstream;


//---------------------------------------------------------------------------
//  Local
//---------------------------------------------------------------------------

namespace {
    vl_var tdata;

    // Stuff the 4 bytes of hex code from c into s
    //
    void stuff(char *s, int c)
    {
        switch (c) {
        default:
        case 'x':
        case 'X':
            *s++ = BitDC;
            *s++ = BitDC;
            *s++ = BitDC;
            *s++ = BitDC;
            return;
        case 'z':
        case 'Z':
            *s++ = BitZ;
            *s++ = BitZ;
            *s++ = BitZ;
            *s++ = BitZ;
            return;
        case '0':
            *s++ = BitL;
            *s++ = BitL;
            *s++ = BitL;
            *s++ = BitL;
            return;
        case '1':
            *s++ = BitH;
            *s++ = BitL;
            *s++ = BitL;
            *s++ = BitL;
            return;
        case '2':
            *s++ = BitL;
            *s++ = BitH;
            *s++ = BitL;
            *s++ = BitL;
            return;
        case '3':
            *s++ = BitH;
            *s++ = BitH;
            *s++ = BitL;
            *s++ = BitL;
            return;
        case '4':
            *s++ = BitL;
            *s++ = BitL;
            *s++ = BitH;
            *s++ = BitL;
            return;
        case '5':
            *s++ = BitH;
            *s++ = BitL;
            *s++ = BitH;
            *s++ = BitL;
            return;
        case '6':
            *s++ = BitL;
            *s++ = BitH;
            *s++ = BitH;
            *s++ = BitL;
            return;
        case '7':
            *s++ = BitH;
            *s++ = BitH;
            *s++ = BitH;
            *s++ = BitL;
            return;
        case '8':
            *s++ = BitL;
            *s++ = BitL;
            *s++ = BitL;
            *s++ = BitH;
            return;
        case '9':
            *s++ = BitH;
            *s++ = BitL;
            *s++ = BitL;
            *s++ = BitH;
            return;
        case 'a':
        case 'A':
            *s++ = BitL;
            *s++ = BitH;
            *s++ = BitL;
            *s++ = BitH;
            return;
        case 'b':
        case 'B':
            *s++ = BitH;
            *s++ = BitH;
            *s++ = BitL;
            *s++ = BitH;
            return;
        case 'c':
        case 'C':
            *s++ = BitL;
            *s++ = BitL;
            *s++ = BitH;
            *s++ = BitH;
            return;
        case 'd':
        case 'D':
            *s++ = BitH;
            *s++ = BitL;
            *s++ = BitH;
            *s++ = BitH;
            return;
        case 'e':
        case 'E':
            *s++ = BitL;
            *s++ = BitH;
            *s++ = BitH;
            *s++ = BitH;
            return;
        case 'f':
        case 'F':
            *s++ = BitH;
            *s++ = BitH;
            *s++ = BitH;
            *s++ = BitH;
            return;
        }
    }


    // Core function for $readmemb/$readmemh.
    //
    bool readmem(char *fname, vl_var *d, int start, int end, bool bin)
    {
        const char *sname = bin ? "$readmemb" : "$readmemh";
        FILE *fp = vl_file_open(fname, "r");
        if (!fp)
            fp = fopen(fname, "r");
        if (!fp) {
            vl_error("in %s, can't open file %s", sname, fname);
            return (false);
        }
        if (d->data_type() != Dbit || d->array().size() <= 0) {
            vl_error("bad data type passed to %s", sname);
            return (false);
        }
        if (start < 0)
            start = d->array().Atou(0);
        if (end < 0)
            end = d->array().Atou(d->array().size()-1);
        if (!d->array().check_range(&start, &start) ||
                !d->array().check_range(&end, &end)) {
            vl_error("bad indices passed to %s", sname);
            return (false);
        }
        char **dt = d->data_ps();
        if (!dt) {
            vl_error("bad array passed to %s", sname);
            return (false);
        }
        int loc = d->array().Astart(start, start);
        int lend = d->array().Aend(end, end);
        int inc = lend > loc ? 1 : -1;
        int bwidth = d->bits().size();
        if (!bin && bwidth % 4) {
            vl_error("word length of vector passed to %s not multiple of 4",
                sname);
            return (false);
        }
        int bcnt = 0;

        char lbuf[256];
        int lbufp = 0;
        int c;
        bool skip = false, skipnl = false;
        while ((c = getc(fp)) != EOF) {
            if (skipnl) {
                if (c == '\n')
                    skipnl = false;
                continue;
            }
            if (skip) {
                if (c == '*') {
                    c = getc(fp);
                    if (c == '/')
                        skip = false;
                }
                continue;
            }
            if (bin) {
                if (c == '0' || c == '1' || c == 'x' || c == 'X' || c == 'z' ||
                        c == 'Z') {
                    lbuf[lbufp++] = c;
                    continue;
                }
                if (c == '_')
                    continue;

                if (lbufp) {
                    for (int i = lbufp-1; i >= 0; i--) {
                        switch (lbuf[i]) {
                        case '0':
                            dt[loc][bcnt++] = BitL;
                            break;
                        case '1':
                            dt[loc][bcnt++] = BitH;
                            break;
                        case 'x':
                        case 'X':
                            dt[loc][bcnt++] = BitDC;
                            break;
                        case 'z':
                        case 'Z':
                            dt[loc][bcnt++] = BitZ;
                            break;
                        }
                        if (bcnt == bwidth) {
                            loc += inc;
                            bcnt = 0;
                        }
                    }
                    if (bcnt) {
                        while (bcnt < bwidth)
                            dt[loc][bcnt++] = BitL;
                        loc += inc;
                        bcnt = 0;
                    }
                    lbufp = 0;
                }
            }
            else {
                if (isxdigit(c) || c == 'x' || c == 'X' || c == 'z' || c == 'Z') {
                    if (lbufp >= (int)sizeof(lbuf)) {
                        vl_error("in %s, word size too large");
                        return (false);
                    }
                    lbuf[lbufp++] = c;
                    continue;
                }
                if (c == '_')
                    continue;
                if (lbufp) {
                    for (int i = lbufp-1; i >= 0; i--) {
                        stuff(dt[loc] + bcnt, lbuf[i]);
                        bcnt += 4;
                        if (bcnt == bwidth) {
                            loc += inc;
                            bcnt = 0;
                        }
                    }
                    if (bcnt) {
                        while (bcnt < bwidth)
                            dt[loc][bcnt++] = BitL;
                        loc += inc;
                        bcnt = 0;
                    }
                    lbufp = 0;
                }
            }
            if (c == '@') {
                char buf[256];
                for (int i = 0; i < 256; i++) {
                    c = getc(fp);
                    if (isspace(c) || c == EOF) {
                        buf[i] = 0;
                        break;
                    }
                    if (!isxdigit(c)) {
                        vl_error("in %s, '@' address syntax in file %s", sname,
                            fname);
                        return (false);
                    }
                    buf[i] = c;
                }
                sscanf(buf, "%x", &loc);
                if ((loc < start && loc < end) || (loc > start && loc > end)) {
                    vl_error("in %s, conflicting address in file %s", sname,
                        fname);
                    return (false);
                }
                loc = d->array().Astart(loc, loc);
                continue;
            }
            if (c == '/') {
                c = getc(fp);
                if (c == '/') {
                    skipnl = true;
                    continue;
                }
                if (c == '*') {
                    skip = true;
                    continue;
                }
                vl_error("in %s, stray '/' in file %s", sname, fname);
                return (false);
            }
            if (loc - inc == lend)
                break;
        }
        return (true);
    }


    //
    // Functions for $dumpvars.
    //

    char *ccode(int i)
    {
        static char buf[16];
        char c1 = '!';
        char c2 = '~';
        int d = c2 - c1 + 1;
        int j = 0;
        buf[j] = 0;
        do {
            buf[j] = i%d + c1;
            i /= d;
            j++;
        } while (i);
        buf[j] = 0;
        return (buf);
    }


    // Surpress leading characters in bit string.
    //
    char *ctrunc(char *bstr)
    {
        char *c = bstr;
        if (*c == '0' || *c == 'x' || *c == 'z') {
            while (*(c+1) == *c)
                c++;
        }
        if (*c == '0' && *(c+1) == '1')
            c++;
        return (c);
    }


    void vl_setup_decls(vl_simulator *sim, lsList<vl_decl*> *decls)
    {
        if (!decls)
            return;
        lsGen<vl_decl*> gen(decls);
        vl_decl* decl;
        while (gen.next(&decl))
            decl->setup(sim);    
    }


    void st_dump(ostream &outs, table<vl_var*> *st, vl_simulator *sim)
    {
        if (!st)
            return;
        table_gen<vl_var*> tgen(st);
        vl_var *data;
        const char *name;
        while (tgen.next(&name, &data)) {
            if (data->array().size())
                continue;
            if (data->data_type() != Dbit && data->data_type() != Dint &&
                    data->data_type() != Dtime)
                continue;
            if (*name == '@')
                // "temporary" variable
                continue;
            const char *typestr = data->decl_type();
            if (!strcmp(typestr, "parameter"))
                continue;
            if (!strcmp(typestr, "unknown"))
                typestr = "wire";
            outs << "$var ";
            outs.width(7);
            outs.setf(ios::left, ios::adjustfield);
            outs << typestr;
            outs.width(4);
            outs.setf(ios::right, ios::adjustfield);
            if (data->data_type() == Dint)
                outs << 8*sizeof(int) << ' ';
            else if (data->data_type() == Dtime)
                outs << 8*sizeof(vl_time_t) << ' ';
            else
                outs << data->bits().size() << ' ';
            outs.width(5);
            outs.setf(ios::left, ios::adjustfield);
            outs << sim->dumpindex(data);
            outs << name;
            if (data->bits().size() > 1)
                outs << '[' << data->bits().hi_index() << ':' <<
                    data->bits().lo_index() << ']';
            outs << "  $end\n";
        }
    }
}
        

template<class T> void
vl_dump_items(ostream &outs, lsList<T> *exprs, vl_simulator *sim)
{
    if (!exprs)
        return;
    lsGen<T> gen(exprs);
    T expr;
    while (gen.next(&expr))
        expr->dumpvars(outs, sim);
}


//---------------------------------------------------------------------------
//  Simulator objects
//---------------------------------------------------------------------------

vl_var &
vl_simulator::sys_time(vl_sys_task_stmt*, lsList<vl_expr*> *)
{
    vl_var &dd = var_factory.new_var();  // so it gets gc'ed
    dd.set_data_type(Dtime);
    dd.set_data_t(s_time);
    return (dd);
}


namespace {
    const char *pts(double t)
    {
        if (t < 5e-15)
            return "1 fs";
        if (t < 5e-14)
            return "10 fs";
        if (t < 5e-13)
            return "100 fs";
        if (t < 5e-12)
            return "1 ps";
        if (t < 5e-11)
            return "10 ps";
        if (t < 5e-10)
            return "100 ps";
        if (t < 5e-9)
            return "1 ns";
        if (t < 5e-8)
            return "10 ns";
        if (t < 5e-7)
            return "100 ns";
        if (t < 5e-6)
            return "1 us";
        if (t < 5e-5)
            return "10 us";
        if (t < 5e-4)
            return "100 us";
        if (t < 5e-3)
            return "1 ms";
        if (t < 5e-2)
            return "10 ms";
        if (t < 5e-1)
            return "100 ms";
        if (t < 5)
            return "1 s";
        if (t < 50)
            return "10 s";
        if (t < 500)
            return "100 s";
        return ("bad");
    }
}


vl_var &
vl_simulator::sys_printtimescale(vl_sys_task_stmt*, lsList<vl_expr*> *args)
{
    char *modname = 0;
    if (args) {
        lsGen<vl_expr*> gen(args);
        vl_expr *e;
        if (gen.next(&e))
            modname = vl_fix_str((char*)e->eval());
    }
    if (modname) {
        vl_inst *inst = s_context->lookup_mp(modname);
        bool found = false;
        if (inst && !inst->type()) {
            vl_mp_inst *mp = (vl_mp_inst*)inst;
            if (mp->inst_list() && mp->inst_list()->mptype() == MPmod) {
                vl_module *mod = (vl_module*)mp->master();
                if (mod) {
                    cout << "Time scale of " << modname << " is ";
                    cout << pts(mod->tunit()) << " / " << pts(mod->tprec());
                    cout << ".\n";
                    found = true;
                }
            }
        }
        if (!found)
            vl_warn("$printtimescale: can't find %s", modname);
    }
    else {
        vl_module *cmod = s_context->currentModule();
        if (cmod) {
            cout << "Time scale of " << cmod->name() << " is ";
            cout << pts(cmod->tunit()) << " / " << pts(cmod->tprec());
            cout << ".\n";
        }
        else
            vl_warn("$printtimescale: no current module!");
    }
    delete [] modname;
    tdata.set_data_type(Dint);
    tdata.set_data_i(0);
    return (tdata);
}


vl_var &
vl_simulator::sys_timeformat(vl_sys_task_stmt*, lsList<vl_expr*> *args)
{
    tdata.set_data_type(Dint);
    tdata.set_data_i(0);
    if (!args)
        return (tdata);
    lsGen<vl_expr*> gen(args);
    vl_expr *e;

    int units = s_tfunit;
    if (gen.next(&e)) {
        if (e)
            units = (int)e->eval();
    }
    else
        return (tdata);
    if (units > 0 || units < -15) {
        vl_warn("$timeformat: arg 1 out of range");
        return (tdata);
    }
    s_tfunit = units;

    int prec = s_tfprec;
    if (gen.next(&e)) {
        if (e)
            prec = (int)e->eval();
    }
    else
        return (tdata);
    if (prec < 0 || prec > 15) {
        vl_warn("$timeformat: arg 2 out of range");
        return (tdata);
    }
    s_tfprec = prec;

    if (gen.next(&e)) {
        if (e) {
            delete [] s_tfsuffix;
            s_tfsuffix = vl_fix_str((char*)e->eval());
        }
    }
    else
        return (tdata);

    int wid = s_tfwidth;
    if (gen.next(&e)) {
        if (e)
            wid = (int)e->eval();
    }
    else
        return (tdata);
    if (wid < 0 || wid > 32) {
        vl_warn("$timeformat: arg 4 out of range");
        return (tdata);
    }
    s_tfwidth = wid;
    return (tdata);
}


vl_var &
vl_simulator::sys_display(vl_sys_task_stmt *t, lsList<vl_expr*> *args)
{
    display_print(args, cout, t->dtype(), t->flags());
    tdata.set_data_type(Dint);
    tdata.set_data_i(0);
    return (tdata);
}


vl_var &
vl_simulator::sys_monitor(vl_sys_task_stmt *t, lsList<vl_expr*> *args)
{
    vl_monitor *mnew = new vl_monitor(chk_copy(s_context), args, t->dtype());
    if (!s_monitors)
        s_monitors = mnew;
    else {
        vl_monitor *m = s_monitors;
        while (m->next())
            m = m->next();
        m->set_next(mnew);
    }
    tdata.set_data_type(Dint);
    tdata.set_data_i(0);
    return (tdata);
}


vl_var &
vl_simulator::sys_monitor_on(vl_sys_task_stmt*, lsList<vl_expr*> *)
{
    s_monitor_state = true;
    tdata.set_data_type(Dint);
    tdata.set_data_i(0);
    return (tdata);
}


vl_var &
vl_simulator::sys_monitor_off(vl_sys_task_stmt*, lsList<vl_expr*> *)
{
    s_monitor_state = false;
    tdata.set_data_type(Dint);
    tdata.set_data_i(0);
    return (tdata);
}


vl_var &
vl_simulator::sys_stop(vl_sys_task_stmt*, lsList<vl_expr*>*)
{
    s_stop = VLstop;
    tdata.set_data_type(Dint);
    tdata.set_data_i(0);
    return (tdata);
}


vl_var &
vl_simulator::sys_finish(vl_sys_task_stmt*, lsList<vl_expr*>*)
{
    s_stop = VLstop;
    tdata.set_data_type(Dint);
    tdata.set_data_i(0);
    return (tdata);
}


vl_var &
vl_simulator::sys_noop(vl_sys_task_stmt*, lsList<vl_expr*>*)
{
    tdata.set_data_type(Dint);
    tdata.set_data_i(0);
    return (tdata);
}


vl_var &
vl_simulator::sys_fopen(vl_sys_task_stmt*, lsList<vl_expr*> *args)
{
    const char *msg = "in $fopen, can't open file %s";
    vl_var &dd = var_factory.new_var();  // so it gets gc'ed
    dd.set_data_type(Dint);
    dd.set_data_i(0);
    if (!args) {
        vl_error(msg, "<null>");
        abort();
        return (dd);
    }
    lsGen<vl_expr*> gen(args);
    vl_expr *e;
    if (!gen.next(&e)) {
        vl_error(msg, "<null>");
        abort();
        return (dd);
    }
    char *name = vl_fix_str((char*)e->eval());
    char *mode = 0;
    if (gen.next(&e))
        mode = vl_fix_str((char*)e->eval());

    int fd;
    for (fd = 3; fd < 32 && s_channels[fd]; fd++) ;
    if (fd > 31) {
        vl_error("in $fopen, too many files open, open of %s failed", name);
        abort();
        delete [] name;
        return (dd);
    }
    ofstream *fs; 
    if (mode && *mode == 'a')
        fs = new ofstream(name, ios::app|ios::out);
    else
        fs = new ofstream(name, ios::trunc|ios::out);
    if (!fs->is_open()) { 
        vl_error(msg, name);
        abort();
        delete fs;
        delete [] name;
        return (dd);
    }
    delete [] name;
    s_channels[fd] = fs;
    int fh = 1 << fd;
    dd.set_data_i(fh);
    return (dd);
}


vl_var &
vl_simulator::sys_fclose(vl_sys_task_stmt*, lsList<vl_expr*> *args)
{
    tdata.set_data_type(Dint);
    tdata.set_data_i(0);
    if (!args)
        return (tdata);
    lsGen<vl_expr*> gen(args);
    vl_expr *e;
    if (!gen.next(&e))
        return (tdata);
    int fh = (int)e->eval();
    for (int i = 0; i < 32; i++) {
        if (i > 2 && (fh & 1)) {
            delete s_channels[i];
            s_channels[i] = 0;
        }
        fh >>= 1;
    }
    return (tdata);
}

 
vl_var &
vl_simulator::sys_fmonitor(vl_sys_task_stmt *t, lsList<vl_expr*> *args)
{
    vl_monitor *mnew = new vl_monitor(chk_copy(s_context), args, t->dtype());
    if (!s_fmonitors)
        s_fmonitors = mnew;
    else {
        vl_monitor *m = s_fmonitors;
        while (m->next())
            m = m->next();
        m->set_next(mnew);
    }
    tdata.set_data_type(Dint);
    tdata.set_data_i(0);
    return (tdata);
}


vl_var &
vl_simulator::sys_fmonitor_on(vl_sys_task_stmt*, lsList<vl_expr*> *)
{
    s_fmonitor_state = true;
    tdata.set_data_type(Dint);
    tdata.set_data_i(0);
    return (tdata);
}


vl_var &
vl_simulator::sys_fmonitor_off(vl_sys_task_stmt*, lsList<vl_expr*> *)
{
    s_fmonitor_state = false;
    tdata.set_data_type(Dint);
    tdata.set_data_i(0);
    return (tdata);
}


vl_var &
vl_simulator::sys_fdisplay(vl_sys_task_stmt *t, lsList<vl_expr*> *args)
{
    tdata.set_data_type(Dint);
    tdata.set_data_i(0);
    fdisplay_print(args, t->dtype(), t->flags());
    return (tdata);
}


namespace {
    randval rnd;
    bool seeded;
}


vl_var &
vl_simulator::sys_random(vl_sys_task_stmt*, lsList<vl_expr*> *args)
{
    vl_var &dd = var_factory.new_var();  // so it gets gc'ed
    dd.set_data_type(Dint);
    if (args && !seeded) {
        lsGen<vl_expr*> gen(args);
        vl_expr *e;
        if (gen.next(&e) && e)
            rnd.rand_seed((int)e->eval());
        seeded = true;
    }
    dd.set_data_i((int)rnd.rand_value());
    return (dd);
}


vl_var &
vl_simulator::sys_dumpfile(vl_sys_task_stmt*, lsList<vl_expr*> *args)
{
    const char *msg = "in $dumpfile, can't open file %s";
    tdata.set_data_type(Dint);
    tdata.set_data_i(0);
    if (!args) {
        vl_error(msg, "<null>");
        abort();
        return (tdata);
    }
    lsGen<vl_expr*> gen(args);
    vl_expr *e;
    if (!gen.next(&e)) {
        vl_error(msg, "<null>");
        abort();
        return (tdata);
    }
    char *name = vl_fix_str((char*)e->eval());

    s_dmpfile = new ofstream(name, ios::trunc|ios::out);
    if (!s_dmpfile->good()) {
        delete s_dmpfile;
        s_dmpfile = 0;
        vl_error(msg, name);
        abort();
    }
    return (tdata);
}


vl_var &
vl_simulator::sys_dumpvars(vl_sys_task_stmt*, lsList<vl_expr*> *args)
{
    const char *msg = "in $dumpvars, unresolved second argument";
    tdata.set_data_type(Dint);
    tdata.set_data_i(0);
    if (args) {
        lsGen<vl_expr*> gen(args);
        vl_expr *e;
        if (gen.next(&e)) {
            s_dmpdepth = (int)e->eval();
            if (gen.next(&e)) {
                const char *mname;
                if (e->etype() == IDExpr)
                    mname = vl_strdup(e->edata().ide.name);
                else
                    mname = vl_fix_str((char*)e->eval());
                if (!mname) {
                    vl_warn(msg);
                    return (tdata);
                }
                const char *tname = mname;
                s_dmpcx = new vl_context;
                if (!s_context->resolve_cx(&tname, *s_dmpcx, false)) {
                    if (tname != mname) {
                        vl_warn(msg);
                        delete s_dmpcx;
                        s_dmpcx = 0;
                        return (tdata);
                    }
                    if (!s_top_modules)
                        return (tdata);
                    for (int i = 0; i < s_top_modules->num(); i++) {
                        if (!strcmp(s_top_modules->mod(i)->name(), tname))
                            s_dmpcx->set_module(s_top_modules->mod(i));
                        break;
                    }
                    if (!s_dmpcx->module())
                        *s_dmpcx = *s_context;
                }
            }
        }
    }
    s_dmpstatus |= (DMP_ACTIVE | DMP_HEADER | DMP_ON);
    return (tdata);
}


vl_var &
vl_simulator::sys_dumpall(vl_sys_task_stmt*, lsList<vl_expr*>*)
{
    tdata.set_data_type(Dint);
    tdata.set_data_i(0);
    if (s_dmpstatus & DMP_ON) {
        *s_dmpfile << "$dumpall\n";
        s_dmpstatus |= DMP_ALL;
        do_dump();
        *s_dmpfile << "$end\n";
    }
    return (tdata);
}


vl_var &
vl_simulator::sys_dumpon(vl_sys_task_stmt*, lsList<vl_expr*>*)
{
    tdata.set_data_type(Dint);
    tdata.set_data_i(0);
    if ((s_dmpstatus & DMP_ACTIVE) && !(s_dmpstatus & DMP_ON)) {
        s_dmpstatus |= DMP_ON;
        *s_dmpfile << "$dumpon\n";
        *s_dmpfile << "$end\n";
    }
    return (tdata);
}


vl_var &
vl_simulator::sys_dumpoff(vl_sys_task_stmt*, lsList<vl_expr*>*)
{
    tdata.set_data_type(Dint);
    tdata.set_data_i(0);
    if ((s_dmpstatus & DMP_ACTIVE) && (s_dmpstatus & DMP_ON)) {
        s_dmpstatus &= ~DMP_ON;
        *s_dmpfile << "$dumpoff\n";
        *s_dmpfile << "$end\n";
    }
    return (tdata);
}


vl_var &
vl_simulator::sys_readmemb(vl_sys_task_stmt*, lsList<vl_expr*> *args)
{
    tdata.set_data_type(Dint);
    tdata.set_data_i(0);
    if (!args)
        return (tdata);
    lsGen<vl_expr*> gen(args);
    vl_expr *e;
    if (gen.next(&e)) {
        char *fname = vl_fix_str((char*)e->eval());
        if (gen.next(&e)) {
            vl_var *v = e->source();
            if (!v) {
                vl_error("in $readmemb, source variable undeclared");
                abort();
                return (tdata);
            }
            int start = -1;
            if (gen.next(&e))
                start = (int)e->eval();
            int end = -1;
            if (gen.next(&e))
                end = (int)e->eval();

            if (readmem(fname, v, start, end, true))
                return (tdata);
        }
    }
    vl_error("$readmemb failed");
    abort();
    return (tdata);
}


vl_var &
vl_simulator::sys_readmemh(vl_sys_task_stmt*, lsList<vl_expr*> *args)
{
    tdata.set_data_type(Dint);
    tdata.set_data_i(0);
    if (!args)
        return (tdata);
    lsGen<vl_expr*> gen(args);
    vl_expr *e;
    if (gen.next(&e)) {
        char *fname = vl_fix_str((char*)e->eval());
        if (gen.next(&e)) {
            vl_var *v = e->source();
            if (!v) {
                vl_error("in $readmemh, source variable undeclared");
                abort();
                return (tdata);
            }
            int start = -1;
            if (gen.next(&e))
                start = (int)e->eval();
            int end = -1;
            if (gen.next(&e))
                end = (int)e->eval();

            if (readmem(fname, v, start, end, false))
                return (tdata);
        }
    }
    vl_error("$readmemh failed");
    abort();
    return (tdata);
}


//
// Functions for $dumpvars.
//

void
vl_simulator::do_dump()
{
    if (!(s_dmpstatus & (DMP_ON | DMP_HEADER)))
        return;
    if (!s_dmpfile)
        return;
    if (s_dmpstatus & DMP_HEADER) {
        *s_dmpfile << "$date\n";
        *s_dmpfile << "    " << vl_datestring() << '\n';
        *s_dmpfile << "$end\n";
        *s_dmpfile << "$version\n";
        *s_dmpfile << "    " << vl_version() << '\n';
        *s_dmpfile << "$end\n\n";
    }
    if (!s_dmpcx) {
        if (!s_top_modules)
            return;
        for (int i = 0; i < s_top_modules->num(); i++)
            s_top_modules->mod(i)->dumpvars(*s_dmpfile, this);
    }
    else {
        if (s_dmpcx->module())
            s_dmpcx->module()->dumpvars(*s_dmpfile, this);
        else if (s_dmpcx->primitive())
            s_dmpcx->primitive()->dumpvars(*s_dmpfile, this);
        else if (s_dmpcx->task())
            s_dmpcx->task()->dumpvars(*s_dmpfile, this);
        else if (s_dmpcx->function())
            s_dmpcx->function()->dumpvars(*s_dmpfile, this);
        else if (s_dmpcx->block())
            s_dmpcx->block()->dumpvars(*s_dmpfile, this);
        else if (s_dmpcx->fjblk())
            s_dmpcx->fjblk()->dumpvars(*s_dmpfile, this);
    }
    if (s_dmpstatus & DMP_HEADER) {
        *s_dmpfile << "$upscope $end\n\n";
        *s_dmpfile << "$enddefinitions      $end\n";
        *s_dmpfile << "$dumpvars\n";
        *s_dmpfile << "$end\n";
        s_dmpstatus &= ~DMP_HEADER;
    }

    *s_dmpfile << '#' << s_time << '\n';
    if (s_dmpstatus & DMP_ALL) {
        for (int i = 0; i < s_dmpindx; i++) {
            char *s = s_dmpdata[i]->bitstr(); 
            if (s_dmpdata[i]->data_type() == Dbit &&
                    s_dmpdata[i]->bits().size() == 1)
                *s_dmpfile << s;
            else
                *s_dmpfile << 'b' << ctrunc(s) << ' ';
            *s_dmpfile << ccode(i) << '\n';
            delete [] s;
        }
        s_dmpstatus &= ~DMP_ALL;
        s_dmpfile->flush();
        return;
    }

    if (!s_dmplast)
        s_dmplast = new vl_var[s_dmpindx];
    for (int i = 0; i < s_dmpindx; i++) {
        vl_var &z = case_neq(s_dmplast[i], *s_dmpdata[i]);
        if (z.data_s()[0] == BitH) {
            char *s = s_dmpdata[i]->bitstr(); 
            if (s_dmpdata[i]->data_type() == Dbit &&
                    s_dmpdata[i]->bits().size() == 1)
                *s_dmpfile << s;
            else
                *s_dmpfile << 'b' << ctrunc(s) << ' ';
            *s_dmpfile << ccode(i) << '\n';
            s_dmplast[i] = *s_dmpdata[i];
            delete [] s;
        }
    }

    s_dmpfile->flush();
}


#define DMP_INCR 20

char *
vl_simulator::dumpindex(vl_var *data)
{
    if (!s_dmpdata) {
        s_dmpdata = new vl_var*[DMP_INCR];
        s_dmpsize = DMP_INCR;
    }
    if (s_dmpindx >= s_dmpsize) {
        s_dmpsize += DMP_INCR;
        vl_var **tmp = new vl_var*[s_dmpsize];
        for (int i = 0; i < s_dmpindx; i++) {
            tmp[i] = s_dmpdata[i];
            s_dmpdata[i] = 0;
        }
        delete [] s_dmpdata;
        s_dmpdata = tmp;
    }
    s_dmpdata[s_dmpindx] = data;
    s_dmpindx++;
    return (ccode(s_dmpindx - 1));
}


//---------------------------------------------------------------------------
//  Verilog descriptiopn objects
//---------------------------------------------------------------------------

void
vl_module::dumpvars(ostream &outs, vl_simulator *sim)
{
    sim->set_context(vl_context::push(sim->context(), this));
    if (sim->dmpstatus() & DMP_HEADER) {
        // $scope module <name> $end
        const char *n;
        if (mp_instance)
            n = mp_instance->name();
        else
            n = mp_name;
        outs << "\n$scope module " << n << " $end\n";
        st_dump(outs, mp_sig_st, sim);
        vl_dump_items(outs, m_mod_items, sim);
    }
    sim->set_context(vl_context::pop(sim->context()));
}


void
vl_primitive::dumpvars(ostream &outs, vl_simulator *sim)
{
    sim->set_context(vl_context::push(sim->context(), this));
    if (sim->dmpstatus() & DMP_HEADER) {
        // $scope primitive <name> $end
        const char *n;
        if (mp_instance)
            n = mp_instance->name();
        else
            n = mp_name;
        outs << "\n$scope primitive " << n << " $end\n";
        st_dump(outs, mp_sig_st, sim);
    }
    sim->set_context(vl_context::pop(sim->context()));
}


//---------------------------------------------------------------------------
//  Module items
//---------------------------------------------------------------------------

void
vl_procstmt::dumpvars(ostream &outs, vl_simulator *sim)
{
    if (pr_stmt)
        pr_stmt->dumpvars(outs, sim);
}


void
vl_mp_inst_list::dumpvars(ostream &outs, vl_simulator *sim)
{
    vl_dump_items(outs, mp_mps, sim);
}


//---------------------------------------------------------------------------
//  Statements
//---------------------------------------------------------------------------

void
vl_begin_end_stmt::dumpvars(ostream &outs, vl_simulator *sim)
{
    sim->set_context(vl_context::push(sim->context(), this));
    if (be_name) {
        if (sim->dmpstatus() & DMP_HEADER) {
            // $scope begin <name> $end
            vl_setup_decls(sim, be_decls);
            outs << "\n$scope begin " << be_name << " $end\n";
            vl_dump_items(outs, be_decls, sim);
            st_dump(outs, be_sig_st, sim);
            outs << "$upscope $end\n\n";
        }
    }
    vl_dump_items(outs, be_stmts, sim);
    sim->set_context(vl_context::pop(sim->context()));
}


void
vl_if_else_stmt::dumpvars(ostream &outs, vl_simulator *sim)
{
    if (ie_if_stmt)
        ie_if_stmt->dumpvars(outs, sim);
    if (ie_else_stmt)
        ie_else_stmt->dumpvars(outs, sim);
}


void
vl_case_stmt::dumpvars(ostream &outs, vl_simulator *sim)
{
    vl_dump_items(outs, c_case_items, sim);
}


void
vl_case_item::dumpvars(ostream &outs, vl_simulator *sim)
{
    if (ci_stmt)
        ci_stmt->dumpvars(outs, sim);
}


void
vl_forever_stmt::dumpvars(ostream &outs, vl_simulator *sim)
{
    if (f_stmt)
        f_stmt->dumpvars(outs, sim);
}


void
vl_repeat_stmt::dumpvars(ostream &outs, vl_simulator *sim)
{
    if (r_stmt)
        r_stmt->dumpvars(outs, sim);
}


void
vl_while_stmt::dumpvars(ostream &outs, vl_simulator *sim)
{
    if (w_stmt)
        w_stmt->dumpvars(outs, sim);
}


void
vl_for_stmt::dumpvars(ostream &outs, vl_simulator *sim)
{
    if (f_initial)
        f_initial->dumpvars(outs, sim);
    if (f_end)
        f_end->dumpvars(outs, sim);
    if (f_stmt)
        f_stmt->dumpvars(outs, sim);
}


void
vl_delay_control_stmt::dumpvars(ostream &outs, vl_simulator *sim)
{
    if (d_stmt)
        d_stmt->dumpvars(outs, sim);
}


void
vl_event_control_stmt::dumpvars(ostream &outs, vl_simulator *sim)
{
    if (ec_stmt)
        ec_stmt->dumpvars(outs, sim);
}


void
vl_wait_stmt::dumpvars(ostream &outs, vl_simulator *sim)
{
    if (w_stmt)
        w_stmt->dumpvars(outs, sim);
}


void
vl_fork_join_stmt::dumpvars(ostream &outs, vl_simulator *sim)
{
    sim->set_context(vl_context::push(sim->context(), this));
    if (fj_name) {
        if (sim->dmpstatus() & DMP_HEADER) {
            // $scope fork <name> $end
            vl_setup_decls(sim, fj_decls);
            outs << "\n$scope fork " << fj_name << " $end\n";
            vl_dump_items(outs, fj_decls, sim);
            st_dump(outs, fj_sig_st, sim);
            outs << "$upscope $end\n\n";
        }
    }
    vl_dump_items(outs, fj_stmts, sim);
    sim->set_context(vl_context::pop(sim->context()));
}


//---------------------------------------------------------------------------
//  Instances
//---------------------------------------------------------------------------

void
vl_mp_inst::dumpvars(ostream &outs, vl_simulator *sim)
{
    if (pi_inst_list && pi_master) {
        pi_master->dumpvars(outs, sim);
        outs << "$upscope $end\n\n";
    }
}

