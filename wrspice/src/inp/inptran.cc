
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

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1987 Wayne A. Christopher
         1992 Stephen R. Whiteley
****************************************************************************/

#include <math.h>
#include "input.h"
#include "inpptree.h"
#include "circuit.h"
#include "simulator.h"
#include "datavec.h"
#include "output.h"
#include "misc.h"
#include "miscutil/random.h"
#include "miscutil/errorrec.h"
#include "spnumber/hash.h"


#ifndef MAXFLOAT
#define MAXFLOAT 3.40282346638528860e+38
#endif

#ifndef M_PI
#define M_PI		3.14159265358979323846	/* pi */
#endif

#ifndef M_LN2
#define M_LN2		0.69314718055994530942	/* log_e 2 */
#endif

namespace {
const double TWOSQRTLN2     = 2.0*sqrt(M_LN2);
const double PHI0_SQRTPI    = wrsCONSTphi0/sqrt(M_PI);
}

//#define DEBUG


//
// "Tran" function setup/evaluation.
//

namespace {
#define ARY_INIT_SZ 10

    // Save a list of doubles.
    //
    struct dblAry
    {
        dblAry()
            {
                da_ary = new double[ARY_INIT_SZ];
                da_size = ARY_INIT_SZ;
                da_cnt = 0;
            }

        ~dblAry()
            {
                delete [] da_ary;
            }

        void add(double d)
            {
                if (da_cnt == da_size) {
                    double *tmp = new double[2*da_size];
                    memcpy(tmp, da_ary, da_size*sizeof(double));
                    delete [] da_ary;
                    da_ary = tmp;
                    da_size += da_size;
                }
                da_ary[da_cnt++] = d;
            }

        double *final() const
            {
                double *tmp = new double[da_cnt + 1];
                memcpy(tmp, da_ary, da_cnt*sizeof(double));
                tmp[da_cnt] = 0.0;
                return (tmp);
            }

        int count() const { return (da_cnt); }

    private:
        double *da_ary;
        int da_size;
        int da_cnt;
    };


    // Double precision equality.
    //
    bool feq(double v1, double v2)
    {
        return (fabs(v1-v2) <= 1e-12*(fabs(v1) + fabs(v2)));
    }


    // Hash table for tran function names.
    sHtab *tran_tab;


    // If the leading token in pstr is a tranfunc name, return the
    // actual function name from the table and advance the pointer.
    //
    IFparseNode::PTtfunc *is_tran_func(const char **pstr)
    {
        if (!pstr)
            return (0);
        if (!tran_tab) {
            tran_tab = new sHtab(true);  // case insensitive
            for (int i = 0; IFparseNode::PTtFuncs[i].name; i++) {
                tran_tab->add(IFparseNode::PTtFuncs[i].name,
                    IFparseNode::PTtFuncs + i);
            }
        }

        char buf[16];
        const char *s = *pstr;
        char *t = buf;
        for (int i = 0; *s; s++, i++) {
            if (i > 15)
                return (0);
            if (isspace(*s) || *s == '(')
                break;
            if (isalpha(*s))
                *t++ = *s;
            else
                return (0);
        }
        *t = 0;
        if (t - buf < 2)
            return (0);
        IFparseNode::PTtfunc *tf =
            (IFparseNode::PTtfunc*)sHtab::get(tran_tab, buf);
        if (tf)
            *pstr = s;
        return (tf);
    }


    // Count the function args in argstr, which includes the trailing
    // ')' but not the initial '('.  Return the space-separated arg
    // string.  If delim_cnt is non-0, return a count of the
    // delimiters found.  This is NOT the argument count, but can be
    // used for bounds testing.  The delimiter count is >= numargs-1. 
    // If comma_cnt is non-0, return the number of comma delimiters.
    //
    char *fix_tran_args(const char *argstr, int *delim_cnt, int *comma_cnt)
    {
        // The arguments to a tran function can be a constant expression,
        // and they can be comma or space separated.

        if (!argstr) {
            if (delim_cnt)
                *delim_cnt = 0;
            if (comma_cnt)
                *comma_cnt = 0;
            return (0);
        }
        int ccnt = 0;
        int dcnt = 0;
        const char *t = argstr;
        sLstr lstr;

        // Remove comma separators, replace with space.
        int l1 = 0, l2 = 0;
        while (*t) {
            char c = *t;
            if (c == '(')
                l1++;
            else if (c == ')') {
                if (*(t+1) == 0)
                    // ignore last ')'
                    break;
                l1--;
            }
            else if (c == '[')
                l2++;
            else if (c == ']')
                l2--;
            if (!l1 && !l2) {
                if (isspace(c)) {
                    const char *s = t+1;
                    while (isspace(*s))
                        s++;
                    lstr.add_c(' ');
                    if (*s != ',' && *s != ')')
                        dcnt++;
                    t = s;
                    continue;
                }
                if (c == ',') {
                    c = ' ';
                    ccnt++;
                    dcnt++;
                }
                else if (c == '-' && (t > argstr && isspace(t[-1])) &&
                        !isspace(t[1]))
                    // The parser will take "a<opt space>-<opt
                    // space>b" as "a-b", so we make an assumption
                    // here.
                    //   <space>-<token> -> <space>;-<token>  (token break)
                    //   <space>-<space>  left alone
                    //   <token>-<token>  left alone
                    //   <token>-<space>  left alone
                    lstr.add_c(';');
            }
            lstr.add_c(c);
            t++;
        }
        if (delim_cnt)
            *delim_cnt = dcnt;
        if (comma_cnt)
            *comma_cnt = ccnt;
        return (lstr.string_trim());
    }


    void strip_quotes(char *string)
    {
        if (string == 0)
            return;
        int l = strlen(string) - 1;
        if (string[l] == '"' && *string == '"') {
            string[l] = '\0';
            char *s = string;
            while (*s) {
                *s = *(s+1);
                s++;
            }
        }
    }
}


// Return the length of the sequence.
//
int
pbitList::length ()
{
    if (!pl_bstring)
        return (0);
    if (!strncasecmp(pl_bstring+1, "prbs", 4)) {
        int n = atoi(pl_bstring+5);
        if (n < 6)
            n = 6;
        else if (n > 12)
            n = 12;
        return ((1 << n) - 1);
    }
    return (strlen(pl_bstring) - 1);
}


// Static function.
// Parse a pattern specification, create and return a linked list
// description.  The string pointer is advanced.  On error, zero is
// returned and a message is found in errstr, caller should free this.
//
pbitList *
pbitList::parse(const char **p, char **errstr)
{
    *errstr = 0;
    pbitList *bs0 = 0, *bse = 0;
    char *tok = IP.getTok(p, true);
    while (tok) {
        strip_quotes(tok);
        if (*tok == 'b' || *tok == 'B') {
            // Found a bstring, expect "b..." [r[=N]] [rb=M]
            if (!bs0)
                bs0 = bse = new pbitList(tok);
            else {
                bse->pl_next = new pbitList(tok);
                bse = bse->pl_next;
            }

            tok = IP.getTok(p, true);
            while (tok) {
                if (lstring::cieq(tok, "rb")) {
                    delete [] tok;
                    int err;
                    int rbval = IP.getFloat(p, &err, true);
                    if (err == OK) {
                        if (rbval < 1)
                            rbval = 1;
                        if (rbval >= bse->length()) {
                            *errstr = lstring::copy("RB value too large");
                            destroy(bs0);
                            return (0);
                        }
                        else
                            bse->pl_rb = rbval;
                    }
                    else {
                        *errstr = lstring::copy("error reading RB value");
                        destroy(bs0);
                        return (0);
                    }
                    tok = IP.getTok(p, true);
                    continue;
                }
                if (lstring::cieq(tok, "r")) {
                    delete [] tok;
                    int err;
                    const char *p0 = *p;
                    int rval = IP.getFloat(p, &err, true);
                    if (err == OK) {
                        if (rval < -1)
                            rval = 0;
                        bse->pl_r = rval;
                        tok = IP.getTok(p, true);
                        continue;
                    }
                    else {
                        *p = p0;
                        continue;
                    }
                }
                break;
            }
        }
        else {
            // unknown token, ignore it.
            delete [] tok;
            tok = IP.getTok(p, true);
        }
    }
    return (bs0);
}


// Static function.
// Duplicate a pattern specification list.
//
pbitList *
pbitList::dup(pbitList *bs)
{
    pbitList *be = 0;
    pbitList *new_pbitList = 0;
    for (pbitList *b = bs; b; b = b->pl_next) {
        pbitList *bn = new pbitList(lstring::copy(b->pl_bstring));
        bn->pl_r = b->pl_r;
        bn->pl_rb = b->pl_rb;
        if (!be)
            new_pbitList = be = bn;
        else {
            be->pl_next = bn;
            be = be->pl_next;
        }
    }
    return (new_pbitList);
}
// End of pbitList functions.


// Create and fill a bit array with pattern data.  The per and
// finaltime are only used in the case of infinite repetition to
// determine the total length.
//
void
pbitAry::add(pbitList *list)
{
    // Construct the pattern array.
    bool done = false;
    for (pbitList *b = list; b && !done; b = b->next()) {
        int strt = ba_cnt;

        const char *s = b->bstring() + 1;
        if (!strncasecmp(s, "prbs", 4)) {
            int n = atoi(s + 4);
            addPRBS(n, b->rb() - 1);

            if (b->r() == -1) {
                ba_rst = strt ;
                if (ba_rst == 0) {
                    addbit(ba_ary[0] & 1);
                    ba_rst = 1;
                }
                done = true;
            }
            else {
                for (int i = 0; i < b->r(); i++)
                    addPRBS(n, b->rb() - 1);
            }
        }
        else {
            const char *z = "0fnFN";
            for ( ; *s; s++)
                addbit(!strchr(z, *s));

            if (b->r() == -1) {
                ba_rst = strt + b->rb() - 1;
                if (ba_rst == 0) {
                    addbit(ba_ary[0] & 1);
                    ba_rst = 1;
                }
                done = true;
            }
            else {
                for (int i = 0; i < b->r(); i++) {
                    s = b->bstring() + b->rb();
                    for ( ; *s; s++)
                        addbit(!strchr(z, *s));
                }
            }
        }
    }
}

/*XXX
// Private function.
//
void
pbitAry::add(const char *s, int bit)
{
    if (!strncasecmp(s, "prbs", 4)) {
        int n = atoi(s+4);
        addPRBS(n, bit);
        return;
    }

    const char *z = "0fnFN";
    for ( ; *s; s++)
        addbit(!strchr(z, *s));
}
*/


// Private function.
// Supported pseudo-random sequences.
//  PRBS6 = x^6 + x^5 + 1
//  PRBS7 = x^7 + x^6 + 1
//  PRBS8 = x^8 + x^6 + x^5 + x^4 + 1;
//  PRBS9 = x^9 + x^5 + 1
//  PRBS10 = x^10 + x^7 + 1
//  PRBS11 = x^11 + x^9 + 1
//  PRBS12 = x^12 + x^11 + x^8 + x^6 + 1;
// The pattern is rotated by bit.
//
void
pbitAry::addPRBS(int len, int bit)
{
    int n1 = len;
    int n2;
    int n3 = 1;
    int n4 = 1;
    if (n1 <= 6) {
        n1 = 6;
        n2 = 5;
    }
    else if (n1 == 7)
        n2 = 6;
    else if (n1 == 8) {
        n2 = 6;
        n3 = 5;
        n4 = 4;
    }
    else if (n1 == 9)
        n2 = 5;
    else if (n1 == 10)
        n2 = 7;
    else if (n1 == 11)
        n2 = 9;
    else if (n1 >= 12) {
        n1 = 12;
        n2 = 11;
        n3 = 8;
        n4 = 6;
    }
    unsigned int mask = (1 << n1) - 1;
    unsigned long start = 0x02;
    unsigned long a = start;
    for (;;) {
        if (!bit)
            start = a;
        unsigned long newbit;
        if (n1 == 8 || n1 == 12) {
            newbit = (((a >> (n1-1)) + (a >> (n2-1)) + (a >> (n3-1)) +
                (a >> (n4-1))) & 1);
        }
        else
            newbit = (((a >> (n1-1)) + (a >> (n2-1))) & 1);
        a = ((a << 1) | newbit) & mask;
        if (bit-- <= 0) {
            addbit(a&1);
            if (a == start)
                break;
        }
    }
}
// End of pbitAry functions.


// Return true if the argument is the name of a "tran" function.
//
bool
SPinput::isTranFunc(const char *name)
{
    return (is_tran_func(&name) != 0);
}


