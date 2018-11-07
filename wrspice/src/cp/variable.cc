
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
Authors: 1985 Wayne A. Christopher
         1992 Stephen R. Whiteley
****************************************************************************/

#include "cshell.h"
#include "commands.h"
#include "simulator.h"
#include "variable.h"
#include "toolbar.h"
#include "inpline.h"
#include "spnumber/hash.h"
#include "spnumber/spnumber.h"
#include "miscutil/lstring.h"
#ifdef WIN32
#include <process.h>
#endif


//
// Do variable substitution.
//

namespace {
    // Non-alphanumeric characters that may appear in variable names. 
    // < is very special...
    //
    const char *VALIDCHARS = "_<#?@.()[]&";

    // Instantiate.
    sVarDb vardb;
}

// Assign static pointer to variables database.
sVarDb *CshPar::cp_vardb = &vardb;


// A parser for range specification.
//
struct rng_t
{
    rng_t(const char *n)
        {
            name = lstring::copy(n);
            range = 0;
            index = -1;
            char *t = strchr(name, '[');
            if (t) {
                *t = 0;
                range = eval_range(t+1);
                index = 0;
                if (range) {
                    for (t = range; isdigit(*t); t++)
                        index = index*10 + *t - '0';
                }
            }
        }

    ~rng_t()
        {
            delete [] name;
            delete [] range;
        }

    static char *eval_range(const char*);

    char *name;
    char *range;
    int index;
};


// Static function.
// Parse and evaluate the range specification, replacing any shell
// variables by integer equivalents.
//
char *
rng_t::eval_range(const char *cstring)
{
    // get rid of any white space
    const char *t = cstring;
    char *s = new char[strlen(cstring)+1];
    char *string = s;
    for ( ; *t; t++) {
        if (isspace(*t))
            continue;
        *s++ = *t;
    }
    *s = '\0';

    if (!strchr(string, '$'))
        return (string);

    char *tok1, *tok2, *tokd = 0;
    s = tok1 = string;
    int lev1 = 0;
    int lev2 = 0;
    while (*s) {
        if (*s == '[')
            lev1++;
        else if (*s == '(')
            lev2++;
        else if (*s == ']') {
            if (lev1)
                lev1--;
            else if (!lev2)
                // [tok1]
                break;
        }
        else if (*s == ')') {
            if (lev2)
                lev2--;
        }
        else if (*s == '-' && !lev1 && !lev2) {
            if (s == tok1)
                // [-tok2]
                tok1 = 0;
            // [tok1-...]
            break;
        }
        s++;
    }
    if (*s == '-') {
        tokd = s;
        *s = '\0';
        s++;
        tok2 = s;
        lev1 = lev2 = 0;
        while (*s) {
            if (*s == '[')
                lev1++;
            else if (*s == '(')
                lev2++;
            else if (*s == ')') {
                if (lev2)
                    lev2--;
            }
            else if (*s == ']') {
                if (lev1)
                    lev1--;
                else if (!lev2) {
                    if (tok2 == s) {
                        // [...-]
                        tok2 = 0;
                        break;
                    }
                    // [...-tok2]
                    *s = '\0';
                    break;
                }
            }
            s++;
        }
        if (!*s && tok2 == s) {
            // no last ], not to worry
            tok2 = 0;
        }
    }
    else {
        if (*s == ']')
            *s = '\0';
        tok2 = 0;
    }

    char buf[128];
    int i;
    *buf = '\0';
    if (tok1) {
        if (*tok1 == '$') {
            tok1++;
            wordlist *wl = CP.VarEval(tok1);
            if (wl) {
                tok1 = wordlist::flatten(wl);
                wordlist::destroy(wl);
                i = (int)(atof(tok1) + .5);
                delete [] tok1;
                tok1 = 0;
            }
            else
                i = 0;
            sprintf(buf, "%d", i);
        }
        else
            sprintf(buf, "%s", tok1);
    }
    if (tokd)
        strcat(buf, "-");
    if (tok2) {
        if (*tok2 == '$') {
            tok2++;
            wordlist *wl = CP.VarEval(tok2);
            if (wl) {
                tok2 = wordlist::flatten(wl);
                wordlist::destroy(wl);
                i = (int)(atof(tok2) + .5);
                delete [] tok2;
                tok2 = 0;
            }
            else
                i = 0;
            sprintf(buf + strlen(buf), "%d", i);
        }
        else
            sprintf(buf + strlen(buf), "%s", tok2);

    }
    strcat(buf, "]");
    delete [] string;
    return (lstring::copy(buf));
}
// End of rng_t functions.


