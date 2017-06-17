
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
 $Id: lstring.h,v 1.42 2017/04/12 05:02:41 stevew Exp $
 *========================================================================*/

#ifndef LSTRING_H
#define LSTRING_H

#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>


namespace lstring {

    // externals in lstring.cc
    bool matching(char**, const char*);
    bool substring(const char*, const char*);
    bool cisubstring(const char*, const char*);
    void unquote_in_place(char*);
    bool is_rooted(const char*);
    int is_utf8(const char*);


    // Return true if the two characters are equal, case insensitive.
    //
    inline bool ciceq(int c1, int c2)
    {
        return ((isupper(c1) ? tolower(c1) : c1) ==
            (isupper(c2) ? tolower(c2) : c2));
    }


    // Return a copy of the string arg.
    //
    inline char *copy(const char *str)
    {
        if (!str)
            return (0);
        char *s = new char[strlen(str) + 1];
        strcpy(s, str);
        return (s);
    }


    // Take a string allocated by a C function, copy to a C++
    // allocation with new[], and free the original string.  This is
    // mostly to keep valgrind happy, as it seems that delete[] and
    // free() are pretty much interchangeable for strings, at least in
    // Linux/MinGW/Cygwin/OS X.
    //
    inline char *tocpp(char *str)
    {
        if (!str)
            return (0);
        char *s = new char[strlen(str) + 1];
        strcpy(s, str);
        free(str);
        return (s);
    }


    // Append s2 to s1, free s1.
    //
    inline char *build_str(char *s1, const char *s2)
    {
        if (!s2 || !*s2)
            return (s1);
        if (!s1 || !*s1) {
            delete [] s1;
            return (copy(s2));
        }

        char *s, *str;
        s = str = new char[strlen(s1) + strlen(s2) + 1];
        char *t = s1;
        while (*s1) *s++ = *s1++;
        while (*s2) *s++ = *s2++;
        *s = '\0';
        delete [] t;
        return (str);
    }


    // A stpcpy() equivalent.
    //
    inline char *stpcpy(char *s, const char *src)
    {
        if (src)
            while ((*s = *src++) != 0) s++;
        return (s);
    }


    // Append one character to a string. Don't check for overflow.
    //
    inline void appendc(char *s, char c)
    {
        if (s) {
            while (*s)
                s++;
            *s++ = c;
            *s = 0;
        }
    }


    // String equality, null string matches nothing.
    //
    inline bool eq(const char *p, const char *s)
    {
        if (!p || !s)
            return (false);
        return (!strcmp(p, s));
    }


    // Case insensitive string equality, null string matches nothing.
    //
    inline bool cieq(const char *p, const char *s)
    {
        if (!p || !s)
            return (false);
        return (!strcasecmp(p, s));
    }


    // Return true if pf is a leading substring of str.
    //
    inline bool prefix(const char *pf, const char *str)
    {
        if (!pf || !*pf || !str)
            return (false);
        return (!strncmp(pf, str, strlen(pf)));
    }


    // Case insensitive prefix.
    //
    inline bool ciprefix(const char *pf, const char *str)
    {
        if (!pf || !*pf || !str)
            return (false);
        return (!strncasecmp(pf, str, strlen(pf)));
    }


    // Return true if sf is a trailing substring of str.
    //
    inline bool suffix(const char *sf, const char *str)
    {
        if (!sf || !*sf || !str)
            return (false);
        const char *t = str + strlen(str) - strlen(sf);
        return (t >= str && !strcmp(sf, str));
    }


    // Case insensitive suffix.
    //
    inline bool cisuffix(const char *sf, const char *str)
    {
        if (!sf || !*sf || !str)
            return (false);
        const char *t = str + strlen(str) - strlen(sf);
        return (t >= str && !strcasecmp(sf, str));
    }


    // Return true if pf matches the first word in str.
    //
    inline bool match(const char *pf, const char *str)
    {
        if (!pf || !*pf || !str)
            return (false);
        while (isspace(*str))
            str++;
        int n = strlen(pf);
        if (strncmp(pf, str, n))
            return (false);
        str += n;
        return (!*str || isspace(*str));
    }


    // Return true if pf matches (case insensitive) the first word in str.
    //
    inline bool cimatch(const char *pf, const char *str)
    {
        if (!pf || !*pf || !str)
            return (false);
        while (isspace(*str))
            str++;
        int n = strlen(pf);
        if (strncasecmp(pf, str, n))
            return (false);
        str += n;
        return (!*str || isspace(*str));
    }


