
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA, http://wrcad.com            *
 *  Copyright (C) 2017 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
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
 * Misc. Utilities Library                                                *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

//
// Utility for execution time measurement.
//

#include "timedbg.h"
#include <stdlib.h>
#include <stdarg.h>
#include <algorithm>


cTimeDbg *cTimeDbg::instancePtr = 0;
int cTimeDbg::td_level = 0;
int cTimeDbg::td_max_level = -1;

cTimeDbg::cTimeDbg()
{
    if (instancePtr) {
        fprintf(stderr, "Singleton class cTimeDbg already instantiated.\n");
        exit(1);
    }
    instancePtr = this;

    td_table = 0;
    td_logfile = 0;
    td_msgs = 0;
    td_active = false;
}


cTimeDbg::~cTimeDbg()
{
    instancePtr = 0;
    delete [] td_logfile;
    stringlist::destroy(td_msgs);
    clear();
}


// Private static error exit.
//
void
cTimeDbg::on_null_ptr()
{
    fprintf(stderr, "Singleton class cTimeDbg used before instantiated.\n");
    exit(1);
}


void
cTimeDbg::set_logfile(const char *lf)
{
    delete [] td_logfile;
    td_logfile = lstring::copy(lf);
    if (td_logfile) {
        FILE *fp = fopen(td_logfile, "w");
        if (fp)
            fclose(fp);
    }
}

#define BFSZ 512

// Save a message.  These are kept in order, and printed when the next
// timing result is printed.
//
void
cTimeDbg::save_message(const char *fmt, ...)
{
    if (!td_active)
        return;
    va_list args;
    char buf[BFSZ];

    if (!fmt)
        fmt = "";
    va_start(args, fmt);
    vsnprintf(buf, BFSZ, fmt, args);
    va_end(args);

    if (!td_msgs)
        td_msgs = new stringlist(lstring::copy(buf), 0);
    else {
        stringlist *s = td_msgs;
        while (s->next)
            s = s->next;
        s->next = new stringlist(lstring::copy(buf), 0);
    }
}


void
cTimeDbg::clear()
{
    stringlist::destroy(td_msgs);
    td_msgs = 0;
    if (!td_table)
        return;
    td_table->clear();
    delete td_table;
    td_table = 0;
}


// Start a time measurement, associated with key.
//
void
cTimeDbg::start_timing_prv(const char *key)
{
    // If too many levels, bail.
    if (td_max_level >= 0 && td_level > td_max_level)
        return;

    if (!td_table)
        td_table = new table_t<tv_elt>;

    tv_elt *tv = td_table->find(key);
    if (!tv) {
        tv = new tv_elt(key);
        td_table->link(tv, false);
        td_table = td_table->check_rehash();
        tv->level = td_level;
    }
    td_level++;
    tv->tv.start();
}


// Add the accumulated time since the corresponding call to
// start_timing_prv and increment the call count.  Nothing is printed
// until print_accum_prv is called.  This returns silently if the key
// is unresolved.
//
void
cTimeDbg::accum_timing_prv(const char *key)
{
    if (!td_table)
        return;
    tv_elt *tv = td_table->find(key);
    if (!tv)
        return;
    tv->tv.stop();
    tv->accum.accum(&tv->tv);
    tv->count++;
    td_level--;
}


// Print the elapsed time for key, and restart.  The num argument is
// as described for stop_timing_prv.
//
void
cTimeDbg::update_timing_prv(const char *key, int num)
{
    if (!td_table)
        return;
    tv_elt *tv = td_table->find(key);
    if (tv)
        tv->tv.stop();
    print(tv, key, num);
    if (tv)
        tv->tv.start();
}


// Print the elapsed time for key, and purge key.  The num is a number
// of operations performed, as provided by the caller.  For example,
// this could be bytes transmitted or figures rendered.  The printout
// will include times per operation.  If num is zero, nothing is
// printed.  If num is negative (the default for this optional
// argument) then it is ignored, and only absolute times are printed.
//
void
cTimeDbg::stop_timing_prv(const char *key, int num)
{
    if (!td_table)
        return;
    tv_elt *tv = td_table->find(key);
    if (tv) {
        tv->tv.stop();
        td_level--;
    }
    print(tv, key, num);
    if (tv) {
        tv = td_table->remove(key);
        delete tv;
    }
}