// Shift a list variable, by default argv, one to the left (or more if a
// second argument is given.
//
void
CommandTab::com_shift(wordlist *wl)
{
    const char *word = "argv";
    int num = 1;
    if (wl) {
        word = wl->wl_word;
        wl = wl->wl_next;
    }
    if (wl)
        num = lstring::scannum(wl->wl_word);

    variable *v = CP.RawVarGet(word);
    if (!v) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "%s: no such variable.\n", word);
        return;
    }
    if (v->type() != VTYP_LIST) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "%s not of type list.\n", word);
        return;
    }

    variable *lv = 0, *vv;
    for (vv = v->list(); vv && (num > 0); num--) {
        lv = vv;
        vv = vv->next();
    }

    if (num) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "variable %s not long enough.\n", word);
        return;
    }

    if (lv)
        lv->set_next(0);
    // This will free the shifted-out vars.
    v->set_list(vv);
}
// End of CommandTab functions.


wordlist *
CshPar::VarList()
{
    return (cp_vardb->wl());
}


// A variable substitution is indicated by a $, and the variable name
// is the following string of non-special characters. All variable values
// are inserted as a single word, except for lists, which are a list of
// words.
//
void
CshPar::VarSubst(wordlist **list)
{
    if (list == 0)
        return;
    wordlist *wlist = *list;
    char *wbuf = 0;
    int wbuflen = 0;
    for (wordlist *wl = wlist; wl; wl = wl->wl_next) {
        char *t = wl->wl_word;
        int i = 0;
        char *s;
        while ((s = strchr(t, cp_dol)) != 0) {
            if (s > t && *(s-1) == '\\') {
                // backslash inhibits substitution
                // if \$ is first in line or preceded by space, rest of
                // line is a comment
                if (s-1 == wl->wl_word && wl == wlist) {
                    // line is a comment, replace with standard form
                    *wl->wl_word = '*';
                    *(wl->wl_word+1) = ' ';
                    delete [] wbuf;
                    return;
                }
                if (s-1 == wl->wl_word) {
                    if (wl->wl_prev)
                        wl = wl->wl_prev;
                    else {
                        // prev ptr not set?
                        wordlist *wz;
                        for (wz = wlist; wz->wl_next != wl; wz = wz->wl_next) ;
                        wl = wz;
                    }
                    wordlist::destroy(wl->wl_next);
                    wl->wl_next = 0;
                    delete [] wbuf;
                    return;
                }
                if (isspace(*(s-2))) {
                    // end line before comment
                    *(s-2) = 0;
                    wordlist::destroy(wl->wl_next);
                    wl->wl_next = 0;
                    delete [] wbuf;
                    return;
                }
                // get rid of '\'
                t = lstring::copy(wl->wl_word);
                i = s - wl->wl_word;
                strcpy(t + (i - 1), s);
                delete [] wl->wl_word;
                wl->wl_word = t;
                t = wl->wl_word + i + 1;
                continue;
            }

            int len = i + s - t + 1;
            if (len > wbuflen || !wbuf) {
                wbuflen = len;
                char *aa = new char[wbuflen];
                if (wbuf) {
                    strcpy(aa, wbuf);
                    delete [] wbuf;
                }
                wbuf = aa;
            }
            while (t < s)
                wbuf[i++] = *t++;
            wbuf[i] = '\0';

            char *tbuf = lstring::copy(++s);
            s = tbuf;
            t++;

            bool curly = (*s == cp_ocurl);
            if (*s && (curly || isalnum(*s) ||
                    lstring::instr(VALIDCHARS, *s) || *s == cp_dol)) {
                t++;
                s++;
            }
            int lev1 = 0;
            int lev2 = 0;
            for ( ; *s; t++, s++) {
                if (curly) {
                    if (*s != cp_ccurl)
                        continue;
                    s++;
                    t++;
                    curly = false;
                    break;
                }
                if (isalnum(*s))
                    continue;
                if (*s == '[') {
                    lev1++;
                    continue;
                }
                if (*s == '(') {
                    lev2++;
                    continue;
                }
                if (*s == ']') {
                    if (lev1) {
                        lev1--;
                        continue;
                    }
                    break;
                }
                if (*s == ')') {
                    if (lev2) {
                        lev2--;
                        continue;
                    }
                    break;
                }
                if (lstring::instr(VALIDCHARS, *s))
                    continue;
                if (lev1 || lev2)
                    continue;
                break;
            }
            if (!*s && (lev1 || lev2 || curly)) {
                // split token, concatenate them
                if (wl->wl_next) {
                    wordlist *ww = wl->wl_next;
                    char *str =
                        new char[strlen(wl->wl_word)+strlen(ww->wl_word)+1];
                    strcpy(str, wl->wl_word);
                    strcat(str, ww->wl_word);
                    delete [] wl->wl_word;
                    wl->wl_word = str;
                    delete [] ww->wl_word;
                    wl->wl_next = ww->wl_next;
                    if (wl->wl_next)
                        wl->wl_next->wl_prev = wl;
                    delete ww;
                    ww = 0;
                    t = wl->wl_word;
                    i = 0;
                    delete [] tbuf;
                    continue;
                }
                else {
                    char *str = wordlist::flatten(wlist);
                    GRpkgIf()->ErrPrintf(ET_ERROR,
                        "unbalanced parentheses or brackets.\n  %s\n", str);
                    delete [] str;
                    delete [] tbuf;
                    delete [] wbuf;
                    *list = 0;
                    return;
                }
            }

            *s = '\0';
            wordlist *nwl = VarEval(tbuf);
            delete [] tbuf;
            if (i) {
                if (nwl) {
                    char *a = new char[strlen(wbuf) +
                        strlen(nwl->wl_word) + 1];
                    strcpy(a, wbuf);
                    strcat(a, nwl->wl_word);
                    delete [] nwl->wl_word;
                    nwl->wl_word = a;
                }
                else
                    nwl = new wordlist(wbuf, 0);
            }
            bool wfirst = (wl == wlist ? true : false);
            // this frees wl and string
            t = lstring::copy(t);
            wordlist *twl = wl->splice(nwl);
            if (!twl) {
                if (!wfirst)
                    wordlist::destroy(wlist);
                *list = 0;
                delete [] wbuf;
                return;
            }
            if (wfirst)
                wlist = nwl;
            wl = twl;

            i = strlen(wl->wl_word);
            char *a = new char[i + strlen(t) + 1];
            strcpy(a, wl->wl_word);
            strcat(a, t);
            delete [] wl->wl_word;
            delete [] t;
            wl->wl_word = a;

            if (i+1 > wbuflen) {
                wbuflen = i+1;
                delete [] wbuf;
                wbuf = new char[i+1];
            }
            t = &wl->wl_word[i];
            s = wl->wl_word;
            for (i = 0; s < t; s++)
                wbuf[i++] = *s;
            wbuf[i] = 0;
        }
    }
    delete [] wbuf;
    *list = wlist;
}


