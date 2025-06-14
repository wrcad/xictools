
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

#include "variable.h"
#include "wlist.h"
#include "misc.h"
#include "ttyio.h"
#include "spnumber/hash.h"
#include "spnumber/spnumber.h"
#include "ginterf/graphics.h"

#include <climits>


//
// The "set" variables database.
//

unsigned int
sVarDb::allocated()
{
    return (variables ? variables->allocated() : 0);
}


// Return the variable struct from the hash table.
//
variable *
sVarDb::get(const char *vname)
{
    if (!variables)
        return (0);
    return ((variable*)sHtab::get(variables, vname));
}


// Add an entry to the database, no constraints.  The variable struct
// is added to the hash table directly.
//
void
sVarDb::add(const char *tag, variable *v)
{
    if (!variables)
        // Variable names are case-sensitive.
        variables = new sHtab(false);
    variables->add(tag, v);
}


// Remove an entry from the table.  Note that the variable struct is
// not freed.
//
bool
sVarDb::remove(const char *tag)
{
    if (variables)
        return (variables->remove(tag) != 0);
    return (false);
}


// Return a list of the variable names in the database.
//
wordlist *
sVarDb::wl()
{
    return (sHtab::wl(variables));
}
// End of sVarDb functions.


variable::variable(const char *n)
{
    va_type = VTYP_NONE;
    va_name = lstring::copy(n);
    va_reference = 0;
    va.v_real = 0.0;
    va_next = 0;
}


variable::~variable()
{
    delete [] va_name;
    delete [] va_reference;
    clear();
}


// The following functions establish the content and type of the
// variable.  Any existing content is freed.

void
variable::set_boolean(bool b)
{
    clear();
    va_type = VTYP_BOOL;
    va.v_bool = b;
}


void
variable::set_integer(int i)
{
    clear();
    va_type = VTYP_NUM;
    va.v_num = i;
}


void
variable::set_real(double r)
{
    clear();
    va_type = VTYP_REAL;
    va.v_real = r;
}


// Note that strings are always copied.
//
void
variable::set_string(const char *s)
{
    if (va_type == VTYP_STRING && s == va.v_string)
        // unlikely case
        return;
    clear();
    va_type = VTYP_STRING;
    va.v_string = lstring::copy(s);
}


// The list is *not* copied, so care must be taken to not invalidate a
// list once it is set into a variable struct.  The list is owned and
// managed by the variable struct.
//
void
variable::set_list(variable *v)
{
    clear();
    va_type = VTYP_LIST;
    va.v_list = v;
}


void
variable::set_reference(const char *r)
{
    char *s = lstring::copy(r);
    delete [] va_reference;
    va_reference = s;
}


// Static function.
// Return a string representing the variable struct type.
//
const char *
variable::typeString(int tp)
{
    switch (tp) {
    case VTYP_BOOL:
        return ("bool");
    case VTYP_NUM:
        return ("int ");
    case VTYP_REAL:
        return ("real");
    case VTYP_STRING:
        return ("str ");
    case VTYP_LIST:
        return ("list");
    default:
        break;
    }
    return ("    ");
}


// Static function.
// Copy a variable structure.
//
variable *
variable::copy(const variable *v)
{
    variable *nv = 0, *n0 = 0;
    while (v) {
        if (n0 == 0)
            n0 = nv = new variable;
        else {
            nv->va_next = new variable;
            nv = nv->va_next;
        }
        nv->va_name = lstring::copy(v->va_name);
        nv->va_reference = lstring::copy(v->va_reference);
        nv->va_type = v->va_type;
        if (v->va_type == VTYP_BOOL)
            nv->va.v_bool = v->va.v_bool;
        else if (v->va_type == VTYP_REAL)
            nv->va.v_real = v->va.v_real;
        else if (v->va_type == VTYP_NUM)
            nv->va.v_num = v->va.v_num;
        else if (v->va_type == VTYP_STRING)
            nv->va.v_string = lstring::copy(v->va.v_string);
        else if (v->va_type == VTYP_LIST)
            nv->va.v_list = copy(v->va.v_list);
        v = v->va_next;
    }
    return (n0);
}


