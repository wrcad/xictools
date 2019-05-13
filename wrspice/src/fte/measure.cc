
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

#include "simulator.h"
#include "runop.h"
#include "parser.h"
#include "misc.h"
#include "datavec.h"
#include "output.h"
#include "cshell.h"
#include "device.h"
#include "rundesc.h"
#include "kwords_fte.h"
#include "spnumber/hash.h"
#include "spnumber/spnumber.h"
#include "miscutil/lstring.h"

//
// Functions for the measurement run operation.
//

//#define M_DEBUG

const char *kw_measure     = "measure";
const char *kw_stop        = "stop";
const char *kw_before      = "before";
const char *kw_at          = "at";
const char *kw_after       = "after";
const char *kw_when        = "when";

namespace {
    // Private keywords
    const char *mkw_trig        = "trig";
    const char *mkw_targ        = "targ";
    const char *mkw_val         = "val";
    const char *mkw_td          = "td";
    const char *mkw_cross       = "cross";
    const char *mkw_rise        = "rise";
    const char *mkw_fall        = "fall";
    const char *mkw_from        = "from";
    const char *mkw_to          = "to";
    const char *mkw_min         = "min";
    const char *mkw_max         = "max";
    const char *mkw_pp          = "pp";
    const char *mkw_avg         = "avg";
    const char *mkw_rms         = "rms";
    const char *mkw_pw          = "pw";
    const char *mkw_rt          = "rt";
    const char *mkw_find        = "find";
    const char *mkw_param       = "param";
    const char *mkw_goal        = "goal";
    const char *mkw_minval      = "minval";
    const char *mkw_weight      = "weight";
    const char *mkw_print       = "print";
    const char *mkw_print_terse = "print_terse";
    const char *mkw_exec        = "exec";
    const char *mkw_call        = "call";
    const char *mkw_silent      = "silent";
    const char *mkw_strobe      = "strobe";
}

/*XXX ideas
resolve measure names in the current circuit in expressions as binary tokens,
true if measure done. name[index] resoves to result of index'th meeasure.
No, resolve as scale start before, measure scale val after.  ?mname resolve
as binary?

Think thru logic, need a new flag?
t_found_flag   point was found for this
t_found_state  when t_found_flag set, this is state
t_strobe       when true, evaluate conj when t_found_flag set, t_found_state
               set accordingly.  Otherwise, t_found_state will be set true
               when t_found_flag set.

t_ready        point was found for conj (is this needed?)
if (!t_found_flag)
   check
else
   state is t_state
so can skip the check if state is false (otherwise this is indicated by
t_found_flag not set).
if (conj->t_found_flag && !conf->t_state) {
    t_found_flag = true;
    t_state = false;
}

When in multi-d run, measure performed for each run, result appended
to vector.

new type:  if expr
evaluate expr at prev point set false if not true
*/

void
sMfunc::print(sLstr &lstr)
{
    lstr.add_c(' ');
    if (f_type == Mmin)
        lstr.add(mkw_min);
    else if (f_type == Mmax)
        lstr.add(mkw_max);
    else if (f_type == Mpp)
        lstr.add(mkw_pp);
    else if (f_type == Mavg)
        lstr.add(mkw_avg);
    else if (f_type == Mrms)
        lstr.add(mkw_rms);
    else if (f_type == Mpw)
        lstr.add(mkw_pw);
    else if (f_type == Mrft)
        lstr.add(mkw_rt);
    else if (f_type == Mfind)
        lstr.add(mkw_find);
    lstr.add_c(' ');
    lstr.add(f_expr);
}
// End of sMfunc functions.


namespace {
    // Grab text enclosed in parentheses, return token with outer
    // parentheses stripped if notok is false.  If notok is true, don't
    // return a token, just advance the string pointer.
    //
    char *parse_parens(const char **pstr, bool notok)
    {
        const char *s = *pstr;
        if (*s != '(')
            return (0);

        s++;
        int np = 1;
        const char *p = s;
        while (*p && np) {
            if (*p == '(')
                np++;
            else if (*p == ')')
                np--;
            p++;
        }
        if (notok) {
            *pstr = p;
            return (0);
        }
        int len = p - s;
        char *t = new char[len+1];
        char *e = t;

        np = 1;
        while (*s && np) {
            if (*s == '(')
                np++;
            else if (*s == ')')
                np--;
            if (np)
                *e++ = *s++;
            else
                s++;
        }
        *e = 0;
        *pstr = s;
        return (t);
    }


    // Return comma or space separated token, with '=' and quoted strings
    // considered as tokens.
    //
    char *gtok(const char **pstr)
    {
        if (pstr == 0 || *pstr == 0)
            return (0);
        const char *s = *pstr;

        // Strip leading "space".
        while (isspace(*s) || *s == ',')
            s++;
        if (!*s)
            return (0);
        char *t = 0;
        if (*s == '=') {
            // This is a token.
            t = lstring::copy("=");
            s++;
        }
        else if (*s == '\'' || *s == '"') {
            // Quoted text, take as one token, include quotes.
            const char *p = s+1;
            for ( ; *p && *p != *s; p++) ;
            if (*p)
                p++;
            int len = p - s;
            t = new char[len+1];
            strncpy(t, s, len);
            t[len] = 0;
            s += len;
        }
        else if (*s == '(') {
            // Treat (....) as a single token.  Expressions can be
            // delimited this way to ensure that they are parsed
            // correctly.  Single quotes can also be used.
            //
            t = parse_parens(&s, false);
        }
        else {
            const char *p = s;
            while (*p && !isspace(*p) &&
                    *p != '=' && *p != ',' && *p != '\'') {
                if (*p == '(')
                    parse_parens(&p, true);
                else
                    p++;
            }
            int len = p - s;
            t = new char[len+1];
            strncpy(t, s, len);
            t[len] = 0;
            s += len;
        }
        while (isspace(*s) || *s == ',')
            s++;
        *pstr = s;
        return (t);
    }

    // Return a double found after '=', set err if problems.
    //
    double gval(const char **s, bool *err)
    {
        char *tok = gtok(s);
        if (!tok) {
            *err = true;
            return (0.0);
        }
        if (*tok == '=') {
            delete [] tok;
            tok = gtok(s);
        }
        const char *t = tok;
        double *dd = SPnum.parse(&t, false);
        delete [] tok;
        if (!dd) {
            *err = true;
            return (0.0);
        }
        *err = false;
        return (*dd);
    }


    inline void listerr(char **errstr, const char *string)
    {
        if (!errstr)
            return;
        if (!*errstr) {
            if (string) {
                char buf[128];
                sprintf(buf, "syntax error after \'%s\'.", string);
                *errstr = lstring::copy(buf);
            }
            else
                *errstr = lstring::copy("syntax error.");
        }
    }


    inline void listerr1(char **errstr, const char *string)
    {
        if (!errstr)
            return;
        if (!*errstr) {
            if (string)
                *errstr = lstring::copy(string);
            else
                *errstr = lstring::copy("syntax error.");
        }
    }


    bool is_kw(const char *s)
    {
        if (lstring::cieq(s, kw_measure)) return (true);
        if (lstring::cieq(s, kw_stop)) return (true);
        if (lstring::cieq(s, kw_before)) return (true);
        if (lstring::cieq(s, kw_at)) return (true);
        if (lstring::cieq(s, kw_after)) return (true);
        if (lstring::cieq(s, kw_when)) return (true);
        if (lstring::cieq(s, mkw_trig)) return (true);
        if (lstring::cieq(s, mkw_targ)) return (true);
        if (lstring::cieq(s, mkw_val)) return (true);
        if (lstring::cieq(s, mkw_td)) return (true);
        if (lstring::cieq(s, mkw_cross)) return (true);
        if (lstring::cieq(s, mkw_rise)) return (true);
        if (lstring::cieq(s, mkw_fall)) return (true);
        if (lstring::cieq(s, mkw_from)) return (true);
        if (lstring::cieq(s, mkw_to)) return (true);
        if (lstring::cieq(s, mkw_min)) return (true);
        if (lstring::cieq(s, mkw_max)) return (true);
        if (lstring::cieq(s, mkw_pp)) return (true);
        if (lstring::cieq(s, mkw_avg)) return (true);
        if (lstring::cieq(s, mkw_rms)) return (true);
        if (lstring::cieq(s, mkw_pw)) return (true);
        if (lstring::cieq(s, mkw_rt)) return (true);
        if (lstring::cieq(s, mkw_find)) return (true);
        if (lstring::cieq(s, mkw_param)) return (true);
        if (lstring::cieq(s, mkw_goal)) return (true);
        if (lstring::cieq(s, mkw_minval)) return (true);
        if (lstring::cieq(s, mkw_weight)) return (true);
        if (lstring::cieq(s, mkw_print)) return (true);
        if (lstring::cieq(s, mkw_print_terse)) return (true);
        if (lstring::cieq(s, mkw_call)) return (true);
        if (lstring::cieq(s, mkw_silent)) return (true);
        if (lstring::cieq(s, mkw_strobe)) return (true);
        return (false);
    }
}


