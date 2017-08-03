
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

#include "lstring.h"
#include "miscmath.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <algorithm>

//
// String manipulation classes and functions
//


// Static function.
// Return a string containing the list entries, with term added to the
// end of each entry, if not null, and not already there.
//
char *
stringlist::flatten(const stringlist *thissl, const char *term)
{
    if (!thissl)
        return (lstring::copy(""));
    int tlen = term ? strlen(term) : 0;
    int cnt = 0;
    for (const stringlist *s = thissl; s; s = s->next)
        cnt += strlen(s->string) + tlen;

    char *str = new char[cnt + 1];
    char *t = str;
    cnt = 0;
    for (const stringlist *s = thissl; s; s = s->next) {
        strcpy(t+cnt, s->string);
        int ncnt = strlen(s->string);
        cnt += ncnt;
        if (tlen) {
            if (ncnt >= tlen && !strcmp(t+cnt-tlen, term))
                continue;
            strcpy(t+cnt, term);
            cnt += tlen;
        }
    }
    return (str);
}


// Static function.
// Return a copy of the stringlist.
//
stringlist *
stringlist::dup(const stringlist *thissl)
{
    stringlist *s0 = 0, *se = 0;
    for (const stringlist *s = thissl; s; s = s->next) {
        if (!s0)
            s0 = se = new stringlist(lstring::copy(s->string), 0);
        else {
            se->next = new stringlist(lstring::copy(s->string), 0);
            se = se->next;
        }
    }
    return (s0);
}


// Static function.
// Reverse a stringlist.
//
stringlist *
stringlist::reverse(stringlist *thissl)
{
    if (!thissl)
        return (0);
    stringlist *sl = thissl->next, *s0 = 0;
    while (sl) {
        stringlist *sn = sl->next;
        sl->next = s0;
        s0 = sl;
        sl = sn;
    }
    return (s0);
}


namespace {
    inline bool
    comp(const char *s1, const char *s2)
    {
        return (strcmp(s1 ? s1 : "", s2 ? s2 : "") < 0);
    }
}


// Static function.
// Stringlist sort function.
//
void
stringlist::sort(stringlist *thissl, bool(*cf)(const char*, const char*))
{
    if (!thissl || !thissl->next)
        return;
    int len = length(thissl);
    char **aa = new char*[len];
    int cnt = 0;
    for (stringlist *s = thissl; s; s = s->next)
        aa[cnt++] = s->string;
    if (cf)
        std::sort(aa, aa + len, cf);
    else
        std::sort(aa, aa + len, comp);
    cnt = 0;
    for (stringlist *s = thissl; s; s = s->next)
        s->string = aa[cnt++];
    delete [] aa;
}


namespace {
    // Dividing the list into cols columns, compute the total width, len is
    // the length of s.
    //
    int totwidth(const stringlist *s, int len, int cols)
    {
        int rows = len/cols + (len % cols ? 1 : 0);
        int tw = 0;
        while (s) {
            int rw = 0;
            for (int i = 0; i < rows && s; i++) {
                int l = strlen(s->string);
                if (l > rw)
                    rw = l;
                s = s->next;
            }
            tw += rw + stringcolumn::col_spa();
        }
        return (tw);
    }
}


// Static function.
// Return a columnar listing of the words.  Each line length is less
// than charcols, if charcols > 25.  Sorting is not done here.
//
stringcolumn *
stringlist::get_columns(const stringlist *thissl, int charcols)
{
    if (!thissl)
        return (0);
    if (charcols < 25)
        charcols = 25;
    int len = length(thissl);

    int lnc = 1;
    for (int nc = 1; nc <= len; nc++) {
        int r = len/nc + (len % nc ? 1 : 0);
        int left = len - r*(nc-1);
        if (left <= 0 || left > r)
            continue;
        if (totwidth(thissl, len, nc) > charcols)
            break;
        lnc = nc;
    }
    int r = len/lnc + (len % lnc ? 1 : 0);

    stringcolumn *c0 = 0, *ce = 0;
    const stringlist *s = thissl;
    for (int i = 0; s; i++) {
        stringcolumn *c = new stringcolumn(&s, r);
        if (!c0)
            c0 = ce = c;
        else {
            ce->set_next(c);
            ce = ce->next();
        }
    }
    return (c0);
}