    //
    // The next two functions basically replace strchr(), which produces a
    // positive test for a '\0' character since it matches the terminator.
    // The following functions return a fail in this case.
    //

    // Return true if c is in str.
    //
    inline bool instr(const char *str, int c)
    {
        if (str) {
            for (const char *s = str; *s; s++)
                if (c == *s)
                    return (true);
        }
        return (false);
    }


    // Return true if c is in str, case insensitive.
    //
    inline bool ciinstr(const char *str, int c)
    {
        if (str) {
            if (isupper(c))
                c = tolower(c);
            for (const char *s = str; *s; s++)
                if (c == (isupper(*s) ? tolower(*s) : *s))
                    return (true);
        }
        return (false);
    }


    // Try to identify an integer that begins a string. Stop when a non-
    // numeric character is reached.
    //
    inline unsigned int scannum(const char *str)
    {
        unsigned int i = 0;
        if (str)
            while (isdigit(*str))
                i = i * 10 + *(str++) - '0';
        return (i);
    }


    // Convert str to lower case.
    //
    inline void strtolower(char *str)
    {
        if (str) {
            while (*str) {
                if (isupper(*str))
                    *str = tolower(*str);
                str++;
            }
        }
    }


    // Convert str to upper case.
    //
    inline void strtoupper(char *str)
    {
        if (str) {
            while (*str) {
                if (islower(*str))
                    *str = toupper(*str);
                str++;
            }
        }
    }


    // Convert to UNIX style path.
    //
    inline void unix_path(char *path)
    {
        if (path) {
            for (char *s = path; *s; s++) {
                if (*s == '\\')
                    *s = '/';
            }
        }
    }


    // Convert to DOS style path.
    //
    inline void dos_path(char *path)
    {
        if (path) {
            for (char *s = path; *s; s++) {
                if (*s == '/')
                    *s = '\\';
            }
        }
    }


    // Return true if c is a directory separator character.
    //
    inline bool is_dirsep(char c)
    {
        if (c == '/')
            return (true);
#ifdef WIN32
        if (c == '\\')
            return (true);
#endif
        return (false);
    }


    // Return a pointer to the first directory separator character found
    // in string.
    //
    template <class T> inline T *
    strdirsep(T *string)
    {
        if (!string)
            return (0);
        for (T *s = string; *s; s++) {
            if (*s == '/'
#ifdef WIN32
                || *s == '\\'
#endif
            ) return (s);
        }
        return (0);
    }


    // Return a pointer to the last directory separator character found
    // in string.
    //
    template <class T> inline T *
    strrdirsep(T *string)
    {
        if (!string)
            return (0);
        for (T *s = string + strlen(string) - 1; s >= string; s--) {
            if (*s == '/'
#ifdef WIN32
                || *s == '\\'
#endif
            ) return (s);
        }
        return (0);
    }


    // Return the base file name.
    //
    template <class T> inline T *
    strip_path(T *sname)
    {
        T *s = strrdirsep(sname);
        if (s) {
            s++;
            sname = s;
        }
        return (sname);
    }


    // Advance past leading white space.
    //
    template <class T> inline T*
    strip_space(T *s)
    {
        if (s) {
            while (isspace(*s))
                s++;
        }
        return (s);
    }


    // Strip any leading or trailing white space.
    //
    inline char *
    clip_space(const char *str)
    {
        if (!str)
            return (0);
        const char *s = str;
        while (isspace(*s))
            s++;
        const char *t = s + strlen(s) - 1;
        while (t > s && isspace(*t))
            t--;
        t++;
        if (t <= s)
            return (0);
        int sz = t-s;
        char *rt = new char[sz + 1]; 
        strncpy(rt, s, sz); 
        rt[sz] = 0;
        return (rt);
    }


    inline bool
    is_sepchar(char c, const char *sepchars)
    {
        if (!c)
            return (false);
        return (isspace(c) || (sepchars && strchr(sepchars, c)));
    }

    inline bool
    is_not_sepchar(char c, const char *sepchars)
    {
        if (!c)
            return (false);
        return (!isspace(c) && (!sepchars || !strchr(sepchars, c)));
    }


