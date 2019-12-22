
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
 * WRspice Circuit Simulation and Analysis Tool                           *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "config.h"
#include <stdio.h>
#include <stdarg.h>
#include "vl_list.h"
#include "vl_st.h"
#include "vl_defs.h"
#include "vl_types.h"
#include "verilog.h"
#include "misc.h"
#include "ifdata.h"
#include "output.h"
#include "circuit.h"
#include "simulator.h"
#include "cshell.h"
#include "parser.h"
#include "miscutil/filestat.h"
#include "ginterf/graphics.h"


// Verilog data files are opened using the sourcepath.
//
FILE *
vl_file_open(const char *fname, const char *mode)
{
    if (*mode != 'r')
        return (0);
    return (Sp.PathOpen(fname, mode));
}


sADC::sADC(const char *string)
{
    lstring::advtok(&string);
    char *tok = lstring::gettok(&string);
    a_dig_var = tok;
    a_range = 0;
    char *s = strchr(tok, '[');
    if (s) {
        *s = 0;
        tok = s+1;
        s = strchr(tok, ']');
        if (s)
            *s = 0;
        a_range = tok;
    }
    a_node = lstring::gettok(&string);
    a_offset = 0.0;
    a_quantum = 0.0;
    if (*string) {
        wordlist *wl = CP.LexString(string);
        pnlist *pl = Sp.GetPtree(wl, true);
        wordlist::destroy(wl);
        if (pl) {
            sDataVec *v = Sp.Evaluate(pl->node());
            if (v) {
                a_offset = v->realval(0);
                if (!v->isreal())
                    a_quantum = v->imagval(0);
            }
            if (pl->next()) {
                v = Sp.Evaluate(pl->next()->node());
                if (v)
                    a_quantum = v->realval(0);
            }
            pnlist::destroy(pl);
        }
    }
    a_indx = 0;
    a_next = 0;
}


void
sADC::set_var(VerilogBlock *blk, double *vars)
{
    if (a_indx > 0 && blk)
        blk->set_var(this, vars[a_indx]);
}
// End of sADC functions.


// The lines consist of a list of .adc lines, followed by the .verilog
// block, including ".verilog" and ".end".
//
VerilogBlock::VerilogBlock(sLine *lines)
{
    vb_adc = 0;
    vb_desc = 0;
    vb_sim = 0;
    if (!lines)
        return;

    sADC *ad = 0;
    sLine *d;
    for (d = lines; d; d = d->next()) {
        if (lstring::cimatch(ADC_KW, d->line())) {
            if (!vb_adc)
                ad = vb_adc = new sADC(d->line());
            else {
                ad->set_next(new sADC(d->line()));
                ad = ad->next();
            }
        }
        else if (lstring::cimatch(VERILOG_KW, d->line())) {
            d = d->next();
            break;
        }
    }
    if (!d) {
        sADC::destroy(vb_adc);
        vb_adc = 0;
        return;
    }

    char *ftmp = filestat::make_temp("vl");
    FILE *fp = fopen(ftmp, "w");
    if (!fp) {
        GRpkgIf()->ErrPrintf(ET_ERROR,
            "can't open termporary verilog file.\n");
        return;
    }
    for ( ; d && d->next(); d = d->next())
        fprintf(fp, "%s\n", d->line());
    fclose(fp);
    fp = fopen(ftmp, "r");
    if (!fp) {
        GRpkgIf()->ErrPrintf(ET_ERROR,
            "can't open termporary verilog file.\n");
        return;
    }
    VP.clear();
    int err = VP.parse(fp);
    fclose(fp);
    unlink(ftmp);
    delete [] ftmp;
    if (err)
        return;
    vb_desc = VP.description;
    VP.description = 0;
    vb_sim = new vl_simulator;
    if (!vb_sim->initialize(vb_desc)) {
        delete vb_sim;
        vb_sim = 0;
    }
}


VerilogBlock::~VerilogBlock()
{
    delete vb_desc;
    delete vb_sim;
    sADC::destroy(vb_adc);
}