// Substitute for shell tokens in the string.  If a substitution is made,
// the old string is freed.
//
void
CshPar::VarSubst(char **str)
{
    // Split line into wordlist that separates out shell tokens.
    char *c = *str, *d;
    if (!c)
        return;
    wordlist *wl = 0, *wl0 = 0;
    while ((d = strchr(c, '$')) != 0) {
        if (d > c && *(d-1) == '\\')
            d--;
        if (d != c) {
            if (wl0 == 0) {
                wl = wl0 = new wordlist;
            }
            else {
                wl->wl_next = new wordlist;
                wl->wl_next->wl_prev = wl;
                wl = wl->wl_next;
            }
            wl->wl_word = new char[d-c+1];
            strncpy(wl->wl_word, c, d-c);
            wl->wl_word[d-c] = '\0';
        }
        c = d;
        d = dollartok(c);
        if (*c == '\\' && c > *str && !isspace(*(c-1))) {
            // preceded by space, add to last token
            char *t = new char[strlen(wl->wl_word) + d - c + 1];
            strcpy(t, wl->wl_word);
            delete [] wl->wl_word;
            wl->wl_word = t;
            while (*t)
                t++;
            while (c < d)
                *t++ = *c++;
            *t = 0;
        }
        else {
            if (wl0 == 0) {
                wl = wl0 = new wordlist;
            }
            else {
                wl->wl_next = new wordlist;
                wl->wl_next->wl_prev = wl;
                wl = wl->wl_next;
            }
            wl->wl_word = new char[d-c+1];
            strncpy(wl->wl_word, c, d-c);
            wl->wl_word[d-c] = '\0';
        }
        c = d;
    }
    if (wl0 == 0)
        // no shell tokens, return
        return;
    if (*c) {
        wl->wl_next = new wordlist;
        wl->wl_next->wl_prev = wl;
        wl = wl->wl_next;
        wl->wl_word = lstring::copy(c);
    }

    // Substitute for the shell tokens.
    CP.VarSubst(&wl0);

    // Now rebuild the line, removing concatenation chars.
    int len = 1;
    for (wl = wl0; wl; wl = wl->wl_next)
        len += strlen(wl->wl_word);
    c = new char[len];
    *c = '\0';
    for (wl = wl0; wl; wl = wl->wl_next) {
        d = wl->wl_word;
        while (*d == cp_var_catchar)
            d++;
        strcat(c, d);
    }
    delete [] *str;
    *str = c;
    wordlist::destroy(wl0);
}