// Here we recognize the 'tran' functions only.  If the leading token
// is a tran function name, and the arg count is consistent with a
// tran function where there is ambiguity with a regular function,
// return the tran function call and args, and advance s.
//
char *
SPinput::getTranFunc(const char **s, bool in_source)
{
    const char *bptr = *s;
    IFparseNode::PTtfunc *tf = is_tran_func(s);
    if (!tf)
        return (0);
    const char *name = tf->name;

    const char *t = *s;
    *s = bptr;
    bool open_paren = false;
    for ( ; *t; t++) {
        if (isspace(*t))
            continue;
        if (*t == '(') {
            open_paren = true;
            continue;
        }
        break;
    }

    if (!open_paren) {
        // The srcprse() function is WRspice adds parentheses, so we
        // never get here unless there is some error.  Even if we did,
        // the parse would fail, since the expression parser expects
        // parentheses.

        return (0);

        /*****
        // Parentheses are optional in source specifications.  In this
        // case, all arguments are numbers.

        if (!in_source)
            return (0);
        int error = 0, argc = 0;
        while (!error) {
            getFloat(&t, &error, true);
            if (!error)
                argc++;
        }
        if (!argc)
            return (0);
        int len = t - *s;
        char *buf = new char[len+1];
        memcpy(buf, *s, len);
        buf[len] = 0;
        *s = t;
        return (buf);
        *****/
    }

    int pcnt = 1;
    for ( ; *t; t++) {
        if (*t == '(')
            pcnt++;
        else if (*t == ')') {
            pcnt--;
            if (!pcnt) {
                t++;
                break;
            }
        }
    }
    int len = t - *s;
    char *buf = new char[len+1];
    memcpy(buf, *s, len);
    buf[len] = 0;
    char *args = strchr(buf, '(') + 1;

    // Since tranfuncs sin, exp, and gauss have the same name as math
    // functions, we try to figure out from the context whether to
    // interpret as a tran func or not.  Return true if it is a tran
    // function.

    // The table shows the number of args expected.  For math
    // functions, recall that i,j converts to a single complex arg, so
    // "2" args is really one when evaluated.  The gauss math function
    // is parsed a bit differently, but in each case the naieve
    // interpretation of the syntax gives the number of args in the
    // third column.  For math functions, a comma is always the
    // delimiter.  For tran functions, either space or a comma is
    // accepted.  However, to resolve ambiguity, if commas are found,
    // the math function is chosen.  Yes, this smells pretty bad.
    //
    // function name   tran args    math args
    // sin             2 or more    1 or 2
    // exp             2 or more    1 or 2
    // gauss           3 or 4       1 - 3
    //
    // If "in_source" favor the tran interpretation.

    if (lstring::cieq(name, "sin") || lstring::cieq(name, "exp")) {
        int dcnt, ccnt;
        char *targs = fix_tran_args(args, &dcnt, &ccnt);
        if (dcnt < 1) {
            // Too few args, not a tran function.
            delete [] targs;
            delete [] buf;
            return (0);
        }
        if (ccnt <= 1) {
            int num = 0;
            if (targs) {
                // Count the args.
                Errs()->push_error();
                const char *ts = targs;
                IFparseTree *tree = IFparseTree::getTree(&ts, 0, 0, true);
                while (tree) {
                    num++;
                    delete tree;
                    if (num > 2)
                        break;
                    if (!*ts)
                        break;
                    tree = IFparseTree::getTree(&ts, 0, 0, true);
                }
                Errs()->pop_error();
            }
            if (num < 2) {
                // Too few args, not a tran function.
                delete [] targs;
                delete [] buf;
                return (0);
            }
            if (num == 2 && ccnt == 1) {
                // Ambiguous, assume math function if comma and not
                // in_source.
                if (!in_source) {
                    delete [] targs;
                    delete [] buf;
                    return (0);
                }
            }
        }
    }
    else if (lstring::cieq(name, "gauss")) {
        int dcnt, ccnt;
        char *targs = fix_tran_args(args, &dcnt, &ccnt);
        if (dcnt < 2) {
            // Too few args, not a tran function.
            delete [] targs;
            delete [] buf;
            return (0);
        }
        int num = 0;
        if (targs) {
            // Count the args.
            Errs()->push_error();
            const char *ts = targs;
            IFparseTree *tree = IFparseTree::getTree(&ts, 0, 0, true);
            while (tree) {
                num++;
                delete tree;
                if (!*ts)
                    break;
                tree = IFparseTree::getTree(&ts, 0, 0, true);
            }
            Errs()->pop_error();
        }
        if (num < 3) {
            // Too few args, not a tran function.
            delete [] targs;
            delete [] buf;
            return (0);
        }
        if (num == 3 && ccnt == 2) {
            // Ambiguous, assume math function if comma and not
            // in_source.
            if (!in_source) {
                delete [] targs;
                delete [] buf;
                return (0);
            }
        }
        // num > 3, must be a tran function
    }
    
    *s += strlen(buf);

    return (buf);
}


// This is to allow lines like "pulse 0 1 ..." (without
// parentheses) to be accepted.  The expression parser expects
// parentheses, so we add them here.  If the string is changed,
// the new string is returned.  Called from the source parser.
//
char *
SPinput::fixParentheses(const char *line, sCKT *ckt, const char *xalias)
{
    const char *nend = line;
    char *nline = 0;
    while (*nend != 0) {
        const char *nlst = nend;
        char *parm = IP.getTok(&nend, true);
        if (!parm)
            // catch error later
            return (0);

        // Try to make this work whether or not getTok() returns '(' as
        // a token.

        const char *tparm = parm;
        IFparseNode::PTtfunc *tranfunc = is_tran_func(&tparm);

        if (tranfunc && *nend != '(') {
            while (!isalpha(*nlst))
                nlst++;
            while (isalpha(*nlst))
                nlst++;
            const char *s = nlst;  // first char after parm
            for ( ; s < nend; s++) {
                if (*s == '(')
                    break;
            }
            if (s == nend) {
                // Have to add the parentheses
                char *nstr = new char[strlen(line) + 4];
                s = line;
                char *t = nstr;
                while (s < nlst)
                    *t++ = *s++;
                *t++ = '(';

                int ac = 0;
                s = nend;  // first char of arg list
                for (;;) {
                    ac++;
                    IFparseTree *tree =
                        IFparseTree::getTree(&nend, ckt, xalias);
                    if (!tree) {
                        if (tranfunc->number == PTF_tPULSE ||
                                tranfunc->number == PTF_tGPULSE) {
                            const char *nn = nend;
                            char *tok = IP.getTok(&nend, true);
                            if (tok) {
                                // Accept bstring and associated.
                                strip_quotes(tok);
                                if (*tok == 'b' || *tok == 'B' ||
                                        !strcasecmp(tok, "r") ||
                                        !strcasecmp(tok, "rb")) {
                                    delete [] tok;
                                    continue;
                                }
                            }
                            nend = nn;
                            delete [] tok;
                        }
                        else if (tranfunc->number == PTF_tPWL) {
                            const char *nn = nend;
                            char *tok = IP.getTok(&nend, true);
                            if (tok) {
                                strip_quotes(tok);
                                if (ac <= 2) {
                                    // Allow an arbitrary vector
                                    // name (which may not exist
                                    // yet) for the first two
                                    // tokens of pwl.  This is a
                                    // new feature.

                                    delete [] tok;
                                    continue;
                                }
                                if (!strcasecmp(tok, "r") ||
                                        !strcasecmp(tok, "td")) {
                                    // Support the Hspice syntax
                                    //  [R [[=] num]] [TD [=] num] 

                                    delete [] tok;
                                    continue;
                                }
                            }
                            nend = nn;
                            delete [] tok;
                        }
                        else if (tranfunc->number == PTF_tINTERP) {
                            const char *nn = nend;
                            char *tok = IP.getTok(&nend, true);
                            if (tok) {
                                delete [] tok;
                                continue;
                            }
                            nend = nn;
                        }
                        break;
                    }
                    delete tree;
                }
                while (s < nend)
                    *t++ = *s++;
                while (isspace(*(t-1))) {
                    t--;
                    s--;
                }

                if (*nend == ')') {
                    // See if we really need new closure, user
                    // might have simply forgotton the first one

                    int cnt = 1;
                    for (const char *tt = line; tt <= nend; tt++) {
                        if (*tt == '(')
                            cnt++;
                        else if (*tt == ')')
                            cnt--;
                    }
                    if (cnt > 0)
                        *t++ = ')';
                }
                else
                    *t++ = ')';

                nend = t;
                while (*s)
                    *t++ = *s++;
                *t = 0;
                delete [] nline;
                nline = nstr;
                line = nline;
            }
        }
        delete [] parm;
    }
    return (nline);
}


// If the top node in the tree is a tran function, write into its
// parameter list.
//
void
SPinput::setTranFuncParam(IFparseTree *pt, double val, int ix)
{
    if (!pt)
        return;
    IFparseNode *p = pt->tree();
    if (!p)
        return;
    IFtranData *td = p->tranData();
    if (td)
        td->set_param(val, ix);
}


// If the top node is a tran function, read from its parameter list.  A
// null return indicates parameter not found.
//
double *
SPinput::getTranFuncParam(IFparseTree *pt, int ix)
{
    if (!pt)
        return (0);
    IFparseNode *p = pt->tree();
    if (!p)
        return (0);
    IFtranData *td = p->tranData();
    if (!td)
        return (0);
    return (td->get_param(ix));
}
// End of SPinput functions.


// Static function.
// Return true if string leads with a tran function call.  If p is
// passed, fill it in from the parsed spec.  The error return will be
// set if p is given and true is returned, if an error occurs.
//
// WARNING:  It is assumed here that the tran function string
// parameters are enclosed in ().
//
bool
IFparseNode::parse_if_tranfunc(IFparseNode *p, const char *string, int *error)
{
    *error = OK;
    if (!string)
        return (false);
    while (isspace(*string))
        string++;

    const char *astart = strchr(string, '(');
    if (!astart)
        return (false); // can't be a tran function
    if (!strchr(astart, ')'))
        return (false); // can't be a tran function
    astart++;
    while (isspace(*astart))
        astart++;

    PTtfunc *ptf = PTtFuncs;
    while (ptf->name) {
        if (lstring::ciprefix(ptf->name, string)) {
            int n = strlen(ptf->name);
            if (string[n] == '(' || isspace(string[n]))
                break;
        }
        ptf++;
    }
    if (!ptf->name)
        return (false);
    // If we get this far, the return value is always true.
    if (!p)
        return (true);

    p->p_type = PT_TFUNC;
    p->p_valname = ptf->name;
    p->p_valindx = ptf->number;
    p->p_func = ptf->funcptr;
    p->p_evfunc = &IFparseNode::p_tran;

    // If we're parsing a macro body, we can stop here.  The macro body
    // will be parsed again and linked into the calling tree.
    //
    if (p->p_tree->inMacro())
        return (true);

    // Parse the arguments, these will be linked to p_left, but not
    // used further.  The arguments must evaluate as constants, and
    // will be evaluated here, initializing the tran func.  We know
    // that if we are parsing a macro body, the macro arguments have
    // been pushed, so as to be resolved in newSnode.

    if (p->p_valindx == PTF_tPULSE)
        IFpulseData::parse(astart, p, error);
    else if (p->p_valindx == PTF_tGPULSE)
        IFgpulseData::parse(astart, p, error);
    else if (p->p_valindx == PTF_tPWL)
        IFpwlData::parse(astart, p, error);
    else if (p->p_valindx == PTF_tSIN)
        IFsinData::parse(astart, p, error);
    else if (p->p_valindx == PTF_tSPULSE)
        IFspulseData::parse(astart, p, error);
    else if (p->p_valindx == PTF_tEXP)
        IFexpData::parse(astart, p, error);
    else if (p->p_valindx == PTF_tSFFM)
        IFsffmData::parse(astart, p, error);
    else if (p->p_valindx == PTF_tAM)
        IFamData::parse(astart, p, error);
    else if (p->p_valindx == PTF_tGAUSS)
        IFgaussData::parse(astart, p, error);
    else if (p->p_valindx == PTF_tINTERP)
        IFinterpData::parse(astart, p, error);
    else if (p->p_valindx == PTF_tTABLE) {
        p->p_evfunc = &IFparseNode::p_table;
        sCKT *ckt = p->p_tree->ckt();
        if (!ckt) {
            *error = E_NOCURCKT;
            return (true);
        }
        char *tt = fix_tran_args(astart, 0, 0);
        char *t0 = tt;
        char *tok = IP.getTok((const char**)&tt, true);
        if (!tok) {
            delete [] t0;
            *error = E_BADPARM;
            return (true);
        }
        int err = IP.tablFind(tok, &p->v.table, ckt);
        if (!err) {
            Errs()->add_error("table %s not found", tok);
            delete [] t0;
            delete [] tok;
            *error = E_NOTFOUND;
            return (true);
        }
        delete [] tok;
        if (!*tt) {
            Errs()->add_error(
                "missing expression argument in table reference");
            delete [] t0;
            *error = E_BADPARM;
            return (true);
        }
        PTelement elements[STACKSIZE];
        Parser P = Parser(elements, PRSR_NODEHACK | PRSR_USRSTR);
        P.init(tt, p->p_tree);
        p->p_left = P.parse();
        if (!p->p_left || !check(p->p_left)) {
            Errs()->add_error(
                "parse failed for table reference expression");
            delete [] t0;
            *error = E_BADPARM;
            return (true);
        }
        delete [] t0;
        return (true);
    }
    else {
        // can't happen
        *error = E_NOTFOUND;
    }
    return (true);
}
// End of IFparseNode functions.


IFtranData::~IFtranData()
{
    delete [] td_coeffs;
    delete [] td_parms;
    delete [] td_cache;
}

void
IFtranData::setup(sCKT*, double, double, bool)
{
}

double
IFtranData::eval_func(double)
{
    return (0.0);
}

double
IFtranData::eval_deriv(double)
{
    return (0.0);
}

