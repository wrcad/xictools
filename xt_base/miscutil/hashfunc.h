
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

#ifndef HASHFUNC_H
#define HASHFUNC_H

#include <ctype.h>

//
// Hash functions
//

//-----------------------------------------------------------------------------
// Hash function for unsigned long (same size as a pointer).

#if 1
// The Jenkins hash is a bit faster (on Inter x86), particularly for
// 64-bits.

#if defined(__GNUC__) && (defined(i386) || defined(__x86_64__))
#define rol_bits(x, k) ({ unsigned int t; \
    asm("roll %1,%0" : "=g" (t) : "cI" (k), "0" (x)); t; })
#else
#define rol_bits(x, k) (((x)<<(k)) | ((x)>>(32-(k))))
#endif


// Hashing function for 4/8 byte integers, adapted from
// lookup3.c, by Bob Jenkins, May 2006, Public Domain.
//
inline unsigned int
number_hash(unsigned long n, unsigned int hashmask)
{
    unsigned int a, b, c; // Assumes 32-bit int.

    if (sizeof(unsigned long) == 8) {
        union { unsigned long l; unsigned int i[2]; } u;
        a = b = c = 0xdeadbeef + 8;
        u.l = n;
        b += u.i[1];
        a += u.i[0];
    }
    else {
        a = b = c = 0xdeadbeef + 4;
        a += n;
    }

    c ^= b; c -= rol_bits(b, 14);
    a ^= c; a -= rol_bits(c, 11);
    b ^= a; b -= rol_bits(a, 25);
    c ^= b; c -= rol_bits(b, 16);
    a ^= c; a -= rol_bits(c, 4);
    b ^= a; b -= rol_bits(a, 14);
    c ^= b; c -= rol_bits(b, 24);

    return (c & hashmask);
}

// Keep this macro private.
#undef rol_bits


#else

// The Fowler/Noll/Vo (FNV-1a) Hash
// http://www.isthe.com/chongo/tech/comp/fnv/
//
// 64BIT
//#define FNV_PRIME 0x100000001b3ULL
//#define FNV1_INIT 0xcbf29ce484222325ULL
// 32BIT
//#define FNV_PRIME 0x01000193
//#define FNV1_INIT 0x811c9dc5
//
inline unsigned int
number_hash(unsigned long n, unsigned int hashmask)
{
    if (!hashmask)
        return (0);
    unsigned char *s = (unsigned char*)&n;
    int i = sizeof(unsigned long);

    unsigned long hval, prime;
    if (i == 8) {
        prime = (unsigned long)0x100000001b3ULL;
        hval = (unsigned long)0xcbf29ce484222325ULL;
    }
    else {
        prime = 0x01000193;
        hval = 0x811c9dc5;
    }

    while (i) {
        hval ^= *s++;
        hval *= prime;
        hval ^= *s++;
        hval *= prime;
        hval ^= *s++;
        hval *= prime;
        hval ^= *s++;
        hval *= prime;
        i -= 4;
    }
    return ((unsigned int)hval & hashmask);
}

#endif


//-----------------------------------------------------------------------------
// An incremental hashing function for elememtal types, for hashing
// data structs (after Bernstein, comp.lang.c).
// Usage example:
//
// struct xxx
// {
//     unsigned int hash()
//     {
//         unsigned int k = INCR_HASH_INIT;
//         k = incr_hash(k, &m);
//         k = incr_hash(k, &x);
//         k = incr_hash(k, &y);
//         return (k);
//     }
//     double m;
//     int x;
//     int y;
// };
//


#define INCR_HASH_INIT 5381

// Use this for integers and reals.
//
template <class T>
unsigned int
incr_hash(unsigned int k, T *v)
{
    unsigned char *s = (unsigned char*)v;
    for (int i = sizeof(T); i > 0; i--)
        k = ((k << 5) + k) ^ *s++;
    return (k);
}


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


// String and number hash.  This is case-sensitive, and maps all
// negative indx values into -1.
//
inline unsigned int
string_num_hash(const char *str, int indx, unsigned int hashmask)
{
    if (!hashmask)
        return (0);
    if (indx < -1)
        indx = -1;
    unsigned int k = INCR_HASH_INIT;
    k = incr_hash_string(k, str);
    k = incr_hash(k, &indx);
    return (k & hashmask);
}


// Case-insensitive variation.
//
inline unsigned int
string_num_hash_ci(const char *str, int indx, unsigned int hashmask)
{
    if (!hashmask)
        return (0);
    if (indx < -1)
        indx = -1;
    unsigned int k = INCR_HASH_INIT;
    k = incr_hash_string_ci(k, str);
    k = incr_hash(k, &indx);
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


// A version that comparse case-sensitive strings and integers.  All
// negative integers are mapped to -1.
//
inline bool
string_num_compare(const char *s1, int n1, const char *s2, int n2)
{
    if (!str_compare(s1, s2))
        return (false);
    if (n1 < -1)
        n1 = -1;
    if (n2 < -1)
        n2 = -1;
    return (n1 == n2);
}


// Case-insensitive variation.
//
inline bool
string_num_compare_ci(const char *s1, int n1, const char *s2, int n2)
{
    if (!str_compare_ci(s1, s2))
        return (false);
    if (n1 < -1)
        n1 = -1;
    if (n2 < -1)
        n2 = -1;
    return (n1 == n2);
}

#endif