namespace {
    // This substitutes in forms like &v($something).  Deletes or
    // returns str.
    //
    char *iv_special(char *str)
    {
        int cnt = 1;
        char *s;
        for (s = str+4; *s; s++) {
            if (*s == '(')
                cnt++;
            else if (*s == ')')
                cnt--;
            if (!cnt)
                break;
        }
        if (*s != ')')
            // no closing paren, error
            return (str);
        *s = '\0';
        wordlist *wl = CP.VarEval(str+4);
        *s = ')';
        char *t = wordlist::flatten(wl);
        wordlist::destroy(wl);
        if (!t)
            // some error occurred
            return (str);
        char *u = new char[strlen(t) + strlen(s) + 4];
        u[0] = '&';
        u[1] = str[1];
        u[2] = '(';
        strcpy(u+3, t);
        strcat(u, s);
        delete [] str;
        delete [] t;
        return (u);
    }


    // Parse and deal with 'range' ...
    //
    wordlist *var_range(variable *v, const char *range)
    {
        if (range) {
            int up, low;
            const char *s = range;
            if (!isdigit(*s) && *s != '-')
                GRpkgIf()->ErrPrintf(ET_WARN,
                    "nonparseable range specified, %s[%s.\n", v->name(), s);

            for (low = 0; isdigit(*s); s++)
                low = low * 10 + *s - '0';
            if ((*s == '-') && isdigit(s[1])) {
                for (up = 0, s++; isdigit(*s); s++)
                    up = up * 10 + *s - '0';
            }
            else if (*s == '-') {
                if (v->type() != VTYP_LIST)
                    up = 0;
                else {
                    variable *vv;
                    for (up = 0, vv = v->list(); vv; vv = vv->next(), up++) ;
                    if (!up) {
                        // variable is an empty list
                        return (0);
                    }
                    up--;
                }
            }
            else
                up = low;
            return (v->var2wl(low, up));
        }
        return (v->varwl());
    }
}