// Static function.
// Return a columnar listing of the words.  Each line length is less
// than charcols, if charcols > 25.  Sorting is not done here.  If the
// col_widp is not null, a 0-terminated list of the column character
// widths is returned.
//
char *
stringlist::col_format(const stringlist *thissl, int charcols, int **col_widp)
{
    stringcolumn *c0 = get_columns(thissl, charcols);
    if (col_widp) {
        int nc = 0;
        for (stringcolumn *c = c0; c; c = c->next())
            nc++;
        if (!nc)
            *col_widp = 0;
        else {
            int *n = new int[nc + 1];
            nc = 0;
            for (stringcolumn *c = c0; c; c = c->next())
                n[nc++] = c->col_width() + stringcolumn::col_spa();
            n[nc] = 0;
            *col_widp = n;
        }
    }
    if (!c0)
        return (lstring::copy(""));
    for (stringcolumn *c = c0; c; c = c->next())
        c->pad();
    stringcolumn *cn;
    for (stringcolumn *c = c0->next(); c; c = cn) {
        cn = c->next();
        c0->cat(c);
        delete c;
    }
    c0->strip_trail_sp();
    char *str = flatten(c0->words(), "\n");
    delete c0;
    return (str);
}


//------------------------------------------------------------------------------
// stringnumlist - list of string/number elements.

// Static function.
// Return a copy of the stringnumlist.
//
stringnumlist *
stringnumlist::dup(const stringnumlist *thissl)
{
    stringnumlist *s0 = 0, *se = 0;
    for (const stringnumlist *s = thissl; s; s = s->next) {
        if (!s0)
            s0 = se = new stringnumlist(lstring::copy(s->string), s->num, 0);
        else {
            se->next = new stringnumlist(lstring::copy(s->string), s->num, 0);
            se = se->next;
        }
    }
    return (s0);
}


namespace {
    struct sn
    {
        char *string;
        int num;
    };

    inline bool sn_str_cmp(const sn &s1, const sn &s2)
    {
        int r = strcmp(s1.string ? s1.string : "", s2.string ? s2.string : "");
        if (!r)
            return (s1.num < s2.num);
        return (r < 0);
    }

    inline bool sn_num_cmp(const sn &s1, const sn &s2)
    {
        int r = s1.num - s2.num;
        if (!r)
            r = strcmp(s1.string ? s1.string : "", s2.string ? s2.string : "");
        return (r < 0);
    }
}


// Static function.
// Sort alphabetically by string.
//
void
stringnumlist::sort_by_string(stringnumlist *thissl)
{
    if (!thissl || !thissl->next)
        return;
    int len = length(thissl);
    sn *aa = new sn[len];
    int cnt = 0;
    for (stringnumlist *s = thissl; s; s = s->next) {
        aa[cnt].string = s->string;
        aa[cnt++].num = s->num;
    }
    std::sort(aa, aa + len, sn_str_cmp);
    cnt = 0;
    for (stringnumlist *s = thissl; s; s = s->next) {
        s->string = aa[cnt].string;
        s->num = aa[cnt++].num;
    }
    delete [] aa;
}


// Static function.
// Sort ascending numerically by number.
//
void
stringnumlist::sort_by_num(stringnumlist *thissl)
{
    if (!thissl || !thissl->next)
        return;
    int len = length(thissl);
    sn *aa = new sn[len];
    int cnt = 0;
    for (stringnumlist *s = thissl; s; s = s->next) {
        aa[cnt].string = s->string;
        aa[cnt++].num = s->num;
    }
    std::sort(aa, aa + len, sn_num_cmp);
    cnt = 0;
    for (stringnumlist *s = thissl; s; s = s->next) {
        s->string = aa[cnt].string;
        s->num = aa[cnt++].num;
    }
    delete [] aa;
}
// End of stringnumlist functions.


