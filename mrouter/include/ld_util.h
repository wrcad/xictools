
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA 2016, http://wrcad.com       *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL STEPHEN R. WHITELEY OR WHITELEY     *
 *   RESEARCH INC. BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,   *
 *   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,   *
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        *
 *   DEALINGS IN THE SOFTWARE.                                            *
 *                                                                        *
 *   Licensed under the Apache License, Version 2.0 (the "License");      *
 *   you may not use this file except in compliance with the License.     *
 *   You may obtain a copy of the License at                              *
 *                                                                        *
 *        http://www.apache.org/licenses/LICENSE-2.0                      *
 *                                                                        *
 *   Unless required by applicable law or agreed to in writing, software  *
 *   distributed under the License is distributed on an "AS IS" BASIS,    *
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or      *
 *   implied. See the License for the specific language governing         *
 *   permissions and limitations under the License.                       *
 *                                                                        *
 *========================================================================*
 *                                                                        *
 * LEF/DEF Database and Maze Router.                                      *
 *                                                                        *
 * Stephen R. Whiteley (stevew@wrcad.com)                                 *
 * Whiteley Research Inc. (wrcad.com)                                     *
 *                                                                        *
 * Portions adapted from Qrouter by Tim Edwards,                          *
 * (www.opencircuitdesign.com) which used code by Steve Beccue.           *
 * See original headers where applicable.                                 *
 *                                                                        *
 *========================================================================*
 $Id: ld_util.h,v 1.4 2017/02/10 05:08:38 stevew Exp $
 *========================================================================*/

#ifndef LD_UTIL_H
#define LD_UTIL_H

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

//
// LEF/DEF Database.
//
// Some utilities, mostly for string handling.
//

namespace lddb {

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

    // A stpcpy() equivalent.
    //
    inline char *stpcpy(char *s, const char *src)
    {
        if (src)
            while ((*s = *src++) != 0) s++;
        return (s);
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
}


// Class for maintaining an arbitrarily long string.
//
struct dbLstr
{
    dbLstr()
        {
            ls_len = 0;
            ls_rlen = 0;
            ls_str = 0;
        }

    ~dbLstr()
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

    // Append a string.
    //
    void add(const char *astr)
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

    // Append a char.  This can be used for binary data if zok is
    // true, in which case null bytes will be accepted.  In either
    // case, an extra null byte will always be added the the end of
    // the string.
    //
    void add_c(char c, bool zok = false)
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

    // Return a trimmed copy.
    //
    char *string_trim()
        {
            if (!ls_str)
                return (0);
            char *bf = new char[ls_len+1];
            memcpy(bf, ls_str, ls_len+1);
            return (bf);
        }

private:
    size_t ls_len;
    size_t ls_rlen;
    char *ls_str;
};


// General purpose linked char* list.
//      
struct dbStringList
{
    dbStringList() 
        {
            next = 0;
            string = 0;  
        }
        
    dbStringList(char *s, dbStringList *n)
        {
            string = s; next = n;
        }   
                
    static int length(const dbStringList *thissl)
        {
            int n=0;
            for (const dbStringList *l = thissl; l; l = l->next)
                n++;
            return (n);
        }
    
    static void destroy(dbStringList *s)
        {
            while (s) {
                dbStringList *sx = s;
                s = s->next;
                delete sx->string;
                delete sx;
            }
        }

    dbStringList *next;
    char *string;
};

#endif