sMpoint::~sMpoint()
{
    delete t_conj;
    delete [] t_expr1;
    delete [] t_expr2;
    delete t_tree1;
    delete t_tree2;
    delete [] t_mname;
}


// The general form of the definition string is
//   [when/at] expr[val][=][expr] [td=offset] [cross=crosses] [rise=rises]
//     [fall=falls]
// The initial keyword (which may be missing if unambiguous) is one of
// "at" or "when".  These are equivalent.  One or two expressions follow,
// with optional '=' or 'val=' ahead of the second expression.  the second
// expression can be missing.
//
// MPexp2:  expr1 and expr2 are both given, then the point is when
//   expr==expr2, and the td,cross,rise,fall keywords apply.  The risis,
//   falls, crosses are integers.  The offset is a numeric value, or the
//   name of another measure.  The trigger is the matching
//   rise/fall/cross found after the offset.
//
// If expr2 is not given, then expr1 is one of:
//
// MPnum:  (numeric value) Gives the point directly, no other keywords
//   apply.
//
// MPmref:  (measure name) Point where given measure completes,
//   numeric td applies, triggers at the referenced measure time plus
//   offset.
//
// MPexpr1:  (expression) Point where expression is boolen true, td
//   applies, can be numeric or measure name, trigers when expr is true
//   after offset.
//
int
sMpoint::parse(const char **pstr, char **errstr, const char *kw)
{
    t_kw1 = kw;
    const char *s = *pstr;
    const char *sbk = s;
    char *tok = gtok(&s);
    if (tok && *tok == '=') {
        delete [] tok;
        sbk = s;
        tok = gtok(&s);
    }
    if (!tok) {
        listerr1(errstr, "unexpected end of line.");
        return (E_SYNTAX);
    }
    if (lstring::cieq(tok, kw_at)) {
        t_kw2 = kw_at;
//XXX
        t_strobe = true;
        delete [] tok;
    }
    else if (lstring::cieq(tok, kw_when)) {
        t_kw2 = kw_when;
        delete [] tok;
    }
    else if (lstring::cieq(tok, kw_before)) {
        t_kw2 = kw_before;
        t_range = MPbefore;
        delete [] tok;
    }
    else if (lstring::cieq(tok, kw_after)) {
        t_kw2 = kw_after;
        t_range = MPafter;
        delete [] tok;
    }
    else {
        delete [] tok;
        s = sbk;
    }

    // parse first expression
    while (isspace(*s))
        s++;
    sbk = s;
    pnode *pn;
    bool found_td = false;
    if (*s == '[') {
        // [ const_expr ]
        // take const_expr as point count.
        s++;
        const char *e = s;
        while (*e && *e != ']')
            e++;
        if (*e != ']') {
            listerr1(errstr, "no first expression found for time mark.");
            return (E_SYNTAX);
        }
        int len = e-s+1;
        tok = new char[len];
        memcpy(tok, s, len-1);
        tok[len-1] = 0;
        s = e+1;
        e = tok;
        pn = Sp.GetPnode(&e, false);
        delete [] tok;
        if (!pn) {
            listerr1(errstr, "no point index found in [ ].");
            return (E_SYNTAX);
        }
        t_ptmode = true;
    }
    else {
        tok = gtok(&s);
        s = sbk;
        if (lstring::cieq(tok, mkw_td)) {
            found_td = true;
            delete [] tok;
        }
        else {
            delete [] tok;
            pn = Sp.GetPnode(&s, false);
            if (!pn) {
                listerr1(errstr, "no first expression found for time mark.");
                return (E_SYNTAX);
            }
        }
    }
    if (!found_td) {
        const char *st = s-1;
        while (st >= sbk && isspace(*st))
            st--;
        st++;
        int len = st - sbk;
        tok = new char[len + 1];
        strncpy(tok, sbk, len);
        tok[len] = 0;
        t_expr1 = tok;
        t_tree1 = pn;
        if (t_tree1)
            t_tree1->copyvecs();
#ifdef M_DEBUG
        printf("expr1 \'%s\' \'%s\'\n", tok, t_tree1->get_string(false));
#endif

        // strip any "val="
        sbk = s;
        tok = gtok(&s);
        if (tok && lstring::cieq(tok, mkw_val)) {
            delete [] tok;
            sbk = s;
            tok = gtok(&s);
        }
        if (tok && *tok == '=') {
            delete [] tok;
            sbk = s;
            tok = gtok(&s);
        }
        if (tok) {
            if (is_kw(tok)) {
                delete [] tok;
                s = sbk;
            }
            else {
                delete [] tok;

                // parse second expression
                while (isspace(*s))
                    s++;
                s = sbk;
                pn = Sp.GetPnode(&s, false);
                if (pn) {
                    st = s-1;
                    while (st >= sbk && isspace(*st))
                        st--;
                    st++;
                    len = st - sbk;
                    tok = new char[len + 1];
                    strncpy(tok, sbk, len);
                    tok[len] = 0;
                    t_expr2 = tok;
                    t_tree2 = pn;
                    t_tree2->copyvecs();
                    t_type = MPexp2;
#ifdef M_DEBUG
        printf("expr2 \'%s\' \'%s\'\n", tok, t_tree2->get_string(false));
#endif
                }
            }
        }
    }
    if (!t_tree2) {
        if (t_tree1) {
            if (t_tree1->is_const()) {
                // numeric or constant expression
                t_type = MPnum;
                sDataVec *dv = eval1();
                if (dv) {
                    if (t_ptmode)
                        t_indx = (int)lrint(dv->realval(0));
                    else {
                        t_td = dv->realval(0);
                        t_td_given = true;
                    }
                }
            }
            else if (t_tree1->optype() == TT_EQ) {
                pn = t_tree1;
                pn->split(&t_tree1, &t_tree2);
                delete [] t_expr1;
                delete [] t_expr2;
                t_expr1 = t_tree1->get_string(false);
                t_expr2 = t_tree2->get_string(false);
                t_type = MPexp2;
                delete pn;
            }
            else
                t_type = MPexp1;
        }
        else if (found_td)
            t_type = MPmref;
    }

    const char *last = s;
    while ((tok = gtok(&s)) != 0) {
        if (lstring::cieq(tok, mkw_td)) {
            delete [] tok;
            tok = gtok(&s);
            if (!tok) {
                listerr(errstr, mkw_td);
                break;
            }
            if (*tok == '=') {
                delete [] tok;
                tok = gtok(&s);
            }
            if (!tok) {
                listerr(errstr, mkw_td);
                break;
            }
            const char *t = tok;
            double *dd = SPnum.parse(&t, false, true);
            if (dd && !*t) {
                t_td = *dd;
                t_td_given = true;
                delete [] tok;
            }
            else
                t_mname = tok;
            last = s;
            continue;
        }
        if (t_type == MPexp2) {
            if (lstring::cieq(tok, mkw_cross)) {
                delete [] tok;
                bool err;
                t_crosses = (int)gval(&s, &err);
                if (err) {
                    listerr(errstr, mkw_cross);
                    break;
                }
                last = s;
                continue;
            }
            if (lstring::cieq(tok, mkw_rise)) {
                delete [] tok;
                bool err;
                t_rises = (int)gval(&s, &err);
                if (err) {
                    listerr(errstr, mkw_rise);
                    return (E_SYNTAX);
                }
                last = s;
                continue;
            }
            if (lstring::cieq(tok, mkw_fall)) {
                delete [] tok;
                bool err;
                t_falls = (int)gval(&s, &err);
                if (err) {
                    listerr(errstr, mkw_fall);
                    return (E_SYNTAX);
                }
                last = s;
                continue;
            }
        }
        s = last;
        break;
    }
#ifdef M_DEBUG
    static int lockout;
#endif
    last = s;
    tok = gtok(&s);
    if (tok) {
        bool c = (lstring::cieq(tok, kw_at) || lstring::cieq(tok, kw_when) ||
            lstring::cieq(tok, kw_before) || lstring::cieq(tok, kw_after));
        delete [] tok;
        s = last;
        if (c) {
            t_conj = new sMpoint();
#ifdef M_DEBUG
            lockout++;
#endif
            int err = t_conj->parse(&s, errstr, 0);
#ifdef M_DEBUG
            lockout--;
#endif
            if (err != OK)
                return (err);
        }
    }

    t_active = true;
    *pstr = s;
#ifdef M_DEBUG
    if (!lockout) {
        sLstr lstr;
        print(lstr);
        printf("sMpoint:  %s\n", lstr.string());
    }
#endif
    return (OK);
}


