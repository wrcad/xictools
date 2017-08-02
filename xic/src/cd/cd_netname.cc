
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
 * Xic Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "cd.h"
#include "cd_types.h"
#include "cd_propnum.h"
#include "cd_terminal.h"
#include "cd_netname.h"
#include "cd_celldb.h"
#include "cd_hypertext.h"
#include <ctype.h>
#include <algorithm>


//
// Net expression handling.
//
// Xic syntax is the same as Cadence, except:
// 1.  In Xic, <...>, [...], and {...} are all equivalent.
// 2.  In Xic, N*, <N*>, [N*], and {N*} are all equivalent prefix mults.
//

char *cTnameTab::tnt_buf = 0;

// Start in case-insensitive net/terminal name mode.  This differs
// from earlier Xic, but follows Virtuoso/Hspice/WRspice defaults for
// net/node names.
//
bool cTnameTab::tnt_case_insens = true;

// Default characters used for net/terminal/instance name subscriping. 
// Although any of <>, [], {} are equivalent, they are converted to
// our chars when subscripted names go into the string table.
//
char cTnameTab::tnt_subopen = DEF_SUBSCR_OPEN;
char cTnameTab::tnt_subclose = DEF_SUBSCR_CLOSE;

namespace {
    // Return the terminating character of the present net expression
    // term.
    const char *find_end(const char *s)
    {
        int prncnt = 0;
        int angcnt = 0;
        int sbcnt = 0;
        int cbcnt = 0;
        for ( ; *s; s++) {
            if (*s == '(') {
                prncnt++;
                continue;
            }
            if (*s == ')') {
                if (prncnt) {
                    prncnt--;
                    continue;
                }
                break;
            }
            if (*s == '<') {
                angcnt++;
                continue;
            }
            if (*s == '>') {
                if (angcnt) {
                    angcnt--;
                    continue;
                }
                break;
            }
            if (*s == '[') {
                sbcnt++;
                continue;
            }
            if (*s == ']') {
                if (sbcnt) {
                    sbcnt--;
                    continue;
                }
                break;
            }
            if (*s == '{') {
                cbcnt++;
                continue;
            }
            if (*s == '}') {
                if (cbcnt) {
                    cbcnt--;
                    continue;
                }
                break;
            }
            if (*s == ',') {
                if (prncnt || angcnt || sbcnt | cbcnt)
                    continue;
                break;
            }
        }
        return (s);
    }


    // Tests for opening/closing subscript characters.  Note that
    // we don't require them to match.

    bool openchar(const char *s)
    {
        return (*s == '[' || *s == '<' || *s == '{');
    }


    bool closechar(const char *s)
    {
        return (*s == ']' || *s == '>' || *s == '}');
    }
}


//-----------------------------------------------------------------------------
// Vector expression list element.

CDvecex::CDvecex(const CDvecex &vx)
{
    vx_next = 0;
    vx_group = 0;
    vx_beg_range = vx.vx_beg_range;
    vx_end_range = vx.vx_end_range;
    vx_incr = vx.vx_incr;
    vx_postmult = vx.vx_postmult;
    if (vx.vx_group)
        vx_group = new CDvecex(*vx.vx_group);
    if (vx.vx_next)
        vx_next = new CDvecex(*vx.vx_next);
}


CDvecex&
CDvecex::operator=(const CDvecex &vx)
{
    vx_next = 0;
    vx_group = 0;
    vx_beg_range = vx.vx_beg_range;
    vx_end_range = vx.vx_end_range;
    vx_incr = vx.vx_incr;
    vx_postmult = vx.vx_postmult;
    if (vx.group())
        vx_group = new CDvecex(*vx.group());
    if (vx.next())
        vx_next = new CDvecex(*vx.next());
    return (*this);
}


bool
CDvecex::operator==(const CDvecex &vx) const
{
    CDvecexGen vg1(this);
    CDvecexGen vg2(&vx);
    for (;;) {
        int n1, n2;
        bool r1 = vg1.next(&n1);
        bool r2 = vg2.next(&n2);
        if (r1 != r2 || n1 != n2)
            return (false);
        if (!r1)
            break;
    }
    return (true);
}