// Evaluate a variable named in string.  The leading '$' is assumed to
// have been stripped.  Returns a wordlist of the text of the
// variable.
//
wordlist *
CshPar::VarEval(const char *cstring)
{
    char *string;
    if (*cstring == cp_ocurl) {
        // {blahblah}
        string = lstring::copy(cstring+1);
        char *e = string + strlen(string) - 1;
        if (*e == cp_ccurl)
            *e = 0;
    }
    else
        string = lstring::copy(cstring);
    if (!string)
        return (0);
    GCarray<char*> gc_string(string);
    Strip(string);

    char *range = 0, *s;
    if ((s = strchr(string, '[')) != 0) {
        *s = '\0';
        range = rng_t::eval_range(s+1);
    }
    GCarray<char*> gc_range(range);

    char buf[BSIZE_SP];
    wordlist *wl = 0;

    if (!*string)
        return (new wordlist("$", 0));
    if (*string == '$') {
        sprintf(buf, "%d", (int)getpid());
        return (new wordlist(buf, 0));
    }
    if (*string == '<') {
        if (Sp.GetFlag(FT_BATCHMODE)) {
            TTY.flush();
            SetupTty(TTY.infileno(), true);
            if (!fgets(buf, BSIZE_SP, TTY.infile())) {
                clearerr(TTY.infile());
                strcpy(buf, "EOF");
            }
            SetupTty(TTY.infileno(), false);
            for (s = buf; *s && (*s != '\n'); s++) ;
            *s = '\0';
            wl = Lexer(buf);
            // This is a hack
            if (!wl->wl_word)
                wl->wl_word = lstring::copy("");
        }
        else {
            cp_input = TTY.infile();
            bool ii = cp_flags[CP_INTERACTIVE];
            cp_flags[CP_INTERACTIVE] = false;
            SetupTty(TTY.infileno(), true);
            wl = Lexer(0);
            SetupTty(TTY.infileno(), false);
            cp_flags[CP_INTERACTIVE] = ii;
            cp_input = 0;
            if (!wl)
                wl = new wordlist("", 0);
        }
        return (wl);
    }

    if (*string == '?') {
        wl = new wordlist;
        s = string + 1;
        if (*s) {
            // Return 1 or 0 depending on whether the string names
            // something or not.

            if (Sp.GetRawVar(s, Sp.CurCircuit()))
                wl->wl_word = lstring::copy("1");
            else {
                variable *v = Sp.EnqPlotVar(s);
                if (v) {
                    wl->wl_word = lstring::copy("1");
                    variable::destroy(v);
                }
                else {
                    // Sp.EnqVectorVar() takes care of range
                    if (range) {
                        char *ts = new char[strlen(s) + strlen(range) + 2];
                        sprintf(ts, "%s[%s", s, range);
                        v = Sp.EnqVectorVar(ts, true);
                        delete [] ts;
                    }
                    else
                        v = Sp.EnqVectorVar(s, true);
                    if (v) {
                        wl->wl_word = lstring::copy("1");
                        variable::destroy(v);
                    }
                    else
                        wl->wl_word = lstring::copy("0");
                }
            }
        }
        else {
            // Return the global return value, which was presumably
            // just set by another script or function.

            sprintf(buf, "%12g", cp_return_val);
            wl->wl_word = lstring::copy(buf);
        }
        return (wl);
    }
    
    if (*string == '#') {
        s = string + 1;
        variable *v = Sp.GetRawVar(s, Sp.CurCircuit());
        int cnt = 0;
        if (v) {
            if (v->type() == VTYP_LIST) {
                for (v = v->list(); v; v = v->next())
                    cnt++;
            }
            else
                cnt = (v->type() != VTYP_BOOL);
            // don't free v, not a copy
        }
        else {
            v = Sp.EnqPlotVar(s);
            if (v) {
                if (v->type() == VTYP_LIST) {
                    for (v = v->list(); v; v = v->next())
                        cnt++;
                }
                else
                    cnt = (v->type() != VTYP_BOOL);
                variable::destroy(v);
            }
            else {
                // Sp.EnqVectorVar() takes care of range
                if (range) {
                    char *ts = new char[strlen(s) + strlen(range) + 2];
                    sprintf(ts, "%s[%s", s, range);
                    v = Sp.EnqVectorVar(ts, true);
                    delete [] ts;
                }
                else
                    v = Sp.EnqVectorVar(s, true);
                if (v) {
                    if (v->type() == VTYP_LIST) {
                        for (v = v->list(); v; v = v->next())
                            cnt++;
                    }
                    else
                        cnt = (v->type() != VTYP_BOOL);
                    variable::destroy(v);
                }
            }
        }
        sprintf(buf, "%d", cnt);
        wl = new wordlist;
        wl->wl_word = lstring::copy(buf);
        return (wl);
    }

    // The notation var[stuff] has two meanings...  If this is a real
    // variable, then the [] denotes range, but if this is a strange
    // (e.g, device parameter) variable, it could be anything...
    //
    variable *v = Sp.GetRawVar(string, Sp.CurCircuit());
    if (v)
        return (var_range(v, range));

    if (isdigit(*string)) {
        v = cp_vardb->get("argv");
        return (v ? var_range(v, string) : 0);
    }

    v = Sp.EnqPlotVar(string);
    if (v) {
        wl = var_range(v, range);
        variable::destroy(v);
        return (wl);
    }

    // Sp.EnqVectorVar() takes care of range
    if (range) {
        char *ts = new char[strlen(string) + strlen(range) + 2];
        sprintf(ts, "%s[%s", string, range);

        // take care of forms like v($something)
        if (*ts == '&' && lstring::ciinstr("vi", *(ts+1)) &&
                *(ts+2) == '(' && *(ts+3) == '$')
            ts = iv_special(ts);

        v = Sp.EnqVectorVar(ts, true);
        delete [] ts;
        if (v) {
            wl = v->varwl();
            variable::destroy(v);
            return (wl);
        }
    }

    char *ts = lstring::copy(string);

    // take care of forms like v($something)
    if (*ts == '&' && lstring::ciinstr("vi", *(ts+1)) &&
            *(ts+2) == '(' && *(ts+3) == '$')
        ts = iv_special(ts);

    v = Sp.EnqVectorVar(ts, true);
    delete [] ts;
    if (v) {
        wl = v->varwl();
        variable::destroy(v);
        return (wl);
    }

    if ((s = getenv(string)) != 0)
        return (new wordlist(s, 0));

    GRpkgIf()->ErrPrintf(ET_ERROR, "%s: no such variable.\n", string);
    return (0);
}