// Print the specification into lstr.
//
void
sMpoint::print(sLstr &lstr)
{
    if (!t_active)
        return;
    lstr.add_c(' ');
    if (t_kw1) {
        lstr.add(t_kw1);
        lstr.add_c(' ');
    }
    if (t_kw2) {
        lstr.add(t_kw2);
        lstr.add_c(' ');
    }
    if (t_type == MPnum) {
        if (t_ptmode) {
            lstr.add_c('[');
            lstr.add_u(t_indx);
            lstr.add_c(']');
        }
        else
            lstr.add_g(t_td);
    }
    else if (t_type == MPmref) {
        lstr.add(t_expr1);
        if (t_td_given) {
            lstr.add_c(' ');
            lstr.add(mkw_td);
            lstr.add_c('=');
            lstr.add_g(t_td);
        }
    }
    else if (t_type == MPexp1) {
        lstr.add(t_expr1);
        if (t_td_given || t_mname) {
            lstr.add_c(' ');
            lstr.add(mkw_td);
            lstr.add_c('=');
            if (t_mname)
                lstr.add(t_mname);
            else
                lstr.add_g(t_td);
        }
    }
    else if (t_type == MPexp2) {
        lstr.add(t_expr1);
        lstr.add_c('=');
        lstr.add(t_expr2);
        if (t_td_given || t_mname) {
            lstr.add_c(' ');
            lstr.add(mkw_td);
            lstr.add_c('=');
            if (t_mname)
                lstr.add(t_mname);
            else
                lstr.add_g(t_td);
        }
        if (t_rises) {
            lstr.add_c(' ');
            lstr.add(mkw_rise);
            lstr.add_c('=');
            lstr.add_u(t_rises);
        }
        if (t_falls) {
            lstr.add_c(' ');
            lstr.add(mkw_fall);
            lstr.add_c('=');
            lstr.add_u(t_falls);
        }
        if (t_crosses) {
            lstr.add_c(' ');
            lstr.add(mkw_cross);
            lstr.add_c('=');
            lstr.add_u(t_crosses);
        }
    }
    if (t_conj)
        t_conj->print(lstr);
}


// Return true if the specified point has been logically reached.
//
bool
sMpoint::check_found(sFtCirc *circuit, bool *err, bool end, sMpoint *mpprev)
{
    if (!t_active)
        return (true);

    bool isready = true;
    if (!t_offset_set) {

        if (t_td_given)
            t_offset = t_td;
        else {
            sDataVec *xs = circuit->runplot()->scale();
            t_offset = xs->unscalarized_first();
        }

        if (t_type == MPnum) {
#ifdef M_DEBUG
            printf("setup_offset:  numeric %s\n", t_expr1);
#endif
                t_offset_set = true;
        }
        else {
            sRunopMeas *m = 0;
            if (t_type == MPexp1) {
                m = sRunopMeas::find(circuit->measures(), t_expr1);
                if (m) {
                    t_type = MPmref;
#ifdef M_DEBUG
                    printf("setup_offset: found expr1 is measure %s\n",
                        t_expr1);
#endif
                }
            }
            if (t_type == MPmref) {
                sMpoint *mp = 0;
                if (!t_expr1) {
                    mp = mpprev;
                    if (!mp) {
                        if (err)
                            *err = true;
                        return (false);
                    }
                }
                if (mp) {
                    if (!mp->t_found_local) {
                        isready = false;
                        goto done;
                    }
                    t_offset += mp->t_found;
                    t_offset_set = true;
                }
                else {
                    if (!m)
                        m = sRunopMeas::find(circuit->measures(), t_expr1);
                    if (!m) {
                        if (err)
                            *err = true;
                        return (false);
                    }
                    if (!m->measure_done()) {
                        isready = false;
                        goto done;
                    }
                    if (m->end().ready())
                        t_offset += m->end().found();
                    else
                        t_offset += m->start().found();
                    t_offset_set = true;
#ifdef M_DEBUG
                    printf(
                        "setup_offset:  expr measure reference %s, offset %g\n",
                        t_expr1, t_offset);
#endif
                }
            }
            else if (t_type == MPexp1 || t_type == MPexp2) {
                if (t_mname) {
                    m = sRunopMeas::find(circuit->measures(), t_mname);
                    if (!m) {
                        if (err)
                            *err = true;
                        return (false);
                    }
                    if (!m->measure_done()) {
                        isready = false;
                        goto done;
                    }
                    if (m->end().ready())
                        t_offset += m->end().found();
                    else
                        t_offset += m->start().found();
#ifdef M_DEBUG
                    printf("setup_offset:  td measure reference %s, offset %g\n",
                        t_mname, t_offset);
#endif
                }
                if (t_type == MPexp2) {
                    // If none of these are set, assume the first crossing.
                    if (t_crosses == 0 && t_rises == 0 && t_falls == 0)
                        t_crosses = 1;
#ifdef M_DEBUG
                    printf("setup_offset:  expr2 \'%s\' \'%s\'\n", t_expr1,
                        t_expr2);
#endif
                }
#ifdef M_DEBUG
                else if (t_type == MPexp1) {
                    printf("setup_offset:  expr1 \'%s\'\n", t_expr1);
                }
#endif
                t_offset_set = true;
            }
        }
    }

    if (t_offset_set && !t_found_local) {
        sDataVec *xs = circuit->runplot()->scale();
        int ix = check_trig(xs);
#ifdef M_DEBUG
        printf("at xs=%g indx=%d\n", xs->realval(0), ix);
#endif
        if (ix < 0) {
            isready = false;
            goto done;
        }

        double fval = t_offset;
        if (t_type == MPnum) {
            if (!mpprev)
                fval = xs->realval(ix);
        }
        else if (t_type == MPmref) {
            // nothing to do
        }
        else if (t_type == MPexp1) {
            sDataVec *dvl = eval1();
            if (!dvl) {
                if (err)
                    *err = true;
                return (false);
            }
            if ((int)dvl->realval(0) == 0) {
                isready = false;
                goto done;
            }
            ix = xs->unscalarized_length() - 1;
            fval = xs->realval(0);
            if (end)
                ix--;
        }
        else if (t_type == MPexp2) {
            sDataVec *dvl = eval1();
            sDataVec *dvr = eval2();
            if (!dvl || !dvr) {
                if (err)
                    *err = true;
                return (false);
            }

            bool foundit = false;
            double v1 = dvl->realval(0);
            double v2 = dvr->realval(0);
            if (!t_last_saved) {
                t_v1 = v1;
                t_v2 = v2;
                t_last_saved = true;
                isready = false;
                goto done;
            }
            double x = xs->realval(0);
            ix = xs->unscalarized_length() - 1;
            if (t_v1 <= t_v2 && v2 < v1) {
                t_rise_cnt++;
                t_cross_cnt++;
            }
            else if (t_v1 > t_v2 && v2 >= v1) {
                t_fall_cnt++;
                t_cross_cnt++;
            }
            else {
                t_v1 = v1;
                t_v2 = v2;
                isready = false;
                goto done;
            }

            if (t_rise_cnt >= t_rises && t_fall_cnt >= t_falls &&
                    t_cross_cnt >= t_crosses) {
                double d = v2 - t_v2 - (v1 - t_v1);
                if (d != 0.0) {
                    double xp = xs->unscalarized_prev_real();
                    fval = xp + (x - xp)*(t_v1 - t_v2)/d;
                }
                else
                    fval = x;
                foundit = true;
                if (end)
                    ix--;
            }
            t_v1 = v1;
            t_v2 = v2;

            if (!foundit) {
                isready = false;
                goto done;
            }
        }

        t_indx = ix;
        t_found = fval;
        t_found_local = true;
    }

done:
    if (t_range == MPbefore)
        isready = !isready;

    if (t_conj) {
        if (!t_conj->check_found(circuit, err, end, this))
            isready = false;
    }
    if (t_found_local && !t_ready) {
        if (!t_conj)
            t_ready = true;
        else if (t_conj->t_ready) {
            if (t_indx < t_conj->t_indx) {
                t_found = t_conj->t_found;
                t_indx = t_conj->t_indx;
            }
            t_ready = true;
        }
    }
    return (isready);
}