bool
CDvecex::operator!=(const CDvecex &vx) const
{
    CDvecexGen vg1(this);
    CDvecexGen vg2(&vx);
    for (;;) {
        int n1, n2;
        bool r1 = vg1.next(&n1);
        bool r2 = vg2.next(&n2);
        if (r1 != r2 || n1 != n2)
            return (true);
        if (!r1)
            break;
    }
    return (false);
}


void
CDvecex::print_this(sLstr *lstr) const
{
    if (vx_group) {
        lstr->add_c('(');
        CDvecex::print_all(vx_group, lstr);
        lstr->add_c(')');
    }
    else {
        lstr->add_i(vx_beg_range);
        if (vx_end_range != vx_beg_range) {
            lstr->add_c(':');
            lstr->add_i(vx_end_range);
            if (vx_incr > 1) {
                lstr->add_c(':');
                lstr->add_i(vx_incr);
            }
        }
    }
    if (vx_postmult > 1) {
        lstr->add_c('*');
        lstr->add_i(vx_postmult);
    }
}


// Static function.
bool
CDvecex::parse(const char *str, CDvecex **pvx, const char **endp)
{
    const char *msg = "vector expression parse error, %s.";

    if (pvx)
        *pvx = 0;
    if (endp)
        *endp = 0;
    if (!str)
        return (true);
    CDvecex *v0 = 0, *ve = 0;
    bool done = false;
    while (*str && !done) {

        // Grab the text for this term.
        const char *t = find_end(str);
        if (endp)
            *endp = t;
        // Terminate parse at unmatched ')'.
        if (*t == ')')
            done = true;
        char *tok = new char[t - str + 1];
        strncpy(tok, str, t-str);
        tok[t - str] = 0;
        if (*t)
            t++;
        str = t;

        t = tok;
        // We'll accept white space at the start of every term,
        // elsewhere it is not allowed.
        while (isspace(*t))
            t++;

        // Possible forms:
        // N[:N[:N]][*N]
        // (term, ...)[*N]

        CDvecex *vecex = 0;
        unsigned int n1 = 0;
        unsigned int n2 = 0;
        unsigned int incr = 1;
        unsigned int mult = 1;
        if (*t == '(') {
            const char *tt;
            if (!CDvecex::parse(t+1, &vecex, &tt) || !vecex) {
                // error
                Errs()->add_error(msg, "expecting expression after \'(\'");
                destroy(v0);
                delete [] tok;
                return (false);
            }
            t = tt;
            if (*t == ')')
                t++;
        }
        else if (isdigit(*t)) {
            n2 = n1 = atoi(t);
            while (isdigit(*t))
                t++;
            if (*t == ':') {
                t++;
                if (isdigit(*t)) {
                    n2 = atoi(t);
                    while (isdigit(*t))
                        t++;
                    if (*t == ':') {
                        t++;
                        if (isdigit(*t)) {
                            incr = atoi(t);
                            while (isdigit(*t))
                                t++;
                        }
                        else {
                            // error
                            Errs()->add_error(msg,
                                "expecting integer after \':\'");
                            destroy(v0);
                            delete [] tok;
                            return (false);
                        }
                    }
                }
                else {
                    // error
                    Errs()->add_error(msg, "expecting integer after \':\'");
                    destroy(v0);
                    delete [] tok;
                    return (false);
                }
            }
        }
        else {
            // error
            Errs()->add_error(msg, "expecting integer or \'(\'");
            destroy(v0);
            delete [] tok;
            return (false);
        }
        if (*t == '*') {
            t++;
            if (isdigit(*t))
                mult = atoi(t);
            else {
                // error;
                Errs()->add_error(msg, "expecting integer after \'*\'");
                destroy(v0);
                delete [] tok;
                return (false);
            }
        }

        if (!v0)
            v0 = ve = new CDvecex(vecex, n1, n2, incr, mult);
        else {
            ve->set_next(new CDvecex(vecex, n1, n2, incr, mult));
            ve = ve->next();
        }
        delete [] tok;
    }
    if (pvx)
        *pvx = v0;
    else
        destroy(v0);
    return (true);
}
// End of CDvecex functions.