// Deal properly with internal "option" variables.  These variables
// are always placed in the database as lower-case, but we want to
// retrieve them case-insensetively.
//
variable *
CshPar::RawVarGet(const char *name)
{
    variable *v = cp_vardb->get(name);
    if (!v) {
        const char *t = name;
        for (t = name; *t; t++) {
            if (isupper(*t))
                break;
        }
        if (*t) {
            if (sHtab::get(Sp.Options(), name)) {
                // The variable name matches an option name.  This may be
                // in the database as lower-case.
                char *tname = lstring::copy(name);
                lstring::strtolower(tname);
                v = cp_vardb->get(tname);
                delete [] tname;
            }
        }
    }
    return (v);
}



// Set or clear a variable.  This takes care of any range given, and
// filters out "argv" and "argc" which are special.  There is no
// application level processing.
//
// If vname is an option name, it is already lower-case here.
//
void
CshPar::RawVarSet(const char *vname, bool isset, variable *v)
{
    rng_t rng(vname);

    if (lstring::eq(rng.name, "argv") || lstring::eq(rng.name, "argc"))
        return;
    variable *vv = cp_vardb->get(rng.name);
    bool alreadythere = false;
    if (vv)
        alreadythere = true;
    else {
        if (rng.index >= 0) {
            GRpkgIf()->ErrPrintf(ET_ERROR, "%s is not set, length unknown.\n",
                rng.name);
            return;
        }
    }
    if (!isset) {
        if (alreadythere) {
            cp_vardb->remove(rng.name);
            variable::destroy(vv);
        }
        ToolBar()->UpdateVariables();
        return;
    }
    if (alreadythere) {
        if (rng.index >= 0) {
            if (vv->type() != VTYP_LIST) {
                GRpkgIf()->ErrPrintf(ET_ERROR, "%s is not a list.\n",
                    rng.name);
                return;
            }
            vv = vv->list();
            for (int i = rng.index; vv && i; vv = vv->next(), i--) ;
            if (!vv) {
                GRpkgIf()->ErrPrintf(ET_ERROR,
                    "%d out of range for %s.\n", rng.index, rng.name);
                return;
            }
        }
        vv->clear();
    }
    else
        vv = new variable(rng.name);

    switch (v->type()) {
    case VTYP_BOOL:
        vv->set_boolean(v->boolean());
        break;
    case VTYP_NUM:
        vv->set_integer(v->integer());
        break;
    case VTYP_REAL:
        vv->set_real(v->real());
        break;
    case VTYP_STRING:
        vv->set_string(v->string());
        break;
    case VTYP_LIST:
        // The list is not copied.  The caller loses the list.
        vv->set_list(v->list());
        v->zero_list();
        break;
    default:
        GRpkgIf()->ErrPrintf(ET_INTERR,
            "RawVarSet: bad variable type %d.\n", v->type());
        return;
    }
    if (!alreadythere)
        cp_vardb->add(rng.name, vv);
    ToolBar()->UpdateVariables();
}