// Return a non-negative index if the scale covers the trigger point
// of val.  Do some fudging to avoid not triggering at the expected
// time point due to numerical error.
//
int
sMpoint::check_trig(sDataVec *xs)
{
    int i = xs->unscalarized_length() - 1;
    if (i > 0) {
        double x = xs->realval(0);
        double xp = xs->unscalarized_prev_real();
        if (t_ptmode) {
            if (i < t_indx)
                return (-1);
            return (i);
        }
        if (x > xp) {
            double dx = (x - xp)*1e-3;
            if (x > t_offset - dx) {
                if (fabs(xp - t_offset) < fabs(x - t_offset))
                    i--;
                return (i);
            }
        }
        else if (x < xp) {
            double dx = (x - xp)*1e-3;
            if (x < t_offset - dx) {
                if (fabs(xp - t_offset) < fabs(x - t_offset))
                    i--;
                return (i);
            }
        }
    }
    return (-1);
}


sDataVec *
sMpoint::eval1()
{
    if (!t_expr1)
        return (0);
    if (!t_tree1) {
        const char *s = t_expr1;
        t_tree1 = Sp.GetPnode(&s, true);
        if (t_tree1)
            t_tree1->copyvecs();
    }
    if (t_tree1)
        return (Sp.Evaluate(t_tree1));
    return (0);
}


sDataVec *
sMpoint::eval2()
{
    if (!t_expr2)
        return (0);
    if (!t_tree2) {
        const char *s = t_expr2;
        t_tree2 = Sp.GetPnode(&s, true);
        if (t_tree2)
            t_tree2->copyvecs();
    }
    if (t_tree2)
        return (Sp.Evaluate(t_tree2));
    return (0);
}
// End of sMpoint functions.


void
sRunopMeas::print(char **pstr)
{
    const char *msg1 = "%c %-4d %s";
    char buf[64];
    sprintf(buf, msg1, ro_active ? ' ' : 'I', ro_number, kw_measure);
    sLstr lstr;
    if (pstr && *pstr)
        lstr.add(*pstr);
    lstr.add(buf);
    if (ro_analysis >= 0) {
        IFanalysis *a = IFanalysis::analysis(ro_analysis);
        if (a) {
            lstr.add_c(' ');
            lstr.add(a->name);
        }
    }
    if (ro_result) {
        lstr.add_c(' ');
        lstr.add(ro_result);
    }
    ro_start.print(lstr);
    ro_end.print(lstr);

    if (ro_end.active()) {
        for (sMfunc *m = ro_funcs; m; m = m->next())
            m->print(lstr);
    }
    for (sMfunc *m = ro_finds; m; m = m->next())
        m->print(lstr);

    if (ro_print_flag == 2) {
        lstr.add_c(' ');
        lstr.add(mkw_print);
    }
    else if (ro_print_flag == 1) {
        lstr.add_c(' ');
        lstr.add(mkw_print_terse);
    }
    if (ro_exec) {
        lstr.add_c(' ');
        lstr.add(mkw_exec);
        lstr.add_c(' ');
        lstr.add(ro_exec);
    }
    if (ro_call) {
        lstr.add_c(' ');
        lstr.add(mkw_call);
        lstr.add_c(' ');
        lstr.add(ro_call);
    }
    if (ro_stop_flag) {
        lstr.add_c(' ');
        lstr.add(kw_stop);
    }

    lstr.add_c('\n');
    if (pstr)
        *pstr = lstr.string_trim();
    else
        TTY.send(lstr.string());
}


void
sRunopMeas::destroy()
{
    delete this;
}


namespace {
    // Return true if name names an analysis, and return the index in
    // which.
    //
    bool get_anal(const char *name, int *which)
    {
        for (int i = 0; ; i++) {
            IFanalysis *a = IFanalysis::analysis(i);
            if (!a)
                break;
            if (lstring::cieq(a->name, name)) {
                *which = i;
                return (true);
            }
        }
        *which = -1;
        return (false);
    }


    void exec(const char *cmds)
    {
        if (cmds && *cmds) {
            bool inter = CP.GetFlag(CP_INTERACTIVE);
            CP.SetFlag(CP_INTERACTIVE, false);
            CP.PushControl();
            if (*cmds == '"') {
                char *t = lstring::copy(cmds);
                CP.Unquote(t);
                CP.EvLoop(t);
                delete [] t;
            }
            else
                CP.EvLoop(cmds);
            CP.PopControl();
            CP.SetFlag(CP_INTERACTIVE, inter);
        }
    }


    ROret call(const char *script_name)
    {
        if (script_name && *script_name) {
            wordlist wl;
            wl.wl_word = lstring::copy(script_name);
            Sp.ExecCmds(&wl);
            delete [] wl.wl_word;
            wl.wl_word = 0;
            if (CP.ReturnVal() == CB_PAUSE)
                return (RO_PAUSE);
            if (CP.ReturnVal() == CB_ENDIT)
                return (RO_ENDIT);
        }
/*XXX
        else if (run->check()) {
            // Run the "controls" bound codeblock.  Stop if the fail
            // flag is set.

            CBret ret = run->check()->evaluate();
            if (ret == CBfail)
                return (RO_PAUSE);
            if (ret == CBendit)
                return (RO_ENDIT);
        }
        else {
            sFtCirc *circ = run->circuit();
            if (circ) {
                circ->controlBlk().exec(true);
                if (CP.ReturnVal() == CB_PAUSE)
                    return (RO_PAUSE);
                if (CP.ReturnVal() == CB_ENDIT)
                    return (RO_ENDIT);
            }
        }
    }
*/
        return (RO_OK);
    }
}