// Return a wordlist of the text of the variable.
//
wordlist *
variable::varwl(const char *unit) const
{
    wordlist *wl = 0, *wx = 0;
    char buf[BSIZE_SP];
    switch(va_type) {
    case VTYP_BOOL:
        snprintf(buf, sizeof(buf), "%s", va.v_bool ? "true" : "false");
        break;
    case VTYP_NUM:
        snprintf(buf, sizeof(buf), "%d", va.v_num);
        break;
    case VTYP_REAL:
        // Old format:
        // snprintf(buf, sizeof(buf), "%.14g", va.v_real);
        // SPnum.fixxp2(buf);
        {
            // SRW 06/14/2025 
            // If the number represents an integer without units, print
            // as an integer.
            double d = va.v_real;
            if ((!unit || !*unit) && d <= LLONG_MAX && d >= LLONG_MIN &&
                    d == (int64_t)d)
                snprintf(buf, sizeof(buf), "%ld", (int64_t)d);
            else
                strcpy(buf, SPnum.printnum(d, unit));
        }
        break;
    case VTYP_STRING:
        if (va.v_string == 0)
            strcpy(buf, "(none)");
        else {
            char *t = lstring::copy(va.v_string);
            // remove quotes if present
            if (*t == '"') {
                strcpy(buf, t+1);
                char *l = buf + strlen(buf) - 1;
                if (*l == '"')
                    *l = '\0';
            }
            else
                strcpy(buf, t);
            delete [] t;
        }
        break;
    case VTYP_LIST:   // The tricky case
        for (const variable *vt = va.v_list; vt; vt = vt->va_next) {
            wordlist *w;
            if ((w = vt->varwl()) == 0)
                continue;
            if (wl == 0)
                wl = wx = w;
            else {
                wx->wl_next = w;
                w->wl_prev = wx;
            }
            while (wx->wl_next)
                wx = wx->wl_next;
        }
        return (wl);
    default:
        GRpkg::self()->ErrPrintf(ET_INTERR, "varwl: bad variable type %d.\n",
            va_type);
        return (0);
    }
    wl = new wordlist(buf, 0);
    return (wl);
}


// Return a wordlist representing the range lo to hi of the variable.
//
wordlist *
variable::var2wl(int lo, int hi) const
{
    struct vlist
    {
        vlist(const variable *v)
            {
                var = v;
                next = 0;
            }

        static void destroy(vlist *v)
            {
                while (v) {
                    vlist *vx = v;
                    v = v->next;
                    delete vx;
                }
            }

        const variable *var;
        vlist *next;
    };

    if (lo < 0)
        lo = 0;
    if (hi < 0)
        return (0);

    if (va_type != VTYP_LIST) {
        if (lo != 0 && hi != 0)
            return (0);
        else
            return (varwl());
    }
    bool rev = false;
    if (hi < lo) {
        int i = hi;
        hi = lo;
        lo = i;
        rev = true;
    }

    const variable *vx = va.v_list;
    for (int i = 0; i < lo; i++) {
        vx = vx->va_next;
        if (vx == 0)
            return (0);
    }
    const variable *vlo = vx;
    for (int i = lo; i < hi; i++) {
        vx = vx->va_next;
        if (vx == 0)
            break;
    }

    vlist *l0 = 0, *lend = 0;
    for (const variable *v = vlo; v; v = v->va_next) {
        if (!l0)
            l0 = lend = new vlist(v);
        else {
            lend->next = new vlist(v);
            lend = lend->next;
        }
        if (v == vx)
            break;
    }
    if (rev) {
        lend = 0;
        while (l0) {
            vlist *lt = l0->next;
            l0->next = lend;
            lend = l0;
            l0 = lt;
        }
        l0 = lend;
    }

    wordlist *wl0 = 0, *wend = 0;
    for (vlist *l = l0; l; l = l->next) {
        wordlist *ww = l->var->varwl();
        if (!ww)
            continue;
        if (!wl0)
            wl0 = wend = ww;
        else {
            while (wend->wl_next)
                wend = wend->wl_next;
            wend->wl_next = ww;
            ww->wl_prev = wend;
        }
    }
    vlist::destroy(l0);
    return (wl0);
}

