
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
 * Misc. Utilities Library                                                *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

//
// Utility for execution time measurement.
//

#ifndef TIMEDBG_H
#define TIMEDBG_H

#include "lstring.h"
#include "tvals.h"
#include "symtab.h"

struct tv_elt
{
    tv_elt(const char *key)
        {
            name = lstring::copy(key);
            next = 0;
            count = 0;
        }

    ~tv_elt() { delete [] name; }

    const char *tab_name()    const { return (name); }
    tv_elt *tab_next()              { return (next); }
    void set_tab_next(tv_elt *t)    { next = t; }
    tv_elt *tgen_next(bool)         { return (next); }

private:
    const char *name;
    tv_elt *next;
public:
    Tvals tv;
    Tvals accum;
    unsigned int count;
    int level;
};

#define TDB_ALL_ACCUM (const char*)-1

inline class cTimeDbg *Tdbg();

class cTimeDbg
{
    static cTimeDbg *ptr()
        {
            if (!instancePtr)
                on_null_ptr();
            return (instancePtr);
        }

    static void on_null_ptr();

public:
    friend inline cTimeDbg *Tdbg() { return (cTimeDbg::ptr()); }

    cTimeDbg();
    ~cTimeDbg();

    void set_active(bool b)     { td_active = b; }
    bool is_active()            { return (td_active); }
    void set_max_level(int n)   { td_max_level = n; }

    void set_logfile(const char*);

    void start_timing(const char *key)
        {
            if (!td_active || !key)
                return;
            start_timing_prv(key);
        }

    void accum_timing(const char *key)
        {
            if (!td_active || !td_table || !key)
                return;
            accum_timing_prv(key);
        }

    void update_timing(const char *key, int num = -1)
        {
            if (!td_active || !td_table || !key)
                return;
            update_timing_prv(key, num);
        }

    void stop_timing(const char *key, int num = -1)
        {
            if (!td_active || !td_table || !key)
                return;
            stop_timing_prv(key, num);
        }

    void print_accum(const char *key)
        {
            if (!td_active || !td_table || !key)
                return;
            print_accum_prv(key);
        }

    void save_message(const char*, ...);
    void clear();

private:
    void start_timing_prv(const char*);
    void accum_timing_prv(const char*);
    void update_timing_prv(const char*, int);
    void stop_timing_prv(const char*, int);
    void print(const tv_elt*, const char*, int);
    void print_accum_prv(const char*);
    void acprint(const tv_elt*, int);

    table_t<tv_elt>     *td_table;
    char                *td_logfile;
    stringlist          *td_msgs;
    bool                td_active;

    static cTimeDbg     *instancePtr;
    static int          td_level;
    static int          td_max_level;
};

// Instaitiate this in a function to monitor.
// WARNING:  the string passed to the constructor must remain
// unchanged until after this is destroyed - should pass a constant
// string.
//
struct TimeDbg
{
    TimeDbg(const char *s)
        {
            word = s;
            Tdbg()->start_timing(s);
        }

    ~TimeDbg()
        {
            Tdbg()->stop_timing(word);
        }

private:
    const char *word;
};

// As above, but accumulate the time.
//
struct TimeDbgAccum
{
    TimeDbgAccum(const char *s)
        {
            word = s;
            Tdbg()->start_timing(s);
        }

    ~TimeDbgAccum()
        {
            Tdbg()->accum_timing(word);
        }

private:
    const char *word;
};

#endif