//------------------------------------------------------------------------------
// stringcolumn     columnar word list helper

// Creation: use h entries from sp, advancing sp.
//
stringcolumn::stringcolumn(const stringlist **sp, int h)
{
    sc_words = 0;
    sc_wid = 0;
    sc_next = 0;
    stringlist *we = 0;
    const stringlist *s = *sp;
    for (sc_hei = 0; sc_hei < h && s; sc_hei++) {
        if (!sc_words)
            sc_words = we = new stringlist(lstring::copy(s->string), 0);
        else {
            we->next = new stringlist(lstring::copy(s->string), 0);
            we = we->next;
        }
        s = s->next;
        if (!we->string)
            we->string = lstring::copy("");
        int l = strlen(we->string);
        if (l > sc_wid)
            sc_wid = l;
    }
    *sp = s;
}


// Left justify and pad with space each word.
//
void
stringcolumn::pad()
{
    for (stringlist *s = sc_words; s; s = s->next) {
        char *t = new char[sc_wid + col_spa() + 1];
        char *e = lstring::stpcpy(t, s->string);
        char *end = t + sc_wid + col_spa();
        while (e < end)
            *e++ = ' ';
        *e = 0;
        delete [] s->string;
        s->string = t;
    }
}


// Add the stringcolumn to this.  The pad method must have been called
// on both.
//
void
stringcolumn::cat(const stringcolumn *sc)
{
    stringlist *s0 = sc_words;
    stringlist *s1 = sc->sc_words;
    while (s0 && s1) {
        char *t = new char[strlen(s0->string) + strlen(s1->string) + 1];
        char *e = lstring::stpcpy(t, s0->string);
        strcpy(e, s1->string);
        delete [] s0->string;
        s0->string = t;
        s0 = s0->next;
        s1 = s1->next;
    }
    sc_wid += sc->sc_wid + col_spa();
}


// Null out trailing white space.
//
void
stringcolumn::strip_trail_sp()
{
    for (stringlist *s = sc_words; s; s = s->next) {
        char *t = s->string + strlen(s->string) - 1;
        while (t >= s->string && isspace(*t))
            *t-- = 0;
    }
}
// End of stringcolumn functions.


//========================================================================
//
// A class for maintaining an arbitrarily long string.  This class
// is used when parsing label text, and elsewhere.
//
//========================================================================

// Append a string.
//
void
sLstr::add(const char *astr)
{
    if (!astr)
        return;
    int alen = strlen(astr);
    if (ls_len + alen < ls_rlen) {
        char *n = ls_str + ls_len;
        const char *o = astr;
        int i = alen;
        while (i--)
           *n++ = *o++;
        *n = '\0';
        ls_len += alen;
        return;
    }
    if (!ls_rlen)
        ls_rlen = 4;
    while (ls_len + alen >= ls_rlen)
        ls_rlen *= 2;
    char *nstr = new char[ls_rlen];
    char *n = nstr;
    const char *o = ls_str;
    int i = ls_len;
    while (i--)
        *n++ = *o++;
    o = astr;
    i = alen;
    while (i--)
        *n++ = *o++;
    *n = '\0';
    ls_len += alen;
    delete [] ls_str;
    ls_str = nstr;
}


// If not the first token, add sep if not null, then str.  The sep is
// skipped on the first call.
//
void
sLstr::append(const char *sep, const char *str)
{
    if (ls_str && sep)
        add(sep);
    add(str);
}