IFtranData *
IFtranData::dup() const
{
    return (0);
}

void
IFtranData::time_limit(const sCKT*, double*)
{
}

void
IFtranData::print(const char *name, sLstr &lstr)
{
    lstr.add(name);
    lstr.add_c('(');
    for (int i = 0; i < td_numcoeffs; i++) {
        lstr.add_c(' ');
        lstr.add_g(td_coeffs[i]);
    }
    lstr.add_c(' ');
    lstr.add_c(')');
}

void
IFtranData::set_param(double, int)
{
}

double *
IFtranData::get_param(int)
{
    return (0);
}
// End of IFtranData functions.


// PULSE
//
IFpulseData::IFpulseData(double *list, int num, pbitList *pbl) :
    IFtranData(PTF_tPULSE)
{
    td_parms = new double[8];
    td_cache = new double[2];
    td_coeffs = list;
    td_numcoeffs = num;
    td_pblist = pbl;
    td_parray = 0;
    td_plen = 0;
    td_prst = 0;

    set_V1(list[0]);
    set_V2(num >= 2 ? list[1] : list[0]);
    set_TD(num >= 3 ? list[2] : 0.0);
    set_TR(num >= 4 ? list[3] : 0.0);
    set_TF(num >= 5 ? list[4] : 0.0);
    set_PW(num >= 6 ? list[5] : 0.0);
    set_PER(num >= 7 ? list[6] : 0.0);
    td_cache[0] = TR() > 0.0 ? (V2() - V1())/TR() : 0.0;
    td_cache[1] = TF() > 0.0 ? (V2() - V1())/TF() : 0.0;
}


// Static function
//
void
IFpulseData::parse(const char *args, IFparseNode *p, int *error)
{
    // Make arguments space-separated (remove separating commas).
    char *tt = fix_tran_args(args, 0, 0);

    const char *ts = tt;
    dblAry da;
    bool list_done = false;

    IFparseNode **plast = &p->p_left;
    IFparseNode *ptmp = p->p_tree->treeParse(&ts);

    while (ptmp) {
        // Evaluate the argument, save result.
        if (!IFparseNode::is_const(ptmp)) {
            if (list_done)
                break;
            da.add(0.0);
            Errs()->add_error("%s function arg %d not constant", "PULSE",
                da.count());
            *error = E_BADPARM;
        }
        else {
            double r;
            int err = (ptmp->*ptmp->p_evfunc)(&r, 0, 0);
            if (err) {
                if (list_done)
                    break;
                da.add(0.0);
                Errs()->add_error("%s function arg %d eval failed: %s",
                    "PULSE", da.count(), Sp.ErrorShort(err));
                *error = E_BADPARM;
            }
            else {
                if (list_done)
                    break;
                da.add(r);
            }
        }

        // Link the argument into a normal argument list.  The parse nodes
        // won't be freed untill the tree is destroyed anyway, so may as
        // well link them.

        const char *ts0 = ts;
        IFparseNode *pnxt = p->p_tree->treeParse(&ts);
        if (pnxt) {
            if (!list_done) {
                *plast = p->p_tree->newBnode(PT_COMMA, ptmp, 0);
                plast = &(*plast)->p_right;
            }
            ptmp = pnxt;
            continue;
        }
        ts = ts0;
        // Strings return null on parse.

        if (!list_done)
            *plast = ptmp;
        break;
    }

    // We've parsed the numbers, look for bstrings.
    char *erstr;
    pbitList *list = pbitList::parse(&ts, &erstr);
    if (!list && erstr) {
        Errs()->add_error("%s function %s.", "PULSE", erstr);
        delete [] erstr;
        *error = E_BADPARM;
    }

    delete [] tt;
    p->v.td = new IFpulseData(da.final(), da.count(), list);
}


void
IFpulseData::setup(sCKT *ckt, double step, double finaltime, bool skipbr)
{
    // This is called with zeroed arguments to disable tran funcs while
    // not in transient analysis.
    //
    if (step > 0.0 && finaltime > 0.0) {
        if (td_enable_tran)
            setup(ckt, 0.0, 0.0, false);
        td_enable_tran = true;
    }
    else
        td_enable_tran = false;

    if (td_enable_tran) {
        if (TR() <= 0.0)
            set_TR(step);
        if (TF() > 0.0 && PW() <= 0.0)
            set_PW(0.0);
        else {
            if (TF() <= 0.0)
                set_TF(step);
            if (PW() <= 0.0)
                set_PW(MAXFLOAT);
        }
        if (PER() <= 0.0)
            set_PER(MAXFLOAT);
        else if (PER() < TR() + PW() + TF())
            set_PER(TR() + PW() + TF());

        // Reset these, may have changed.
        td_cache[0] = (V2() - V1())/TR();
        td_cache[1] = (V1() - V2())/TF();

        if (!skipbr && ckt) {
            if (PER() < finaltime) {
                double time = TD();
                ckt->breakSetLattice(time, PER());
                time += TR();
                ckt->breakSetLattice(time, PER());
                if (PW() != 0) {
                    time += PW();
                    ckt->breakSetLattice(time, PER());
                }
                if (TF() != 0) {
                    time += TF();
                    ckt->breakSetLattice(time, PER());
                }
            }
            else {
                double time = TD();
                ckt->breakSet(time);
                time += TR();
                ckt->breakSet(time);
                if (PW() != 0) {
                    time += PW();
                    ckt->breakSet(time);
                }
                if (TF() != 0) {
                    time += TF();
                    ckt->breakSet(time);
                }
            }
            // set for additional offsets
            for (int i = 7; i < td_numcoeffs; i++) {
                if (PER() < finaltime) {
                    double time = td_coeffs[i];
                    ckt->breakSetLattice(time, PER());
                    time += TR();
                    ckt->breakSetLattice(time, PER());
                    if (PW() != 0) {
                        time += PW();
                        ckt->breakSetLattice(time, PER());
                    }
                    if (TF() != 0) {
                        time += TF();
                        ckt->breakSetLattice(time, PER());
                    }
                }
                else {
                    double time = td_coeffs[i];
                    ckt->breakSet(time);
                    time += TR();
                    ckt->breakSet(time);
                    if (PW() != 0) {
                        time += PW();
                        ckt->breakSet(time);
                    }
                    if (TF() != 0) {
                        time += TF();
                        ckt->breakSet(time);
                    }
                }
            }
        }
        if (PER() < finaltime) {
            // Construct the pattern array.
            if (td_pblist) {
                pbitAry ba;
                ba.add(td_pblist);
                if (ba.count()) {
                    td_parray = ba.final();
                    td_plen = ba.count();
                    td_prst = ba.rep_start();
                }
            }
        }
    }
    else {
        memset(td_parms, 0, 8*sizeof(double));
        set_V1(td_coeffs[0]);
        set_V2(td_numcoeffs >= 2 ? td_coeffs[1] : td_coeffs[0]);
        set_TD(td_numcoeffs >= 3 ? td_coeffs[2] : 0.0);
        set_TR(td_numcoeffs >= 4 ? td_coeffs[3] : 0.0);
        set_TF(td_numcoeffs >= 5 ? td_coeffs[4] : 0.0);
        set_PW(td_numcoeffs >= 6 ? td_coeffs[5] : 0.0);
        set_PER(td_numcoeffs >= 7 ? td_coeffs[6] : 0.0);
        td_cache[0] = TR() > 0.0 ? (V2() - V1())/TR() : 0.0;
        td_cache[1] = TF() > 0.0 ? (V2() - V1())/TF() : 0.0;

        delete [] td_parray;
        td_parray = 0;
        td_plen = 0;
        td_prst = 0;
    }
}


double
IFpulseData::eval_func(double t)
{
    if (!td_enable_tran)
        return (V1());
    double value = V1();
    double pw = TR() + PW();
    double time = t - TD();
    bool skip = false;
    if (PER() > 0.0) {
        // Repeating signal - figure out where we are in period.
        int pnum = 0;
        if (time > PER()) {
            pnum = (int)(time/PER());
            time -= (PER() * pnum);
        }
        if (td_parray) {
            if (pnum >= td_plen) {
                if (!td_prst)
                    skip = true;
                else {
                    int d = td_plen - td_prst;
                    int j = td_prst + (pnum-td_prst)%d;
                    unsigned long mask =
                        (1 << (j % sizeof(unsigned long)));
                    skip = !(td_parray[j/sizeof(unsigned long)] & mask);
                }
            }
            else {
                unsigned long mask = (1 << (pnum % sizeof(unsigned long)));
                skip = !(td_parray[pnum/sizeof(unsigned long)] & mask);
            }
        }
    }
    if (!skip) {
        if (time < TR() || time >= pw + TF()) {
            value = V1();
            if (time > 0 && time < TR())
                value += time* *td_cache;
        }
        else {
            value = V2();
            if (time > pw)
                value += (time - pw)* *(td_cache+1);
        }
    }

    // New feature: additional entries are added  as in
    // pulse(V1 V2 TD ...) + pulse(0 V2-V1 TD1 ...) + ...
    // extra entries have offset subtracted.
    //
    for (int i = 7; i < td_numcoeffs; i++) {
        time = t - td_coeffs[i];
        skip = false;
        if (PER() > 0.0) {
            // Repeating signal - figure out where we are in period.
            int pnum = 0;
            if (time > PER()) {
                pnum = (int)(time/PER());
                time -= (PER() * pnum);
            }
            if (td_parray) {
                if (pnum >= td_plen) {
                    if (!td_prst)
                        skip = true;
                    else {
                        int d = td_plen - td_prst;
                        int j = td_prst + (pnum-td_prst)%d;
                        unsigned long mask =
                            (1 << (j % sizeof(unsigned long)));
                        skip = !(td_parray[j/sizeof(unsigned long)] & mask);
                    }
                }
                else {
                    unsigned long mask = (1 << (pnum % sizeof(unsigned long)));
                    skip = !(td_parray[pnum/sizeof(unsigned long)] & mask);
                }
            }
        }
        if (!skip) {
            if (time < TR() || time >= pw + TF()) {
                if (time > 0 && time < TR())
                    value += time* *td_cache;
            }
            else {
                value += (V2()-V1());
                if (time > pw)
                    value += (time - pw)* *(td_cache+1);
            }
        }
    }
    return (value);
}


double
IFpulseData::eval_deriv(double t)
{
    if (!td_enable_tran)
        return (0.0);
    double value = 0.0;
    double pw = TR() + PW();
    double time = t - TD();
    bool skip = false;
    if (PER() > 0.0) {
        // Repeating signal - figure out where we are in period.
        int pnum = 0;
        if (time > PER()) {
            pnum = (int)(time/PER());
            time -= (PER() * pnum);
        }
        if (td_parray) {
            if (pnum >= td_plen) {
                if (!td_prst)
                    skip = true;
                else {
                    int d = td_plen - td_prst;
                    int j = td_prst + (pnum-td_prst)%d;
                    unsigned long mask =
                        (1 << (j % sizeof(unsigned long)));
                    skip = !(td_parray[j/sizeof(unsigned long)] & mask);
                }
            }
            else {
                unsigned long mask = (1 << (pnum % sizeof(unsigned long)));
                skip = !(td_parray[pnum/sizeof(unsigned long)] & mask);
            }
        }
    }
    if (!skip) {
        if (time < TR() || time >= pw + TF()) {
            if (time > 0 && time < TR())
                value = *td_cache;
        }
        else {
            if (time > pw)
                value = *(td_cache+1);
        }
    }

    // New feature: additional entries are added  as in
    // pulse(V1 V2 TD ...) + pulse(0 V2-V1 TD1 ...) + ...
    // extra entries have offset subtracted.
    //
    for (int i = 7; i < td_numcoeffs; i++) {
        time = t - td_coeffs[i];
        skip = false;
        if (PER() > 0.0) {
            // Repeating signal - figure out where we are in period.
            int pnum = 0;
            if (time > PER()) {
                pnum = (int)(time/PER());
                time -= (PER() * pnum);
            }
            if (td_parray) {
                if (pnum >= td_plen) {
                    if (!td_prst)
                        skip = true;
                    else {
                        int d = td_plen - td_prst;
                        int j = td_prst + (pnum-td_prst)%d;
                        unsigned long mask =
                            (1 << (j % sizeof(unsigned long)));
                        skip = !(td_parray[j/sizeof(unsigned long)] & mask);
                    }
                }
                else {
                    unsigned long mask = (1 << (pnum % sizeof(unsigned long)));
                    skip = !(td_parray[pnum/sizeof(unsigned long)] & mask);
                }
            }
        }
        if (!skip) {
            if (time < TR() || time >= pw + TF()) {
                if (time > 0 && time < TR())
                    value = *td_cache;
            }
            else {
                if (time > pw)
                    value = *(td_cache+1);
            }
        }
    }
    return (value);
}


