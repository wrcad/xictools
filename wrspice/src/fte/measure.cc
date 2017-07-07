
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
 * Whiteley Research Circuit Simulation and Analysis Tool                 *
 *                                                                        *
 *========================================================================*
 $Id: measure.cc,v 2.62 2015/08/22 22:02:34 stevew Exp $
 *========================================================================*/

#include "frontend.h"
#include "ftemeas.h"
#include "fteparse.h"
#include "lstring.h"
#include "misc.h"
#include "ftedata.h"
#include "cshell.h"
#include "hash.h"
#include "spnumber.h"


//
// Functions for the measurement post-processor:
// 1. "measure" button on plot window
// 2.  .measure statements
// 3.  measure debug
//

namespace {
    bool get_anal(const char*, int*);
    char *gtok(const char**, bool = false);
    double gval(const char**, bool*);
    sDataVec *evaluate(const char*);
}


// Constructor.
//
sMeas::sMeas(const char *str, char **errstr)
{
    memset(this, 0, sizeof(sMeas));
    parse(str, errstr);
}


// Destructor.
//
sMeas::~sMeas()
{
    delete [] result;
    delete [] start_name;
    delete [] end_name;
    delete [] expr2;
    delete [] start_when_expr1;
    delete [] start_when_expr2;
    delete [] end_when_expr1;
    delete [] end_when_expr2;

    while (funcs) {
        sMfunc *f = funcs->next;
        delete funcs;
        funcs = f;
    }
    while (finds) {
        sMfunc *f = finds->next;
        delete finds;
        finds = f;
    }
}