//-----------------------------------------------------------------------------
// Vector expression iterator.

CDvecexGen::CDvecexGen(const CDvecex *vx)
{
    vg_next = 0;
    vg_sub = 0;
    vg_group = 0;
    vg_beg = 0;
    vg_end = 0;
    vg_incr = 1;
    vg_mult = 1;
    vg_mcnt = 0;
    if (vx) {
        vg_next = vx->next();
        vg_group = vx->group();
        vg_beg = vx->beg_range();
        vg_end = vx->end_range();
        vg_incr = vx->increment();
        vg_mult = vx->postmult();
        vg_mcnt = vg_mult - 1;
    }
    vg_ascend = (vg_end >= vg_beg);
    vg_done = false;
}


CDvecexGen::~CDvecexGen()
{
    delete vg_sub;
}


bool
CDvecexGen::next(int *n)
{
    if (vg_done)
        return (false);
    if (!vg_group) {
        *n = vg_beg;
        if (vg_mcnt > 0) {
            vg_mcnt--;
            return (true);
        }
        if (vg_ascend) {
            vg_beg += vg_incr;
            if (vg_beg > vg_end) {
                if (!vg_next)
                    vg_done = true;
                else {
                    vg_group = vg_next->group();
                    vg_beg = vg_next->beg_range();
                    vg_end = vg_next->end_range();
                    vg_incr = vg_next->increment();
                    vg_mult = vg_next->postmult();
                    vg_ascend = (vg_end >= vg_beg);
                    vg_next = vg_next->next();
                }
            }
        }
        else {
            vg_beg -= vg_incr;
            if (vg_beg < vg_end) {
                if (!vg_next)
                    vg_done = true;
                else {
                    vg_group = vg_next->group();
                    vg_beg = vg_next->beg_range();
                    vg_end = vg_next->end_range();
                    vg_incr = vg_next->increment();
                    vg_ascend = (vg_end >= vg_beg);
                    vg_next = vg_next->next();
                }
            }
        }
        vg_mcnt = vg_mult - 1;
        return (true);
    }
    else {
        if (!vg_sub)
            vg_sub = new CDvecexGen(vg_group);
        if (vg_sub->next(n))
            return (true);
        if (vg_mcnt > 0) {
            vg_mcnt--;
            delete vg_sub;
            vg_sub = new CDvecexGen(vg_group);
            if (vg_sub->next(n))
                return (true);
        }
        delete vg_sub;
        vg_sub = 0;
        if (!vg_next)
            return (false);
        vg_group = vg_next->group();
        vg_beg = vg_next->beg_range();
        vg_end = vg_next->end_range();
        vg_incr = vg_next->increment();
        vg_ascend = (vg_end >= vg_beg);
        vg_next = vg_next->next();
        return (next(n));
    }
}
// End of CDvecexGen functions.


//-----------------------------------------------------------------------------
// Net expression list element.

cTnameTab *CDnetex::nx_nametab = 0;

CDnetex::CDnetex(const CDnetex &nx)
{
    nx_next = 0;
    nx_group = 0;
    nx_name = nx.nx_name;
    nx_vecex = 0;
    nx_multip = nx.nx_multip;
    if (nx.nx_vecex)
        nx_vecex = new CDvecex(*nx.nx_vecex);
    if (nx.nx_group)
        nx_group = new CDnetex(*nx.nx_group);
    if (nx.nx_next)
        nx_next = new CDnetex(*nx.nx_next);
}


CDnetex &
CDnetex::operator=(const CDnetex &nx)
{
    nx_next = 0;
    nx_group = 0;
    nx_name = nx.nx_name;
    nx_vecex = 0;
    nx_multip = nx.nx_multip;
    if (nx.nx_vecex)
        nx_vecex = new CDvecex(*nx.nx_vecex);
    if (nx.nx_group)
        nx_group = new CDnetex(*nx.nx_group);
    if (nx.nx_next)
        nx_next = new CDnetex(*nx.nx_next);
    return (*this);
}