void
VerilogBlock::initialize()
{
    if (vb_sim)
        vb_sim->step(0);
}


void
VerilogBlock::finalize(bool pause)
{
    if (vb_sim) {
        if (pause)
            vb_sim->flush_files();
        else
            vb_sim->close_files();
    }
}


void
VerilogBlock::run_step(sOUTdata *outd)
{
    if (!vb_sim)
        return;
    if (outd->count <= 1) {
        int num;
        IFuid *list;
        outd->circuitPtr->names(&num, &list);
        for (sADC *a = vb_adc; a; a = a->next()) {
            char name[128];
            // gotta strip v(), i()
            if ((*a->node() == 'v' || *a->node() == 'V') &&
                    *(a->node()+1) == '(') {
                const char *s = a->node() + 2;
                char *t = name;
                while (*s) {
                    if (!isspace(*s))
                        *t++ = *s;
                    s++;
                }
                *t = 0;
                if (t > name && *(t-1) == ')')
                    *(t-1) = 0;
            }
            else if ((*a->node() == 'i' || *a->node() == 'I') &&
                    *(a->node()+1) == '(') {
                const char *s = a->node() + 2;
                char *t = name;
                while (*s) {
                    if (!isspace(*s))
                        *t++ = *s;
                    s++;
                }
                *t = 0;
                if (t > name && *(t-1) == ')')
                    *(t-1) = 0;
                strcat(name, "#branch");
            }
            else
                strcpy(name, a->node());

            int i;
            for (i = 0; i < num; i++) {
                if (!strcmp(name, (const char*)list[i])) {
                    a->set_indx(i+1);
                    break;
                }
            }
            if (i == num) {
                vl_error("warning: couldn't find %s in saved vector list",
                    a->node());
            }
        }
        delete [] list;
    }
    for (sADC *a = vb_adc; a; a = a->next())
        a->set_var(this, outd->circuitPtr->CKTrhsOld);
    vb_sim->step(outd->count);
}


bool
VerilogBlock::query_var(const char *name, const char *range, double *d)
{
    if (vb_sim && vb_sim->top_modules) {
        vl_context cx;
        cx.module = vb_sim->top_modules->mods[0];
        vl_var *data = cx.lookup_var((char*)name, true);
        if (data) {
            if (range && *range) {
                int l = -1, r = -1;
                while (isspace(*range))
                    range++;
                l = atoi(range);
                while (isdigit(*range))
                    range++;
                if (*range == ':' || *range == ',') {
                    range++;
                    while (isspace(*range))
                        range++;
                    if (isdigit(*range))
                        r = atoi(range);
                }
                if (l >= 0 && r < 0)
                    r = l;
                vl_var rdata;
                rdata.assign(0, data, l >= 0 ? &l : 0, r >= 0 ? &r : 0);
                *d = (double)rdata;
                return (true);
            }
            *d =(double)*data;
            return (true);
        }
    }
    *d = 0.0;
    return (false);
}


bool
VerilogBlock::set_var(sADC *a, double val)
{
    if (vb_sim && vb_sim->top_modules) {
        vl_context cx;
        cx.module = vb_sim->top_modules->mods[0];
//XXX
        vl_var *data = cx.lookup_var((char*)a->dig_var(), true);
        if (data) {
            const char *range = a->range();
            if (range && *range) {
                int l = -1, r = -1;
                while (isspace(*range))
                    range++;
                l = atoi(range);
                while (isdigit(*range))
                    range++;
                if (*range == ':' || *range == ',') {
                    range++;
                    while (isspace(*range))
                        range++;
                    if (isdigit(*range))
                        r = atoi(range);
                }
                if (l >= 0 && r < 0)
                    r = l;
                data->assign_to(val, a->offset(), a->quantum(), l, r);
                return (true);
            }
            data->assign_to(val, a->offset(), a->quantum(), -1, -1);
            return (true);
        }
    }
    return (false);
}