// Parse the string and set up the structure appropriately.
//
bool
sRunopMeas::parse(const char *str, char **errstr)
{
    bool err = false;
    bool gotanal = false;
    ro_analysis = -1;
    const char *s = str;
    for (;;) {
        const char *last = s;
        char *tok = gtok(&s);
        if (!tok)
            break;
        if (!gotanal) {
            if (get_anal(tok, &ro_analysis)) {
                gotanal = true;
                delete [] tok;
                ro_result = gtok(&s);
                if (!ro_result) {
                    err = true;
                    listerr1(errstr, "missing result name.");
                    break;
                }
                continue;
            }
            err = true;
            listerr1(errstr, "missing analysis name.");
            break;
        }
        if (lstring::cieq(tok, mkw_trig)) {
            delete [] tok;
            int ret = ro_start.parse(&s, errstr, mkw_trig);
            if (ret != OK) {
                err = true;
                break;
            }
            continue;
        }
        if (lstring::cieq(tok, mkw_targ)) {
            delete [] tok;
            int ret = ro_end.parse(&s, errstr, mkw_targ);
            if (ret != OK) {
                err = true;
                break;
            }
            continue;
        }
        if (lstring::cieq(tok, mkw_from)) {
            delete [] tok;
            int ret = ro_start.parse(&s, errstr, mkw_from);
            if (ret != OK) {
                err = true;
                break;
            }
            continue;
        }
        if (lstring::cieq(tok, mkw_to)) {
            delete [] tok;
            int ret = ro_end.parse(&s, errstr, mkw_to);
            if (ret != OK) {
                err = true;
                break;
            }
            continue;
        }
        if (lstring::cieq(tok, kw_when) ||
                lstring::cieq(tok, kw_at) ||
                lstring::cieq(tok, kw_before) ||
                lstring::cieq(tok, kw_after)) {

            delete [] tok;
            s = last;
            int ret = ro_start.parse(&s, errstr, 0);
            if (ret != OK) {
                err = true;
                break;
            }
            continue;
        }
        if (lstring::cieq(tok, mkw_min)) {
            delete [] tok;
            addMeas(Mmin, gtok(&s));
            continue;
        }
        if (lstring::cieq(tok, mkw_max)) {
            delete [] tok;
            addMeas(Mmax, gtok(&s));
            continue;
        }
        if (lstring::cieq(tok, mkw_pp)) {
            delete [] tok;
            addMeas(Mpp, gtok(&s));
            continue;
        }
        if (lstring::cieq(tok, mkw_avg)) {
            delete [] tok;
            addMeas(Mavg, gtok(&s));
            continue;
        }
        if (lstring::cieq(tok, mkw_rms)) {
            delete [] tok;
            addMeas(Mrms, gtok(&s));
            continue;
        }
        if (lstring::cieq(tok, mkw_pw)) {
            delete [] tok;
            addMeas(Mpw, gtok(&s));
            continue;
        }
        if (lstring::cieq(tok, mkw_rt)) {
            delete [] tok;
            addMeas(Mrft, gtok(&s));
            continue;
        }
        if (lstring::cieq(tok, mkw_find)) {
            delete [] tok;
            addMeas(Mfind, gtok(&s));
            continue;
        }
        if (lstring::cieq(tok, mkw_param)) {
            delete [] tok;
            ro_prmexpr = gtok(&s);
            if (ro_prmexpr && *ro_prmexpr == '=') {
                delete [] ro_prmexpr;
                ro_prmexpr = gtok(&s);
            }
            if (!ro_prmexpr) {
                err = true;
                listerr(errstr, mkw_param);
                break;
            }
            continue;
        }
        if (lstring::cieq(tok, mkw_goal)) {
            delete [] tok;
            gval(&s, &err);
            if (err) {
                listerr(errstr, mkw_goal);
                break;
            }
            continue;
        }
        if (lstring::cieq(tok, mkw_minval)) {
            delete [] tok;
            gval(&s, &err);
            if (err) {
                listerr(errstr, mkw_minval);
                break;
            }
            continue;
        }
        if (lstring::cieq(tok, mkw_weight)) {
            delete [] tok;
            gval(&s, &err);
            if (err) {
                listerr(errstr, mkw_weight);
                break;
            }
            continue;
        }
        if (lstring::cieq(tok, mkw_print)) {
            delete [] tok;
            ro_print_flag = 2;
            continue;
        }
        if (lstring::cieq(tok, mkw_print_terse)) {
            ro_print_flag = 1;
            continue;
        }
        if (lstring::cieq(tok, kw_stop)) {
            delete [] tok;
            ro_stop_flag = true;
            continue;
        }
        if (lstring::cieq(tok, mkw_exec)) {
            delete [] tok;
            ro_exec = gtok(&s);
            continue;
        }
        if (lstring::cieq(tok, mkw_call)) {
            delete [] tok;
            ro_call = gtok(&s);
            continue;
        }

        // Something unknown, probably a bare number.  Consider an
        // implicit "at".
        delete [] tok;
        s = last;
        int ret = ro_start.parse(&s, errstr, 0);
        if (ret != OK) {
            err = true;
            break;
        }
    }
    if (err)
        ro_measure_skip = true;
    return (true);
}


// Reset the measurement.
//
void
sRunopMeas::reset(sPlot *pl)
{
    if (pl) {
        // If the result vector is found in pl, delete it after copying
        // to a history list in pl.
        sDataVec *v = pl->get_perm_vec(ro_result);
        if (v) {
            char buf[128];
            pl->remove_perm_vec(ro_result);
            sprintf(buf, "%s_scale", ro_result);
            sDataVec *vs = pl->get_perm_vec(buf);
            if (vs) {
                pl->remove_perm_vec(buf);
                sprintf(buf, "%s_hist", ro_result);
                sDataVec *vh = pl->get_perm_vec(buf);
                if (!vh) {
                    vh = new sDataVec(*v->units());
                    vh->set_name(buf);
                    vh->set_flags(0);
                    sPlot *px = OP.curPlot();
                    OP.setCurPlot(pl);
                    vh->newperm();
                    OP.setCurPlot(px);
                }
                sDataVec *vhs = vh->scale();
                if (!vhs) {
                    sprintf(buf, "%s_hist_scale", ro_result);
                    vhs = new sDataVec(*vs->units());
                    vhs->set_name(buf);
                    vhs->set_flags(0);
                    sPlot *px = OP.curPlot();
                    OP.setCurPlot(pl);
                    vhs->newperm();
                    OP.setCurPlot(px);
                    vh->set_scale(vhs);
                }

                int len = vh->length();
                int nlen = len + v->length();
                double *old = vh->realvec();
                vh->set_realvec(new double[nlen]);
                vh->set_length(nlen);
                vh->set_allocated(nlen);
                int i;
                for (i = 0; i < len; i++)
                    vh->set_realval(i, old[i]);
                for (i = len; i < nlen; i++)
                    vh->set_realval(i, v->realval(i-len));
                delete [] old;

                len = vhs->length();
                nlen = len + vs->length();
                old = vhs->realvec();
                vhs->set_realvec(new double[nlen]);
                vhs->set_length(nlen);
                vhs->set_allocated(nlen);
                for (i = 0; i < len; i++)
                    vhs->set_realval(i, old[i]);
                for (i = len; i < nlen; i++)
                    vhs->set_realval(i, vs->realval(i-len));
                delete [] old;
                delete vs;
            }
            delete v;
        }
    }

    ro_start.reset();
    ro_end.reset();

    ro_found_rises      = 0;
    ro_found_falls      = 0;
    ro_found_crosses    = 0;
    ro_measure_done     = false;
    ro_measure_error    = false;
    ro_measure_skip     = false;
    ro_stop_flag        = false;
    ro_end_flag         = false;
    ro_queue_measure    = false;
}


// Check if the measurement is triggered, and if so perform the
// measurement.  This is called periodically as the analysis
// progresses, or can be called after the analysis is complete.  True
// is returned if the measurement was performed.
//
bool
sRunopMeas::check_measure(sRunDesc *run)
{
    if (!ro_active || !run || !run->circuit())
        return (true);
    if (ro_measure_done || ro_measure_error || ro_measure_skip)
        return (true);

    sFtCirc *circuit = run->circuit();
    sRunopMeas *measures = circuit->measures();
    if (!measures)
       return (true);  // "can't happen"
    bool ready = true;
    if (ro_prmexpr) {
        for (sRunopMeas *m = measures; m; m = m->next()) {
            if (ro_analysis != m->ro_analysis)
                continue;
            if (m == this)
                continue;
            if (m->ro_prmexpr)
                continue;
            if (!m->ro_measure_done && !m->ro_measure_error)
                ready = false;
        }
        // All non-param measurements done.
    }
    else {
        if (!ro_start.check_found(circuit, &ro_measure_error, false))
            ready = false;
        if (!ro_end.check_found(circuit, &ro_measure_error, true))
            ready = false;
    }
    ro_cktptr = circuit;
    ro_queue_measure = ready;

    return (ready);
}


// Called with vectors segmentized.
//
bool
sRunopMeas::do_measure(sRunDesc *run)
{
//XXX
(void)run;
    if (!ro_queue_measure)
        return (false);
    ro_queue_measure = false;

    sDataVec *dv;
    int cnt;
    if (!measure(&dv, &cnt))
        return (false);
    if (!update_plot(dv, cnt))
        return (false);
    ro_measure_done = true;


    if (ro_print_flag && !Sp.GetFlag(FT_SERVERMODE)) {
        char *s = print_meas();
        TTY.printf_force("%s", s);
        delete [] s;
    }

    // exec command
    exec(ro_exec);

    // call function
    ROret ret = ::call(ro_call);
    if (ret == RO_PAUSE)
        ro_stop_flag = true;
    if (ret == RO_ENDIT)
        ro_end_flag = true;
    return (true);
}


namespace {
    // Return a data vector structure representing the expression
    // evaluation.
    //
    sDataVec *evaluate(const char *str)
    {
        if (!str)
            return (0);
        const char *s = str;
        pnode *pn = Sp.GetPnode(&s, true);
        if (!pn)
            return (0);
        sDataVec *dv = Sp.Evaluate(pn);
        delete pn;
        return (dv);
    }
}