bool
CDnetex::operator==(const CDnetex &nx) const
{
    CDnetexGen ng1(this);
    CDnetexGen ng2(&nx);
    for (;;) {
        CDnetName nm1, nm2;
        int n1, n2;
        bool r1 = ng1.next(&nm1, &n1);
        bool r2 = ng2.next(&nm2, &n2);
        if (r1 != r2 || nm1 != nm2 || n1 != n2)
            return (false);
        if (!r1)
            break;
    }
    return (true);
}


bool
CDnetex::operator!=(const CDnetex &nx) const
{
    CDnetexGen ng1(this);
    CDnetexGen ng2(&nx);
    for (;;) {
        CDnetName nm1, nm2;
        int n1, n2;
        bool r1 = ng1.next(&nm1, &n1);
        bool r2 = ng2.next(&nm2, &n2);
        if (r1 != r2 || nm1 != nm2 || n1 != n2)
            return (true);
        if (!r1)
            break;
    }
    return (false);
}


void
CDnetex::print_this(sLstr *lstr) const
{
    if (nx_multip > 1) {
        lstr->add("<*");
        lstr->add_i(nx_multip);
        lstr->add_c('>');
    }
    if (nx_group) {
        lstr->add_c('(');
        print_all(nx_group, lstr);
        lstr->add_c(')');
    }
    else {
        lstr->add(Tstring(nx_name));
        if (nx_vecex) {
            lstr->add_c('<');
            CDvecex::print_all(nx_vecex, lstr);
            lstr->add_c('>');
        }
    }
}


// Static function.
// Return true if all bits in nx1 can be resolved in nx2, or
// vice-versa.
//
bool
CDnetex::check_compatible(const CDnetex *nx1, const CDnetex *nx2)
{
    // A null netex implies an unlabeled object, which can connect to
    // anything.
    if (!nx1 || !nx2)
        return (true);

    if (!nx1->nx_name || !nx2->nx_name) {
        // At least one is a "tap" name.  For compatibility, both must
        // be simple, and all bits of one must be found in the other.

        int b1, e1;
        if (!nx1->is_simple(0, &b1, &e1))
            return (false);
        int b2, e2;
        if (!nx2->is_simple(0, &b2, &e2))
            return (false);

        if (e1 < b1)
            { int t = e1; e1 = b1; b1 = t; }
        if (e2 < b2)
            { int t = e2; e2 = b2; b2 = t; }
        if (b1 <= b2 && e1 >= e2)
            return (true);
        if (b2 <= b1 && e2 >= e1)
            return (true);
        return (false);
    }

    CDnetexGen ngen(nx1);
    CDnetName nm;
    int n;
    bool nogo = false;
    while (ngen.next(&nm, &n)) {
        if (!nx2->resolve(nm, n)) {
            nogo = false;
            break;
        }
    }
    if (!nogo)
        // All bits resolved, ok. 
        return (true);
    ngen = CDnetexGen(nx2);
    while (ngen.next(&nm, &n)) {
        if (!nx1->resolve(nm, n))
            return (false);
    }
    return (true);
}


// Private support function.
// Return true if all bits in this can be resolved in nx.  Fill in
// the name if this is a tap.
//
bool
CDnetex::check_set_compatible_prv(const CDnetex *nx)
{
    // A null netex implies an unlabeled object, which can connect to
    // anything.
    if (!nx)
        return (true);

    if (!nx_name) {
        // Must be a tap wire, which must be a bus.
        int b, e;
        if (!is_simple(0, &b, &e))
            return (false);

        // The nx must also be a bus.
        CDnetName nm;
        int bx, ex;
        if (!nx->is_simple(&nm, &bx, &ex))
            return (false);

        // The tap range must be included.
        if (ex >= bx) {
            if (b < bx || e < bx)
                return (false);
            if (b > ex || e > ex)
                return (false);
        }
        else {
            if (b < ex || e < ex)
                return (false);
            if (b > bx || e > bx)
                return (false);
        }

        // Success, add the name (assume it exists).
        nx_name = nm;
        return (true);
    }

    // The names are compatible if all of the bits in one can be
    // resolved in the other.

    CDnetexGen ngen(this);
    CDnetName nm;
    int n;
    bool nogo = false;
    while (ngen.next(&nm, &n)) {
        if (!nx->resolve(nm, n)) {
            nogo = false;
            break;
        }
    }
    if (!nogo)
        // All bits resolved, ok. 
        return (true);
    ngen = CDnetexGen(nx);
    while (ngen.next(&nm, &n)) {
        if (!resolve(nm, n))
            return (false);
    }
    return (true);
}