// Return a variable struct filled from the parsed wordlist.
//
variable *
CshPar::ParseSet(wordlist *wl)
{
    variable *vars = 0;
    char *name = 0;
    while (wl) {
        variable *vv;
        name = lstring::copy(wl->wl_word);
        Unquote(name);
        wl = wl->wl_next;
        if (((wl == 0) || (*wl->wl_word != '=')) && strchr(name, '=') == 0) {
            vv = new variable(name);
            delete [] name;;
            vv->set_boolean(true);
            vv->set_next(vars);
            vars = vv;
            continue;
        }
        char *val, *s;
        if (wl && lstring::eq(wl->wl_word, "=")) {
            wl = wl->wl_next;
            if (wl == 0 || strchr(name, '=') != 0)
                goto bad;
            val = wl->wl_word;
            wl = wl->wl_next;
        }
        else if (wl && (*wl->wl_word == '=')) {
            if (strchr(name, '=') != 0)
                goto bad;
            val = wl->wl_word + 1;
            wl = wl->wl_next;
        }
        else if ((s = strchr(name, '=')) != 0) {
            if (s == name) goto bad;
            val = s + 1;
            *s = '\0';
            if (*val == '\0') {
                if (wl == 0) {
                    GRpkgIf()->ErrPrintf(ET_ERROR, "%s equals what?.\n", name);
                    goto bad;
                }
                val = wl->wl_word;
                wl = wl->wl_next;
            }
        }
        else
            goto bad;
        val = lstring::copy(val);
        Unquote(val);
        if (lstring::eq(val, "(")) {
            delete [] val;
            val = 0;
            variable *lv = GetList(&wl);
            if (!lv)
                goto bad;
            vv = new variable(name);
            delete [] name;
            vv->set_list(lv);
            vv->set_next(vars);
            vars = vv;
            continue;
        }

        const char *st = val;
        double *td = SPnum.parse(&st, true);
        vv = new variable(name);
        delete [] name;
        vv->set_next(vars);
        vars = vv;
        if (td && *st == '\0')
            vv->set_real(*td);
        else
            vv->set_string(val);
        delete [] val;
        val = 0;
    }
    return (vars);
bad:
    GRpkgIf()->ErrPrintf(ET_ERROR, "bad set form.\n");
    delete [] name;
    variable::destroy(vars);
    return (0);
}


// Function to grab a list into a variable list.  The wordlist is advanced.
// This function assumes that the initial "(" has been removed.
//
variable *
CshPar::GetList(wordlist **wlist)
{
    wordlist *wl = *wlist;
    variable *listv = 0, *vv = 0;
    while (wl && wl->wl_word) {
        if (lstring::eq(wl->wl_word, ")")) {
            wl = wl->wl_next;
            *wlist = wl;
            return (listv);
        }
        if (!listv)
            listv = vv = new variable;
        else {
            vv->set_next(new variable);
            vv = vv->next();
        }
        if (lstring::eq(wl->wl_word, "(")) {
            wl = wl->wl_next;
            vv->set_list(GetList(&wl));
            if (vv->list() == 0) {
                variable::destroy(listv);
                *wlist = wl;
                return (0);
            }
            continue;
        }
        char *st = lstring::copy(wl->wl_word);
        Unquote(st);
        const char *ss = st;
        double *td = SPnum.parse(&ss, true);
        if (td && *ss == '\0')
            vv->set_real(*td);
        else
            vv->set_string(st);
        delete [] st;
        wl = wl->wl_next;
    }
    *wlist = wl;
    variable::destroy(listv);
    return (0);
}