// Do the measurement, call after successfully identifying the
// measure interval.
//
bool
sRunopMeas::measure(sDataVec **dvp, int *cntp)
{
    if (dvp)
        *dvp = 0;
    if (cntp)
        *cntp = 0;
    if (!ro_cktptr)
        return (false);
    sDataVec *xs = ro_cktptr->runplot()->scale();
    sDataVec *dv0 = 0;
    int count = 0;
    if (ro_start.ready() && ro_end.ready()) {
        for (sMfunc *ff = ro_funcs; ff; ff = ff->next(), count++) {
            sDataVec *dv = evaluate(ff->expr());
            if (dv && ((dv->length() > ro_start.indx() &&
                    dv->length() > ro_end.indx()) || dv->length() == 1)) {
                if (!dv0)
                    dv0 = dv;
                if (ff->type() == Mmin) {
                    if (dv->length() == 1)
                        ff->set_val(dv->realval(0));
                    else {
                        double mn = dv->realval(ro_start.indx());
                        for (int i = ro_start.indx()+1; i <= ro_end.indx();
                                i++) {
                            if (dv->realval(i) < mn)
                                mn = dv->realval(i);
                        }
                        double d;
                        d = startval(dv, xs);
                        if (d < mn)
                            mn = d;
                        d = endval(dv, xs);
                        if (d < mn)
                            mn = d;
                        ff->set_val(mn);
                    }
                }
                else if (ff->type() == Mmax) {
                    if (dv->length() == 1)
                        ff->set_val(dv->realval(0));
                    else {
                        double mx = dv->realval(ro_start.indx());
                        for (int i = ro_start.indx()+1; i <= ro_end.indx();
                                i++) {
                            if (dv->realval(i) > mx)
                                mx = dv->realval(i);
                        }
                        double d;
                        d = startval(dv, xs);
                        if (d > mx)
                            mx = d;
                        d = endval(dv, xs);
                        if (d > mx)
                            mx = d;
                        ff->set_val(mx);
                    }
                }
                else if (ff->type() == Mpp) {
                    if (dv->length() == 1)
                        ff->set_val(0.0);
                    else {
                        double mn = dv->realval(ro_start.indx());
                        double mx = mn;
                        for (int i = ro_start.indx()+1; i <= ro_end.indx();
                                i++) {
                            if (dv->realval(i) < mn)
                                mn = dv->realval(i);
                            else if (dv->realval(i) > mx)
                                mx = dv->realval(i);
                        }
                        double d;
                        d = startval(dv, xs);
                        if (d > mx)
                            mx = d;
                        if (d < mn)
                            mn = d;
                        d = endval(dv, xs);
                        if (d > mx)
                            mx = d;
                        if (d < mn)
                            mn = d;
                        ff->set_val(mx - mn);
                    }
                }
                else if (ff->type() == Mavg) {
                    if (dv->length() == 1)
                        ff->set_val(dv->realval(0));
                    else {
                        sDataVec *txs = ro_cktptr->runplot()->scale();
                        ff->set_val(findavg(dv, txs));
                    }
                }
                else if (ff->type() == Mrms) {
                    if (dv->length() == 1)
                        ff->set_val(dv->realval(0));
                    else {
                        sDataVec *txs = ro_cktptr->runplot()->scale();
                        ff->set_val(findrms(dv, txs));
                    }
                }
                else if (ff->type() == Mpw) {
                    if (dv->length() == 1)
                        ff->set_val(0);
                    else {
                        sDataVec *txs = ro_cktptr->runplot()->scale();
                        ff->set_val(findpw(dv, txs));
                    }
                }
                else if (ff->type() == Mrft) {
                    if (dv->length() == 1)
                        ff->set_val(0);
                    else {
                        sDataVec *txs = ro_cktptr->runplot()->scale();
                        ff->set_val(findrft(dv, txs));
                    }
                }
            }
            else {
                ff->set_error(true);
                ff->set_val(0);
            }
        }
        for (sMfunc *ff = ro_finds; ff; ff = ff->next(), count++) {
            sDataVec *dv = evaluate(ff->expr());
            if (dv) {
                ff->set_val(endval(dv, xs) - startval(dv, xs));
                if (!dv0)
                    dv0 = dv;
            }
            else {
                ff->set_error(true);
                ff->set_val(0.0);
            }
        }
    }
    else if (ro_start.ready()) {
        for (sMfunc *ff = ro_finds; ff; ff = ff->next(), count++) {
            sDataVec *dv = evaluate(ff->expr());
            if (dv) {
                ff->set_val(startval(dv, xs));
                if (!dv0)
                    dv0 = dv;
            }
            else {
                ff->set_error(true);
                ff->set_val(0.0);
            }
        }
    }
    else if (ro_prmexpr) {
        dv0 = evaluate(ro_prmexpr);
        if (!dv0) {
            ro_measure_error = true;
            return (false);
        }
    }
    else
        return (false);

    if (dvp)
        *dvp = dv0;
    if (cntp)
        *cntp = count;
    return (true);
}


// Add a vector containing the results to the plot.
//
bool
sRunopMeas::update_plot(sDataVec *dv0, int count)
{
    if (!ro_cktptr)
        return (false);
    if (ro_cktptr->runplot()) {

        // create the output vector and scale
        sPlot *pl = ro_cktptr->runplot();
        sDataVec *xs = pl->scale();
        sDataVec *nv = 0;
        if (ro_end.ready()) {
            // units test
            int uv = 0, us = 0;
            for (sMfunc *ff = ro_funcs; ff; ff = ff->next()) {
                if (ff->type() == Mpw || ff->type() == Mrft)
                    us++;
                else
                    uv++;
            }
            if (us) {
                if (!uv)
                    nv = new sDataVec(*xs->units());
                else
                    nv = new sDataVec();
            }
        }
        if (!nv) {
            if (dv0)
                nv = new sDataVec(*dv0->units());
            else {
                sUnits u;
                nv = new sDataVec(u);
            }
        }
        nv->set_name(ro_result);
        nv->set_flags(0);
        sPlot *px = OP.curPlot();
        OP.setCurPlot(pl);
        nv->newperm();
        OP.setCurPlot(px);
        char scname[128];
        sprintf(scname, "%s_scale", ro_result);
        sDataVec *ns = new sDataVec(*xs->units());
        ns->set_name(scname);
        ns->set_flags(0);
        px = OP.curPlot();
        OP.setCurPlot(pl);
        ns->newperm();
        OP.setCurPlot(px);
        nv->set_scale(ns);

        if (ro_prmexpr) {
            nv->set_length(1);
            nv->set_allocated(1);
            nv->set_realvec(new double[1]);
            if (dv0)
                nv->set_realval(0, dv0->realval(0));
            ns->set_length(1);
            ns->set_allocated(1);
            ns->set_realvec(new double[1]);
            ns->set_realval(0, xs ? xs->realval(xs->length()-1) : 0.0);
            if (!dv0) {
                // No measurement, the named result is the time value.
                nv->set_realval(0, ns->realval(0));
            }
        }
        else {
            if (count == 0) {
                count = 1;
                if (ro_end.ready())
                    count++;
                nv->set_realvec(new double[count]);
                // No measurement, the named result is the time value.
                nv->set_realval(0, ro_start.found());
                if (ro_end.ready())
                    nv->set_realval(1, ro_end.found());
            }
            else
                nv->set_realvec(new double[count]);
            nv->set_length(count);
            nv->set_allocated(count);
            count = 0;
            if (ro_end.ready()) {
                for (sMfunc *ff = ro_funcs; ff; ff = ff->next(), count++)
                    nv->set_realval(count, ff->val());
            }
            for (sMfunc *ff = ro_finds; ff; ff = ff->next(), count++)
                nv->set_realval(count, ff->val());
            count = 1;
            if (ro_end.ready())
                count++;
            ns->set_realvec(new double[count]);
            ns->set_length(count);
            ns->set_allocated(count);
            ns->set_realval(0, ro_start.found());
            if (ro_end.ready())
                ns->set_realval(1, ro_end.found());
        }
    }
    return (true);
}


