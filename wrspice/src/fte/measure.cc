
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
#include "spnumber/hash.h"
#include "spnumber/spnumber.h"
#include "miscutil/lstring.h"


//
// Functions for the measurement post-processor:
//

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


    // Return comma or space separated token, with '=' considered as a
    // token.  If kw is true, terminate tokens at '(', splitting forms
    // like "param(val)".  Otherwise the parentheses will be included
    // in the token.  The former is appropriate when a keyword is
    // expected, the latter if an expression is expected.
    //
    char *gtok(const char **s, bool kw = false)
    {
        if (s == 0 || *s == 0)
            return (0);
        while (isspace(**s) || **s == ',')
            (*s)++;
        if (!**s)
            return (0);
        char buf[BSIZE_SP];
        char *t = buf;
        if (**s == '=')
            *t++ = *(*s)++;
        else {
            // Treat (....) as a single token.  Expressions can be
            // delimited this way to ensure that they are parsed
            // correctly.  Single quotes were originally used for this
            // purpose, but single quoted exporessions are now
            // evaluated at circuit load time.
            //
            if (**s == '(') {
                (*s)++;
                int np = 1;
                while (**s && np) {
                    if (**s == '(')
                        np++;
                    else if (**s == ')')
                        np--;
                    if (np)
                        *t++ = *(*s)++;
                    else
                        (*s)++;
                }
            }
            else {
                // Here's the problem:  Hspice can apparently handle
                // forms like
                //   ... trig vin'expression'
                // where there is no space between the vin and the
                // single quote.  We translate this to
                //   ... trig vin(expression)
                // implying that we need to stop parsing the trig at '('.
                // However, this breaks forms like
                //   ... trig v(vin) ...
                // The fix is to
                // 1. Add a space ahead of the leading '(' when we
                //    translate.
                // 2. Recognize and perform the following translations
                //    here:
                //    v(x) -> x
                //    i(x) -> x#branch

                const char *p = *s;
                if (kw && p[1] == '(') {
                    char ch = isupper(*p) ? tolower(*p) : *p;
                    if (ch == 'v' || ch == 'i') {
                        p += 2;
                        while (isspace(*p))
                            p++;
                        while (*p && !isspace(*p) && *p != ',' && *p != ')' &&
                                *p != '(') {
                            *t++ = *p++;
                        }
                        while (*p && *p != ')')
                            p++;
                        if (ch == 'i')
                            t = lstring::stpcpy(t, "#branch");
                        if (*p)
                            p++;
                    }
                }
                if (p != *s)
                    *s = p;
                else {
                    while (**s && !isspace(**s) && **s != '=' && **s != ',' &&
                            (!kw || **s != '(')) {
                        *t++ = *(*s)++;
                    }
                }
            }
        }
        *t = 0;
        while (isspace(**s) || **s == ',')
            (*s)++;
        return (lstring::copy(buf));
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
                sprintf(buf, ".measure syntax error after \'%s\'.", string);
                *errstr = lstring::copy(buf);
            }
            else
                *errstr = lstring::copy(".measure syntax error.");
        }
    }
}


void
sRunopMeas::print(char**)
{
//XXX store line, puke it back here
}


void
sRunopMeas::destroy()
{
    delete this;
}