IFtranData *
IFpulseData::dup() const
{
    IFpulseData *td = new IFpulseData(*this);
    if (td_coeffs) {
        td->td_coeffs = new double[td_numcoeffs];
        memcpy(td->td_coeffs, td_coeffs, td_numcoeffs*sizeof(double));
    }
    if (td_parms) {
        td->td_parms = new double[8];
        memcpy(td->td_parms, td_parms, 8*sizeof(double));
    }
    if (td_cache) {
        td->td_cache = new double[2];
        memcpy(td->td_cache, td_cache, 2*sizeof(double));  
    }
    if (td_pblist) {
        td->td_pblist = pbitList::dup(td_pblist);
    }
    if (td_parray) {
        int sz = td_plen/sizeof(unsigned long) + td_plen%sizeof(unsigned long);
        td->td_parray = new unsigned long[sz];
        memcpy(td->td_parray, td_parray, sz*sizeof(unsigned long));
    }
    return (td);
}


// 0  V1    prm1
// 1  V2    prm2
// 2  TD    prm3
// 3  TR    prm4
// 4  TF    prm5
// 5  PW    prm6
// 6  PER   prm7
//
void
IFpulseData::set_param(double val, int ix)
{
    if (ix < 0 || ix > 6)
        return;
    td_coeffs[ix] = val;
    if (td_enable_tran)
        td_parms[ix] = val;
}


double *
IFpulseData::get_param(int ix)
{
    if (ix < 0 || ix > 6)
        return (0);
    return (td_coeffs + ix);
}
// End of IFpulseData functions.


// GPULSE
//
IFgpulseData::IFgpulseData(double *list, int num, pbitList *pbl) :
    IFtranData(PTF_tGPULSE)
{
    td_parms = new double[8];
    td_coeffs = list;
    td_numcoeffs = num;
    td_pblist = pbl;
    td_parray = 0;
    td_plen = 0;
    td_prst = 0;

    set_V1(list[0]);
    set_V2(num >= 2 ? list[1] : list[0]);
    set_TD(num >= 3 ? list[2] : 0.0);

    // New in 4.3.3, default is to take pulse width as FWHM, used
    // to just take this as the "variance".  However, if the value
    // is negative, use the old setup with the absolute value.
    set_GPW(num >= 4 ?
        (list[3] >= 0.0 ? list[3]/TWOSQRTLN2 : -list[3]) : 0.0);

    set_RPT(num >= 5 ? list[4] : 0.0);
}


// Static function
//
void
IFgpulseData::parse(const char *args, IFparseNode *p, int *error)
{
    // Make arguments space-separated (remove separating commas).
    char *tt = fix_tran_args(args, 0, 0);

    const char *ts = tt;
    dblAry da;
    bool list_done = false;

    IFparseNode **plast = &p->p_left;
    IFparseNode *ptmp = p->p_tree->treeParse(&ts);

    while (ptmp) {
        // Evaluate the argument, save result.
        if (!IFparseNode::is_const(ptmp)) {
            if (list_done)
                break;
            da.add(0.0);
            Errs()->add_error("%s function arg %d not constant", "GPULSE",
                da.count());
            *error = E_BADPARM;
        }
        else {
            double r;
            int err = (ptmp->*ptmp->p_evfunc)(&r, 0, 0);
            if (err) {
                if (list_done)
                    break;
                da.add(0.0);
                Errs()->add_error("%s function arg %d eval failed: %s",
                    "GPULSE", da.count(), Sp.ErrorShort(err));
                *error = E_BADPARM;
            }
            else {
                if (list_done)
                    break;
                da.add(r);
            }
        }

        // Link the argument into a normal argument list.  The parse nodes
        // won't be freed untill the tree is destroyed anyway, so may as
        // well link them.

        const char *ts0 = ts;
        IFparseNode *pnxt = p->p_tree->treeParse(&ts);
        if (pnxt) {
            if (!list_done) {
                *plast = p->p_tree->newBnode(PT_COMMA, ptmp, 0);
                plast = &(*plast)->p_right;
            }
            ptmp = pnxt;
            continue;
        }
        ts = ts0;
        // Strings return null on parse.

        if (!list_done)
            *plast = ptmp;
        break;
    }

    // We've parsed the numbers, look for bstrings.
    char *erstr;
    pbitList *list = pbitList::parse(&ts, &erstr);
    if (!list && erstr) {
        Errs()->add_error("%s function %s.", "GPULSE", erstr);
        delete [] erstr;
        *error = E_BADPARM;
    }

    delete [] tt;
    p->v.td = new IFgpulseData(da.final(), da.count(), list);
}


void
IFgpulseData::setup(sCKT *ckt, double step, double finaltime, bool skipbr)
{
    (void)skipbr;
    // This is called with zeroed arguments to disable tran funcs while
    // not in transient analysis.
    //
    if (step > 0.0 && finaltime > 0.0) {
        if (td_enable_tran)
            setup(ckt, 0.0, 0.0, false);
        td_enable_tran = true;
    }
    else
        td_enable_tran = false;

    if (td_enable_tran) {
        if (GPW() == 0.0) {

            // If no pulse width was given, new default in 4.3.3 is to
            // generate an SFQ pulse with given amplitude, or if the
            // amplitude is zero, use TSTEP as FWHM for SFQ.

            if (V2() != V1())
                set_GPW(PHI0_SQRTPI/fabs(V2() - V1()));
            else
                set_GPW(step/TWOSQRTLN2);
        }

        // If the pulse has zero amplitude, take it to be a single
        // flux quantum (SFQ) pulse.  This is a pulse that when
        // applied to an inductor will induce one quantum of flux (I
        // * L = the physical constant PHI0).  Such pulses are
        // encountered in superconducting electronics.

        if (V2() == V1())
            set_V2(V1() + PHI0_SQRTPI/GPW());

        if (RPT() > 0.0) {
            // Construct the pattern array.
            if (td_pblist) {
                pbitAry ba;
                ba.add(td_pblist);
                if (ba.count()) {
                    td_parray = ba.final();
                    td_plen = ba.count();
                    td_prst = ba.rep_start();
                }
            }
        }
    }
    else {
        memset(td_parms, 0, 8*sizeof(double));
        set_V1(td_coeffs[0]);
        set_V2(td_numcoeffs >= 2 ? td_coeffs[1] : td_coeffs[0]);
        set_TD(td_numcoeffs >= 3 ? td_coeffs[2] : 0.0);

        // New in 4.3.3, default is to take pulse width as FWHM, used
        // to just take this as the "variance".  However, if the value
        // is negative, use the old setup with the absolute value.
        set_GPW(td_numcoeffs >= 4 ? (td_coeffs[3] >= 0.0 ?
            td_coeffs[3]/TWOSQRTLN2 : -td_coeffs[3]) : 0.0);

        set_RPT(td_numcoeffs >= 5 ? td_coeffs[4] : 0.0);
        if (RPT() < 2*GPW())
            set_RPT(0.0);

        delete [] td_parray;
        td_parray = 0;
        td_plen = 0;
        td_prst = 0;
    }
}


double
IFgpulseData::eval_func(double t)
{
    if (!td_enable_tran)
        return (V1());
    double V = 0;
    const double rchk = 5.0;
    if (RPT() > 0.0) {
        double per = RPT();
        int ifirst, ilast;
        if (t < TD()) {
            ifirst = 0;
            ilast = 0;
        }
        else {
            double del = rchk*GPW();
            ifirst = (int)rint((t - TD() - del)/per);
            ilast = (int)rint((t - TD() + del)/per);
        }
        for (int i = ifirst; i <= ilast; i++) {
            bool skip = false;
            if (td_parray) {
                if (i >= td_plen) {
                    if (!td_prst)
                        skip = true;
                    else {
                        int d = td_plen - td_prst;
                        int j = td_prst + (i-td_prst)%d;
                        unsigned long mask =
                            (1 << (j % sizeof(unsigned long)));
                        skip = !(td_parray[j/sizeof(unsigned long)] & mask);
                    }
                }
                else {
                    unsigned long mask = (1 << (i % sizeof(unsigned long)));
                    skip = !(td_parray[i/sizeof(unsigned long)] & mask);
                }
            }
            if (!skip) {
                double time = t - i*per;
                double a = (time - TD())/GPW();
                V += exp(-a*a);
            }
        }
        for (int c = 5; c < td_numcoeffs; c++) {
            double td = td_coeffs[c];
            if (t < td) {
                ifirst = 0;
                ilast = 0;
            }
            else {
                double del = rchk*GPW();
                ifirst = (int)rint((t - td - del)/per);
                ilast = (int)rint((t - td + del)/per);
            }
            for (int i = ifirst; i <= ilast; i++) {
                bool skip = false;
                if (td_parray) {
                    if (i >= td_plen) {
                        if (!td_prst)
                            skip = true;
                        else {
                            int d = td_plen - td_prst;
                            int j = td_prst + (i-td_prst)%d;
                            unsigned long mask =
                                (1 << (j % sizeof(unsigned long)));
                            skip = !(td_parray[j/sizeof(unsigned long)] & mask);
                        }
                    }
                    else {
                        unsigned long mask = (1 << (i % sizeof(unsigned long)));
                        skip = !(td_parray[i/sizeof(unsigned long)] & mask);
                    }
                }
                if (!skip) {
                    double time = t - i*per;
                    double a = (time - td)/GPW();
                    V += exp(-a*a);
                }
            }
        }
    }
    else {
        double a = (t - TD())/GPW();
        if (fabs(a) < rchk)
            V += exp(-a*a);

        for (int i = 5; i < td_numcoeffs; i++) {
            a = (t - td_coeffs[i])/GPW();
            if (fabs(a) < rchk)
                V += exp(-a*a);
        }
    }
    return (V1() + (V2() - V1())*V);
}


double
IFgpulseData::eval_deriv(double t)
{
    if (!td_enable_tran)
        return (0.0);
    double V = 0;
    const double rchk = 5.0;
    if (RPT() > 0.0) {
        double per = RPT();
        int ifirst, ilast;
        if (t < TD()) {
            ifirst = 0;
            ilast = 0;
        }
        else {
            double del = rchk*GPW();
            ifirst = (int)rint((t - TD() - del)/per);
            ilast = (int)rint((t - TD() + del)/per);
        }
        for (int i = ifirst; i <= ilast; i++) {
            bool skip = false;
            if (td_parray) {
                if (i >= td_plen) {
                    if (!td_prst)
                        skip = true;
                    else {
                        int d = td_plen - td_prst;
                        int j = td_prst + (i-td_prst)%d;
                        unsigned long mask =
                            (1 << (j % sizeof(unsigned long)));
                        skip = !(td_parray[j/sizeof(unsigned long)] & mask);
                    }
                }
                else {
                    unsigned long mask = (1 << (i % sizeof(unsigned long)));
                    skip = !(td_parray[i/sizeof(unsigned long)] & mask);
                }
            }
            if (!skip) {
                double time = t - i*per;
                double a = (time - TD())/GPW();
                V += exp(-a*a)*(-2*a/GPW());
            }
        }
        for (int c = 5; c < td_numcoeffs; c++) {
            double td = td_coeffs[c];
            if (t < td) {
                ifirst = 0;
                ilast = 0;
            }
            else {
                double del = rchk*GPW();
                ifirst = (int)rint((t - td - del)/per);
                ilast = (int)rint((t - td + del)/per);
            }
            for (int i = ifirst; i <= ilast; i++) {
                bool skip = false;
                if (td_parray) {
                    if (i >= td_plen) {
                        if (!td_prst)
                            skip = true;
                        else {
                            int d = td_plen - td_prst;
                            int j = td_prst + (i-td_prst)%d;
                            unsigned long mask =
                                (1 << (j % sizeof(unsigned long)));
                            skip = !(td_parray[j/sizeof(unsigned long)] & mask);
                        }
                    }
                    else {
                        unsigned long mask = (1 << (i % sizeof(unsigned long)));
                        skip = !(td_parray[i/sizeof(unsigned long)] & mask);
                    }
                }
                if (!skip) {
                    double time = t - i*per;
                    double a = (time - td)/GPW();
                    V += exp(-a*a)*(-2*a/GPW());
                }
            }
        }
    }
    else {
        double a = (t - TD())/GPW();
        if (fabs(a) < rchk)
            V += exp(-a*a)*(-2*a/GPW());

        for (int i = 5; i < td_numcoeffs; i++) {
            a = (t - td_coeffs[i])/GPW();
            if (fabs(a) < rchk)
                V += exp(-a*a)*(-2*a/GPW());
        }
    }
    return ((V2() - V1())*V);
}


IFtranData *
IFgpulseData::dup() const
{
    IFgpulseData *td = new IFgpulseData(*this);
    if (td_coeffs) {
        td->td_coeffs = new double[td_numcoeffs];
        memcpy(td->td_coeffs, td_coeffs, td_numcoeffs*sizeof(double));
    }
    if (td_parms) {
        td->td_parms = new double[8];
        memcpy(td->td_parms, td_parms, 8*sizeof(double));
    }
    if (td_pblist) {
        td->td_pblist = pbitList::dup(td_pblist);
    }
    if (td_parray) {
        int sz = td_plen/sizeof(unsigned long) + td_plen%sizeof(unsigned long);
        td->td_parray = new unsigned long[sz];
        memcpy(td->td_parray, td_parray, sz*sizeof(unsigned long));
    }
    return (td);
}


// 0  V1    prm1
// 1  V2    prm2
// 2  TD    prm3
// 3  GPW   prm4
// 4  RPT   prm5
//
void
IFgpulseData::set_param(double val, int ix)
{
    if (ix < 0 || ix > 4)
        return;
    td_coeffs[ix] = val;
    if (td_enable_tran)
        td_parms[ix] = val;
}