namespace {
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


// Parse the string and set up the structure appropriately.
//
bool
sMeas::parse(const char *str, char **errstr)
{
    bool err = false;
    const char *s = str;
    char *tok = gtok(&s);
    delete [] tok;
    bool in_trig = false;  // beginning
    bool in_targ = false;  // ending
    bool in_when = false;
    bool gotanal = false;
    analysis = -1;
    while ((tok = gtok(&s, true)) != 0) {
        if (!gotanal) {
            if (get_anal(tok, &analysis)) {
                gotanal = true;
                delete [] tok;
                result = gtok(&s);
                if (!result) {
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
                start_at = gval(&s, &err);
                if (err) {
                    listerr(errstr, "at");
                    break;
                }
                start_at_given = true;
                in_trig = false;
            }
            else if (lstring::cieq(tok, "when")) {
                delete [] tok;
                start_when_expr1 = gtok(&s);
                if (!start_when_expr1) {
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
                    start_when_expr2 = gtok(&s);
                }
                else
                    start_when_expr2 = tok;
                if (!start_when_expr2) {
                    err = true;
                    listerr(errstr, "when");
                    break;
                }
                start_when_given = true;
            }
            else
                start_name = tok;
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
                end_at = gval(&s, &err);
                if (err) {
                    listerr(errstr, "at");
                    break;
                }
                end_at_given = true;
                in_trig = false;
            }
            else if (lstring::cieq(tok, "when")) {
                end_when_expr1 = gtok(&s);
                if (!end_when_expr1) {
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
                    end_when_expr2 = gtok(&s);
                }
                else
                    end_when_expr2 = tok;
                if (!end_when_expr2) {
                    err = true;
                    listerr(errstr, "when");
                    break;
                }
                end_when_given = true;
            }
            else
                end_name = tok;
            continue;
        }
        if (lstring::cieq(tok, "val")) {
            if (in_trig)
                start_val = gval(&s, &err);
            else if (in_targ)
                end_val = gval(&s, &err);
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
                start_delay = gval(&s, &err);
            else if (in_targ)
                end_delay = gval(&s, &err);
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
                start_crosses = (int)gval(&s, &err);
            else if (in_targ)
                end_crosses = (int)gval(&s, &err);
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
                start_rises = (int)gval(&s, &err);
            else if (in_targ)
                end_rises = (int)gval(&s, &err);
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
                start_falls = (int)gval(&s, &err);
            else if (in_targ)
                end_falls = (int)gval(&s, &err);
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
            start_at = gval(&s, &err);
            start_at_given = true;
            delete [] tok;
            if (err) {
                start_meas = gtok(&s);
                // should be the name of another measure
                if (!start_meas) {
                    listerr(errstr, "from");
                    break;
                }
            }
            continue;
        }
        if (lstring::cieq(tok, "to")) {
            end_at = gval(&s, &err);
            end_at_given = true;
            delete [] tok;
            if (err) {
                end_meas = gtok(&s);
                // should be the name of another measure
                if (!end_meas) {
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
            start_at = gval(&s, &err);
            if (err) {
                listerr(errstr, "at");
                break;
            }
            start_at_given = true;
            in_trig = false;
            continue;
        }
        if (lstring::cieq(tok, "when")) {
            in_when = true;
            in_trig = false;
            in_targ = false;
            delete [] tok;
            start_when_expr1 = gtok(&s);
            if (!start_when_expr1) {
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
                start_when_expr2 = gtok(&s);
            }
            else
                start_when_expr2 = tok;
            if (!start_when_expr2) {
                err = true;
                listerr(errstr, "when");
                break;
            }
            when_given = true;
            continue;
        }
        if (lstring::cieq(tok, "param")) {
            delete [] tok;
            expr2 = gtok(&s);
            if (expr2 && *expr2 == '=') {
                delete [] expr2;
                expr2 = gtok(&s);
            }
            if (!expr2) {
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
            print_flag = 2;
            continue;
        }
        if (lstring::cieq(tok, "print_terse")) {
            print_flag = 1;
            continue;
        }
        if (lstring::cieq(tok, "stop")) {
            stop_flag = true;
            continue;
        }
        const char *t = tok;
        double *dd = SPnum.parse(&t, false);
        if (dd) {
            // the "val=" is optional in these cases
            if (in_trig || in_when) {
                start_val = *dd;
                delete [] tok;
                continue;
            }
            if (in_targ) {
                end_val = *dd;
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
        measure_skip = true;
    return (true);
}


// Add a measurement to the appropriate list.  This is a private function
// called during the parse.
//
void
sMeas::addMeas(Mfunc type, const char *expr)
{
    sMfunc *m = new sMfunc(type, expr);
    if (type == Mfind) {
        if (!finds)
            finds = m;
        else {
            sMfunc *mm = finds;
            while (mm->next)
                mm = mm->next;
            mm->next = m;
        }
    }
    else {
        if (!funcs)
            funcs = m;
        else {
            sMfunc *mm = funcs;
            while (mm->next)
                mm = mm->next;
            mm->next = m;
        }
    }
}


namespace {
    // Return true if name names and analysis, and return the index in
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
    char *gtok(const char **s, bool kw)
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


    // Return the indexed value, or the last value if index is too big.
    //
    inline double value(sDataVec *dv, int i)
    {
        if (i < dv->length())
            return (dv->realval(i));
        return (dv->realval(dv->length() - 1));
    }
}


// Reset the measurement.
//
void
sMeas::reset(sPlot *pl)
{
    if (pl) {
        // if the result vector is found in pl, delete it after copying
        // to a history list in pl
        sDataVec *v = pl->get_perm_vec(result);
        if (v) {
            char buf[128];
            pl->remove_perm_vec(result);
            sprintf(buf, "%s_scale", result);
            sDataVec *vs = pl->get_perm_vec(buf);
            if (vs) {
                pl->remove_perm_vec(buf);
                sprintf(buf, "%s_hist", result);
                sDataVec *vh = pl->get_perm_vec(buf);
                if (!vh) {
                    vh = new sDataVec(*v->units());
                    vh->set_name(buf);
                    vh->set_flags(0);
                    sPlot *px = Sp.CurPlot();
                    Sp.SetCurPlot(pl);
                    vh->newperm();
                    Sp.SetCurPlot(px);
                }
                sDataVec *vhs = vh->scale();
                if (!vhs) {
                    sprintf(buf, "%s_hist_scale", result);
                    vhs = new sDataVec(*vs->units());
                    vhs->set_name(buf);
                    vhs->set_flags(0);
                    sPlot *px = Sp.CurPlot();
                    Sp.SetCurPlot(pl);
                    vhs->newperm();
                    Sp.SetCurPlot(px);
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

    start_dv = 0;
    end_dv = 0;
    start_indx = 0;
    end_indx = 0;
    found_rises = 0;
    found_falls = 0;
    found_crosses = 0;
    found_start = 0.0;
    found_end = 0.0;
    start_delay = 0.0;
    end_delay = 0.0;
    found_start_flag = 0;
    found_end_flag = 0;
    measure_done = false;
    measure_error = false;
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


// Check if the measurement is triggered, and if so perform the measurement.
// This is called periodically as the analysis progresses, or can be called
// after the analysis is complete.  True is returned if the measurement
// was performed.
//
bool
sMeas::check(sFtCirc *circuit)
{
    if (measure_done || measure_error || measure_skip)
        return (true);
    sDataVec *xs = circuit->runplot()->scale();
    if (expr2) {
        for (sMeas *m = circuit->measures(); m; m = m->next) {
            if (analysis != m->analysis)
                continue;
            if (m == this)
                continue;
            if (m->expr2)
                continue;
            if (!m->measure_done && !m->measure_error)
                return (false);
        }
        // All non-param measurements done.
    }
    else if (when_given) {
        // 'when' was given, measure at a single point
        if (!found_start_flag) {
            double start = start_delay;
            int i = chk_trig(start, xs);
            if (i < 0)
                return (false);

            sDataVec *dvl = evaluate(start_when_expr1);
            sDataVec *dvr = evaluate(start_when_expr2);
            if (!dvl || !dvr) {
                measure_error = true;
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
                if (r >= start_rises && f >= start_falls &&
                        c >= start_crosses) {
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
                    start_indx = i;
                    break;
                }
            }
            if (!found)
                return (false);
            found_start = start;
            found_start_flag = true;
        }
    }
    else {
        // 'trig' and maybe 'targ' were given, measure over an interval
        // if targ given
        if (start_name && !start_dv) {
            sMeas *m = sMeas::find(circuit->measures(), start_name);
            if (m) {
                if (!m->measure_done)
                    return (false);
                if (m->found_end_flag)
                    start_delay += m->found_end;
                else
                    start_delay += m->found_start;
                start_dv = circuit->runplot()->scale();
            }
            else {
                start_dv = circuit->runplot()->find_vec(start_name);
                if (!start_dv) {
                    measure_error = true;
                    return (false);
                }
            }
        }
        if (start_at_given && !start_dv) {
            if (start_meas) {
                sMeas *m = sMeas::find(circuit->measures(), start_meas);
                if (!m) {
                    measure_error = true;
                    return (false);
                }
                if (!m->measure_done)
                    return (false);
                if (m->found_end_flag)
                    start_at = m->found_end;
                else
                    start_at = m->found_start;
            }
            start_dv = circuit->runplot()->scale();
        }
        if (end_name && !end_dv) {
            sMeas *m = sMeas::find(circuit->measures(), end_name);
            if (m) {
                if (!m->measure_done)
                    return (false);
                if (m->found_end_flag)
                    end_delay += m->found_end;
                else
                    end_delay += m->found_start;
                end_dv = circuit->runplot()->scale();
            }
            else {
                end_dv = circuit->runplot()->find_vec(end_name);
                if (!end_dv) {
                    measure_error = true;
                    return (false);
                }
            }
        }
        if (end_at_given && !end_dv) {
            if (end_meas) {
                sMeas *m = sMeas::find(circuit->measures(), end_meas);
                if (!m) {
                    measure_error = true;
                    return (false);
                }
                if (!m->measure_done)
                    return (false);
                if (m->found_end_flag)
                    end_at = m->found_end;
                else
                    end_at = m->found_start;
            }
            end_dv = circuit->runplot()->scale();
        }

        if (!found_start_flag && start_dv) {
            double start;
            if (start_at_given)
                start = start_at;
            else
                start = start_delay;
            int i = chk_trig(start, xs);
            if (i < 0)
                return (false);
            if (start_at_given)
                start = xs->realval(i);
            else if (start_when_given) {
                sDataVec *dvl = evaluate(start_when_expr1);
                sDataVec *dvr = evaluate(start_when_expr2);
                if (!dvl || !dvr) {
                    measure_error = true;
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
                    if (r >= start_rises && f >= start_falls &&
                            c >= start_crosses) {
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
            else if (start_rises > 0 || start_falls > 0 || start_crosses > 0) {
                int r = 0, f = 0, c = 0;
                bool found = false;
                for ( ; i < start_dv->length(); i++) {
                    if (i && start_dv->realval(i-1) <= start_val &&
                            start_val < start_dv->realval(i)) {
                        r++;
                        c++;
                    }
                    else if (i && start_dv->realval(i-1) > start_val &&
                            start_val >= start_dv->realval(i)) {
                        f++;
                        c++;
                    }
                    else
                        continue;
                    if (r >= start_rises && f >= start_falls &&
                            c >= start_crosses) {
                        start = xs->realval(i-1) +
                            (xs->realval(i) - xs->realval(i-1))*
                            (start_val - start_dv->realval(i-1))/
                            (start_dv->realval(i) - start_dv->realval(i-1));
                        found = true;
                        break;
                    }
                }
                if (!found)
                    return (false);
            }
            else if (start_dv != circuit->runplot()->scale())
                return (false);
            found_start = start;
            start_indx = i;
            found_start_flag = true;
        }

        if (!found_end_flag && end_dv) {
            double end;
            if (end_at_given)
                end = end_at;
            else
                end = end_delay;
            int i = chk_trig(end, xs);
            if (i < 0)
                return (false);
            if (end_at_given)
                end = xs->realval(i);
            else if (end_when_given) {
                sDataVec *dvl = evaluate(end_when_expr1);
                sDataVec *dvr = evaluate(end_when_expr2);
                if (!dvl || !dvr) {
                    measure_error = true;
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
                    if (r >= end_rises && f >= end_falls &&
                            c >= end_crosses) {
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
            else if (end_rises > 0 || end_falls > 0 || end_crosses > 0) {
                int r = 0, f = 0, c = 0;
                bool found = false;
                for ( ; i < end_dv->length(); i++) {
                    if (i && end_dv->realval(i-1) <= end_val &&
                            end_val < end_dv->realval(i)) {
                        r++;
                        c++;
                    }
                    else if (i && end_dv->realval(i-1) > end_val &&
                            end_val >= end_dv->realval(i)) {
                        f++;
                        c++;
                    }
                    else
                        continue;
                    if (r >= end_rises && f >= end_falls &&
                            c >= end_crosses) {
                        end = xs->realval(i-1) +
                            (xs->realval(i) - xs->realval(i-1))*
                            (end_val - end_dv->realval(i-1))/
                            (end_dv->realval(i) - end_dv->realval(i-1));
                        found = true;
                        i--;
                        break;
                    }
                }
                if (!found)
                    return (false);
            }
            else if (end_dv != circuit->runplot()->scale())
                return (false);
            found_end = end;
            end_indx = i;
            found_end_flag = true;
        }
    }
    cktptr = circuit;

    //
    // Successfully identified measurement interval, do measurement.
    // Add a vector containing the results to the plot.
    //
    sDataVec *dv0 = 0;
    int count = 0;
    if (found_start_flag && found_end_flag) {
        for (sMfunc *ff = funcs; ff; ff = ff->next, count++) {
            sDataVec *dv = evaluate(ff->expr);
            if (dv && 
                    ((dv->length() > start_indx && dv->length() > end_indx) ||
                    dv->length() == 1)) {
                if (!dv0)
                    dv0 = dv;
                if (ff->type == Mmin) {
                    if (dv->length() == 1)
                        ff->val = dv->realval(0);
                    else {
                        double mn = dv->realval(start_indx);
                        for (int i = start_indx+1; i <= end_indx; i++) {
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
                        double mx = dv->realval(start_indx);
                        for (int i = start_indx+1; i <= end_indx; i++) {
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
                        double mn = dv->realval(start_indx);
                        double mx = mn;
                        for (int i = start_indx+1; i <= end_indx; i++) {
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
    else if (found_start_flag) {
        for (sMfunc *ff = finds; ff; ff = ff->next, count++) {
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
    else if (expr2) {
        dv0 = evaluate(expr2);
        if (!dv0) {
            measure_error = true;
            return (false);
        }
    }
    else
        return (false);

    if (circuit->runplot()) {

        // create the output vector and scale
        sPlot *pl = circuit->runplot();
        sDataVec *nv = 0;
        if (found_end_flag) {
            // units test
            int uv = 0, us = 0;
            for (sMfunc *ff = funcs; ff; ff = ff->next) {
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
        nv->set_name(result);
        nv->set_flags(0);
        sPlot *px = Sp.CurPlot();
        Sp.SetCurPlot(pl);
        nv->newperm();
        Sp.SetCurPlot(px);
        char scname[128];
        sprintf(scname, "%s_scale", result);
        sDataVec *ns = new sDataVec(*xs->units());
        ns->set_name(scname);
        ns->set_flags(0);
        px = Sp.CurPlot();
        Sp.SetCurPlot(pl);
        ns->newperm();
        Sp.SetCurPlot(px);
        nv->set_scale(ns);

        if (expr2) {
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
                if (found_end_flag)
                    count++;
                nv->set_realvec(new double[count]);
                // No measurement, the named result is the time value.
                nv->set_realval(0, found_start);
                if (found_end_flag)
                    nv->set_realval(1, found_end);
            }
            else
                nv->set_realvec(new double[count]);
            nv->set_length(count);
            nv->set_allocated(count);
            count = 0;
            if (found_end_flag) {
                for (sMfunc *ff = funcs; ff; ff = ff->next, count++)
                    nv->set_realval(count, ff->val);
            }
            else {
                for (sMfunc *ff = finds; ff; ff = ff->next, count++)
                    nv->set_realval(count, ff->val);
            }
            count = 1;
            if (found_end_flag)
                count++;
            ns->set_realvec(new double[count]);
            ns->set_length(count);
            ns->set_allocated(count);
            ns->set_realval(0, found_start);
            if (found_end_flag)
                ns->set_realval(1, found_end);
        }
    }

    measure_done = true;
    if (print_flag && !Sp.GetFlag(FT_SERVERMODE)) {
        char *s = print();
        TTY.printf_force("%s", s);
        delete [] s;
    }
    return (true);
}


// Return a string containing text of measurement result.
//
char *
sMeas::print()
{
    if (!measure_done)
        return (0);
    sLstr lstr;
    char buf[BSIZE_SP];
    if (print_flag > 1) {
        sprintf(buf, "measure: %s\n", result);
        lstr.add(buf);
    }
    if (found_start_flag && found_end_flag) {
        if (print_flag > 1) {
            if (cktptr && cktptr->runplot() && cktptr->runplot()->scale()) {
                const char *zz = cktptr->runplot()->scale()->name();
                if (zz && *zz) {
                    sprintf(buf, " %s\n", zz);
                    lstr.add(buf);
                }
            }
            sprintf(buf, "    start: %-16g end: %-16g delta: %g\n",
                found_start, found_end, found_end - found_start);
            lstr.add(buf);
        }
        for (sMfunc *ff = funcs; ff; ff = ff->next) {
            if (print_flag > 1) {
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
            if (print_flag > 1) {
                if (ff->error)
                    sprintf(buf, "    %s: (error occurred)\n", str);
                else
                    sprintf(buf, "    %s: %g\n", str, ff->val);
            }
            else {
                if (ff->error)
                    sprintf(buf, "%s %s: (error occurred)\n", result, str);
                else
                    sprintf(buf, "%s %s: %g\n", result, str, ff->val);
            }
            lstr.add(buf);
        }
    }
    else if (found_start_flag) {
        if (print_flag > 1) {
            if (cktptr && cktptr->runplot() && cktptr->runplot()->scale()) {
                const char *zz = cktptr->runplot()->scale()->name();
                if (zz && *zz) {
                    sprintf(buf, " %s\n", zz);
                    lstr.add(buf);
                }
            }
            sprintf(buf, "    start: %g\n", found_start);
            lstr.add(buf);
        }
        for (sMfunc *ff = finds; ff; ff = ff->next) {
            if (print_flag > 1) {
                if (ff->error)
                    sprintf(buf, " %s = (error occurred)\n", ff->expr);
                else
                    sprintf(buf, " %s = %g\n", ff->expr, ff->val);
            }
            else {
                if (ff->error)
                    sprintf(buf, "%s %s = (error occurred)\n", result,
                        ff->expr);
                else
                    sprintf(buf, "%s %s = %g\n", result, ff->expr, ff->val);
            }
            lstr.add(buf);
        }
    }
    char *str = lstr.string_trim();
    lstr.clear();
    return (str);
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


// Return the interpolated end values of dv.  If end is false, look at
// the start side, where the interval starts ahead of the index point.
// Otherwise, the value is after the index point.
//
double
sMeas::endval(sDataVec *dv, sDataVec *xs, bool end)
{
    if (!end) {
        int i = start_indx;
        if (found_start != xs->realval(i) && i > 0) {
            double y0 = value(dv, start_indx);
            double y1 = value(dv, start_indx - 1);
            return (y0 + (y1 - y0)*(found_start - xs->realval(i))/
                (xs->realval(i-1) - xs->realval(i)));
        }
        else
            return (value(dv, start_indx));
    }
    else {
        int i = end_indx;
        if (found_end != xs->realval(i) && i+1 < dv->length()) {
            double y0 = value(dv, end_indx);
            double y1 = value(dv, end_indx + 1);
            return (y0 + (y1 - y0)*(found_end - xs->realval(i))/
                (xs->realval(i+1) - xs->realval(i)));
        }
        else
            return (value(dv, end_indx));
    }
}


// Find the average of dv over the interval.  We use trapezoid
// integration and divide by the scale interval.
//
double
sMeas::findavg(sDataVec *dv, sDataVec *xs)
{
    double sum = 0.0;
    double d, delt;
    // compute the sum over the scale intervals
    int i;
    for (i = start_indx+1; i <= end_indx; i++) {
        delt = xs->realval(i) - xs->realval(i-1);
        sum += 0.5*delt*(dv->realval(i-1) + dv->realval(i));
    }

    // found_start is ahead of the start index (between i-1 and i)
    delt = xs->realval(start_indx) - found_start;
    if (delt != 0) {
        i = start_indx;
        d = dv->realval(i) + (dv->realval(i-1) - dv->realval(i))*
            (found_start - xs->realval(start_indx))/
            (xs->realval(i-1) - xs->realval(i));
        sum += 0.5*delt*(d + dv->realval(i));
    }

    // found_end is ahead if the end index (between i-1 and i)
    delt = found_end - xs->realval(end_indx);
    if (delt != 0.0) {
        i = end_indx;
        d = dv->realval(i) + (dv->realval(i+1) - dv->realval(i))*
            (found_end - xs->realval(end_indx))/
            (xs->realval(i+1) - xs->realval(i));
        sum += 0.5*delt*(d + dv->realval(i));
    }

    double dt = found_end - found_start;
    if (dt != 0.0)
        sum /= dt;
    return (sum);
}


// Find the rms of dv over the interval.  We use trapezoid
// integration of the magnitudes.
//
double
sMeas::findrms(sDataVec *dv, sDataVec *xs)
{
    double sum = 0.0;
    double d, delt;
    // compute the sum over the scale intervals
    int i;
    for (i = start_indx+1; i <= end_indx; i++) {
        delt = xs->realval(i) - xs->realval(i-1);
        sum += 0.5*delt*(dv->realval(i-1)*dv->realval(i-1) +
            dv->realval(i)*dv->realval(i));
    }

    // found_start is ahead of the start index
    delt = xs->realval(start_indx) - found_start;
    if (delt != 0.0) {
        i = start_indx;
        d = dv->realval(i) + (dv->realval(i-1) - dv->realval(i))*
            (found_start - xs->realval(start_indx))/
            (xs->realval(i-1) - xs->realval(i));
        sum += 0.5*delt*(d*d + dv->realval(i)*dv->realval(i));
    }

    // found_end is behind the end index
    delt = found_end - xs->realval(end_indx);
    if (delt != 0.0) {
        i = end_indx;
        d = dv->realval(i) + (dv->realval(i+1) - dv->realval(i))*
            (found_end - xs->realval(end_indx))/
            (xs->realval(i+1) - xs->realval(i));
        sum += 0.5*delt*(d*d + dv->realval(i)*dv->realval(i));
    }

    double dt = found_end - found_start;
    if (dt != 0.0)
        sum /= dt;
    return (sqrt(fabs(sum)));
}


// Find the fwhm of a pulse assumed to be contained in the interval.
//
double
sMeas::findpw(sDataVec *dv, sDataVec *xs)
{
    // find the max/min
    double mx = dv->realval(start_indx);
    double mn = mx;
    int imx = -1;
    int imn = -1;
    for (int i = start_indx+1; i <= end_indx; i++) {
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
    for (int i = start_indx + 1; i <= end_indx; i++) {
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
sMeas::findrft(sDataVec *dv, sDataVec *xs)
{
    double start = endval(dv, xs, false);
    double end = endval(dv, xs, true);
    double th1 = start + 0.1*(end - start);
    double th2 = start + 0.9*(end - start);

    int ibeg = -1, iend = -1;
    for (int i = start_indx + 1; i <= end_indx; i++) {
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