// Private support function.
// Return true if nm<n> exists in this, nm can not be 0.
//
bool
CDnetex::resolve(CDnetName nm, int n) const
{
    if (!nm)
        return (false);
    for (const CDnetex *nx = this; nx; nx = nx->nx_next) {
        if (nx->nx_name != nm)
            continue;

        CDnetexGen ngen(nx);
        CDnetName nmx;
        int vax;
        while (ngen.next(&nmx, &vax)) {
            if (nmx != nm)
                return (false);
            if (vax == n)
                return (true);
        }
    }
    return (false);
}


// Static function.
bool
CDnetex::parse(const char *str, CDnetex **pnx)
{
    const char *msg = "net expression parse error, %s.";

    if (pnx)
        *pnx = 0;
    if (!str)
        return (true);
    CDnetex *n0 = 0, *ne = 0;
    while (*str) {

        // Grab the text for this term.
        const char *t = find_end(str);
        char *tok = new char[t - str + 1];
        strncpy(tok, str, t-str);
        tok[t - str] = 0;
        if (*t)
            t++;
        str = t;

        t = tok;
        // We'll accept white space at the start of every term,
        // elsewhere it is not allowed.
        while (isspace(*t))
            t++;

        // First look for a multiplier.  Accept the forms "N*" and
        // "<*N>", etc.

        int multip = 1;
        if (isdigit(*t)) {
            const char *t0 = t;
            multip = atoi(t);
            while (isdigit(*t))
                t++;
            if (*t == '*')
                t++;
            else {
                t = t0;
                multip = 1;
            }
        }
        else {
            const char *t0 = t;
            if (openchar(t) && *(t+1) == '*' && isdigit(*(t+2))) {
                t += 2;
                multip = atoi(t);
                while (isdigit(*t))
                    t++;
                if (closechar(t))
                    t++;
                else {
                    t = t0;
                    multip = 1;
                }
            }
        }
        const char *nstart = t;
        while (*t && !openchar(t) && *t != '(')
            t++;

        CDnetName name = 0;
        CDvecex *vecex = 0;
        CDnetex *netex = 0;
        int oc = *t;
        if (openchar(t)) {
            if (t > nstart) {
                int n = t - nstart;
                char *bf = new char[n + 1];
                strncpy(bf, nstart, n);
                bf[n] = 0;
                name = name_tab_add(bf);
                delete [] bf;
            }

            // Start of vector expression.
            if (!CDvecex::parse(t+1, &vecex) || !vecex) {
                // error
                Errs()->add_error(msg,
                    "expecting vector expression after \'%c\'", oc);
                destroy(n0);
                delete [] tok;
                return (false);
            }
        }
        else if (*t == '(') {
            if (t > nstart) {
                // error
                Errs()->add_error(msg, "unexpected character before \'(\'");
                destroy(n0);
                delete [] tok;
                return (false);
            }
            if (!parse(t+1, &netex) || !netex) {
                // error
                Errs()->add_error(msg, "expecting net expression after \'(\'");
                destroy(n0);
                delete [] tok;
                return (false);
            }
        }
        else {
            if (t > nstart) {
                int n = t - nstart;
                char *bf = new char[n + 1];
                strncpy(bf, nstart, n);
                bf[n] = 0;
                name = name_tab_add(bf);
                delete [] bf;
            }
            else {
                Errs()->add_error(msg,
                    "term with no name or vector expression");
                destroy(n0);
                delete [] tok;
                return (false);
            }
        }

        if (!n0)
            n0 = ne = new CDnetex(name, multip, vecex, netex);
        else {

            if (netex) {
                if (!netex->first_name()) {
                    // The lead of the group is unnamed.  Set the
                    // name from the previous component.

                    netex->set_first_name(ne->last_name());
                }
            }
            else if (!name) {
                // If the name is missing, just tack the vecex onto
                // the end of the prior entry.  That is, e.g.,
                // foo<2:3>,<5> is equivalent to foo<2:3,5>
                // We know vecex is not null.

                if (multip > 1) {
                    if (!vecex->next())
                        vecex->set_postmult(multip * vecex->postmult());
                    else
                        vecex = new CDvecex(vecex, 0, 0, 1, multip);
                }
                if (!ne->nx_vecex)
                    name = ne->last_name();
                else {
                    CDvecex *vx = ne->nx_vecex;
                    while (vx->next())
                        vx = vx->next();
                    vx->set_next(vecex);
                    continue;
                }
            }

            ne->set_next(new CDnetex(name, multip, vecex, netex));
            ne = ne->next();
        }
        delete [] tok;
    }
    if (pnx)
        *pnx = n0;
    else
        destroy(n0);
    return (true);
}