// Append a char.  This can be used for binary data if zok is true, in
// which case null bytes will be accepted.  In either case, an extra null
// byte will always be added the the end of the string.
//
void
sLstr::add_c(char c, bool zok)
{
    if (!c && !zok)
        return;
    if (ls_len+1 < ls_rlen) {
        ls_str[ls_len] = c;
        ls_len++;
        ls_str[ls_len] = '\0';
        return;
    }
    if (!ls_rlen)
        ls_rlen = 4;
    else
        ls_rlen *= 2;
    char *nstr = new char[ls_rlen];
    char *n = nstr;
    char *o = ls_str;
    int i = ls_len++;
    while (i--)
        *n++ = *o++;
    *n++ = c;
    *n = '\0';
    delete [] ls_str;
    ls_str = nstr;
}


// Append a decimal integer string representation.
//
void
sLstr::add_i(long i)
{
    char buf[64];
    mmItoA(buf, i);
    add(buf);
}


// Append a decimal unsigned integer string representation.
//
void
sLstr::add_u(unsigned long u)
{
    char buf[64];
    mmUtoA(buf, u);
    add(buf);
}


// Append a hex-encoded integer string representation.  If add_0x is
// true, include an "0x" prefix.
//
void
sLstr::add_h(unsigned long u, bool add_0x)
{
    char buf[64];
    char *t = buf;
    if (add_0x) {
        *t++ = '0';
        *t++ = 'x';
    }
    mmHtoA(t, u);
    add(buf);
}


// Append a floating point value representation, x.yyy notation (if
// possible, otherwise exponential).  If gstrip, strip off trailing 0s
// and maybe decimal point.
//
void
sLstr::add_d(double d, int prec, bool gstrip)
{
    char buf[64];
    mmDtoA(buf, d, prec, gstrip);
    add(buf);
}


// Append a floating point value in '%g' format.
//
void
sLstr::add_g(double d)
{
    char buf[64];
    sprintf(buf, "%g", d);
    add(buf);
}


// Append a floating point value in '%e' format.
//
void
sLstr::add_e(double d, int prec)
{
    char buf[64];
    if (prec < 1)
        prec = 1;
    else if (prec > 20)
        prec = 20;
    sprintf(buf, "%.*e", prec, d);
    add(buf);
}


// If the string is longer than tlen characters, truncate it to tlen
// charaters, and append appstr if not null.
//
void
sLstr::truncate(unsigned int tlen, const char *appstr)
{
    if (ls_len > tlen) {
        ls_len = tlen;
        ls_str[ls_len] = 0;
        if (appstr)
            add(appstr);
    }
}


// Return a trimmed copy.
//
char *
sLstr::string_trim()
{
    if (!ls_str)
        return (0);
    char *bf = new char[ls_len+1];
    memcpy(bf, ls_str, ls_len+1);
    return (bf);
}


//========================================================================
//
// A class for handling macros.  Macros are text tokens, optionally
// immediately followed by a set of formal arguments in parentheses.
//
//========================================================================


namespace {
    char *getarg(const char**);
    bool ftest(const char*);
    char *token(const char**);
}

// macros can have up to this many arguments
#define MAC_MAX_ARGS 25

// Macro names and formal arguments start with alpha or underscore,
// may contain digits
#define macro_start(c) (isalpha(c) || c == '_')
#define macro_char(c) (isalnum(c) || c == '_')