double *
IFgpulseData::get_param(int ix)
{
    if (ix < 0 || ix > 4)
        return (0);
    return (td_coeffs + ix);
}
// End of IFgpulseData functions.


// PWL
//
IFpwlData::IFpwlData(double *list, int num, bool hasR , int Rix, double TDval) :
IFtranData(PTF_tPWL)
{
    td_pwlRgiven = hasR;
    td_pwlRstart = Rix;
    td_pwlindex = 0;
    td_pwldelay = TDval;

    // Add the delay, if any.
    if (td_pwldelay > 0.0) {
        for (int i = 0; i < num; i += 2)
            list[i] += td_pwldelay;
    }

    // If the first ordinate is larger than 0, add a 0,v1
    // pair.  If the length is odd, add a value at the end.

    bool add0 = (list[0] > 0.0);
    bool add1 = (num & 1);
    if (add0 || add1) {
        int nnum = num + 2*add0 + add1;
        double *nlst = new double[nnum];
        double *d = nlst;
        if (add0) {
            *d++ = 0.0;
            *d++ = num > 1 ? list[1] : 0.0;
        }
        for (int i = 0; i < num; i++)
            *d++ = list[i];
        if (add1) {
            if (num > 1)
                *d = *(d-2);
            else
                *d = 0.0;
        }
        num = nnum;
        delete [] list;
        list = nlst;
    }
    td_coeffs = list;

    if (add0 && td_pwlRgiven)
        td_pwlRstart++;

    // Throw out pairs that are not monotonic in first value.
    bool foo = false;
    for (int i = 2; i < num; ) {
        if (td_coeffs[i] <= td_coeffs[i-2]) {
            // monotonicity error
            for (int k = i; k+2 < num; k += 2) {
                td_coeffs[k] = td_coeffs[k+2];
                td_coeffs[k+1] = td_coeffs[k+3];
            }
            num -= 2;
            foo = true;
            continue;
        }
        i += 2;
    }
    if (foo) {
        IP.logError(IP_CUR_LINE,
            "PWL list not monotonic, some points were rejected.");
    }
    td_numcoeffs = num;

    // Cache the slopes.
    int i = num/2 - 1;
    if (i > 0) {
        td_cache = new double[i + td_pwlRgiven];
        for (int k = 0; k < i; k++) {
            td_cache[k] =
                (td_coeffs[2*k+3] - td_coeffs[2*k+1])/
                (td_coeffs[2*k+2] - td_coeffs[2*k]);
        }
        if (td_pwlRgiven) {
            // We need one more slope value.  In our scheme,
            // the Rstart point is "mapped" to the final list
            // point, so that the next value is the one
            // following Rstart.

            int k = td_pwlRstart;
            td_cache[i] =
                (td_coeffs[2*k+3] - td_coeffs[2*i+1])/
                (td_coeffs[2*k+2] - td_coeffs[2*k]);
        }
    }
    else {
        td_cache = new double;
        *td_cache = 0.0;
    }
}


// Static function
//
void
IFpwlData::parse(const char *args, IFparseNode *p, int *error)
{
    // Make arguments space-separated (remove separating commas).
    char *tt = fix_tran_args(args, 0, 0);

    // The parser will gag on an equal sign in, e.g., R=tn,
    // convert to spaces.
    for (char *tx = tt; *tx; tx++) {
        if (*tx == '=')
            *tx = ' ';
    }

    const char *ts = tt;
    dblAry da;
    bool list_done = false;
    bool hasR = false;
    bool lookingR = false;
    bool lookingTD = false;
    double Rval = 0.0;
    double TDval = 0.0;

    IFparseNode **plast = &p->p_left;
    IFparseNode *ptmp = p->p_tree->treeParse(&ts);

    for (;;) {
        if (!ptmp) {
            if (!list_done) {
                // Here is support for a PWL option where the points
                // list is supplied in named vectors.
                //
                //   PWL vec1 [vec2] [R=xxx] [TD=xxx]
                //
                // If one vector name is given, that vector is
                // expected to contain interleaved values in the order
                // normally given explicitly.  If two vectors are
                // given, the first provides the "time" values, the
                // second supplies the amplitudes.  The first vector
                // is likely the scale for the second.

                char *tok = IP.getTok(&ts, true);
                if (tok) {
                    sDataVec *d1 = OP.vecGet(tok, p->p_tree->ckt());
                    if (!d1) {
                        Errs()->add_error("Unknown vector %s.", tok);
                        delete [] tok;
                        *error = E_NOVEC;
                        p->v.td = new IFpwlData(0, 0, false, 0, 0.0);
                        return;
                    }
                    sDataVec *d2 = 0;
                    delete [] tok;
                    const char *ts0 = ts;
                    tok = IP.getTok(&ts, true);
                    if (tok) {
                        if (!lstring::cieq(tok, "r") &&
                                !lstring::cieq(tok, "td")) {
                            d2 = OP.vecGet(tok, p->p_tree->ckt());
                            if (!d2) {
                                Errs()->add_error("Unknown vector %s.", tok);
                                delete [] tok;
                                *error = E_NOVEC;
                                p->v.td = new IFpwlData(0, 0, false, 0, 0.0);
                                return;
                            }
                        }
                        else
                            ts = ts0;
                        delete [] tok;
                    }
                    if (d2) {
                        int len = d1->length();
                        if (d2->length() < len)
                            len = d2->length();
                        for (int i = 0; i < len; i++) {
                            da.add(d1->realval(i));
                            da.add(d2->realval(i));
                        }
                    }
                    else {
                        for (int i = 0; i < d1->length(); i++)
                            da.add(d1->realval(i));
                    }
                    list_done = true;
                    goto again;
                }
            }
            break;
        }
        else {
            // Evaluate the argument, save result.
            if (!IFparseNode::is_const(ptmp)) {
                if (lookingR) {
                    lookingR = false;
                    Errs()->add_error("PWL function R val not constant.");
                }
                else if (lookingTD) {
                    lookingTD = false;
                    Errs()->add_error("PWL function TD val not constant.");
                }
                else if (list_done)
                    break;
                else {
                    da.add(0.0);
                    Errs()->add_error("%s function arg %d not constant",
                        "PWL", da.count());
                }
                *error = E_BADPARM;
            }
            else {
                double r;
                int err = (ptmp->*ptmp->p_evfunc)(&r, 0, 0);
                if (err) {
                    if (lookingR) {
                        lookingR = false;
                        Errs()->add_error("PWL function R eval failed: %s",
                            Sp.ErrorShort(err));
                    }
                    else if (lookingTD) {
                        lookingTD = false;
                        Errs()->add_error("PWL function TD eval failed: %s",
                            Sp.ErrorShort(err));
                    }
                    else if (list_done)
                        break;
                    else {
                        da.add(0.0);
                        Errs()->add_error("%s function arg %d eval failed: %s",
                            "PWL", da.count(), Sp.ErrorShort(err));
                    }
                    *error = E_BADPARM;
                }
                else {
                    if (lookingR) {
                        Rval = r;
                        lookingR = false;
                    }
                    else if (lookingTD) {
                        TDval = r;
                        lookingTD = false;
                    }
                    else if (list_done)
                        break;
                    else
                        da.add(r);
                }
            }
        }

        // Link the argument into a normal argument list.  The parse nodes
        // won't be freed untill the tree is destroyed anyway, so may as
        // well link them.  The PWL R,TD values aren't linked.

again:
        const char *ts0 = ts;
        IFparseNode *pnxt = p->p_tree->treeParse(&ts);
        if (pnxt) {
            if (!list_done) {
                *plast = p->p_tree->newBnode(PT_COMMA, ptmp, 0);
                plast = &(*plast)->p_right;
            }
            ptmp = pnxt;
            continue;
        }
        // The R and TD literals return null on parse.
        ts = ts0;
        char *tok = IP.getTok(&ts, true);
        if (tok) {
            // Handle the R and TD tokens,  Either terminates the
            // pair list.

            if (lstring::cieq(tok, "r")) {
                hasR = true;
                lookingR = true;
                lookingTD = false;
                list_done = true;
            }
            else if (lstring::cieq(tok, "td")) {
                lookingR = false;
                lookingTD = true;
                list_done = true;
            }
            delete [] tok;
            if (lookingR || lookingTD)
                goto again;
        }

        if (!list_done)
            *plast = ptmp;
        break;
    }

    delete [] tt;
    double *list = da.final();
    int num = da.count();

    int Rix = -2;
    if (hasR) {
        if (!feq(Rval, list[0]) && Rval < list[0])
            Rix = -1;
        else { 
            for (int j = 0; j < num-1; j++) {
                if (feq(Rval, list[2*j])) {
                    Rix = j;
                    break;
                }
            }
        }
        if (Rix == -2) {
            *error = E_BADPARM;
            Errs()->add_error(
                "Bad value for R, doesn't match a PWL time value.");
        }
    }
    p->v.td = new IFpwlData(list, num, hasR, Rix, TDval);
}


void
IFpwlData::setup(sCKT *ckt, double step, double finaltime, bool skipbr)
{
    // This is called with zeroed arguments to disable tran funcs while
    // not in transient analysis.
    //
    if (step > 0.0 && finaltime > 0.0) {
        if (td_enable_tran)
            setup(ckt, 0.0, 0.0, false);
        td_enable_tran = true;
    }
    else
        td_enable_tran = false;

    td_pwlindex = 0;
    if (td_enable_tran && !skipbr && ckt) {
        // Only call this when time is independent variable.

        int n = td_numcoeffs/2;
        double ta = 0.0;
        int istart = 0;
        for (;;) {
            for (int i = istart; i < n; i++) {
                double t = td_coeffs[2*i] + ta;
                if (t > finaltime)
                    return;
                ckt->breakSet(t);
#ifdef DEBUG
                printf("B %g\n", t);
#endif
            }
            if (!td_pwlRgiven)
                break;
            ta += td_coeffs[2*(n-1)] - td_coeffs[2*td_pwlRstart];
            istart = td_pwlRstart + 1;
        }
    }
}


namespace {
    struct twovals { double x; double y; };
}

double
IFpwlData::eval_func(double t)
{
    // Does not requires that setup be called, t is voltage in DC sweep.
    twovals *tv = (twovals*)td_coeffs;
    if (!tv)
        return (0.0);
    int nvals = td_numcoeffs/2;

    if (t >= tv[nvals-1].x) {
        if (!td_pwlRgiven)
            return (tv[nvals-1].y);
        t -= tv[nvals-1].x;
        double ts = tv[td_pwlRstart].x;
        double dt = tv[nvals-1].x - ts;
        double tx;
        while ((tx = t - dt) > 0.0)
            t = tx;
        t += ts;
        if (t <= tv[td_pwlRstart+1].x)
            return (tv[nvals-1].y + td_cache[nvals-1]*(t - ts));
    }

    int i = td_pwlindex;
    while (i + 1 < nvals) {
        if (t <= tv[i+1].x)
            break;
        i++;
    }
    while (t < tv[i].x && i)
            i--;
    td_pwlindex = i;
    if (!i && t <= tv[0].x)
        return (tv[0].y);
    return (tv[i].y + td_cache[i]*(t - tv[i].x));
}


double
IFpwlData::eval_deriv(double t)
{
    // Does not requires that setup be called, t is voltage in DC sweep.
    twovals *tv = (twovals*)td_coeffs;
    if (!tv)
        return (0.0);
    int nvals = td_numcoeffs/2;

    if (t >= tv[nvals-1].x) {
        if (!td_pwlRgiven)
            return (0.0);
        t -= tv[nvals-1].x;
        double ts = tv[td_pwlRstart].x;
        double dt = tv[nvals-1].x - ts;
        double tx;
        while ((tx = t - dt) > 0.0)
            t = tx;
        t += ts;
        if (t <= tv[td_pwlRstart+1].x)
            return (td_cache[nvals-1]);
    }

    int i = td_pwlindex;
    while (i + 1 < nvals) {
        if (t <= tv[i+1].x)
            break;
        i++;
    }
    while (t < tv[i].x && i)
            i--;
    td_pwlindex = i;
    if (!i && t <= tv[0].x)
        return (0.0);
    return (td_cache[i]);
}


IFtranData *
IFpwlData::dup() const
{
    IFpwlData *td = new IFpwlData(*this);
    if (td_coeffs) {
        td->td_coeffs = new double[td_numcoeffs];
        memcpy(td->td_coeffs, td_coeffs, td_numcoeffs*sizeof(double));
    }
    if (td_parms) {
        td->td_parms = new double[8];
        memcpy(td->td_parms, td_parms, 8*sizeof(double));
    }
    // Make sure to copy the extra location for R.
    int num = td->td_numcoeffs/2;
    td->td_cache = new double[num];
    memcpy(td->td_cache, td_cache, num*sizeof(double));
    return (td);
}