    // Return a malloc'ed token, advance the pointer to next token. 
    // Tokens are separated by white space or characters in sepchars,
    // it this is not null.
    //
    template <class T> char *
    gettok(T **s, const char *sepchars = 0)
    {
        if (s == 0 || *s == 0)
            return (0);
        while (is_sepchar(**s, sepchars))
            (*s)++;
        if (!**s)
            return (0);
        T *st = *s;
        while (is_not_sepchar(**s, sepchars))
            (*s)++;
        char *cbuf = new char[*s - st + 1];
        char *c = cbuf;
        while (st < *s)
            *c++ = *st++;
        *c = 0;
        while (is_sepchar(**s, sepchars))
            (*s)++;
        return (cbuf);
    }


    // Advance past the terminating quote character, copying into bf
    // if given.  If inclq, include the quotes in bf.  The character
    // referenced by s should be a single or double quote (no checking
    // here).  This keeps track of nesting.  Note that bf is *not*
    // given a terminating 0.
    //
    template <class T> void
    advq(T **s, char **bf, bool inclq)
    {
        char quotechar = **s;
        if (bf && inclq)
            *(*bf)++ = quotechar;
        (*s)++;
        bool bs = false;
        while (**s && (**s != quotechar || bs)) {
            if (**s == '\\') {
                bs = true;
                if (bf)
                    *(*bf)++ = **s;
                (*s)++;
            }
            else if ((**s == '"' || **s == '\'') && !bs)
                advq(s, bf, true);
            else {
                if (bf) {
                    if (**s == quotechar && bs && !inclq)
                        (*bf)--;
                    *(*bf)++ = **s;
                }
                bs = false;
                (*s)++;
            }
        }
        if (**s == quotechar) {
            if (bf && inclq)
                *(*bf)++ = **s;
            (*s)++;
        }
    }


    // As for gettok(), but handle single and double quoted
    // substrings.  The outermost quotes are stripped, and the
    // enclosing characters are added to adjacent tokens, if any. 
    // Alternate nested quoting is preserved.  The backslash can be
    // used to hide the quote marks.
    //
    // Unlike gettok(), this can return an empty string.
    //
    template <class T> char *
    getqtok(T **s, const char *sepchars = 0)
    {
        if (s == 0 || *s == 0)
            return (0);
        while (is_sepchar(**s, sepchars))
            (*s)++;
        if (!**s)
            return (0);
        T *st = *s;
        bool bs = false;
        while (is_not_sepchar(**s, sepchars)) {
            if (**s == '\\') {
                bs = true;
                (*s)++;
            }
            else if ((**s == '"' || **s == '\'') && !bs)
                advq(s, 0, false);
            else {
                bs = false;
                (*s)++;
            }
        }
        char *cbuf = new char[*s - st + 1];
        char *c = cbuf;
        bs = false;
        while (st < *s) {
            if (*st == '\\') {
                bs = true;
                *c++ = *st++;
            }
            else if ((*st == '"' || *st == '\'') && !bs)
                advq(&st, &c, false);
            else {
                if ((*st == '"' || *st == '\'') && bs)
                    c--;
                bs = false;
                *c++ = *st++;
            }
        }
        *c = 0;
        while (is_sepchar(**s, sepchars))
            (*s)++;
        return (cbuf);
    }


    // Copy token to caller's buffer dst, advance the pointer to next
    // token.  Returns true if token is valid.
    //
    template <class T> int
    copytok(char *dst, T **s, const char *sepchars = 0)
    {
        if (s == 0 || *s == 0)
            return (false);
        while (is_sepchar(**s, sepchars))
            (*s)++;
        if (!**s)
            return (false);
        while (is_not_sepchar(**s, sepchars))
            *dst++ = *(*s)++;
        *dst = '\0';
        while (is_sepchar(**s, sepchars))
            (*s)++;
        return (true);
    }


    // Copy token to caller's buffer dst, advance the pointer to next
    // token.  Leave a space after the copied token.  Returns true if
    // token is valid.
    //
    template <class T> int
    copytok1(char *dst, T **s, const char *sepchars = 0)
    {
        if (s == 0 || *s == 0)
            return (false);
        while (is_sepchar(**s, sepchars))
            (*s)++;
        if (!**s)
            return (false);
        while (is_not_sepchar(**s, sepchars))
            *dst++ = *(*s)++;
        *dst++ = ' ';
        *dst++ = '\0';
        while (is_sepchar(**s, sepchars))
            (*s)++;
        return (true);
    }