// Parse the macro in line.  The macro is specified as name(arg, arg...),
// and the rest of the line is taken as substitution text.  The parentheses
// are omitted if there are no arguments.  If success, add the macro to the
// list, or replace an existing macro with the same name and arg count,
// and return true.
//
// If nosub is true, the macro will be registered, but never used for
// substitutions.  Thus, one can #define FOO, and test #ifdef FOO,
// etc, but the token FOO would be untouched after macro expansion.
//
bool
MacroHandler::parse_macro(const char *line, bool predef, bool nosub)
{
    while (isspace(*line))
        line++;
    const char *s = line;
    if (!macro_start(*s))
        return (false);
    while (macro_char(*s))
        s++;
    if (*s == '(') {
        // macro has args
        char *name = new char[s - line + 1];
        strncpy(name, line, s - line);
        *(name + (s - line)) = 0;

        s++;
        char *args[MAC_MAX_ARGS];
        int ac;
        for (ac = 0; ac < MAC_MAX_ARGS; ac++) {
            char *t = getarg(&s);
            if (!t) {
                // parse error
                for (int i = 0; i < ac; i++)
                    delete [] args[i];
                delete [] name;
                return (false);
            }
            if (*t == ')') {
                delete [] t;
                args[ac] = 0;
                break;
            }
            if (!ftest(t)) {
                // parse error
                for (int i = 0; i < ac; i++)
                    delete [] args[i];
                delete [] name;
                delete [] t;
                return (false);
            }
            args[ac] = t;
        }
        char **nargs = new char*[ac + 1];
        for (int i = 0; i < ac; i++)
            nargs[i] = args[i];
        nargs[ac] = 0;
        while (isspace(*s))
            s++;
        char *text = lstring::copy(s);
        if (text) {
            char *t = text + strlen(text) - 1;
            while (t >= text && isspace(*t))
               *t-- = 0;
        }
        for (sMacro *m = Macros; m; m = m->next) {
            if (!strcmp(m->m_name, name) && m->m_argc == ac) {
                delete [] m->m_text;
                m->m_text = text;
                for (int i = 0; i < m->m_argc; i++)
                    delete [] m->m_argv[i];
                delete [] m->m_argv;
                m->m_argv = nargs;
                delete [] name;
                return (true);
            }
        }
        if (!Macros)
            Macros = new sMacro(name, text, ac, nargs, predef, nosub, 0);
        else {
            sMacro *m = Macros;
            while (m->next)
                m = m->next;
            m->next = new sMacro(name, text, ac, nargs, predef, nosub, 0);
        }
    }
    else if (isspace(*s) || !*s) {
        char *name = lstring::gettok(&line);
        if (!name)
            return (false);
        char *text = lstring::copy(line);
        if (text) {
            char *t = text + strlen(text) - 1;
            while (t >= text && isspace(*t))
               *t-- = 0;
        }
        for (sMacro *m = Macros; m; m = m->next) {
            if (!strcmp(m->m_name, name) && m->m_argc == 0) {
                delete [] m->m_text;
                m->m_text = text;
                delete [] name;
                return (true);
            }
        }
        if (!Macros)
            Macros = new sMacro(name, text, 0, 0, predef, nosub, 0);
        else {
            sMacro *m = Macros;
            while (m->next)
                m = m->next;
            m->next = new sMacro(name, text, 0, 0, predef, nosub, 0);
        }
    }
    else
        return (false);
    return (true);
}


// Return a macro-expanded version of the given string.  If a macro was
// actually expanded, sub is set true.  A copy of the string is returned
// in any case.
//
char *
MacroHandler::macro_expand(const char *str, bool *sub, const char *skipme,
    int depth)
{
    if (sub)
        *sub = false;
    if (!str)
        return (0);
    sLstr lstr;
    char *tok;
    while ((tok = token(&str)) != 0) {
        bool didsub = false;
        if (macro_start(*tok) && depth < 20) {
            for (sMacro *m = Macros; m; m = m->next) {
                if (m->nosub())
                    continue;
                if (!strcmp(tok, m->m_name) &&
                        (!skipme || strcmp(tok, skipme))) {
                    char **actual = m->get_actual((char**)&str);

                    // We're loose with argument count mismatch,
                    // however if the macro has args, the text must
                    // give args.
                    if (!actual && m->m_argc > 0)
                        continue;

                    char *a = m->subst_text(actual);
                    char *b = macro_expand(a, 0, tok, depth + 1);
                    delete [] a;
                    m->free_actual(actual);
                    lstr.add(b);
                    delete [] b;
                    didsub = true;
                    if (sub)
                        *sub = true;
                    break;
                }
            }
        }
        if (!didsub)
            lstr.add(tok);
        delete [] tok;
    }
    if (lstr.string())
        return (lstr.string_trim());
    return (lstring::copy(""));
}