// Implelentation of a stack for argc, argv[] variables.
// These variables are set and cleared through these routines
// only, not through com_set(), com_unset().
//
namespace {
    struct avstack
    {
        int ac;
        variable *av;
    };

#define DEPTH 100
    avstack av_stack[DEPTH];
    int stackp;
}


// Push wl onto the argc/argv stack.  The values of argc and argv are
// the length of wl, and a list of wl entries, respectively.
//
void
CshPar::PushArg(wordlist *wl)
{
    variable *listv = 0, *lv = 0;
    variable *v, **pv = 0;
    int *pc = 0;
    if (stackp == 0) {
        v = new variable("argc");
        v->set_integer(0);
        pc = (int*)v->dataptr();
        cp_vardb->add("argc", v);

        v = new variable("argv");
        v->set_list(0);
        pv = (variable**)v->dataptr();
        cp_vardb->add("argv", v);
    }
    else {
        v = cp_vardb->get("argv");
        if (v)
            pv = (variable**)v->dataptr();
        v = cp_vardb->get("argc");
        if (v)
            pc = (int*)v->dataptr();
    }
    if (pv && pc) {
        if (stackp) {
            av_stack[stackp].av = *pv;
            av_stack[stackp].ac = *pc;
        }
        stackp++;
        if (stackp == DEPTH) {
            GRpkgIf()->ErrPrintf(ET_WARN, "stack overflow.\n");
            stackp--;
            return;
        }

        int n = 0;
        while (wl && wl->wl_word) {
            v = new variable;
            char *st = lstring::copy(wl->wl_word);
            Unquote(st);
            const char *ss = st;
            double *td = SPnum.parse(&ss, true);
            if (td && *ss == '\0')
                v->set_real(*td);
            else
                v->set_string(st);
            delete [] st;

            if (listv) {
                lv->set_next(v);
                lv = v;
            }
            else
                listv = lv = v;
            n++;
            wl = wl->wl_next;
        }
        *pv = listv;
        *pc = n;
    }
}


// Pop the previous argc/argv values.  If the stack is empty, unset
// argc and argv.
//
void
CshPar::PopArg()
{
    if (stackp == 0)
        return;
    variable *vv = cp_vardb->get("argv");
    variable *vc = cp_vardb->get("argc");
    stackp--;
    if (stackp == 0) {
        if (vv) {
            cp_vardb->remove("argv");
            variable::destroy(vv);
        }
        if (vc) {
            cp_vardb->remove("argc");
            variable::destroy(vc);
        }
    }
    else {
        if (vv)
            vv->set_list(av_stack[stackp].av);
        if (vc)
            vc->set_integer(av_stack[stackp].ac);
    }
}


void
CshPar::ClearArg()
{
    while (stackp)
        PopArg();
}


// Return a pointer to the first char following a $ or \$ token
//
char *
CshPar::dollartok(char *tok)
{
    char *t = tok;
    if (*tok == '\\') {
        t += 2;
        while (*t) {
            if (isspace(*t))
                break;
            if (*t == '\\' && *(t+1) == cp_dol)
                break;
            if (*t == cp_dol)
                break;
            t++;
        }
    }
    else {
        // *tok == '$'
        if (*++t == '\0')
            return (t);
        if  (isalnum(*t) || lstring::instr(VALIDCHARS, *t) ||
                *t == cp_dol)
            t++;
        if (*t == '\0')
            return (t);

        int lev1 = 0, lev2 = 0;
        for ( ; *t; t++) {
            if (isalnum(*t))
                continue;
            if (*t == '[') {
                lev1++;
                continue;
            }
            if (*t == '(') {
                lev2++;
                continue;
            }
            if (*t == ']') {
                if (lev1) lev1--;
                continue;
            }
            if (*t == ')') {
                if (lev2) lev2--;
                continue;
            }
            if (lstring::instr(VALIDCHARS, *t))
                continue;
            if (lev1 || lev2)
                continue;
            break;
        }
    }
    return (t);
}