// Static function.
// The str represents of a single scalar name token or a named vector
// 1-bit expression.  If nnok, allow absence of a name but with a scalar
// vector expression.
//
bool
CDnetex::parse_bit(const char *str, CDnetName *pname, int *pindx, bool nnok)
{
    if (pname)
        *pname = 0;
    if (pindx)
        *pindx = -1;  // negative indicates a scalar
    if (!str) {
        Errs()->add_error("parse_bit: null string.");
        return (false);
    }

    const char *t = find_end(str);
    while (isspace(*t) || *t == ')')
        t++;
    if (*t) {
        Errs()->add_error("parse_bit: junk after bit token.");
        return (false);
    }
    t = str;

    // We'll accept white space at the start of every term,
    // elsewhere it is not allowed.
    while (isspace(*t))
        t++;

    // First look for a multiplier.  Accept the forms "N*" and
    // "<*N>", etc.

    int multip = 1;
    if (isdigit(*t)) {
        const char *t0 = t;
        multip = atoi(t);
        while (isdigit(*t))
            t++;
        if (*t == '*')
            t++;
        else {
            t = t0;
            multip = 1;
        }
    }
    else {
        const char *t0 = t;
        if (openchar(t) && *(t+1) == '*' && isdigit(*(t+2))) {
            t += 2;
            multip = atoi(t);
            while (isdigit(*t))
                t++;
            if (closechar(t))
                t++;
            else {
                t = t0;
                multip = 1;
            }
        }
    }
    if (multip != 1) {
        Errs()->add_error("parse_bit: non-unit prrefix multiplier.");
        return (false);
    }

    const char *nstart = t;
    while (*t && !openchar(t) && *t != '(')
        t++;

    int oc = *t;
    if (openchar(t)) {
        if (t == nstart && !nnok) {
            Errs()->add_error("parse_bit: no name found.");
            return (false);
        }
        if (pname && t != nstart) {
            int n = t - nstart;
            char *bf = new char[n + 1];
            strncpy(bf, nstart, n);
            bf[n] = 0;
            *pname = name_tab_add(bf);
            delete [] bf;
        }

        // Start of vector expression.
        CDvecex *vecex;
        if (!CDvecex::parse(t+1, &vecex) || !vecex) {
            // error
            Errs()->add_error(
                "parse_bit: expecting vector expression after \'%c\'", oc);
            return (false);
        }
        int beg, end;
        if (vecex->is_simple(&beg, &end) && beg == end) {
            if (pindx)
                *pindx = beg;
            CDvecex::destroy(vecex);
            return (true);
        }
        CDvecex::destroy(vecex);
        Errs()->add_error("parse_bit: vector expression not single-bit");
        return (false);
    }
    if (*t == '(') {
        if (t > nstart) {
            // error
            Errs()->add_error("parse_bit: unexpected character before \'(\'");
            return (false);
        }
        return (parse_bit(t+1, pname, pindx));
    }
    if (t == nstart) {
        Errs()->add_error("parse_bit: no name found.");
        return (false);
    }
    if (pname) {
        int n = t - nstart;
        char *bf = new char[n + 1];
        strncpy(bf, nstart, n);
        bf[n] = 0;
        *pname = name_tab_add(bf);
        delete [] bf;
    }
    return (true);
}