void
IFpwlData::print(const char *name, sLstr &lstr)
{
    lstr.add(name);
    lstr.add_c('(');
    if (td_pwldelay > 0.0) {
        for (int i = 0; i < td_numcoeffs; i++) {
            if (i == 0 && td_coeffs[i] == 0.0) {
                i++;
                continue;
            }
            lstr.add_c(' ');
            lstr.add_g(td_coeffs[i] - td_pwldelay);
        }
    }
    else {
        for (int i = 0; i < td_numcoeffs; i++) {
            lstr.add_c(' ');
            lstr.add_g(td_coeffs[i]);
        }
    }
    if (td_pwlRgiven) {
        lstr.add(" R ");
        double r = 0.0;
        if (td_pwlRstart >= 0) {
            r = td_coeffs[2*td_pwlRstart];
            if (r > 0.0 && td_pwldelay > 0.0)
                r -= td_pwldelay;
        }
        if (r < 0.0)
            r = 0.0;
        lstr.add_g(r);
    }
    if (td_pwldelay > 0.0) {
        lstr.add(" TD ");
        lstr.add_g(td_pwldelay);
    }
    lstr.add_c(' ');
    lstr.add_c(')');
}
// End of IFpwlData functions.


// SIN
//
IFsinData::IFsinData(double *list, int num) : IFtranData(PTF_tSIN)
{
    td_parms = new double[8];
    td_coeffs = list;
    td_numcoeffs = num;

    set_VO(list[0]);
    set_VA(num >= 2 ? list[1] : 0.0);
    set_FREQ(num >= 3 ? list[2] : 0.0);
    set_TDL(num >= 4 ? list[3] : 0.0);
    set_THETA(num >= 5 ? list[4] : 0.0);
    set_PHI(num >= 6 ? list[5] : 0.0);
}


// Static function
//
void
IFsinData::parse(const char *args, IFparseNode *p, int *error)
{
    // Make arguments space-separated (remove separating commas).
    char *tt = fix_tran_args(args, 0, 0);

    const char *ts = tt;
    dblAry da;
    bool list_done = false;

    IFparseNode **plast = &p->p_left;
    IFparseNode *ptmp = p->p_tree->treeParse(&ts);

    while (ptmp) {
        // Evaluate the argument, save result.
        if (!IFparseNode::is_const(ptmp)) {
            if (list_done)
                break;
            da.add(0.0);
            Errs()->add_error("%s function arg %d not constant", "SIN",
                da.count());
            *error = E_BADPARM;
        }
        else {
            double r;
            int err = (ptmp->*ptmp->p_evfunc)(&r, 0, 0);
            if (err) {
                if (list_done)
                    break;
                da.add(0.0);
                Errs()->add_error("%s function arg %d eval failed: %s",
                    "SIN", da.count(), Sp.ErrorShort(err));
                *error = E_BADPARM;
            }
            else {
                if (list_done)
                    break;
                da.add(r);
            }
        }

        // Link the argument into a normal argument list.  The parse nodes
        // won't be freed untill the tree is destroyed anyway, so may as
        // well link them.

        IFparseNode *pnxt = p->p_tree->treeParse(&ts);
        if (pnxt) {
            if (!list_done) {
                *plast = p->p_tree->newBnode(PT_COMMA, ptmp, 0);
                plast = &(*plast)->p_right;
            }
            ptmp = pnxt;
            continue;
        }

        if (!list_done)
            *plast = ptmp;
        break;
    }

    delete [] tt;
    p->v.td = new IFsinData(da.final(), da.count());
}


void
IFsinData::setup(sCKT *ckt, double step, double finaltime, bool skipbr)
{
    // This is called with zeroed arguments to disable tran funcs while
    // not in transient analysis.
    //
    if (step > 0.0 && finaltime > 0.0) {
        if (td_enable_tran)
            setup(ckt, 0.0, 0.0, false);
        td_enable_tran = true;
    }
    else
        td_enable_tran = false;

    if (td_enable_tran) {
        if (!FREQ())
            set_FREQ(1.0/finaltime);
        if (!skipbr) {
            if (TDL() > 0.0)
                ckt->breakSet(TDL());
        }
    }
    else {
        memset(td_parms, 0, 8*sizeof(double));
        set_VO(td_coeffs[0]);
        set_VA(td_numcoeffs >= 2 ? td_coeffs[1] : 0.0);
        set_FREQ(td_numcoeffs >= 3 ? td_coeffs[2] : 0.0);
        set_TDL(td_numcoeffs >= 4 ? td_coeffs[3] : 0.0);
        set_THETA(td_numcoeffs >= 5 ? td_coeffs[4] : 0.0);
        set_PHI(td_numcoeffs >= 6 ? td_coeffs[5] : 0.0);
    }
}


double
IFsinData::eval_func(double t)
{
    if (!td_enable_tran) {
        if (PHI() != 0.0)
            return VO() + VA()*sin(M_PI*(PHI()/180));
        return (VO());
    }
    double time = t - TDL();
    if (time <= 0) {
        if (PHI() != 0.0)
            return VO() + VA()*sin(M_PI*(PHI()/180));
        return (VO());
    }
    double a;
    if (PHI() != 0.0)
        a = VA()*sin(2*M_PI*(FREQ()*time + (PHI()/360)));
    else
        a = VA()*sin(2*M_PI*FREQ()*time);
    if (THETA() != 0.0)
        a *= exp(-time*THETA());
    return (VO() + a);
}


double
IFsinData::eval_deriv(double t)
{
    if (!td_enable_tran)
        return (0);
    double time = t - TDL();
    if (time <= 0)
        return (0);
    double w = FREQ()*2*M_PI;
    if (PHI() != 0.0) {
        double phi = M_PI*(PHI()/180);
        if (THETA() != 0.0) {
            return (VA()*(w*cos(w*time + phi) -
                THETA()*sin(w*time + phi))*exp(-time*THETA()));
        }
        return (VA()*w*cos(w*time + phi));
    }
    if (THETA() != 0.0)
        return (VA()*(w*cos(w*time) - THETA()*sin(w*time))*exp(-time*THETA()));
    return (VA()*w*cos(w*time));
}


IFtranData *
IFsinData::dup() const
{
    IFsinData *td = new IFsinData(*this);
    if (td_coeffs) {
        td->td_coeffs = new double[td_numcoeffs];
        memcpy(td->td_coeffs, td_coeffs, td_numcoeffs*sizeof(double));
    }
    if (td_parms) {
        td->td_parms = new double[8];
        memcpy(td->td_parms, td_parms, 8*sizeof(double));
    }
    return (td);
}


void
IFsinData::time_limit(const sCKT *ckt, double *plim)
{
    if (!ckt || !plim)
        return;
    if (ckt->CKTtime >= TDL()) {
        double per = ckt->CKTcurTask->TSKdphiMax/(2*M_PI*FREQ());
        if (*plim > per)
            *plim = per;
    }
}


// 0  VO    prm1
// 1  VA    prm2
// 2  FREQ  prm3
// 3  TDL   prm4
// 4  THETA prm5
// 5  PHI   prm6
//
void
IFsinData::set_param(double val, int ix)
{
    if (ix < 0 || ix > 5)
        return;
    td_coeffs[ix] = val;
    if (td_enable_tran)
        td_parms[ix] = val;
}


double *
IFsinData::get_param(int ix)
{
    if (ix < 0 || ix > 5)
        return (0);
    return (td_coeffs + ix);
}
// End of IFsinData functions.


// SPULSE
//
IFspulseData::IFspulseData(double *list, int num) : IFtranData(PTF_tSPULSE)
{
    td_parms = new double[8];
    td_coeffs = list;
    td_numcoeffs = num;

    set_V1(list[0]);
    set_V2(num >= 2 ? list[1] : list[0]);
    set_SPER(num >= 3 ? list[2] : 0.0);
    set_SDEL(num >= 4 ? list[3] : 0.0);
    set_THETA(num >= 5 ? list[4] : 0.0);
}


// Static function
//
void
IFspulseData::parse(const char *args, IFparseNode *p, int *error)
{
    // Make arguments space-separated (remove separating commas).
    char *tt = fix_tran_args(args, 0, 0);

    const char *ts = tt;
    dblAry da;
    bool list_done = false;

    IFparseNode **plast = &p->p_left;
    IFparseNode *ptmp = p->p_tree->treeParse(&ts);

    while (ptmp) {
        // Evaluate the argument, save result.
        if (!IFparseNode::is_const(ptmp)) {
            if (list_done)
                break;
            da.add(0.0);
            Errs()->add_error("%s function arg %d not constant", "SPULSE",
                da.count());
            *error = E_BADPARM;
        }
        else {
            double r;
            int err = (ptmp->*ptmp->p_evfunc)(&r, 0, 0);
            if (err) {
                if (list_done)
                    break;
                da.add(0.0);
                Errs()->add_error("%s function arg %d eval failed: %s",
                    "SPULSE", da.count(), Sp.ErrorShort(err));
                *error = E_BADPARM;
            }
            else {
                if (list_done)
                    break;
                da.add(r);
            }
        }

        // Link the argument into a normal argument list.  The parse nodes
        // won't be freed untill the tree is destroyed anyway, so may as
        // well link them.

        IFparseNode *pnxt = p->p_tree->treeParse(&ts);
        if (pnxt) {
            if (!list_done) {
                *plast = p->p_tree->newBnode(PT_COMMA, ptmp, 0);
                plast = &(*plast)->p_right;
            }
            ptmp = pnxt;
            continue;
        }

        if (!list_done)
            *plast = ptmp;
        break;
    }

    delete [] tt;
    p->v.td = new IFspulseData(da.final(), da.count());
}


void
IFspulseData::setup(sCKT *ckt, double step, double finaltime, bool skipbr)
{
    // This is called with zeroed arguments to disable tran funcs while
    // not in transient analysis.
    //
    if (step > 0.0 && finaltime > 0.0) {
        if (td_enable_tran)
            setup(ckt, 0.0, 0.0, false);
        td_enable_tran = true;
    }
    else
        td_enable_tran = false;

    if (td_enable_tran) {
        if (!SPER())
            set_SPER(finaltime);
        if (!skipbr && ckt) {
            if (SDEL() > 0.0)
                ckt->breakSet(SDEL());
        }
    }
    else {
        memset(td_parms, 0, 8*sizeof(double));
        set_V1(td_coeffs[0]);
        set_V2(td_numcoeffs >= 2 ? td_coeffs[1] : td_coeffs[0]);
        set_SPER(td_numcoeffs >= 3 ? td_coeffs[2] : 0.0);
        set_SDEL(td_numcoeffs >= 4 ? td_coeffs[3] : 0.0);
        set_THETA(td_numcoeffs >= 5 ? td_coeffs[4] : 0.0);
    }
}


double
IFspulseData::eval_func(double t)
{
    if (!td_enable_tran)
        return (V1());
    double time = t - SDEL();
    if (time <= 0)
        return (V1());
    if (THETA() != 0.0) {
        return (V1() + (V2()-V1())*( 1 -
            cos(2*M_PI*time/SPER())*exp(-time*THETA()) )/2);
    }
    return (V1() + (V2()-V1())*( 1 - cos(2*M_PI*time/SPER()) )/2);
}


double
IFspulseData::eval_deriv(double t)
{
    if (!td_enable_tran)
        return (0.0);
    double time = t - SDEL();
    if (time <= 0)
        return (0);
    double w = 2*M_PI/SPER();
    if (THETA() != 0.0) {
        return ((V2()-V1())*(w*sin(w*time) +
            THETA()*cos(w*time))*exp(-time*THETA())/2);
    }
    return ((V2()-V1())*w*sin(w*time)/2);
}


IFtranData *
IFspulseData::dup() const
{
    IFspulseData *td = new IFspulseData(*this);
    if (td_coeffs) {
        td->td_coeffs = new double[td_numcoeffs];
        memcpy(td->td_coeffs, td_coeffs, td_numcoeffs*sizeof(double));
    }
    if (td_parms) {
        td->td_parms = new double[8];
        memcpy(td->td_parms, td_parms, 8*sizeof(double));
    }
    return (td);
}


void 
IFspulseData::time_limit(const sCKT *ckt, double *plim)
{
    if (!ckt || !plim)
        return;
    if (ckt->CKTtime >= SDEL()) {
        double per = ckt->CKTcurTask->TSKdphiMax*SPER()/(2*M_PI);
        if (*plim > per)
            *plim = per;
    }
}


// 0  V1    prm1
// 1  V2    prm2
// 2  SPER  prm3
// 3  SDEL  prm4
// 4  THETA prm5
//
void
IFspulseData::set_param(double val, int ix)
{
    if (ix < 0 || ix > 4)
        return;
    td_coeffs[ix] = val;
    if (td_enable_tran)
        td_parms[ix] = val;
}


double *
IFspulseData::get_param(int ix)
{
    if (ix < 0 || ix > 4)
        return (0);
    return (td_coeffs + ix);
}
// End of IFspulseData functions.


// EXP
//
IFexpData::IFexpData(double *list, int num) : IFtranData(PTF_tEXP)
{
    td_parms = new double[8];
    td_coeffs = list;
    td_numcoeffs = num;

    set_V1(list[0]);
    set_V2(num >= 2 ? list[1] : list[0]);
    set_TD1(num >= 3 ? list[2] : 0.0);
    set_TAU1(num >= 4 ? list[3] : 0.0);
    set_TD2(num >= 5 ? list[4] : 0.0);
    set_TAU2(num >= 6 ? list[5] : 0.0);
}