// Return the substitution text if the def matches the name of a macro in
// the list.
//
const char *
MacroHandler::find_text(const char *def)
{
    if (def) {
        for (sMacro *m = Macros; m; m = m->next) {
            if (!strcmp(def, m->m_name)) {
                if (m->m_text)
                    return (m->m_text);
                else
                    return ((char*)"");
            }
        }
    }
    return (0);
}


// Print the macro list in the form "invoke name() text".
//
void
MacroHandler::print(FILE *fp, const char *invoke, bool show_predef)
{
    if (fp) {
        for (sMacro *m = Macros; m; m = m->next)
            m->print(fp, invoke, show_predef);
    }
}
// End of MacroHandler functions


// Return an array of the actual arguments obtained from str, which is
// advanced.  Return 0 if there are no formal args, or error.  The number
// of actual args is always equal to the number of formal args.  If too
// few are given, the unspecified args are empty.  If too many are given,
// extras are ignored.
//
char **
sMacro::get_actual(char **str)
{
    if (!m_argc)
        return (0);
    const char *s = *str;
    while (isspace(*s))
        s++;
    if (*s != '(')
        // no actual args
        return (0);
    s++;
    char **nargs = new char*[m_argc + 1];
    int ac;
    for (ac = 0; ac <= m_argc; ac++)
        nargs[ac] = 0;
    for (ac = 0; ac < MAC_MAX_ARGS; ac++) {
        char *t = getarg(&s);
        if (!t)
            break;
        if (*t == ')') {
            delete [] t;
            break;
        }
        if (ac < m_argc)
            nargs[ac] = t;
        else
            delete [] t;
    }
    for (ac = 0; ac < m_argc; ac++)
        if (nargs[ac] == 0)
            nargs[ac] = lstring::copy("");
    *str = (char*)s;
    return (nargs);
}


// Convenience function to delete an actual argument array.
//
void
sMacro::free_actual(char **actual)
{
    if (!actual)
        return;
    for (int i = 0; i < m_argc; i++)
        delete [] actual[i];
    delete [] actual;
}


// If arg matches the name of a formal argument, return the corresponding
// string from the actual argument array
//
const char *
sMacro::find_formal(const char *arg, char **actual)
{
    if (actual) {
        for (int i = 0; i < m_argc; i++) {
            if (!strcmp(m_argv[i], arg))
                return (actual[i]);
        }
    }
    return (0);
}


// Return the substitution text, with the actual arguments replacing the
// formal arguments (if any)
//
char *
sMacro::subst_text(char **actual)
{
    // Don't call this function with m_nosub set!
    if (m_nosub)
        return (0);

    sLstr lstr;
    const char *line = m_text;
    char *tok;
    while ((tok = token(&line)) != 0) {
        if (macro_start(*tok)) {
            const char *a = find_formal(tok, actual);
            if (a)
                lstr.add(a);
            else
                lstr.add(tok);
        }
        else
            lstr.add(tok);
        delete [] tok;
    }
    return (lstr.string_trim());
}


// Print the macro in the form "invoke name() text".
//
void
sMacro::print(FILE *fp, const char *invoke, bool show_predef)
{
    if (!show_predef && m_predef)
        return;
    if (invoke && *invoke)
        fprintf(fp, "%s %s", invoke, m_name);
    else
        fprintf(fp, "%s", m_name);
    if (m_argc > 0) {
        fprintf(fp, "(");
        for (int i = 0; i < m_argc; i++)
            fprintf(fp, i == m_argc-1 ? "%s" : "%s, ", m_argv[i]);
        fprintf(fp, ")");
    }
    if (m_text && *m_text)
        fprintf(fp, " %s", m_text);
    fprintf(fp, "\n");
}
// End of sMacro functions