    // Advance the pointer to next token.
    //
    template <class T> bool
    advtok(T **s, const char *sepchars = 0)
    {
        if (s == 0 || *s == 0)
            return (false);
        while (is_sepchar(**s, sepchars))
            (*s)++;
        if (!**s)
            return (false);
        while (is_not_sepchar(**s, sepchars))
            (*s)++;
        while (is_sepchar(**s, sepchars))
            (*s)++;
        return (true);
    }


    // As for advtok(), but handle single and double quoted substrings.
    //
    template <class T> bool
    advqtok(T **s, const char *sepchars = 0)
    {
        if (s == 0 || *s == 0)
            return (false);
        while (is_sepchar(**s, sepchars))
            (*s)++;
        if (!**s)
            return (false);
        bool bs = false;
        while (is_not_sepchar(**s, sepchars)) {
            if (**s == '\\') {
                bs = true;
                (*s)++;
            }
            else if ((**s == '"' || **s == '\'') && !bs)
                advq(s, 0, false);
            else {
                bs = false;
                (*s)++;
            }
        }
        while (is_sepchar(**s, sepchars))
            (*s)++;
        return (true);
    }
}


// General purpose linked char* list.
//
struct stringlist
{
    stringlist()
        {
            next = 0;
            string = 0;
        }

    stringlist(char *s, stringlist *n)
        {
            string = s; next = n;
        }

    int length()
        {
            int n=0;
            for (stringlist *l = this; l; l = l->next)
                n++;
            return (n);
        }

    void free()
        {
            stringlist *cn;
            for (stringlist *c = this; c; c = cn) {
                cn = c->next;
                delete [] c->string;
                delete c;
            }
        }

    char *flatten(const char*);
    stringlist *dup();
    void reverse();
    void sort(bool(*)(const char*, const char*) = 0);
    struct stringcolumn  *get_columns(int);
    char *col_format(int, int** = 0);

    stringlist *next;
    char *string;
};

// A general purpose linked list of string/string pairs.
//
struct string2list
{
    string2list()
        {
            next = 0;
            string = 0;
            value = 0;
        }

    string2list(char *s, char *v, string2list *n)
        {
            next = n;
            string = s;
            value = v;
        }

    int length()
        {
            int n=0;
            for (string2list *l = this; l; l = l->next)
                n++;
            return (n);
        }

    void free()
        {
            string2list *cn;
            for (string2list *c = this; c; c = cn) {
                cn = c->next;
                delete [] c->string;
                delete [] c->value;
                delete c;
            }
        }

    string2list *next;
    char *string;
    char *value;
};

// General purpose linked char* and number list.
//
struct stringnumlist
{
    stringnumlist()
        {
            next = 0;
            string = 0;
            num = 0;
        }

    stringnumlist(char *s, int n, stringnumlist *nx)
        {
            string = s;
            num = n;
            next = nx;
        }

    int length()
        {
            int n=0;
            for (stringnumlist *l = this; l; l = l->next)
                n++;
            return (n);
        }

    void free()
        {
            stringnumlist *cn;
            for (stringnumlist *c = this; c; c = cn) {
                cn = c->next;
                delete [] c->string;
                delete c;
            }
        }

    stringnumlist *dup();
    void sort_by_string();
    void sort_by_num();

    stringnumlist *next;
    char *string;
    int num;
};


// A representation of a column of words, used for formatting lists of
// file names, for example.
//
struct stringcolumn
{
    stringcolumn(stringlist**, int);
    ~stringcolumn()
        {
            sc_words->free();
        }

    void pad();
    void cat(const stringcolumn*);
    void strip_trail_sp();

    stringlist *words()     const { return (sc_words); }
    int col_width()         const { return (sc_wid); }
    int col_height()        const { return (sc_hei); }
    stringcolumn *next()          { return (sc_next); }
    void set_next(stringcolumn *c) { sc_next = c; }

    static int col_spa()          { return (2); }

private:
    stringlist *sc_words;   // list of words
    int sc_wid;             // max string length
    int sc_hei;             // length of list
    stringcolumn *sc_next;  // column to right
};


// General purpose linked list template.
//
template <class T>
struct itemlist
{
    itemlist()
        {
            next = 0;
            item = 0;
        }

    itemlist(T i, itemlist *n)
        {
            next = n;
            item = i;
        }

    int length()
        {
            int n=0;
            for (itemlist *l = this; l; l = l->next)
                n++;
            return (n);
        }

    void free()
        {
            itemlist *cn;
            for (itemlist *c = this; c; c = cn) {
                cn = c->next;
                delete c;
            }
        }