// Static function
//
void
IFexpData::parse(const char *args, IFparseNode *p, int *error)
{
    // Make arguments space-separated (remove separating commas).
    char *tt = fix_tran_args(args, 0, 0);

    const char *ts = tt;
    dblAry da;
    bool list_done = false;

    IFparseNode **plast = &p->p_left;
    IFparseNode *ptmp = p->p_tree->treeParse(&ts);

    while (ptmp) {
        // Evaluate the argument, save result.
        if (!IFparseNode::is_const(ptmp)) {
            if (list_done)
                break;
            da.add(0.0);
            Errs()->add_error("%s function arg %d not constant", "EXP",
                da.count());
            *error = E_BADPARM;
        }
        else {
            double r;
            int err = (ptmp->*ptmp->p_evfunc)(&r, 0, 0);
            if (err) {
                if (list_done)
                    break;
                da.add(0.0);
                Errs()->add_error("%s function arg %d eval failed: %s",
                    "EXP", da.count(), Sp.ErrorShort(err));
                *error = E_BADPARM;
            }
            else {
                if (list_done)
                    break;
                da.add(r);
            }
        }

        // Link the argument into a normal argument list.  The parse nodes
        // won't be freed untill the tree is destroyed anyway, so may as
        // well link them.

        IFparseNode *pnxt = p->p_tree->treeParse(&ts);
        if (pnxt) {
            if (!list_done) {
                *plast = p->p_tree->newBnode(PT_COMMA, ptmp, 0);
                plast = &(*plast)->p_right;
            }
            ptmp = pnxt;
            continue;
        }

        if (!list_done)
            *plast = ptmp;
        break;
    }

    delete [] tt;
    p->v.td = new IFexpData(da.final(), da.count());
}


void
IFexpData::setup(sCKT *ckt, double step, double finaltime, bool skipbr)
{
    // This is called with zeroed arguments to disable tran funcs while
    // not in transient analysis.
    //
    if (step > 0.0 && finaltime > 0.0) {
        if (td_enable_tran)
            setup(ckt, 0.0, 0.0, false);
        td_enable_tran = true;
    }
    else
        td_enable_tran = false;

    if (td_enable_tran) {
        if (!TAU2())
            set_TAU2(step);
        if (!TD1())
            set_TD1(step);
        if (!TAU1())
            set_TAU1(step);
        if (!TD2())
            set_TD2(TD1() + step);
        if (!skipbr && ckt) {
            if (TD1() > 0.0)
                ckt->breakSet(TD1());
            if (TD2() > TD1())
                ckt->breakSet(TD2());
        }
    }
    else {
        memset(td_parms, 0, 8*sizeof(double));
        set_V1(td_coeffs[0]);
        set_V2(td_numcoeffs >= 2 ? td_coeffs[1] : td_coeffs[0]);
        set_TD1(td_numcoeffs >= 3 ? td_coeffs[2] : 0.0);
        set_TAU1(td_numcoeffs >= 4 ? td_coeffs[3] : 0.0);
        set_TD2(td_numcoeffs >= 5 ? td_coeffs[4] : 0.0);
        set_TAU2(td_numcoeffs >= 6 ? td_coeffs[5] : 0.0);
    }
}


double
IFexpData::eval_func(double t)
{
    if (!td_enable_tran)
        return (V1());
    double value = V1();
    if (t <= TD1())
        return (value);
    value += (V2()-V1())*(1-exp(-(t-TD1())/TAU1()));
    if (t <= TD2())
        return (value);
    value += (V1()-V2())*(1-exp(-(t-TD2())/TAU2()));
    return (value);
}


double
IFexpData::eval_deriv(double t)
{
    if (!td_enable_tran)
        return (0.0);
    double value = 0.0;
    if (t <= TD1())
        return (value);
    value += (V2()-V1())*exp(-(t-TD1())/TAU1())/TAU1();
    if (t <= TD2())
        return (value);
    value += (V1()-V2())*exp(-(t-TD2())/TAU2())/TAU2();
    return (value);
}


IFtranData *
IFexpData::dup() const
{
    IFexpData *td = new IFexpData(*this);
    if (td_coeffs) {
        td->td_coeffs = new double[td_numcoeffs];
        memcpy(td->td_coeffs, td_coeffs, td_numcoeffs*sizeof(double));
    }
    if (td_parms) {
        td->td_parms = new double[8];
        memcpy(td->td_parms, td_parms, 8*sizeof(double));
    }
    return (td);
}


void
IFexpData::time_limit(const sCKT *ckt, double *plim)
{
    if (!ckt || !plim)
        return;
    double t = ckt->CKTtime;
    double f = ckt->CKTcurTask->TSKdphiMax/(2*M_PI);
    if (t >= TD1() && t < TD1() + 5.0*TAU1()) {
        double per = TAU1()*f*exp((t - TD1())/TAU1());
        if (*plim > per)
            *plim = per;
    }
    if (t >= TD2() && t < TD2() + 5.0*TAU2()) {
        double per = TAU2()*f*exp((t - TD2())/TAU2());
        if (*plim > per)
            *plim = per;
    }
}


// 0  V1    prm1
// 1  V2    prm2
// 2  TD1   prm3
// 3  TAU1  prm4
// 4  TD2   prm5
// 5  TAU2  prm6
//
void
IFexpData::set_param(double val, int ix)
{
    if (ix < 0 || ix > 5)
        return;
    td_coeffs[ix] = val;
    if (td_enable_tran)
        td_parms[ix] = val;
}


double *
IFexpData::get_param(int ix)
{
    if (ix < 0 || ix > 5)
        return (0);
    return (td_coeffs + ix);
}
// End of IFexpData functions.


// SFFM
//
IFsffmData::IFsffmData(double *list, int num) : IFtranData(PTF_tSFFM)
{
    td_parms = new double[8];
    td_coeffs = list;
    td_numcoeffs = num;

    set_VO(list[0]);
    set_VA(num >= 2 ? list[1] : 0.0);
    set_FC(num >= 3 ? list[2] : 0.0);
    set_MDI(num >= 4 ? list[3] : 0.0);
    set_FS(num >= 5 ? list[4] : 0.0);
}


// Static function
//
void
IFsffmData::parse(const char *args, IFparseNode *p, int *error)
{
    // Make arguments space-separated (remove separating commas).
    char *tt = fix_tran_args(args, 0, 0);

    const char *ts = tt;
    dblAry da;
    bool list_done = false;

    IFparseNode **plast = &p->p_left;
    IFparseNode *ptmp = p->p_tree->treeParse(&ts);

    while (ptmp) {
        // Evaluate the argument, save result.
        if (!IFparseNode::is_const(ptmp)) {
            if (list_done)
                break;
            da.add(0.0);
            Errs()->add_error("%s function arg %d not constant", "SFFM",
                da.count());
            *error = E_BADPARM;
        }
        else {
            double r;
            int err = (ptmp->*ptmp->p_evfunc)(&r, 0, 0);
            if (err) {
                if (list_done)
                    break;
                da.add(0.0);
                Errs()->add_error("%s function arg %d eval failed: %s",
                    "SFFM", da.count(), Sp.ErrorShort(err));
                *error = E_BADPARM;
            }
            else {
                if (list_done)
                    break;
                da.add(r);
            }
        }

        // Link the argument into a normal argument list.  The parse nodes
        // won't be freed untill the tree is destroyed anyway, so may as
        // well link them.

        IFparseNode *pnxt = p->p_tree->treeParse(&ts);
        if (pnxt) {
            if (!list_done) {
                *plast = p->p_tree->newBnode(PT_COMMA, ptmp, 0);
                plast = &(*plast)->p_right;
            }
            ptmp = pnxt;
            continue;
        }

        if (!list_done)
            *plast = ptmp;
        break;
    }

    delete [] tt;
    p->v.td = new IFsffmData(da.final(), da.count());
}


void
IFsffmData::setup(sCKT *ckt, double step, double finaltime, bool skipbr)
{
    (void)skipbr;
    // This is called with zeroed arguments to disable tran funcs while
    // not in transient analysis.
    //
    if (step > 0.0 && finaltime > 0.0) {
        if (td_enable_tran)
            setup(ckt, 0.0, 0.0, false);
        td_enable_tran = true;
    }
    else
        td_enable_tran = false;

    if (td_enable_tran) {
        if (!FC())
            set_FC(1.0/finaltime);
        if (!FS())
            set_FS(1.0/finaltime);
        // no breakpoints
    }
    else {
        memset(td_parms, 0, 8*sizeof(double));
        set_VO(td_coeffs[0]);
        set_VA(td_numcoeffs >= 2 ? td_coeffs[1] : 0.0);
        set_FC(td_numcoeffs >= 3 ? td_coeffs[2] : 0.0);
        set_MDI(td_numcoeffs >= 4 ? td_coeffs[3] : 0.0);
        set_FS(td_numcoeffs >= 5 ? td_coeffs[4] : 0.0);
    }
}


double
IFsffmData::eval_func(double t)
{
    if (!td_enable_tran)
        return (VO());
    double w1 = 2*M_PI*FC();
    double w2 = 2*M_PI*FS();
    return (VO() + VA()*sin(w1*t + MDI()*sin(w2*t)));
}


double
IFsffmData::eval_deriv(double t)
{
    if (!td_enable_tran)
        return (0.0);
    double w1 = 2*M_PI*FC();
    double w2 = 2*M_PI*FS();
    return (VA()*cos(w1*t + MDI()*sin(w2*t))*(w1 + MDI()*w2*cos(w2*t)));
}


IFtranData *
IFsffmData::dup() const
{
    IFsffmData *td = new IFsffmData(*this);
    if (td_coeffs) {
        td->td_coeffs = new double[td_numcoeffs];
        memcpy(td->td_coeffs, td_coeffs, td_numcoeffs*sizeof(double));
    }
    if (td_parms) {
        td->td_parms = new double[8];
        memcpy(td->td_parms, td_parms, 8*sizeof(double));
    }
    return (td);
}


void
IFsffmData::time_limit(const sCKT *ckt, double *plim)
{
    if (!ckt || !plim)
        return;
    double per = ckt->CKTcurTask->TSKdphiMax / (2*M_PI*(FC() + FS()));
    if (*plim > per)
        *plim = per;
}


// 0  VO    prm1
// 1  VA    prm2
// 2  FC    prm3
// 3  MDI   prm4
// 4  FS    prm5
//
void
IFsffmData::set_param(double val, int ix)
{
    if (ix < 0 || ix > 4)
        return;
    td_coeffs[ix] = val;
    if (td_enable_tran)
        td_parms[ix] = val;
}


double *
IFsffmData::get_param(int ix)
{
    if (ix < 0 || ix > 4)
        return (0);
    return (td_coeffs + ix);
}
// End of IFsffmData functions.


// AM
//
IFamData::IFamData(double *list, int num) : IFtranData(PTF_tAM)
{
    td_parms = new double[8];
    td_coeffs = list;
    td_numcoeffs = num;

    set_SA(list[0]);
    set_OC(num >= 2 ? list[1] : 0.0);
    set_MF(num >= 3 ? list[2] : 0.0);
    set_CF(num >= 4 ? list[3] : 0.0);
    set_DL(num >= 5 ? list[4] : 0.0);
}


// Static function
//
void
IFamData::parse(const char *args, IFparseNode *p, int *error)
{
    // Make arguments space-separated (remove separating commas).
    char *tt = fix_tran_args(args, 0, 0);

    const char *ts = tt;
    dblAry da;
    bool list_done = false;

    IFparseNode **plast = &p->p_left;
    IFparseNode *ptmp = p->p_tree->treeParse(&ts);

    while (ptmp) {
        // Evaluate the argument, save result.
        if (!IFparseNode::is_const(ptmp)) {
            if (list_done)
                break;
            da.add(0.0);
            Errs()->add_error("%s function arg %d not constant", "AM",
                da.count());
            *error = E_BADPARM;
        }
        else {
            double r;
            int err = (ptmp->*ptmp->p_evfunc)(&r, 0, 0);
            if (err) {
                if (list_done)
                    break;
                da.add(0.0);
                Errs()->add_error("%s function arg %d eval failed: %s",
                    "AM", da.count(), Sp.ErrorShort(err));
                *error = E_BADPARM;
            }
            else {
                if (list_done)
                    break;
                da.add(r);
            }
        }

        // Link the argument into a normal argument list.  The parse nodes
        // won't be freed untill the tree is destroyed anyway, so may as
        // well link them.

        IFparseNode *pnxt = p->p_tree->treeParse(&ts);
        if (pnxt) {
            if (!list_done) {
                *plast = p->p_tree->newBnode(PT_COMMA, ptmp, 0);
                plast = &(*plast)->p_right;
            }
            ptmp = pnxt;
            continue;
        }

        if (!list_done)
            *plast = ptmp;
        break;
    }

    delete [] tt;
    p->v.td = new IFamData(da.final(), da.count());
}


void
IFamData::setup(sCKT *ckt, double step, double finaltime, bool skipbr)
{
    (void)skipbr;
    // This is called with zeroed arguments to disable tran funcs while
    // not in transient analysis.
    //
    if (step > 0.0 && finaltime > 0.0) {
        if (td_enable_tran)
            setup(ckt, 0.0, 0.0, false);
        td_enable_tran = true;
    }
    else
        td_enable_tran = false;

    if (td_enable_tran) {
        if (!MF())
            set_MF(1.0/finaltime);
    }
    else {
        memset(td_parms, 0, 8*sizeof(double));
        set_SA(td_coeffs[0]);
        set_OC(td_numcoeffs >= 2 ? td_coeffs[1] : 0.0);
        set_MF(td_numcoeffs >= 3 ? td_coeffs[2] : 0.0);
        set_CF(td_numcoeffs >= 4 ? td_coeffs[3] : 0.0);
        set_DL(td_numcoeffs >= 5 ? td_coeffs[4] : 0.0);
    }
}