namespace {
    // Test arg to make sure that it is a reasonable formal argument name.
    //
    bool
    ftest(const char *arg)
    {
        if (!arg)
            return (false);
        if (!macro_start(*arg))
            return (false);
        for (const char *t = arg; *t; t++) {
            if (!macro_char(*t))
                return (false);
        }
        return (true);
    }


    // Return the next argument.  Arguments are in the form (xxx, yyy,
    // ...).  The initial '(' must be skipped before the first call.
    // Each successive call yields an argument, with the final call
    // returning ")".  Zero is returned if there is a syntax error.
    //
    char *
    getarg(const char **str)
    {
        const char *s = *str;
        while (isspace(*s))
            s++;
        if (!*s)
            return (0);
        if (*s == ')') {
            s++;
            *str = s;
            return (lstring::copy(")"));
        }
        char buf[256];
        char *t = buf;
        int pcnt = 0;
        int bcnt = 0;
        bool insq = false;
        bool indq = false;
        const char *tstart = s;
        for (;;) {
            if (!*s)
                break;
            if (*s == '(' && !insq && !indq)
                pcnt++;
            else if (*s == ')' && !insq && !indq) {
                if (pcnt)
                    pcnt--;
                else
                    break;  // done
            }
            else if (*s == '[' && !insq && !indq)
                bcnt++;
            else if (*s == ']' && !insq && !indq) {
                if (bcnt)
                    bcnt--;
            }
            else if (*s == '"' && *(s-1) != '\\')
                indq ^= true;
            else if (*s == '\'' && *(s-1) != '\\')
                insq ^= true;
            else if (*s == ',') {
                if (!insq && !indq && !pcnt && !bcnt)
                    break;
            }
            *t++ = *s++;
        }
        *t-- = 0;
        while (t >= tstart && isspace(*t))
            *t-- = 0;

        while (isspace(*s) || *s == ',')
            s++;
        *str = s;
        return (lstring::copy(buf));
    }


    // Obtain a token and advance str.  Tokens are logical groups of
    // characters, such as a name string, space group, or punctuation
    // group.  There is no filtering; the original string is a
    // concatenation of the tokens.  Return 0 as the last token.
    //
    char *
    token(const char **str)
    {
        char buf[256];
        char *t = buf;
        const char *s = *str;
        if (!*s)
            return (0);
        if (isspace(*s)) {
            while (isspace(*s))
               *t++ = *s++;
        }
        else if (macro_start(*s)) {
            while (macro_char(*s))
               *t++ = *s++;
        }
        else if (ispunct(*s)) {
            while (ispunct(*s))
               *t++ = *s++;
        }
        else if (isdigit(*s)) {
            while (isdigit(*s) || *s == '.' || *s == 'e' || *s == 'E')
               *t++ = *s++;
        }
        else {
            while (*s && !isspace(*s) && !ispunct(*s) && !macro_char(*s))
               *t++ = *s++;
        }
        *str = s;
        *t = 0;
        return (lstring::copy(buf));
    }
}


#ifdef MACRO_TEST
// A test routine for the macro classes
//
int
main()
{
    FILE *fp = fopen("macin", "r");
    FILE *op = fopen("macout", "w");
    char buf[512];
    MacroHandler M;
    while (fgets(buf, 512, fp) != 0) {
        char *s = buf;
        char *tok = gettok(&s);
        if (!tok)
            continue;
        if (!strcmp(tok, "define")) {
            bool b = M.parse_macro(s);
            printf("parse macro %s\n", b ? "passed" : "failed");
            continue;
        }
        s = buf;
        char *str = M.macro_expand(s);
        fprintf(op, "%s\n", str);
    }
    fclose(fp);
    fclose(op);
    return (0);
}
#endif