// Parse the string and set up the structure appropriately.
//
bool
sRunopMeas::parse(const char *str, char **errstr)
{
    bool err = false;
    const char *s = str;
    char *tok = gtok(&s);
    delete [] tok;
    bool in_trig = false;  // beginning
    bool in_targ = false;  // ending
    bool in_when = false;
    bool gotanal = false;
    ro_analysis = -1;
    while ((tok = gtok(&s, true)) != 0) {
        if (!gotanal) {
            if (get_anal(tok, &ro_analysis)) {
                gotanal = true;
                delete [] tok;
                ro_result = gtok(&s);
                if (!ro_result) {
                    err = true;
                    listerr(errstr, 0);
                    break;
                }
                continue;
            }
        }
        if (lstring::cieq(tok, "trig")) {
            in_trig = true;
            in_targ = false;
            in_when = false;
            delete [] tok;
            tok = gtok(&s, true);
            if (!tok) {
                err = true;
                listerr(errstr, "trig");
                break;
            }
            if (lstring::cieq(tok, "at")) {
                delete [] tok;
                ro_start_at = gval(&s, &err);
                if (err) {
                    listerr(errstr, "at");
                    break;
                }
                ro_start_at_given = true;
                in_trig = false;
            }
            else if (lstring::cieq(tok, "when")) {
                delete [] tok;
                ro_start_when_expr1 = gtok(&s);
                if (!ro_start_when_expr1) {
                    err = true;
                    listerr(errstr, "when");
                    break;
                }
                tok = gtok(&s);
                if (!tok) {
                    err = true;
                    listerr(errstr, "when");
                    break;
                }
                if (*tok == '=') {
                    delete [] tok;
                    ro_start_when_expr2 = gtok(&s);
                }
                else
                    ro_start_when_expr2 = tok;
                if (!ro_start_when_expr2) {
                    err = true;
                    listerr(errstr, "when");
                    break;
                }
                ro_start_when_given = true;
            }
            else
                ro_start_name = tok;
            continue;
        }
        if (lstring::cieq(tok, "targ")) {
            in_targ = true;
            in_trig = false;
            in_when = false;
            delete [] tok;
            tok = gtok(&s, true);
            if (!tok) {
                err = true;
                listerr(errstr, "targ");
                break;
            }
            if (lstring::cieq(tok, "at")) {
                delete [] tok;
                ro_end_at = gval(&s, &err);
                if (err) {
                    listerr(errstr, "at");
                    break;
                }
                ro_end_at_given = true;
                in_trig = false;
            }
            else if (lstring::cieq(tok, "when")) {
                ro_end_when_expr1 = gtok(&s);
                if (!ro_end_when_expr1) {
                    err = true;
                    listerr(errstr, "when");
                    break;
                }
                tok = gtok(&s);
                if (!tok) {
                    err = true;
                    listerr(errstr, "when");
                    break;
                }
                if (*tok == '=') {
                    delete [] tok;
                    ro_end_when_expr2 = gtok(&s);
                }
                else
                    ro_end_when_expr2 = tok;
                if (!ro_end_when_expr2) {
                    err = true;
                    listerr(errstr, "when");
                    break;
                }
                ro_end_when_given = true;
            }
            else
                ro_end_name = tok;
            continue;
        }
        if (lstring::cieq(tok, "val")) {
            if (in_trig)
                ro_start_val = gval(&s, &err);
            else if (in_targ)
                ro_end_val = gval(&s, &err);
            else
                err = true;
            delete [] tok;
            if (err) {
                listerr(errstr, "val");
                break;
            }
            continue;
        }
        if (lstring::cieq(tok, "td")) {
            if (in_trig || in_when)
                ro_start_delay = gval(&s, &err);
            else if (in_targ)
                ro_end_delay = gval(&s, &err);
            else
                err = true;
            delete [] tok;
            if (err) {
                listerr(errstr, "td");
                break;
            }
            continue;
        }
        if (lstring::cieq(tok, "cross")) {
            if (in_trig || in_when)
                ro_start_crosses = (int)gval(&s, &err);
            else if (in_targ)
                ro_end_crosses = (int)gval(&s, &err);
            else
                err = true;
            delete [] tok;
            if (err) {
                listerr(errstr, "cross");
                break;
            }
            continue;
        }
        if (lstring::cieq(tok, "rise")) {
            if (in_trig || in_when)
                ro_start_rises = (int)gval(&s, &err);
            else if (in_targ)
                ro_end_rises = (int)gval(&s, &err);
            else
                err = true;
            delete [] tok;
            if (err) {
                listerr(errstr, "rise");
                break;
            }
            continue;
        }
        if (lstring::cieq(tok, "fall")) {
            if (in_trig || in_when)
                ro_start_falls = (int)gval(&s, &err);
            else if (in_targ)
                ro_end_falls = (int)gval(&s, &err);
            else
                err = true;
            delete [] tok;
            if (err) {
                listerr(errstr, "fall");
                break;
            }
            continue;
        }
        if (lstring::cieq(tok, "from")) {
            ro_start_at = gval(&s, &err);
            ro_start_at_given = true;
            delete [] tok;
            if (err) {
                ro_start_meas = gtok(&s);
                // should be the name of another measure
                if (!ro_start_meas) {
                    listerr(errstr, "from");
                    break;
                }
            }
            continue;
        }
        if (lstring::cieq(tok, "to")) {
            ro_end_at = gval(&s, &err);
            ro_end_at_given = true;
            delete [] tok;
            if (err) {
                ro_end_meas = gtok(&s);
                // should be the name of another measure
                if (!ro_end_meas) {
                    listerr(errstr, "to");
                    break;
                }
            }
            continue;
        }
        if (lstring::cieq(tok, "min")) {
            delete [] tok;
            addMeas(Mmin, gtok(&s));
            continue;
        }
        if (lstring::cieq(tok, "max")) {
            delete [] tok;
            addMeas(Mmax, gtok(&s));
            continue;
        }
        if (lstring::cieq(tok, "pp")) {
            delete [] tok;
            addMeas(Mpp, gtok(&s));
            continue;
        }
        if (lstring::cieq(tok, "avg")) {
            delete [] tok;
            addMeas(Mavg, gtok(&s));
            continue;
        }
        if (lstring::cieq(tok, "rms")) {
            delete [] tok;
            addMeas(Mrms, gtok(&s));
            continue;
        }
        if (lstring::cieq(tok, "pw")) {
            delete [] tok;
            addMeas(Mpw, gtok(&s));
            continue;
        }
        if (lstring::cieq(tok, "rt")) {
            delete [] tok;
            addMeas(Mrft, gtok(&s));
            continue;
        }
        if (lstring::cieq(tok, "find")) {
            delete [] tok;
            addMeas(Mfind, gtok(&s));
            continue;
        }
        if (lstring::cieq(tok, "at")) {
            delete [] tok;
            ro_start_at = gval(&s, &err);
            if (err) {
                listerr(errstr, "at");
                break;
            }
            ro_start_at_given = true;
            in_trig = false;
            continue;
        }
        if (lstring::cieq(tok, "when")) {
            in_when = true;
            in_trig = false;
            in_targ = false;
            delete [] tok;
            ro_start_when_expr1 = gtok(&s);
            if (!ro_start_when_expr1) {
                err = true;
                listerr(errstr, "when");
                break;
            }
            tok = gtok(&s);
            if (!tok) {
                err = true;
                listerr(errstr, "when");
                break;
            }
            if (*tok == '=') {
                delete [] tok;
                ro_start_when_expr2 = gtok(&s);
            }
            else
                ro_start_when_expr2 = tok;
            if (!ro_start_when_expr2) {
                err = true;
                listerr(errstr, "when");
                break;
            }
            ro_when_given = true;
            continue;
        }
        if (lstring::cieq(tok, "param")) {
            delete [] tok;
            ro_expr2 = gtok(&s);
            if (ro_expr2 && *ro_expr2 == '=') {
                delete [] ro_expr2;
                ro_expr2 = gtok(&s);
            }
            if (!ro_expr2) {
                err = true;
                listerr(errstr, "param");
                break;
            }
            continue;
        }
        if (lstring::cieq(tok, "goal")) {
            gval(&s, &err);
            delete [] tok;
            if (err) {
                listerr(errstr, "goal");
                break;
            }
            continue;
        }
        if (lstring::cieq(tok, "minval")) {
            gval(&s, &err);
            delete [] tok;
            if (err) {
                listerr(errstr, "minval");
                break;
            }
            continue;
        }
        if (lstring::cieq(tok, "weight")) {
            gval(&s, &err);
            delete [] tok;
            if (err) {
                listerr(errstr, "weight");
                break;
            }
            continue;
        }
        if (lstring::cieq(tok, "print")) {
            ro_print_flag = 2;
            continue;
        }
        if (lstring::cieq(tok, "print_terse")) {
            ro_print_flag = 1;
            continue;
        }
        if (lstring::cieq(tok, "stop")) {
            ro_stop_flag = true;
            continue;
        }
        const char *t = tok;
        double *dd = SPnum.parse(&t, false);
        if (dd) {
            // the "val=" is optional in these cases
            if (in_trig || in_when) {
                ro_start_val = *dd;
                delete [] tok;
                continue;
            }
            if (in_targ) {
                ro_end_val = *dd;
                delete [] tok;
                continue;
            }
        }
        if (errstr && !*errstr) {
            char buf[128];
            sprintf(buf, ".measure syntax error, unknown token \'%s\'.", tok);
            *errstr = lstring::copy(buf);
            err = true;
            delete [] tok;
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
        // if the result vector is found in pl, delete it after copying
        // to a history list in pl
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

    ro_start_dv         = 0;
    ro_end_dv           = 0;
    ro_start_indx       = 0;
    ro_end_indx         = 0;
    ro_found_rises      = 0;
    ro_found_falls      = 0;
    ro_found_crosses    = 0;
    ro_found_start      = 0.0;
    ro_found_end        = 0.0;
    ro_start_delay      = 0.0;
    ro_end_delay        = 0.0;
    ro_found_start_flag = 0;
    ro_found_end_flag   = 0;
    ro_measure_done     = false;
    ro_measure_error    = false;
}


namespace {
    // Return a non-negative index if the scale covers the trigger point
    // of val.  Do some fudging to avoid not triggering at the expected
    // time point due to numerical error.
    //
    int chk_trig(double val, sDataVec *xs)
    {
        int len = xs->length();
        if (len > 1) {
            if (xs->realval(1) > xs->realval(0)) {
                // Assume monotonically increasing.
                if (xs->realval(0) >= val)
                    return (0);
                for (int i = 1; i < len; i++) {
                    double dx = (xs->realval(i) - xs->realval(i-1)) * 1e-3;
                    if (xs->realval(i) >= val - dx) {
                        if (fabs(xs->realval(i-1) - val) <
                                fabs(xs->realval(i) - val))
                            i--;
                        return (i);
                    }
                }
            }
            else {
                // Assume monotonically decreasing.
                if (xs->realval(0) <= val)
                    return (0);
                for (int i = 1; i < len; i++) {
                    double dx = (xs->realval(i) - xs->realval(i-1)) * 1e-3;
                    if (xs->realval(i) <= val - dx) {
                        if (fabs(xs->realval(i-1) - val) <
                                fabs(xs->realval(i) - val))
                            i--;
                        return (i);
                    }
                }
            }
        }
        else if (len == 1) {
            if (xs->realval(0) == val)
                return (0);
        }
        return (-1);
    }
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


    // Return the indexed value, or the last value if index is too big.
    //
    inline double value(sDataVec *dv, int i)
    {
        if (i < dv->length())
            return (dv->realval(i));
        return (dv->realval(dv->length() - 1));
    }
}


// Check if the measurement is triggered, and if so perform the measurement.
// This is called periodically as the analysis progresses, or can be called
// after the analysis is complete.  True is returned if the measurement
// was performed.
//
bool
sRunopMeas::check(sFtCirc *circuit)
{
    if (!circuit)
        return (true);
    if (ro_measure_done || ro_measure_error || ro_measure_skip)
        return (true);
    sDataVec *xs = circuit->runplot()->scale();
    sRunopMeas *measures = circuit->measures();
    if (!measures)
       return (true);  // "can't happen"
    if (ro_expr2) {
        for (sRunopMeas *m = measures; m; m = m->next()) {
            if (ro_analysis != m->ro_analysis)
                continue;
            if (m == this)
                continue;
            if (m->ro_expr2)
                continue;
            if (!m->ro_measure_done && !m->ro_measure_error)
                return (false);
        }
        // All non-param measurements done.
    }
    else if (ro_when_given) {
        // 'when' was given, measure at a single point
        if (!ro_found_start_flag) {
            double start = ro_start_delay;
            int i = chk_trig(start, xs);
            if (i < 0)
                return (false);

            sDataVec *dvl = evaluate(ro_start_when_expr1);
            sDataVec *dvr = evaluate(ro_start_when_expr2);
            if (!dvl || !dvr) {
                ro_measure_error = true;
                return (false);
            }
            int r = 0, f = 0, c = 0;
            bool found = false;
            for ( ; i < dvl->length(); i++) {
                if (i && value(dvl, i-1) <= value(dvr, i-1) &&
                        value(dvr, i) < value(dvl, i)) {
                    r++;
                    c++;
                }
                else if (i && value(dvl, i-1) > value(dvr, i-1) &&
                        value(dvr, i) >= value(dvl, i)) {
                    f++;
                    c++;
                }
                else
                    continue;
                if (r >= ro_start_rises && f >= ro_start_falls &&
                        c >= ro_start_crosses) {
                    double d = value(dvr, i) - value(dvr, i-1) -
                        (value(dvl, i) - value(dvl, i-1));
                    if (d != 0.0) {
                        start = xs->realval(i-1) +
                            (xs->realval(i) - xs->realval(i-1))*
                            (value(dvl, i-1) - value(dvr, i-1))/d;
                    }
                    else
                        start = xs->realval(i);
                    found = true;
                    ro_start_indx = i;
                    break;
                }
            }
            if (!found)
                return (false);
            ro_found_start = start;
            ro_found_start_flag = true;
        }
    }
    else {
        // 'trig' and maybe 'targ' were given, measure over an interval
        // if targ given
        if (ro_start_name && !ro_start_dv) {
            sRunopMeas *m = sRunopMeas::find(measures, ro_start_name);
            if (m) {
                if (!m->ro_measure_done)
                    return (false);
                if (m->ro_found_end_flag)
                    ro_start_delay += m->ro_found_end;
                else
                    ro_start_delay += m->ro_found_start;
                ro_start_dv = circuit->runplot()->scale();
            }
            else {
                ro_start_dv = circuit->runplot()->find_vec(ro_start_name);
                if (!ro_start_dv) {
                    ro_measure_error = true;
                    return (false);
                }
            }
        }
        if (ro_start_at_given && !ro_start_dv) {
            if (ro_start_meas) {
                sRunopMeas *m = sRunopMeas::find(measures, ro_start_meas);
                if (!m) {
                    ro_measure_error = true;
                    return (false);
                }
                if (!m->ro_measure_done)
                    return (false);
                if (m->ro_found_end_flag)
                    ro_start_at = m->ro_found_end;
                else
                    ro_start_at = m->ro_found_start;
            }
            ro_start_dv = circuit->runplot()->scale();
        }
        if (ro_end_name && !ro_end_dv) {
            sRunopMeas *m = sRunopMeas::find(measures, ro_end_name);
            if (m) {
                if (!m->ro_measure_done)
                    return (false);
                if (m->ro_found_end_flag)
                    ro_end_delay += m->ro_found_end;
                else
                    ro_end_delay += m->ro_found_start;
                ro_end_dv = circuit->runplot()->scale();
            }
            else {
                ro_end_dv = circuit->runplot()->find_vec(ro_end_name);
                if (!ro_end_dv) {
                    ro_measure_error = true;
                    return (false);
                }
            }
        }
        if (ro_end_at_given && !ro_end_dv) {
            if (ro_end_meas) {
                sRunopMeas *m = sRunopMeas::find(measures, ro_end_meas);
                if (!m) {
                    ro_measure_error = true;
                    return (false);
                }
                if (!m->ro_measure_done)
                    return (false);
                if (m->ro_found_end_flag)
                    ro_end_at = m->ro_found_end;
                else
                    ro_end_at = m->ro_found_start;
            }
            ro_end_dv = circuit->runplot()->scale();
        }

        if (!ro_found_start_flag && ro_start_dv) {
            double start;
            if (ro_start_at_given)
                start = ro_start_at;
            else
                start = ro_start_delay;
            int i = chk_trig(start, xs);
            if (i < 0)
                return (false);
            if (ro_start_at_given)
                start = xs->realval(i);
            else if (ro_start_when_given) {
                sDataVec *dvl = evaluate(ro_start_when_expr1);
                sDataVec *dvr = evaluate(ro_start_when_expr2);
                if (!dvl || !dvr) {
                    ro_measure_error = true;
                    return (false);
                }
                int r = 0, f = 0, c = 0;
                bool found = false;
                for ( ; i < dvl->length(); i++) {
                    if (i && value(dvl, i-1) <= value(dvr, i-1) &&
                            value(dvr, i) < value(dvl, i)) {
                        r++;
                        c++;
                    }
                    else if (i && value(dvl, i-1) > value(dvr, i-1) &&
                            value(dvr, i) >= value(dvl, i)) {
                        f++;
                        c++;
                    }
                    else
                        continue;
                    if (r >= ro_start_rises && f >= ro_start_falls &&
                            c >= ro_start_crosses) {
                        double d = value(dvr, i) - value(dvr, i-1) -
                            (value(dvl, i) - value(dvl, i-1));
                        if (d != 0.0) {
                            start = xs->realval(i-1) +
                                (xs->realval(i) - xs->realval(i-1))*
                                (value(dvl, i-1) - value(dvr, i-1))/d;
                        }
                        else
                            start = xs->realval(i);
                        found = true;
                        break;
                    }
                }
                if (!found)
                    return (false);
            }
            else if (ro_start_rises > 0 || ro_start_falls > 0 || ro_start_crosses > 0) {
                int r = 0, f = 0, c = 0;
                bool found = false;
                for ( ; i < ro_start_dv->length(); i++) {
                    if (i && ro_start_dv->realval(i-1) <= ro_start_val &&
                            ro_start_val < ro_start_dv->realval(i)) {
                        r++;
                        c++;
                    }
                    else if (i && ro_start_dv->realval(i-1) > ro_start_val &&
                            ro_start_val >= ro_start_dv->realval(i)) {
                        f++;
                        c++;
                    }
                    else
                        continue;
                    if (r >= ro_start_rises && f >= ro_start_falls &&
                            c >= ro_start_crosses) {
                        start = xs->realval(i-1) +
                            (xs->realval(i) - xs->realval(i-1))*
                            (ro_start_val - ro_start_dv->realval(i-1))/
                            (ro_start_dv->realval(i) - ro_start_dv->realval(i-1));
                        found = true;
                        break;
                    }
                }
                if (!found)
                    return (false);
            }
            else if (ro_start_dv != circuit->runplot()->scale())
                return (false);
            ro_found_start = start;
            ro_start_indx = i;
            ro_found_start_flag = true;
        }

        if (!ro_found_end_flag && ro_end_dv) {
            double end;
            if (ro_end_at_given)
                end = ro_end_at;
            else
                end = ro_end_delay;
            int i = chk_trig(end, xs);
            if (i < 0)
                return (false);
            if (ro_end_at_given)
                end = xs->realval(i);
            else if (ro_end_when_given) {
                sDataVec *dvl = evaluate(ro_end_when_expr1);
                sDataVec *dvr = evaluate(ro_end_when_expr2);
                if (!dvl || !dvr) {
                    ro_measure_error = true;
                    return (false);
                }
                int r = 0, f = 0, c = 0;
                bool found = false;
                for ( ; i < dvl->length(); i++) {
                    if (i && value(dvl, i-1) <= value(dvr, i-1) &&
                            value(dvr, i) < value(dvl, i)) {
                        r++;
                        c++;
                    }
                    else if (i && value(dvl, i-1) > value(dvr, i-1) &&
                            value(dvr, i) >= value(dvl, i)) {
                        f++;
                        c++;
                    }
                    else
                        continue;
                    if (r >= ro_end_rises && f >= ro_end_falls &&
                            c >= ro_end_crosses) {
                        double d = value(dvr, i) - value(dvr, i-1) -
                            (value(dvl, i) - value(dvl, i-1));
                        if (d != 0.0) {
                            end = xs->realval(i-1) +
                                (xs->realval(i) - xs->realval(i-1))*
                                (value(dvl, i-1) - value(dvr, i-1))/d;
                        }
                        else
                            end = xs->realval(i);
                        found = true;
                        i--;
                        break;
                    }
                }
                if (!found)
                    return (false);
            }
            else if (ro_end_rises > 0 || ro_end_falls > 0 || ro_end_crosses > 0) {
                int r = 0, f = 0, c = 0;
                bool found = false;
                for ( ; i < ro_end_dv->length(); i++) {
                    if (i && ro_end_dv->realval(i-1) <= ro_end_val &&
                            ro_end_val < ro_end_dv->realval(i)) {
                        r++;
                        c++;
                    }
                    else if (i && ro_end_dv->realval(i-1) > ro_end_val &&
                            ro_end_val >= ro_end_dv->realval(i)) {
                        f++;
                        c++;
                    }
                    else
                        continue;
                    if (r >= ro_end_rises && f >= ro_end_falls &&
                            c >= ro_end_crosses) {
                        end = xs->realval(i-1) +
                            (xs->realval(i) - xs->realval(i-1))*
                            (ro_end_val - ro_end_dv->realval(i-1))/
                            (ro_end_dv->realval(i) - ro_end_dv->realval(i-1));
                        found = true;
                        i--;
                        break;
                    }
                }
                if (!found)
                    return (false);
            }
            else if (ro_end_dv != circuit->runplot()->scale())
                return (false);
            ro_found_end = end;
            ro_end_indx = i;
            ro_found_end_flag = true;
        }
    }
    ro_cktptr = circuit;

    //
    // Successfully identified measurement interval, do measurement.
    // Add a vector containing the results to the plot.
    //
    sDataVec *dv0 = 0;
    int count = 0;
    if (ro_found_start_flag && ro_found_end_flag) {
        for (sMfunc *ff = ro_funcs; ff; ff = ff->next, count++) {
            sDataVec *dv = evaluate(ff->expr);
            if (dv && 
                    ((dv->length() > ro_start_indx && dv->length() > ro_end_indx) ||
                    dv->length() == 1)) {
                if (!dv0)
                    dv0 = dv;
                if (ff->type == Mmin) {
                    if (dv->length() == 1)
                        ff->val = dv->realval(0);
                    else {
                        double mn = dv->realval(ro_start_indx);
                        for (int i = ro_start_indx+1; i <= ro_end_indx; i++) {
                            if (dv->realval(i) < mn)
                                mn = dv->realval(i);
                        }
                        double d;
                        d = endval(dv, xs, false);
                        if (d < mn)
                            mn = d;
                        d = endval(dv, xs, true);
                        if (d < mn)
                            mn = d;
                        ff->val = mn;
                    }
                }
                else if (ff->type == Mmax) {
                    if (dv->length() == 1)
                        ff->val = dv->realval(0);
                    else {
                        double mx = dv->realval(ro_start_indx);
                        for (int i = ro_start_indx+1; i <= ro_end_indx; i++) {
                            if (dv->realval(i) > mx)
                                mx = dv->realval(i);
                        }
                        double d;
                        d = endval(dv, xs, false);
                        if (d > mx)
                            mx = d;
                        d = endval(dv, xs, true);
                        if (d > mx)
                            mx = d;
                        ff->val = mx;
                    }
                }
                else if (ff->type == Mpp) {
                    if (dv->length() == 1)
                        ff->val = 0.0;
                    else {
                        double mn = dv->realval(ro_start_indx);
                        double mx = mn;
                        for (int i = ro_start_indx+1; i <= ro_end_indx; i++) {
                            if (dv->realval(i) < mn)
                                mn = dv->realval(i);
                            else if (dv->realval(i) > mx)
                                mx = dv->realval(i);
                        }
                        double d;
                        d = endval(dv, xs, false);
                        if (d > mx)
                            mx = d;
                        if (d < mn)
                            mn = d;
                        d = endval(dv, xs, true);
                        if (d > mx)
                            mx = d;
                        if (d < mn)
                            mn = d;
                        ff->val = mx - mn;
                    }
                }
                else if (ff->type == Mavg) {
                    if (dv->length() == 1)
                        ff->val = dv->realval(0);
                    else {
                        sDataVec *txs = circuit->runplot()->scale();
                        ff->val = findavg(dv, txs);
                    }
                }
                else if (ff->type == Mrms) {
                    if (dv->length() == 1)
                        ff->val = dv->realval(0);
                    else {
                        sDataVec *txs = circuit->runplot()->scale();
                        ff->val = findrms(dv, txs);
                    }
                }
                else if (ff->type == Mpw) {
                    if (dv->length() == 1)
                        ff->val = 0;
                    else {
                        sDataVec *txs = circuit->runplot()->scale();
                        ff->val = findpw(dv, txs);
                    }
                }
                else if (ff->type == Mrft) {
                    if (dv->length() == 1)
                        ff->val = 0;
                    else {
                        sDataVec *txs = circuit->runplot()->scale();
                        ff->val = findrft(dv, txs);
                    }
                }
            }
            else {
                ff->error = true;
                ff->val = 0;
            }
        }
    }
    else if (ro_found_start_flag) {
        for (sMfunc *ff = ro_finds; ff; ff = ff->next, count++) {
            sDataVec *dv = evaluate(ff->expr);
            if (dv) {
                ff->val = endval(dv, xs, false);
                if (!dv0)
                    dv0 = dv;
            }
            else {
                ff->error = true;
                ff->val = 0.0;
            }
        }
    }
    else if (ro_expr2) {
        dv0 = evaluate(ro_expr2);
        if (!dv0) {
            ro_measure_error = true;
            return (false);
        }
    }
    else
        return (false);

    if (circuit->runplot()) {

        // create the output vector and scale
        sPlot *pl = circuit->runplot();
        sDataVec *nv = 0;
        if (ro_found_end_flag) {
            // units test
            int uv = 0, us = 0;
            for (sMfunc *ff = ro_funcs; ff; ff = ff->next) {
                if (ff->type == Mpw || ff->type == Mrft)
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

        if (ro_expr2) {
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
                if (ro_found_end_flag)
                    count++;
                nv->set_realvec(new double[count]);
                // No measurement, the named result is the time value.
                nv->set_realval(0, ro_found_start);
                if (ro_found_end_flag)
                    nv->set_realval(1, ro_found_end);
            }
            else
                nv->set_realvec(new double[count]);
            nv->set_length(count);
            nv->set_allocated(count);
            count = 0;
            if (ro_found_end_flag) {
                for (sMfunc *ff = ro_funcs; ff; ff = ff->next, count++)
                    nv->set_realval(count, ff->val);
            }
            else {
                for (sMfunc *ff = ro_finds; ff; ff = ff->next, count++)
                    nv->set_realval(count, ff->val);
            }
            count = 1;
            if (ro_found_end_flag)
                count++;
            ns->set_realvec(new double[count]);
            ns->set_length(count);
            ns->set_allocated(count);
            ns->set_realval(0, ro_found_start);
            if (ro_found_end_flag)
                ns->set_realval(1, ro_found_end);
        }
    }

    ro_measure_done = true;
    if (ro_print_flag && !Sp.GetFlag(FT_SERVERMODE)) {
        char *s = print();
        TTY.printf_force("%s", s);
        delete [] s;
    }
    return (true);
}


// Return a string containing text of measurement result.
//
char *
sRunopMeas::print()
{
    if (!ro_measure_done)
        return (0);
    sLstr lstr;
    char buf[BSIZE_SP];
    if (ro_print_flag > 1) {
        sprintf(buf, "measure: %s\n", ro_result);
        lstr.add(buf);
    }
    if (ro_found_start_flag && ro_found_end_flag) {
        if (ro_print_flag > 1) {
            if (ro_cktptr && ro_cktptr->runplot() && ro_cktptr->runplot()->scale()) {
                const char *zz = ro_cktptr->runplot()->scale()->name();
                if (zz && *zz) {
                    sprintf(buf, " %s\n", zz);
                    lstr.add(buf);
                }
            }
            sprintf(buf, "    start: %-16g end: %-16g delta: %g\n",
                ro_found_start, ro_found_end, ro_found_end - ro_found_start);
            lstr.add(buf);
        }
        for (sMfunc *ff = ro_funcs; ff; ff = ff->next) {
            if (ro_print_flag > 1) {
                sprintf(buf, " %s\n", ff->expr);
                lstr.add(buf);
            }
            const char *str = "???";
            if (ff->type == Mmin)
                str = "min";
            else if (ff->type == Mmax)
                str = "max";
            else if (ff->type == Mpp)
                str = "p-p";
            else if (ff->type == Mavg)
                str = "avg";
            else if (ff->type == Mrms)
                str = "rms";
            else if (ff->type == Mpw)
                str = "pw";
            else if (ff->type == Mrft)
                str = "rt";
            if (ro_print_flag > 1) {
                if (ff->error)
                    sprintf(buf, "    %s: (error occurred)\n", str);
                else
                    sprintf(buf, "    %s: %g\n", str, ff->val);
            }
            else {
                if (ff->error)
                    sprintf(buf, "%s %s: (error occurred)\n", ro_result, str);
                else
                    sprintf(buf, "%s %s: %g\n", ro_result, str, ff->val);
            }
            lstr.add(buf);
        }
    }
    else if (ro_found_start_flag) {
        if (ro_print_flag > 1) {
            if (ro_cktptr && ro_cktptr->runplot() && ro_cktptr->runplot()->scale()) {
                const char *zz = ro_cktptr->runplot()->scale()->name();
                if (zz && *zz) {
                    sprintf(buf, " %s\n", zz);
                    lstr.add(buf);
                }
            }
            sprintf(buf, "    start: %g\n", ro_found_start);
            lstr.add(buf);
        }
        for (sMfunc *ff = ro_finds; ff; ff = ff->next) {
            if (ro_print_flag > 1) {
                if (ff->error)
                    sprintf(buf, " %s = (error occurred)\n", ff->expr);
                else
                    sprintf(buf, " %s = %g\n", ff->expr, ff->val);
            }
            else {
                if (ff->error)
                    sprintf(buf, "%s %s = (error occurred)\n", ro_result,
                        ff->expr);
                else
                    sprintf(buf, "%s %s = %g\n", ro_result, ff->expr, ff->val);
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
            while (mm->next)
                mm = mm->next;
            mm->next = m;
        }
    }
    else {
        if (!ro_funcs)
            ro_funcs = m;
        else {
            sMfunc *mm = ro_funcs;
            while (mm->next)
                mm = mm->next;
            mm->next = m;
        }
    }
}


// Return the interpolated end values of dv.  If end is false, look at
// the start side, where the interval starts ahead of the index point.
// Otherwise, the value is after the index point.
//
double
sRunopMeas::endval(sDataVec *dv, sDataVec *xs, bool end)
{
    if (!end) {
        int i = ro_start_indx;
        if (ro_found_start != xs->realval(i) && i > 0) {
            double y0 = value(dv, ro_start_indx);
            double y1 = value(dv, ro_start_indx - 1);
            return (y0 + (y1 - y0)*(ro_found_start - xs->realval(i))/
                (xs->realval(i-1) - xs->realval(i)));
        }
        else
            return (value(dv, ro_start_indx));
    }
    else {
        int i = ro_end_indx;
        if (ro_found_end != xs->realval(i) && i+1 < dv->length()) {
            double y0 = value(dv, ro_end_indx);
            double y1 = value(dv, ro_end_indx + 1);
            return (y0 + (y1 - y0)*(ro_found_end - xs->realval(i))/
                (xs->realval(i+1) - xs->realval(i)));
        }
        else
            return (value(dv, ro_end_indx));
    }
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
    for (i = ro_start_indx+1; i <= ro_end_indx; i++) {
        delt = xs->realval(i) - xs->realval(i-1);
        sum += 0.5*delt*(dv->realval(i-1) + dv->realval(i));
    }

    // ro_found_start is ahead of the start index (between i-1 and i)
    delt = xs->realval(ro_start_indx) - ro_found_start;
    if (delt != 0) {
        i = ro_start_indx;
        d = dv->realval(i) + (dv->realval(i-1) - dv->realval(i))*
            (ro_found_start - xs->realval(ro_start_indx))/
            (xs->realval(i-1) - xs->realval(i));
        sum += 0.5*delt*(d + dv->realval(i));
    }

    // ro_found_end is ahead if the end index (between i-1 and i)
    delt = ro_found_end - xs->realval(ro_end_indx);
    if (delt != 0.0) {
        i = ro_end_indx;
        d = dv->realval(i) + (dv->realval(i+1) - dv->realval(i))*
            (ro_found_end - xs->realval(ro_end_indx))/
            (xs->realval(i+1) - xs->realval(i));
        sum += 0.5*delt*(d + dv->realval(i));
    }

    double dt = ro_found_end - ro_found_start;
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
    for (i = ro_start_indx+1; i <= ro_end_indx; i++) {
        delt = xs->realval(i) - xs->realval(i-1);
        sum += 0.5*delt*(dv->realval(i-1)*dv->realval(i-1) +
            dv->realval(i)*dv->realval(i));
    }

    // ro_found_start is ahead of the start index
    delt = xs->realval(ro_start_indx) - ro_found_start;
    if (delt != 0.0) {
        i = ro_start_indx;
        d = dv->realval(i) + (dv->realval(i-1) - dv->realval(i))*
            (ro_found_start - xs->realval(ro_start_indx))/
            (xs->realval(i-1) - xs->realval(i));
        sum += 0.5*delt*(d*d + dv->realval(i)*dv->realval(i));
    }

    // ro_found_end is behind the end index
    delt = ro_found_end - xs->realval(ro_end_indx);
    if (delt != 0.0) {
        i = ro_end_indx;
        d = dv->realval(i) + (dv->realval(i+1) - dv->realval(i))*
            (ro_found_end - xs->realval(ro_end_indx))/
            (xs->realval(i+1) - xs->realval(i));
        sum += 0.5*delt*(d*d + dv->realval(i)*dv->realval(i));
    }

    double dt = ro_found_end - ro_found_start;
    if (dt != 0.0)
        sum /= dt;
    return (sqrt(fabs(sum)));
}


// Find the fwhm of a pulse assumed to be contained in the interval.
//
double
sRunopMeas::findpw(sDataVec *dv, sDataVec *xs)
{
    // find the max/min
    double mx = dv->realval(ro_start_indx);
    double mn = mx;
    int imx = -1;
    int imn = -1;
    for (int i = ro_start_indx+1; i <= ro_end_indx; i++) {
        if (dv->realval(i) > mx) {
            mx = dv->realval(i);
            imx = i;
        }
        if (dv->realval(i) < mn) {
            mn = dv->realval(i);
            imn = i;
        }
    }
    double ds = endval(dv, xs, false);
    double de = endval(dv, xs, true);
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
    for (int i = ro_start_indx + 1; i <= ro_end_indx; i++) {
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
    double start = endval(dv, xs, false);
    double end = endval(dv, xs, true);
    double th1 = start + 0.1*(end - start);
    double th2 = start + 0.9*(end - start);

    int ibeg = -1, iend = -1;
    for (int i = ro_start_indx + 1; i <= ro_end_indx; i++) {
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