// Return a string containing text of measurement results.
//
char *
sRunopMeas::print_meas()
{
    if (!ro_measure_done)
        return (0);
    sLstr lstr;
    char buf[BSIZE_SP];
    if (ro_print_flag > 1) {
        sprintf(buf, "measure: %s\n", ro_result);
        lstr.add(buf);
    }
    if (ro_start.ready()) {
        const char *ftype;
        if (ro_end.ready()) {
            ftype = "diff";
            if (ro_print_flag > 1) {
                if (ro_cktptr && ro_cktptr->runplot() &&
                        ro_cktptr->runplot()->scale()) {
                    const char *zz = ro_cktptr->runplot()->scale()->name();
                    if (zz && *zz) {
                        sprintf(buf, " %s\n", zz);
                        lstr.add(buf);
                    }
                }
                sprintf(buf, "    start: %-16g end: %-16g delta: %g\n",
                    ro_start.found(), ro_end.found(),
                    ro_end.found() - ro_start.found());
                lstr.add(buf);
            }
            for (sMfunc *ff = ro_funcs; ff; ff = ff->next()) {
                if (ro_print_flag > 1) {
                    sprintf(buf, " %s\n", ff->expr());
                    lstr.add(buf);
                }
                const char *str = "???";
                if (ff->type() == Mmin)
                    str = mkw_min;
                else if (ff->type() == Mmax)
                    str = mkw_max;
                else if (ff->type() == Mpp)
                    str = mkw_pp;
                else if (ff->type() == Mavg)
                    str = mkw_avg;
                else if (ff->type() == Mrms)
                    str = mkw_rms;
                else if (ff->type() == Mpw)
                    str = mkw_pw;
                else if (ff->type() == Mrft)
                    str = mkw_rt;
                if (ro_print_flag > 1) {
                    if (ff->error())
                        sprintf(buf, "    %s: (error occurred)\n", str);
                    else
                        sprintf(buf, "    %s: %g\n", str, ff->val());
                }
                else {
                    if (ff->error()) {
                        sprintf(buf, "%s %s: (error occurred)\n", ro_result,
                            str);
                    }
                    else
                        sprintf(buf, "%s %s: %g\n", ro_result, str, ff->val());
                }
                lstr.add(buf);
            }
        }
        else {
            ftype = "find";
            if (ro_print_flag > 1) {
                if (ro_cktptr && ro_cktptr->runplot() &&
                        ro_cktptr->runplot()->scale()) {
                    const char *zz = ro_cktptr->runplot()->scale()->name();
                    if (zz && *zz) {
                        sprintf(buf, " %s\n", zz);
                        lstr.add(buf);
                    }
                }
                sprintf(buf, "    start: %g\n", ro_start.found());
                lstr.add(buf);
            }
        }
        for (sMfunc *ff = ro_finds; ff; ff = ff->next()) {
            if (ro_print_flag > 1) {
                if (ff->error()) {
                    sprintf(buf, " %s %s = (error occurred)\n", ftype,
                        ff->expr());
                }
                else {
                    sprintf(buf, " %s %s = %g\n", ftype, ff->expr(),
                        ff->val());
                }
            }
            else {
                if (ff->error()) {
                    sprintf(buf, "%s %s %s = (error occurred)\n", ro_result,
                        ftype, ff->expr());
                }
                else {
                    sprintf(buf, "%s %s %s = %g\n", ro_result, ftype,
                        ff->expr(), ff->val());
                }
            }
            lstr.add(buf);
        }
    }
    char *str = lstr.string_trim();
    lstr.clear();
    return (str);
}


// Add a measurement to the appropriate list.  This is a private function
// called during the parse.
//
void
sRunopMeas::addMeas(Mfunc mtype, const char *expr)
{
    sMfunc *m = new sMfunc(mtype, expr);
    if (mtype == Mfind) {
        if (!ro_finds)
            ro_finds = m;
        else {
            sMfunc *mm = ro_finds;
            while (mm->next())
                mm = mm->next();
            mm->set_next(m);
        }
    }
    else {
        if (!ro_funcs)
            ro_funcs = m;
        else {
            sMfunc *mm = ro_funcs;
            while (mm->next())
                mm = mm->next();
            mm->set_next(m);
        }
    }
}


namespace {
    // Return the indexed value, or the last value if index is too big.
    //
    inline double value(sDataVec *dv, int i)
    {
        if (i < dv->length())
            return (dv->realval(i));
        return (dv->realval(dv->length() - 1));
    }
}


// Return the interpolated start value of dv.
//
double
sRunopMeas::startval(sDataVec *dv, sDataVec *xs)
{
    int i = ro_start.indx();
    if (ro_start.found() != xs->realval(i) && i > 0) {
        double y0 = value(dv, ro_start.indx());
        double y1 = value(dv, ro_start.indx() - 1);
        return (y0 + (y1 - y0)*(ro_start.found() - xs->realval(i))/
            (xs->realval(i-1) - xs->realval(i)));
    }
    return (value(dv, ro_start.indx()));
}


// Return the interpolated end value of dv.
//
double
sRunopMeas::endval(sDataVec *dv, sDataVec *xs)
{
    int i = ro_end.indx();
    if (ro_end.found() != xs->realval(i) && i+1 < dv->length()) {
        double y0 = value(dv, ro_end.indx());
        double y1 = value(dv, ro_end.indx() + 1);
        return (y0 + (y1 - y0)*(ro_end.found() - xs->realval(i))/
            (xs->realval(i+1) - xs->realval(i)));
    }
    return (value(dv, ro_end.indx()));
}


// Find the average of dv over the interval.  We use trapezoid
// integration and divide by the scale interval.
//
double
sRunopMeas::findavg(sDataVec *dv, sDataVec *xs)
{
    double sum = 0.0;
    double d, delt;
    // compute the sum over the scale intervals
    int i;
    for (i = ro_start.indx()+1; i <= ro_end.indx(); i++) {
        delt = xs->realval(i) - xs->realval(i-1);
        sum += 0.5*delt*(dv->realval(i-1) + dv->realval(i));
    }

    // ro_start.found() is ahead of the start index (between i-1 and i)
    delt = xs->realval(ro_start.indx()) - ro_start.found();
    if (delt != 0) {
        i = ro_start.indx();
        d = dv->realval(i) + (dv->realval(i-1) - dv->realval(i))*
            (ro_start.found() - xs->realval(ro_start.indx()))/
            (xs->realval(i-1) - xs->realval(i));
        sum += 0.5*delt*(d + dv->realval(i));
    }

    // ro_end.found() is ahead if the end index (between i-1 and i)
    delt = ro_end.found() - xs->realval(ro_end.indx());
    if (delt != 0.0) {
        i = ro_end.indx();
        d = dv->realval(i) + (dv->realval(i+1) - dv->realval(i))*
            (ro_end.found() - xs->realval(ro_end.indx()))/
            (xs->realval(i+1) - xs->realval(i));
        sum += 0.5*delt*(d + dv->realval(i));
    }

    double dt = ro_end.found() - ro_start.found();
    if (dt != 0.0)
        sum /= dt;
    return (sum);
}


// Find the rms of dv over the interval.  We use trapezoid
// integration of the magnitudes.
//
double
sRunopMeas::findrms(sDataVec *dv, sDataVec *xs)
{
    double sum = 0.0;
    double d, delt;
    // compute the sum over the scale intervals
    int i;
    for (i = ro_start.indx()+1; i <= ro_end.indx(); i++) {
        delt = xs->realval(i) - xs->realval(i-1);
        sum += 0.5*delt*(dv->realval(i-1)*dv->realval(i-1) +
            dv->realval(i)*dv->realval(i));
    }

    // ro_start.found() is ahead of the start index
    delt = xs->realval(ro_start.indx()) - ro_start.found();
    if (delt != 0.0) {
        i = ro_start.indx();
        d = dv->realval(i) + (dv->realval(i-1) - dv->realval(i))*
            (ro_start.found() - xs->realval(ro_start.indx()))/
            (xs->realval(i-1) - xs->realval(i));
        sum += 0.5*delt*(d*d + dv->realval(i)*dv->realval(i));
    }

    // ro_end.found() is behind the end index
    delt = ro_end.found() - xs->realval(ro_end.indx());
    if (delt != 0.0) {
        i = ro_end.indx();
        d = dv->realval(i) + (dv->realval(i+1) - dv->realval(i))*
            (ro_end.found() - xs->realval(ro_end.indx()))/
            (xs->realval(i+1) - xs->realval(i));
        sum += 0.5*delt*(d*d + dv->realval(i)*dv->realval(i));
    }

    double dt = ro_end.found() - ro_start.found();
    if (dt != 0.0)
        sum /= dt;
    return (sqrt(fabs(sum)));
}