    itemlist    *next;
    T           item;
};


// Class for maintaining an arbitrarily long string.  This is used when
// parsing label text, and elsewhere.
//
struct sLstr
{
    sLstr()
        {
            ls_len = 0;
            ls_rlen = 0;
            ls_str = 0;
        }

    ~sLstr()
        {
            delete [] ls_str;
        }

    size_t length()             const { return (ls_len); }
    const char *string()        const { return (ls_str); }
    void clear()
        {
            ls_str = 0;
            ls_len = ls_rlen = 0;
        }

    char *string_clear()
        {
            char *t = ls_str;
            clear();
            return (t);
        }

    void free()
        {
            delete [] ls_str;
            clear();
        }

    bool rem_c()
        {
            if (ls_len) {
                ls_len--;
                ls_str[ls_len] = 0;
                return (true);
            }
            return (false);
        }

    void to_dosdirsep()
        {
            for (size_t i = 0; i < ls_len; i++) {
                if (ls_str[i] == '/')
                    ls_str[i] = '\\';
            }
        }

    void to_unixdirsep()
        {
            for (size_t i = 0; i < ls_len; i++) {
                if (ls_str[i] == '\\')
                    ls_str[i] = '/';
            }
        }

    void add(const char*);
    void append(const char*, const char*);
    void add_c(char, bool = false);
    void add_i(long);
    void add_u(unsigned long);
    void add_h(unsigned long, bool=false);
    void add_d(double, int=6, bool=false);
    void add_g(double);
    void add_e(double, int=6);
    void truncate(unsigned int, const char*);
    char *string_trim();

private:
    size_t ls_len;
    size_t ls_rlen;
    char *ls_str;
};


// Struct to hold the contents of a macro, in a list of macros
//
struct sMacro
{
    friend class MacroHandler;

    sMacro(char *na, char *t, int ac, char **av, bool pd, bool ns, sMacro *nx)
        {
            next = nx;
            m_name = na;
            m_text = t;
            m_argc = ac;
            m_argv = av;
            m_predef = pd;
            m_nosub = ns;
        }

    ~sMacro()
        {
            delete [] m_name;
            delete [] m_text;
            for (int i = 0; i < m_argc; i++)
                delete [] m_argv[i];
            delete [] m_argv;
        }

    void free()
        {
            sMacro *m = this;
            while (m) {
                sMacro *mx = m;
                m = m->next;
                delete mx;
            }
        }

    bool nosub()    { return (m_nosub); }

    char **get_actual(char**);
    void free_actual(char**);
    const char *find_formal(const char*, char**);
    char *subst_text(char**);
    void print(FILE*, const char*, bool);

private:
    sMacro *next;
    char *m_name;       // macro name
    char *m_text;       // substitution text
    int m_argc;         // number of arguments
    char **m_argv;      // formal argument names
    bool m_predef;      // "predefined" macro
    bool m_nosub;       // don't do substitution
};


// Main class for macro handling
//
class MacroHandler
{
public:
    MacroHandler() { Macros = 0; }
    ~MacroHandler() { Macros->free(); }

    bool parse_macro(const char*, bool, bool = false);
    char *macro_expand(const char*, bool* = 0, const char* = 0, int = 0);
    const char *find_text(const char*);
    void print(FILE*, const char*, bool);

private:
    sMacro *Macros;
};


// These can be instantiated on the stack for objects that should be
// deleted when out of scope.  This is often more convenient than
// explicit deletions.

template <class T>
struct GCobject
{
    GCobject(T x)       { ptr = x; }
    ~GCobject()         { delete ptr; }
    void set(T x)       { ptr = x; }
    void clear()        { ptr = 0; }
    void flush()        { delete ptr; ptr = 0; }

private:
    T ptr;
};

template <class T>
struct GCarray
{
    GCarray(T x)        { ptr = x; }
    ~GCarray()          { delete [] ptr; }
    void set(T x)       { ptr = x; }
    void clear()        { ptr = 0; }
    void flush()        { delete [] ptr; ptr = 0; }

private:
    T ptr;
};

template <class T>
struct GCfree
{
    GCfree(T x)         { ptr = x; }
    ~GCfree()           { ptr->free(); }
    void set(T x)       { ptr = x; }
    void clear()        { ptr = 0; }
    void flush()        { ptr->free(); ptr = 0; }

private:
    T ptr;
};

#endif
