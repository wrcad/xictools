
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
 *========================================================================*
 *                                                                        *
 * Verilog Interface                                                      *
 *                                                                        *
 *========================================================================*
 $Id: verilog.cc,v 2.52 2015/06/20 01:58:12 stevew Exp $
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
#include "outdata.h"
#include "circuit.h"
#include "frontend.h"
#include "cshell.h"
#include "fteparse.h"
#include "filestat.h"
#include "graphics.h"


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
    dig_var = tok;
    range = 0;
    char *s = strchr(tok, '[');
    if (s) {
        *s = 0;
        tok = s+1;
        s = strchr(tok, ']');
        if (s)
            *s = 0;
        range = tok;
    }
    node = lstring::gettok(&string);
    offset = 0.0;
    quantum = 0.0;
    if (*string) {
        wordlist *wl = CP.LexString(string);
        pnlist *pl = Sp.GetPtree(wl, true);
        wordlist::destroy(wl);
        if (pl) {
            sDataVec *v = Sp.Evaluate(pl->node());
            if (v) {
                offset = v->realval(0);
                if (!v->isreal())
                    quantum = v->imagval(0);
            }
            if (pl->next()) {
                v = Sp.Evaluate(pl->next()->node());
                if (v)
                    quantum = v->realval(0);
            }
            pnlist::destroy(pl);
        }
    }
    indx = 0;
    next = 0;
}


void
sADC::free()
{
    sADC *an;
    for (sADC *a = this; a; a = an) {
        an = a->next;
        delete a;
    }
}


void
sADC::set_var(VerilogBlock *blk, double *vars)
{
    if (indx > 0)
        blk->set_var(this, vars[indx]);
}


// The lines consist of a list of .adc lines, followed by the .verilog
// block, including ".verilog" and ".end".
//
VerilogBlock::VerilogBlock(sLine *lines)
{
    adc = 0;
    desc = 0;
    sim = 0;
    if (!lines)
        return;

    sADC *ad = 0;
    sLine *d;
    for (d = lines; d; d = d->next()) {
        if (lstring::cimatch(ADC_KW, d->line())) {
            if (!adc)
                ad = adc = new sADC(d->line());
            else {
                ad->next = new sADC(d->line());
                ad = ad->next;
            }
        }
        else if (lstring::cimatch(VERILOG_KW, d->line())) {
            d = d->next();
            break;
        }
    }
    if (!d) {
        adc->free();
        adc = 0;
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
    desc = VP.description;
    VP.description = 0;
    sim = new vl_simulator;
    if (!sim->initialize(desc)) {
        delete sim;
        sim = 0;
    }
}


VerilogBlock::~VerilogBlock()
{
    delete desc;
    delete sim;
    adc->free();
}


void
VerilogBlock::initialize()
{
    const VerilogBlock *thisvb = this;
    if (thisvb && sim)
        sim->step(0);
}


void
VerilogBlock::finalize(bool pause)
{
    const VerilogBlock *thisvb = this;
    if (thisvb && sim) {
        if (pause)
            sim->flush_files();
        else
            sim->close_files();
    }
}


void
VerilogBlock::run_step(sOUTdata *outd)
{
    if (!sim)
        return;
    if (outd->count <= 1) {
        int num;
        IFuid *list;
        outd->circuitPtr->names(&num, &list);
        for (sADC *a = adc; a; a = a->next) {
            char name[128];
            // gotta strip v(), i()
            if ((*a->node == 'v' || *a->node == 'V') &&
                    *(a->node+1) == '(') {
                const char *s = a->node + 2;
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
            else if ((*a->node == 'i' || *a->node == 'I') &&
                    *(a->node+1) == '(') {
                const char *s = a->node + 2;
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
                strcpy(name, a->node);

            int i;
            for (i = 0; i < num; i++) {
                if (!strcmp(name, (char*)list[i])) {
                    a->indx = i+1;
                    break;
                }
            }
            if (i == num)
                vl_error("warning: couldn't find %s in saved vector list",
                    a->node);
        }
        delete [] list;
    }
    for (sADC *a = adc; a; a = a->next)
        a->set_var(this, outd->circuitPtr->CKTrhsOld);
    const VerilogBlock *thisvb = this;
    if (thisvb && sim)
        sim->step(outd->count);
}


bool
VerilogBlock::query_var(const char *name, const char *range, double *d)
{
    const VerilogBlock *thisvb = this;
    if (thisvb && sim && sim->top_modules) {
        vl_context cx;
        cx.module = sim->top_modules->mods[0];
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
    const VerilogBlock *thisvb = this;
    if (thisvb && sim && sim->top_modules) {
        vl_context cx;
        cx.module = sim->top_modules->mods[0];
        vl_var *data = cx.lookup_var((char*)a->dig_var, true);
        if (data) {
            const char *range = a->range;
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
                data->assign_to(val, a->offset, a->quantum, l, r);
                return (true);
            }
            data->assign_to(val, a->offset, a->quantum, -1, -1);
            return (true);
        }
    }
    return (false);
}