// Static function.
// If name is null, space, ?, DEF_TERM_CHAR, or DEF_TERM_CHAR(digits)
// return true.
//
bool
CDnetex::is_default_name(const char *name)
{
    if (!name || !*name)
        return (true);
    if (*name == '?' && !*(name+1))
        return (true);
    if (*name != DEF_TERM_CHAR)
        return (false);
    name++;
    if (!*name)
        return (true);
    while (*name) {
        if (!isdigit(*name))
            return (false);
        name++;
    }
    return (true);
}
// End of CDnetex functions.


//-----------------------------------------------------------------------------
// Net expression iterator.

CDnetexGen::CDnetexGen(const CDnetex *netex)
{
    ng_sub = 0;
    ng_vecgen = 0;
    ng_next = 0;
    ng_group = 0;
    ng_vecex = 0;
    ng_name = 0;
    ng_netex = 0;
    ng_mult = 1;
    ng_mcnt = 0;
    if (netex) {
        ng_next = netex->next();
        ng_group = netex->group();
        ng_vecex = netex->vecexp();
        ng_name = netex->name();
        ng_mult = netex->multip();
    }
}

CDnetexGen::CDnetexGen(const CDp_bnode *pb)
{
    ng_sub = 0;
    ng_vecgen = 0;
    ng_next = 0;
    ng_group = 0;
    ng_vecex = 0;
    ng_name = 0;
    ng_netex = 0;
    ng_mult = 1;
    ng_mcnt = 0;
    if (!pb)
        return;
    const CDnetex *netex = pb->bundle_spec();
    if (!netex) {
        // Fake a CDnetex for the bus.
        CDvecex *vx = new CDvecex(0, pb->beg_range(), pb->end_range(), 1, 1);
        ng_netex = new CDnetex(pb->get_term_name(), 1, vx, 0);
        netex = ng_netex;
    }
    if (netex) {
        ng_next = netex->next();
        ng_group = netex->group();
        ng_vecex = netex->vecexp();
        ng_name = netex->name();
        ng_mult = netex->multip();
    }
}


CDnetexGen::~CDnetexGen()
{
    delete ng_sub;
    delete ng_vecgen;
    CDnetex::destroy(ng_netex);
}


bool
CDnetexGen::next(CDnetName *np, int *n)
{
    if (!ng_group) {
        *np = ng_name;
        if (ng_vecex) {
            if (!ng_vecgen)
                ng_vecgen = new CDvecexGen(ng_vecex);
            if (ng_vecgen->next(n))
                return (true);
            ng_mcnt++;
        }
        else
            *n = -1;
        if (ng_mcnt < ng_mult) {
            if (ng_vecex) {
                delete ng_vecgen;
                ng_vecgen = new CDvecexGen(ng_vecex);
                if (ng_vecgen->next(n))
                    return (true);
            }
            else {
                ng_mcnt++;
                return (true);
            }
        }
    }
    else {
        if (!ng_sub)
            ng_sub = new CDnetexGen(ng_group);
        if (ng_sub->next(np, n))
            return (true);
        ng_mcnt++;
        if (ng_mcnt < ng_mult) {
            delete ng_sub;
            ng_sub = new CDnetexGen(ng_group);
            if (ng_sub->next(np, n))
                return (true);
        }
    }
    if (!ng_next)
        return (false);
    delete ng_sub;
    ng_sub = 0;
    delete ng_vecgen;
    ng_vecgen = 0;
    ng_mcnt = 0;
    ng_group = ng_next->group();
    ng_vecex = ng_next->vecexp();
    ng_name = ng_next->name();
    ng_mult = ng_next->multip();
    ng_next = ng_next->next();
    return (next(np, n));
}
// End of CDnetexGen functions.