// Find the fwhm of a pulse assumed to be contained in the interval.
double
sRunopMeas::findpw(sDataVec *dv, sDataVec *xs)
{
    // find the max/min
    double mx = dv->realval(ro_start.indx());
    double mn = mx;
    int imx = -1;
    int imn = -1;
    for (int i = ro_start.indx()+1; i <= ro_end.indx(); i++) {
        if (dv->realval(i) > mx) {
            mx = dv->realval(i);
            imx = i;
        }
        if (dv->realval(i) < mn) {
            mn = dv->realval(i);
            imn = i;
        }
    }
    double ds = startval(dv, xs);
    double de = endval(dv, xs);
    double mid;
    int imid;
    if (mx - SPMAX(ds, de) > SPMIN(ds, de) - mn) {
        mid = 0.5*(mx + SPMAX(ds, de));
        imid = imx;
    }
    else {
        mid = 0.5*(mn + SPMIN(ds, de));
        imid = imn;
    }

    int ibeg = -1, iend = -1;
    for (int i = ro_start.indx() + 1; i <= ro_end.indx(); i++) {
        if ((dv->realval(i-1) < mid && dv->realval(i) >= mid) ||
                (dv->realval(i-1) > mid && dv->realval(i) <= mid)) {
            if (ibeg >= 0)
                iend = i;
            else
                ibeg = i;
        }
        if (ibeg >=0 && iend >= 0)
            break;
    }
    if (ibeg >= 0 && iend >= 0 && ibeg < imid && iend > imid) {
        double x0 = xs->realval(ibeg-1);
        double x1 = xs->realval(ibeg);
        double y0 = dv->realval(ibeg-1);
        double y1 = dv->realval(ibeg);
        double tbeg = x0 + (x1 - x0)*(mid - y0)/(y1 - y0);
        x0 = xs->realval(iend-1);
        x1 = xs->realval(iend);
        y0 = dv->realval(iend-1);
        y1 = dv->realval(iend);
        double tend = x0 + (x1 - x0)*(mid - y0)/(y1 - y0);
        return(tend - tbeg);
    }
    return (0.0);
}


// Find the 10-90% rise or fall time of an edge contained in the
// interval.
//
double
sRunopMeas::findrft(sDataVec *dv, sDataVec *xs)
{
    double vstart = startval(dv, xs);
    double vend = endval(dv, xs);
    double th1 = vstart + 0.1*(vend - vstart);
    double th2 = vstart + 0.9*(vend - vstart);

    int ibeg = -1, iend = -1;
    for (int i = ro_start.indx() + 1; i <= ro_end.indx(); i++) {
        if ((dv->realval(i-1) < th1 && dv->realval(i) >= th1) ||
                (dv->realval(i-1) > th1 && dv->realval(i) <= th1) ||
                (dv->realval(i-1) < th2 && dv->realval(i) >= th2) ||
                (dv->realval(i-1) > th2 && dv->realval(i) <= th2)) {
            if (ibeg >= 0)
                iend = i;
            else
                ibeg = i;
        }
        if (ibeg >=0 && iend >= 0)
            break;
    }
    if (ibeg >= 0 && iend >= 0 && iend > ibeg) {
        double x0 = xs->realval(ibeg-1);
        double x1 = xs->realval(ibeg);
        double y0 = dv->realval(ibeg-1);
        double y1 = dv->realval(ibeg);
        double tbeg = x0 + (x1 - x0)*(th1 - y0)/(y1 - y0);
        x0 = xs->realval(iend-1);
        x1 = xs->realval(iend);
        y0 = dv->realval(iend-1);
        y1 = dv->realval(iend);
        double tend = x0 + (x1 - x0)*(th2 - y0)/(y1 - y0);
        return(tend - tbeg);
    }
    return (0.0);
}
// End of sRunopMeas functions.


void
sRunopStop::print(char **retstr)
{
    const char *msg1 = "%c %-4d ";
    if (!retstr) {
        TTY.printf(msg1, ro_active ? ' ' : 'I', ro_number);
        print_cond(0, true);
    }
    else {
        char buf[64];
        sprintf(buf, msg1, ro_active ? ' ' : 'I', ro_number);
        *retstr = lstring::build_str(*retstr, buf);
        print_cond(retstr, true);
    }
}


void
sRunopStop::destroy()
{
    delete this;
}


// Parse the string and set up the structure appropriately.
//
bool
sRunopStop::parse(const char *str, char **errstr)
{
    bool err = false;
    bool gotanal = false;
    ro_analysis = -1;
    const char *s = str;
    char *tok;
    for (;;) {
        const char *last = s;
        tok = gtok(&s);
        if (!tok)
            break;
        if (!gotanal) {
            if (get_anal(tok, &ro_analysis)) {
                gotanal = true;
                delete [] tok;
                continue;
            }
            err = true;
            listerr1(errstr, "missing analysis name.");
            break;
        }
        if (lstring::cieq(tok, kw_when) ||
                lstring::cieq(tok, kw_at) ||
                lstring::cieq(tok, kw_before) ||
                lstring::cieq(tok, kw_after)) {
            delete [] tok;
            s = last;
            int ret = ro_start.parse(&s, errstr, 0);
            if (ret != OK) {
                err = true;
                break;
            }
            continue;
        }
        if (lstring::cieq(tok, mkw_exec)) {
            delete [] tok;
            ro_exec = gtok(&s);;
            continue;
        }
        if (lstring::cieq(tok, mkw_call)) {
            delete [] tok;
            ro_call = gtok(&s);;
            continue;
        }
        if (lstring::cieq(tok, mkw_silent)) {
            delete [] tok;
            ro_silent = true;
            continue;
        }

        // Something unknown, probably a bare number.  Consider an
        // implicit "at".
        delete [] tok;
        s = last;
        int ret = ro_start.parse(&s, errstr, 0);
        if (ret != OK) {
            err = true;
            break;
        }
    }
    if (err)
        ro_stop_skip = true;
    return (true);
}


// Reset the measurement.
//
void
sRunopStop::reset()
{

    ro_start.reset();

    ro_found_rises      = 0;
    ro_found_falls      = 0;
    ro_found_crosses    = 0;
    ro_stop_done        = false;
    ro_stop_error       = false;
    ro_stop_flag        = false;
    ro_end_flag         = false;
}


ROret
sRunopStop::check_stop(sRunDesc *run)
{
    if (!ro_active || !run || !run->circuit())
        return (RO_OK);
    if (ro_stop_done || ro_stop_error || ro_stop_skip)
        return (RO_OK);

    sFtCirc *circuit = run->circuit();
    if (!ro_start.check_found(circuit, &ro_stop_error, false))
        return (RO_OK);

    // Execute command if any.
    exec(ro_exec);

    // Call the callback, if any.  This can override the stop.
    ROret ret = ::call(ro_call);

    ro_stop_done = true;

    if (ret == RO_OK)
        return (RO_PAUSE);
    if (ret == RO_PAUSE)
        return (RO_OK);
    return (RO_ENDIT);
}


// Print the condition.  If retstr is not 0, add the text to *retstr,
// otherwise print to standard output.  The status arg is true when
// printing for the status command.
//
void
sRunopStop::print_cond(char **retstr, bool status)
{
    (void)status;
    sLstr lstr;
    if (retstr) {
        lstr.add(*retstr);
        delete [] *retstr;
        *retstr = 0;
    }
    lstr.add(kw_stop);
    if (ro_analysis >= 0) {
        IFanalysis *a = IFanalysis::analysis(ro_analysis);
        if (a) {
            lstr.add_c(' ');
            lstr.add(a->name);
        }
    }
    ro_start.print(lstr);

    if (ro_exec) {
        lstr.add_c(' ');
        lstr.add(mkw_exec);
        lstr.add_c(' ');
        lstr.add(ro_exec);
    }
    if (ro_call) {
        lstr.add_c(' ');
        lstr.add(mkw_call);
        lstr.add_c(' ');
        lstr.add(ro_call);
    }
    if (status) {
        if (ro_silent) {
            lstr.add_c(' ');
            lstr.add(mkw_silent);
        }
    }
    lstr.add_c('\n');

    if (retstr)
        *retstr = lstr.string_trim();
    else
        TTY.send(lstr.string());
}