// Print a measurement, called from update_timing_prv and
// print_timing_prv.  Print the messages in any case.  Print a warning
// if tv is null.  Print nothing further if num is zero.
//
void
cTimeDbg::print(const tv_elt *tv, const char *key, int num)
{
    FILE *fp = td_logfile ? fopen(td_logfile, "a") : 0;
    if (!fp)
        fp = stdout;
    if (td_msgs) {
        for (stringlist *s = td_msgs; s; s = s->next)
            fprintf(fp, "%s\n", s->string);
        stringlist::destroy(td_msgs);
        td_msgs = 0;
    }
    if (!tv)
        fprintf(fp, "%s:  unknown key.\n", key);
    else if (num) {
        for (int i = 0; i < td_level; i++) {
            putc(' ', fp);
            putc(' ', fp);
        }
        if (num >= 0)
            fprintf(fp, "%s: num=%d real=%g n/s=%g user=%g system=%g\n", key,
                num, tv->tv.realTime(), num/tv->tv.realTime(),
                tv->tv.userTime(), tv->tv.systemTime());
        else
            fprintf(fp, "%s:  real=%g  user=%g  system=%g\n", key,
                tv->tv.realTime(), tv->tv.userTime(), tv->tv.systemTime());
    }
    if (fp != stdout)
        fclose(fp);
}


namespace {
    bool tv_cmp(const tv_elt *ta, const tv_elt *tb)
    {
        if (ta->level > tb->level)
            return (false);
        if (ta->level < tb->level)
            return (true);
        return (strcmp(ta->tab_name(), tb->tab_name()) < 0);
    }

    tv_elt *tvsort(tv_elt *tv0)
    {
        int cnt = 0;
        for (tv_elt *tv = tv0; tv; tv = tv->tab_next())
            cnt++;
        if (cnt < 2)
            return (tv0);
        tv_elt **ary = new tv_elt*[cnt];
        cnt = 0;
        for (tv_elt *tv = tv0; tv; tv = tv->tab_next())
            ary[cnt++] = tv;
        std::sort(ary, ary + cnt, tv_cmp);
        cnt--;
        ary[cnt]->set_tab_next(0);
        cnt--;
        while (cnt >= 0) {
            ary[cnt]->set_tab_next(ary[cnt+1]);
            cnt--;
        }
        tv0 = ary[0];
        delete [] ary;
        return (tv0);
    }
}


// Private function to print the accumulated time for key, and
// terminate that measurement.  If key is TDB_ALL_ACCUM, then all
// measurements with a level equal to or larger than the present level
// will be printed and terminated.
//
void
cTimeDbg::print_accum_prv(const char *key)
{
    if (!td_table)
        return;
    if (key == TDB_ALL_ACCUM) {

        // Remove the entries with level equal to the current level
        // and larger, and with at least one call.

        tgen_t<tv_elt> gen(td_table);
        tv_elt *tv, *tv0 = 0;
        while ((tv = gen.next()) != 0) {
            if (tv->level >= td_level && tv->count) {
                tv = td_table->remove(tv->tab_name());
                tv->set_tab_next(tv0);
                tv0 = tv;
            }
        }
        // Sort, print, and delete the entries.
        tv0 = tvsort(tv0);
        tv_elt *tn;
        for (tv = tv0; tv; tv = tn) {
            tn = tv->tab_next();
            acprint(tv, tv->level);
        }
        return;
    }
    tv_elt *tv = td_table->remove(key);
    acprint(tv, td_level);
}


// Format and print an accumulated time measurement.  The lev is the
// indentation level to use.  THIS FREES tv;
//
void
cTimeDbg::acprint(const tv_elt *tv, int lev)
{
    FILE *fp = td_logfile ? fopen(td_logfile, "a") : 0;
    if (!fp)
        fp = stdout;
    if (td_msgs) {
        for (stringlist *s = td_msgs; s; s = s->next)
            fprintf(fp, "%s\n", s->string);
        stringlist::destroy(td_msgs);
        td_msgs = 0;
    }
    if (tv) {
        for (int i = 0; i < lev; i++) {
            putc(' ', fp);
            putc(' ', fp);
        }
        fprintf(fp,
            "%-*s:  calls=%-7u real=%-7.3f  user=%-7.3f  system=%-7.3f\n",
            18-(2*lev), tv->tab_name(), tv->count, tv->accum.realTime(),
            tv->accum.userTime(), tv->accum.systemTime());
        delete tv;
    }
    if (fp != stdout)
        fclose(fp);
}

