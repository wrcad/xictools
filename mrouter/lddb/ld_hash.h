
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
 $Id: ld_hash.h,v 1.1 2017/02/07 04:19:44 stevew Exp $
 *========================================================================*/

#ifndef LD_HAsh_h
#define LD_HAsh_h

#include <ctype.h>


//
// ld_hash.h
//
// Hash table definitions for LDDB.
//

#define INCR_HASH_INIT 5381

// Use this for strings.
//
inline unsigned int
incr_hash_string(unsigned int k, const char *s)
{
    if (s) {
        unsigned char *t = (unsigned char*)s;
        while (*t)
            k = ((k << 5) + k) ^ *t++;
    }
    return (k);
}


// Use this for strings, for case-insensitive hash.
//
inline unsigned int
incr_hash_string_ci(unsigned int k, const char *s)
{
    if (s) {
        unsigned char *t = (unsigned char*)s;
        while (*t) {
            unsigned int c = *t++;
            if (isupper(c))
                c = tolower(c);
            k = ((k << 5) + k) ^ c;
        }
    }
    return (k);
}


//-----------------------------------------------------------------------------
// A string hashing function (Bernstein, comp.lang.c).
//
inline unsigned int
string_hash(const char *str, unsigned int hashmask)
{
    if (!hashmask || !str)
        return (0);
    unsigned int k = INCR_HASH_INIT;
    k = incr_hash_string(k, str);
    return (k & hashmask);
}


// Case-insensitive variation.
//
inline unsigned int
string_hash_ci(const char *str, unsigned int hashmask)
{
    if (!hashmask || !str)
        return (0);
    unsigned int k = INCR_HASH_INIT;
    k = incr_hash_string_ci(k, str);
    return (k & hashmask);
}


// A string comparison function that deals with nulls, return true if
// match.
//
inline bool
str_compare(const char *s1, const char *s2)
{
    if (s1 == s2)
        return (true);
    if (!s1 || !s2)
        return (false);
#if defined(i386) || defined(__x86_64__)
    // This seems a little faster under x86/Linux.
    for (;;) {
        if (!*s1 || !*s2)
            return (*s1 == *s2);
        const char *s10 = s1++;
        const char *s20 = s2++;
        if (!*s1 || !*s2)
            return (*(unsigned short*)s10 == *(unsigned short*)s20);
        s1++;
        s2++;
        if (!*s1 || !*s2)
            return (*s1 == *s2 &&
                *(unsigned short*)s10 == *(unsigned short*)s20);
        s1++;
        s2++;
        if (!*s1 || !*s2)
            return (*(unsigned int*)s10 == *(unsigned int*)s20);
        s1++;
        s2++;
            if (*(unsigned int*)s10 != *(unsigned int*)s20)
                return (false);
    }
#else
    while (*s1 && *s2) {
        if (*s1++ != *s2++)
            return (false);
    }
    return (*s1 == *s2);
#endif
}


// Case-insensitive variation.
//
inline bool
str_compare_ci(const char *s1, const char *s2)
{
    if (s1 == s2)
        return (true);
    if (!s1 || !s2)
        return (false);
    while (*s1 && *s2) {
        int i1 = *s1++;
        int i2 = *s2++;
        if (isupper(i1))
            i1 = tolower(i1);
        if (isupper(i2))
            i2 = tolower(i2);
        if (i1 != i2)
            return (false);
    }
    return (*s1 == *s2);
}


//
// The hash table.  This is specialized to our needs in LDDB:
// 1.  The tab strings are not copied, they must remain valid in the
//     application as long as the table is in use.
// 2.  The payload is an integer (which will be an array index).
//     This is a long to make use of space with 8-byte alignment.
// 3.  The table is never shrunk, and elements are never removed.
// 4.  We know how many elements are needed before insertion starts.


// "not found" return value
#define ST_NIL          (unsigned long)(-1)

// Flags
#define ST_CASE_INSENS  0x1

// Initial width - 1 (width MUST BE POWER OF 2)
#define ST_START_MASK   31

// Symbol table entry
struct dbHtabEnt
{
    dbHtabEnt()
        {
            stNext = 0;
            stTag = 0;
            stData = 0;
        }

    dbHtabEnt(const char *t, unsigned long d)
        {
            stNext = 0;
            stTag = t;
            stData = d;
        }

    dbHtabEnt *stNext;
    const char *stTag;
    unsigned long stData;
};



// Symbol table.
//
struct dbHtab
{
    dbHtab(bool, int);
    ~dbHtab();

    void    incsize(int);
    bool    add(const char*, unsigned long);
    unsigned long get(const char*);

    unsigned int allocated()    const { return (tNumAllocated); }

private:
    dbHtabEnt *newent();
    void rehash();

    unsigned int str_hash(const char *str, unsigned int mask)
        {
            return ((tFlags & ST_CASE_INSENS) ?
                string_hash_ci(str, mask) : string_hash(str, mask));
        }


    bool str_comp(const char *s1, const char *s2)
        {
            return ((tFlags & ST_CASE_INSENS)
                ? str_compare_ci(s1, s2) : str_compare(s1, s2));
        }

    dbHtabEnt **tEnt;       // element list heads
    u_int tNumAllocated;    // element count
    u_int tMask;            // hashsize - 1, hashsize is power 2
    u_int tFlags;           // options
    u_int tSize;            // size of element pool
    dbHtabEnt *tPool;       // element pool
};

#endif