double
IFamData::eval_func(double t)
{
    if (!td_enable_tran)
        return (0.0);
    if (t <= DL())
        return (0.0);
    double w = 2*M_PI*(t - DL());
    return (SA()*(OC() + sin(w*MF()))*sin(w*CF()));
}


double
IFamData::eval_deriv(double t)
{
    if (!td_enable_tran)
        return (0.0);
    if (t <= DL())
        return (0.0);
    double w = 2*M_PI*(t - DL());
    double f1 = OC() + sin(w*MF());
    double df1 = 2*M_PI*MF()*cos(w*MF());
    double f2 = sin(w*CF());
    double df2 = 2*M_PI*CF()*cos(w*CF());
    return (SA()*(f1*df2 + f2*df1));
}


IFtranData *
IFamData::dup() const
{
    IFamData *td = new IFamData(*this);
    if (td_coeffs) {
        td->td_coeffs = new double[td_numcoeffs];
        memcpy(td->td_coeffs, td_coeffs, td_numcoeffs*sizeof(double));
    }
    if (td_parms) {
        td->td_parms = new double[8];
        memcpy(td->td_parms, td_parms, 8*sizeof(double));
    }
    return (td);
}


void
IFamData::time_limit(const sCKT *ckt, double *plim)
{
    if (!ckt || !plim)
        return;
    double per = ckt->CKTcurTask->TSKdphiMax / (2*M_PI*(CF() + MF()));
    if (*plim > per)
        *plim = per;
}


// 0  SA    prm1
// 1  QC    prm2
// 2  MF    prm3
// 3  CF    prm4
// 4  DL    prm5
//
void
IFamData::set_param(double val, int ix)
{
    if (ix < 0 || ix > 4)
        return;
    td_coeffs[ix] = val;
    if (td_enable_tran)
        td_parms[ix] = val;
}


double *
IFamData::get_param(int ix)
{
    if (ix < 0 || ix > 4)
        return (0);
    return (td_coeffs + ix);
}
// End of IFamData functions.


// GAUSS
//
IFgaussData::IFgaussData(double *list, int num) : IFtranData(PTF_tGAUSS)
{
    td_parms = new double[8];
    td_coeffs = list;
    td_numcoeffs = num;

    set_SD(list[0]);
    if (SD() == 0.0)
        set_SD(1.0);
    if (num > 1)
        set_MEAN(list[1]);
    else
        set_MEAN(0.0);
    if (num > 2) {
        set_LATTICE(list[2]);
        if (LATTICE() < 0.0)
            set_LATTICE(0.0);
    }
    else
        set_LATTICE(0.0);
    if (num > 3) {
        set_ILEVEL(list[3]);
        if (ILEVEL() != 0.0)
            set_ILEVEL(1.0);
    }
    else
        set_ILEVEL(1.0);
    set_LVAL(MEAN());
    set_VAL(MEAN());
    if (LATTICE() > 0.0)
        set_NVAL(SD()*Rnd.gauss() + MEAN());
    else
        set_NVAL(MEAN());
    set_TIME(0.0);
}


// Static function
//
void
IFgaussData::parse(const char *args, IFparseNode *p, int *error)
{
    // Make arguments space-separated (remove separating commas).
    char *tt = fix_tran_args(args, 0, 0);

    const char *ts = tt;
    dblAry da;
    bool list_done = false;

    IFparseNode **plast = &p->p_left;
    IFparseNode *ptmp = p->p_tree->treeParse(&ts);

    while (ptmp) {
        // Evaluate the argument, save result.
        if (!IFparseNode::is_const(ptmp)) {
            if (list_done)
                break;
            da.add(0.0);
            Errs()->add_error("%s function arg %d not constant", "GAUSS",
                da.count());
            *error = E_BADPARM;
        }
        else {
            double r;
            int err = (ptmp->*ptmp->p_evfunc)(&r, 0, 0);
            if (err) {
                if (list_done)
                    break;
                da.add(0.0);
                Errs()->add_error("%s function arg %d eval failed: %s",
                    "GAUSS", da.count(), Sp.ErrorShort(err));
                *error = E_BADPARM;
            }
            else {
                if (list_done)
                    break;
                da.add(r);
            }
        }

        // Link the argument into a normal argument list.  The parse nodes
        // won't be freed untill the tree is destroyed anyway, so may as
        // well link them.

        IFparseNode *pnxt = p->p_tree->treeParse(&ts);
        if (pnxt) {
            if (!list_done) {
                *plast = p->p_tree->newBnode(PT_COMMA, ptmp, 0);
                plast = &(*plast)->p_right;
            }
            ptmp = pnxt;
            continue;
        }

        if (!list_done)
            *plast = ptmp;
        break;
    }

    delete [] tt;
    p->v.td = new IFgaussData(da.final(), da.count());
}


void
IFgaussData::setup(sCKT *ckt, double step, double finaltime, bool skipbr)
{
    // This is called with zeroed arguments to disable tran funcs while
    // not in transient analysis.
    //
    if (step > 0.0 && finaltime > 0.0) {
        if (td_enable_tran)
            setup(ckt, 0.0, 0.0, false);
        td_enable_tran = true;
    }
    else
        td_enable_tran = false;

    if (td_enable_tran && !skipbr && ckt) {
        if (LATTICE() != 0.0)
            ckt->breakSetLattice(0.0, LATTICE());
    }
}


double
IFgaussData::eval_func(double t)
{
    if (!td_enable_tran)
        return (0.0);
    if (LATTICE() == 0.0) {
        if (t != TIME()) {
            set_VAL(SD()*Rnd.gauss() + MEAN());
            set_TIME(t);
        }
        return (VAL());
    }
    while (t > TIME() + LATTICE()) {
        set_LVAL(VAL());
        set_VAL(NVAL());
        set_NVAL(SD()*Rnd.gauss() + MEAN());
        unsigned int n = (unsigned int)(TIME()/LATTICE() + 0.5) + 1;
        set_TIME(n*LATTICE());
    }
    if (t < TIME()) {
        if (TIME() - t > LATTICE() || ILEVEL() == 0.0)
            return (LVAL());
        return ((VAL() - LVAL())*(t - TIME())/LATTICE() + VAL());
    }
    else {
        if (ILEVEL() == 0.0)
            return (VAL());
        return ((NVAL() - VAL())*(t - TIME())/LATTICE() + VAL());
    }
}


double
IFgaussData::eval_deriv(double t)
{
    if (!td_enable_tran)
        return (0.0);
    if (ILEVEL() == 0.0)
        return (0.0);
    if (LATTICE() == 0.0)
        return (0.0);
    while (t > TIME() + LATTICE()) {
        set_LVAL(VAL());
        set_VAL(NVAL());
        set_NVAL(SD()*Rnd.gauss() + MEAN());
        unsigned int n = (unsigned int)(TIME()/LATTICE() + 0.5) + 1;
        set_TIME(n*LATTICE());
    }
    if (t < TIME()) {
        if (TIME() - t > LATTICE())
            return (0.0);
        return ((VAL() - LVAL())/LATTICE());
    }
    else
        return ((NVAL() - VAL())/LATTICE());
}


IFtranData *
IFgaussData::dup() const
{
    IFgaussData *td = new IFgaussData(*this);
    if (td_coeffs) {
        td->td_coeffs = new double[td_numcoeffs];
        memcpy(td->td_coeffs, td_coeffs, td_numcoeffs*sizeof(double));
    }
    if (td_parms) {
        td->td_parms = new double[8];
        memcpy(td->td_parms, td_parms, 8*sizeof(double));
    }
    return (td);
}


// 0  SD        prm1
// 1  MEAN      prm2
// 2  LATTICE   prm3
// 3  ILEVEL    prm4
//
void
IFgaussData::set_param(double val, int ix)
{
    if (ix < 0 || ix > 3)
        return;
    td_coeffs[ix] = val;
    if (td_enable_tran)
        td_parms[ix] = val;
}


double *
IFgaussData::get_param(int ix)
{
    if (ix < 0 || ix > 3)
        return (0);
    return (td_coeffs + ix);
}
// End of IFgaussData functions.


// INTERP
//
IFinterpData::IFinterpData(double *list, int num, double *list2) :
    IFtranData(PTF_tINTERP)
{
    if (num > 0) {
        td_parms = new double[num];
        td_coeffs = new double[num];
        if (list)
            memcpy(td_parms, list, num*sizeof(double));
        else
            memset(td_parms, 0, num*sizeof(double));
        if (list2)
            memcpy(td_coeffs, list2, num*sizeof(double));
        else
            memset(td_coeffs, 0, num*sizeof(double));
        td_numcoeffs = num;
    }
}


// Static function.
//
void
IFinterpData::parse(const char *args, IFparseNode *p, int *error)
{
    *error = OK;
    // Use named vector/scale as basis for interpolating output
    // values.
    const char *s = args;
    char buf[256];
    char *tt = buf;
    while (*s) {
        if (!isspace(*s))
            *tt++ = *s;
        s++;
    }
    if (tt > buf && *--tt == ')')
        *tt = '\0';
    else {
        Errs()->add_error("syntax error in interp function");
        *error = E_SYNTAX;
        *buf = 0;
    }

    sDataVec *d = 0;
    if (*buf) {
        d = OP.vecGet(buf, p->p_tree->ckt());
        if (!d)
            *error = E_NOVEC;
        else if (d->iscomplex()) {
            d = 0;
            Errs()->add_error("complex vector in interp function");
            *error = E_NOVEC;
        }
    }

    sDataVec *scale = 0;
    if (d) {
        scale = d->scale();
        if (!scale)
            scale = OP.curPlot()->scale();
        if (!scale) {
            d = 0;
            Errs()->add_error("no scale for interp function");
            *error = E_NOVEC;
        }
        else if (scale->iscomplex()) {
            d = 0;
            scale = 0;
            Errs()->add_error("complex vector in interp function");
            *error = E_NOVEC;
        }
    }

    int len = 0;
    if (scale) {
        len = d->length();
        if (scale->length() < len)
            len = scale->length();
        double *x = scale->realvec();
        int i = 0;
        if (*x < *(x+1)) {
            for (i = 1; i < len-1; i++)
                if (*(x+i) >= *(x+i+1))
                    break;
        }
        else if (*x > *(x+1)) {
            for (i = 1; i < len-1; i++)
                if (*(x+i) <= *(x+i+1))
                    break;
        }
        if (i != len-1) {
            scale = 0;
            d = 0;
            len = 0;
            Errs()->add_error("non-monotonic scale for interp function");
            *error = E_NOVEC;
        }
    }
    p->v.td = new IFinterpData(scale->realvec(), len, d->realvec());
}


void
IFinterpData::setup(sCKT *ckt, double step, double finaltime, bool skipbr)
{
    (void)skipbr;
    // This is called with zeroed arguments to disable tran funcs while
    // not in transient analysis.
    //
    if (step > 0.0 && finaltime > 0.0) {
        if (td_enable_tran)
            setup(ckt, 0.0, 0.0, false);
        td_enable_tran = true;
    }
    else
        td_enable_tran = false;

    td_index = 0;
}


double
IFinterpData::eval_func(double t)
{
    double *tvec = td_parms;   // time values
    double *vec = td_coeffs;   // source values
    if (!td_numcoeffs || !tvec || !vec)
        return (0.0);

    if (t >= tvec[td_numcoeffs-1])
        return (vec[td_numcoeffs-1]);
    else if (t <= tvec[0])
        return (vec[0]);
    int i = td_index;
    while (t > tvec[i+1] && i < td_numcoeffs - 2)
        i++;
    while (t < tvec[i] && i > 0)
        i--;
    td_index = i;
    return (vec[i] + (vec[i+1] - vec[i])*(t - tvec[i])/(tvec[i+1] - tvec[i]));
}


double
IFinterpData::eval_deriv(double t)
{
    // Doesn't need setup call.
    double *tvec = td_parms;   // time values
    double *vec = td_coeffs;   // source values
    if (!td_numcoeffs || !tvec || !vec)
        return (0.0);

    if (t >= tvec[td_numcoeffs-1])
        return (0);
    else if (t <= tvec[0])
        return (0);
    int i = td_index;
    while (t > tvec[i+1] && i < td_numcoeffs - 2)
        i++;
    while (t < tvec[i] && i > 0)
        i--;
    td_index = i;
    return ((vec[i+1] - vec[i])/(tvec[i+1] - tvec[i]));
}


IFtranData *
IFinterpData::dup() const
{
    IFinterpData *td = new IFinterpData(*this);
    if (td_coeffs) {
        td->td_coeffs = new double[td_numcoeffs];
        memcpy(td->td_coeffs, td_coeffs, td_numcoeffs*sizeof(double));
    }
    if (td_parms) {
        td->td_parms = new double[td_numcoeffs];
        memcpy(td->td_parms, td_parms, td->td_numcoeffs*sizeof(double));
    }
    return (td);
}
// End of IFinterpData functions.