//========================================================================
//
// Some string-handling utility functions.
//
//========================================================================

// If str matches what, advance str to the next token and strip
// trailing white space, and return true.
//
bool
lstring::matching(char **str, const char *what)
{
    if (str && *str && what) {
        char *s = *str;
        while (*what) {
            if (*what++ != *s++)
                return (false);
        }
        if (isspace(*s) || !*s) {
            while (isspace(*s))
                s++;
            *str = s;
            // trim trailing space
            char *t = s + strlen(s) - 1;
            while (isspace(*t) && t >= s)
                *t-- = 0;
            return (true);
        }
    }
    return (false);
}


// Determine whether sub is a substring of str.
//
bool
lstring::substring(const char *sub, const char *str)
{
    if (!str || !sub)
        return (false);
    while (*str) {
        if (*str == *sub) {
            const char *s;
            for (s = sub; *s; s++, str++) {
                if (!*str)
                    return (false);
                if (*s != *str)
                    break;
            }
            if (!*s)
                return (true);
        }
        else
            str++;
    }
    return (false);
}


// Determine whether sub is a substring of str, case insensitive.
//
bool
lstring::cisubstring(const char *sub, const char *str)
{
    if (!str || !sub)
        return (false);
    while (*str) {
        if (ciceq(*str, *sub)) {
            const char *s;
            for (s = sub; *s; s++, str++) {
                if (!*str)
                    return (false);
                if (!ciceq(*s, *str))
                    break;
            }
            if (!*s)
                return (true);
        }
        else
            str++;
    }
    return (false);
}


// Remove, in place, the top level of quoting.
//
void
lstring::unquote_in_place(char *s)
{
    if (!s)
        return;
    char *d = s;
    bool bs = false;
    while (*s) {
        if (*s == '\\') {
            bs = true;
            *d++ = *s++;
        }
        else if ((*s == '"' || *s == '\'') && !bs)
            advq(&s, &d, false);
        else {
            if ((*s == '"' || *s == '\'') && bs)
                d--;
            bs = false;
            *d++ = *s++;
        }
    }
    *d = 0;
}


// Return true if the string represents an absolute path.
//
bool
lstring::is_rooted(const char *string)
{
    if (!string || !*string)
        return (false);
    if (*string == '/')
        return (true);
    if (*string == '~')
        return (true);
#ifdef WIN32
    if (*string == '\\')
        return (true);
    if (strlen(string) >= 3 && isalpha(string[0]) && string[1] == ':' &&
            (string[2] == '/' || string[2] == '\\'))
        return (true);
#endif
    return (false);
}


// Check if the given string is a valid utf-8 sequence.
// 
// Return value :
// If the string is valid utf-8, 0 is returned.
// Else the position, starting from 1, is returned.
//
// Valid utf-8 sequences look like this :
// 0xxxxxxx
// 110xxxxx 10xxxxxx
// 1110xxxx 10xxxxxx 10xxxxxx
// 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
// 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
// 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
//
int
lstring::is_utf8(const char *instr)
{
    size_t len = strlen(instr);
    size_t continuation_bytes = 0;
    const unsigned char *str = (const unsigned char*)instr;

    size_t i = 0;
    while (i < len)
    {
        if (str[i] <= 0x7F)
            continuation_bytes = 0;
        else if (str[i] >= 0xC0 && str[i] <= 0xDF)
            continuation_bytes = 1;
        else if (str[i] >= 0xE0 && str[i] <= 0xEF)
            continuation_bytes = 2;
        else if (str[i] >= 0xF0 && str[i] <= 0xF4)
            continuation_bytes = 3;
        else
            return (i + 1);
        i += 1;
        while (i < len && continuation_bytes > 0 &&
                str[i] >= 0x80 && str[i] <= 0xBF) {
            i += 1;
            continuation_bytes -= 1;
        }
        if (continuation_bytes != 0)
            return (i + 1);
    }
    return (0);
}

