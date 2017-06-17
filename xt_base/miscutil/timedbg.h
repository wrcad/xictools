
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2005 Whiteley Research Inc, all rights reserved.        *
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
 * This library is available for non-commercial use under the terms of    *
 * the GNU Library General Public License as published by the Free        *
 * Software Foundation; either version 2 of the License, or (at your      *
 * option) any later version.                                             *
 *                                                                        *
 * A commercial license is available to those wishing to use this         *
 * library or a derivative work for commercial purposes.  Contact         *
 * Whiteley Research Inc., 456 Flora Vista Ave., Sunnyvale, CA 94086.     *
 * This provision will be enforced to the fullest extent possible under   *
 * law.                                                                   *
 *                                                                        *
 * This library is distributed in the hope that it will be useful,        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 * Library General Public License for more details.                       *
 *                                                                        *
 * You should have received a copy of the GNU Library General Public      *
 * License along with this library; if not, write to the Free             *
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.     *
 *========================================================================*
 *                                                                        *
 * Misc. Utilities Library                                                *
 *                                                                        *
 *========================================================================*
 $Id: timedbg.h,v 1.13 2015/10/11 19:35:44 stevew Exp $
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

