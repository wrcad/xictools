
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
 * MOZY html help viewer files                                            *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

/*------------------------------------------------------------------------*
 * This file is part of the gtkhtm widget library.  The gtkhtm library
 * was derived from the gtk-xmhtml library by:
 *
 *   Stephen R. Whiteley  <stevew@wrcad.com>
 *   Whiteley Research Inc.
 *------------------------------------------------------------------------*
 *  The gtk-xmhtml widget was derived from the XmHTML library by
 *  Miguel de Icaza  <miguel@nuclecu.unam.mx> and others from the GNOME
 *  project.
 *  11/97 - 2/98
 *------------------------------------------------------------------------*
 * Author:  newt
 * (C)Copyright 1995-1996 Ripley Software Development
 * All Rights Reserved
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *------------------------------------------------------------------------*/

#include "htm_widget.h"
#include "htm_string.h"
#include "htm_escape.h"
#include "htm_hashtab.h"
#include <sys/types.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

// Character translation table for converting from upper to lower case
// Since this is a table lookup, it might perform better than the libc
// tolower routine on a number of systems.
//
namespace {
    const unsigned char translation_table[256]= {
    0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,
    24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,
    45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,97,98,
    99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,
    116,117,118,119,120,121,122,91,92,93,94,95,96,97,98,99,100,101,102,
    103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,
    120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,136,
    137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,
    154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,
    171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,
    188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,
    205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,
    222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,
    239,240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255 };
}

#define FastLower(x) (translation_table[(unsigned char)x])

namespace {
    char *to_ascii(int);
    char *to_roman(int);
#ifdef USE_UTF8
    const char *tokenToEscape(const char**, bool, htmWidget*);
#else
    char tokenToEscape(const char**, bool, htmWidget*);
#endif

    const char * const Ones[] =
        {"i", "ii", "iii", "iv", "v", "vi", "vii", "viii", "ix"};
    const char * const Tens[] =
        {"x", "xx", "xxx", "xl", "l", "lx", "lxx", "lxxx", "xc"};
    const char * const Hundreds[] =
        {"c", "cc", "ccc", "cd", "d", "dc", "dcc", "dccc", "cm"};
}


namespace htm
{
    // Replace character escape sequences with the appropriate char.
    //
    char *
    expandEscapes(const char *string, bool warn, htmWidget *html)
    {
        if (!string)
            return (0);

        sLstr lstr;

        // scan the entire text in search of escape codes (yuck)
        while (*string) {
            // fix 02/26/97-02, dp
            switch (*string) {
            case '&':
                {
#ifdef USE_UTF8
                    const char *esc = tokenToEscape(&string, warn, html);
                    if (esc) {
                        while (*esc)
                           lstr.add_c(*esc++);
                    }
                    else
                        lstr.add_c(*(string++));
#else
                    char escape;    // value of escape character
                    if ((escape = tokenToEscape(&string, warn, html)) != 0)
                        lstr.add_c(escape);
#endif
                }
                break;
            default:
                lstr.add_c(*(string++));
            }
            if (*string == 0)
                break;
        }
        return (lstr.string_trim());
    }


    // Return the abc representation of the given number.
    //
    const char *
    ToAsciiLower(int val)
    {
        return ((to_ascii(val)));
    }


    // Return the ABC representation of the given number.
    //
    const char *
    ToAsciiUpper(int val)
    {
        char *s = to_ascii(val);
        lstring::strtoupper(s);
        return (s);
    }


    // Convert numbers between 1-3999 to roman numerals, lowercase.
    //
    const char *
    ToRomanLower(int val)
    {
        return (to_roman(val));
    }


    // Convert numbers between 1-3999 to roman numerals, uppercase.
    //
    const char *
    ToRomanUpper(int val)
    {
        char *s = to_roman(val);
        lstring::strtoupper(s);
        return (s);
    }


    // Return the starting address of s2 in s1, ignoring case.
    //
    char *
    htm_strcasestr(const char *s1, const char *s2)
    {
        int i;
        const char *p1, *p2, *s = s1;
        for (p2 = s2, i = 0; *s; p2 = s2, i++, s++) {
            for (p1 = s; *p1 && *p2 && FastLower(*p1) == FastLower(*p2);
                    p1++, p2++)
                ;
            if (!*p2)
                break;
        }
        if (!*p2)
            return ((char*)s1 + i);
        return (0);
    }


    // Duplicate up to len chars of string s1.  Returns a ptr to the
    // duplicated string, padded with 0 if len is larger then s1.  Return
    // value is always 0 terminated.
    //
    char *
    htm_strndup(const char *s1, size_t len)
    {
        // no negative lengths
        if (!s1 || !*s1)
            return (0);

        // size of text + a terminating \0
        char *s2 = new char[len+1];

        size_t i;
        char *p2;
        const char *p1 = s1;
        for (p2 = s2, i = 0; *p1 && i < len; *(p2++) = *(p1++), i++) ;

        // 0 padding
        while (i++ < len)
            *(p2++) = 0;

        *p2 = 0;  // 0 terminate

        return (s2);
    }


    // Return the next token from a comma-separated list.  Leading and
    // trailing white space is stripped from the tokens.
    //
    char *
    htm_csvtok(char **s)
    {
        for (;;) {
            if (s == 0 || *s == 0)
                return (0);
            while (**s == ',')
                (*s)++;
            if (!**s)
                return (0);
            char *st = *s;
            while (**s && **s != ',')
                (*s)++;

            const char *e = *s;
            while (st < e && isspace(*st))
                st++;
            while (e-1 > st && isspace(*(e-1)))
                e--;
            if (st == e)
                continue;

            char *cbuf = new char[e - st + 1];
            char *c = cbuf;
            while (st < e)
                *c++ = *st++;
            *c = 0;
            while (**s == ',')
                (*s)++;
            return (cbuf);
        }
    }
}


namespace {
    // Convert a numerical value to an abc representation.
    char *
    to_ascii(int val)
    {
        static char out[12];    // return buffer
        char number[12];
        int i = 0, j = 0, value = val, remainder;

        do {
            remainder = (value % 26);
            number[i++] = (remainder ? remainder + 96 : 'z');
        }
        while ((value = (remainder ? (int)(value/26) : (int)((value-1)/26)))
            && i < 10); // no buffer overruns

        for (j = 0; i > 0 && j < 10; i--, j++)
            out[j] = number[i-1];

        out[j] = 0;  // 0 terminate

        return (out);
    }


    // Convert the given number to a lowercase roman numeral.  This
    // function is based on a similar one found in the Arena browser.
    //
    char *
    to_roman(int val)
    {
        static char buf[20];

        int value = val;
        snprintf(buf, sizeof(buf), "%i", val);

        int thousand = value/1000;
        value = value % 1000;
        int hundred = value/100;
        value = value % 100;
        int ten = value/10;
        int one = value % 10;

        char *p = buf;
        while (thousand-- > 0)
            *p++ = 'm';

        if (hundred) {
            char *q = (char*)Hundreds[hundred-1];
            while ((*p++ = *q++)) ;
            --p;
        }
        if (ten) {
            char *q = (char*)Tens[ten-1];
            while ((*p++ = *q++)) ;
            --p;
        }
        if (one) {
            char *q = (char*)Ones[one-1];
            while ((*p++ = *q++)) ;
            --p;
        }
        *p = '\0';

        return (buf);
    }

#ifdef USE_UTF8

    struct htmEscEnt : public htmHashEnt
    {
        htmEscEnt(const char *nm, const char *e) : htmHashEnt(nm)
            {
                h_esc = e;
            }

        const char *escape()    const { return (h_esc); }
    private:
        const char *h_esc;
    };

    htmHashTab *escape_tab;

    void
    setup_escape_tab()
    {
        if (escape_tab)
            return;
        escape_tab = new htmHashTab;
        for (esc_t *e = encodings; e->name; e++) {
            htmEscEnt *h = new htmEscEnt(e->name, e->enc);
            escape_tab->add(h);
            h = new htmEscEnt(e->esc, e->enc);
            escape_tab->add(h);
        }
    }


    // Return the UTF8 encoding of the number in str.
    //
    // U+0000    U+007F     0xxxxxxx
    // U+0080    U+07FF     110xxxxx 10xxxxxx
    // U+0800    U+FFFF     1110xxxx 10xxxxxx 10xxxxxx
    // U+10000   U+1FFFFF   11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
    // U+200000  U+3FFFFFF  111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
    // U+4000000 U+7FFFFFFF 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
    //
    const char *encode_chars(const char *str)
    {
        static char buf[8];
        char *t = buf;
        if (!str)
            return (0);
        if (*str == '#')
            str++;
        if (!isdigit(*str))
            return (0);
        int val = atoi(str);
        if (val <= 0x7f) {
            *t++ = val;
            *t = 0;
        }
        else if (val <= 0x7ff) {
            *t++ = 0xc0 | ((val >> 6) & 0x1f);
            *t++ = 0x80 | (val & 0x3f);
            *t++ = 0;
        }
        else if (val <= 0xffff) {
            *t++ = 0xe0 | ((val >> 12) & 0xf);
            *t++ = 0x80 | ((val >> 6) & 0x3f);
            *t++ = 0x80 | (val & 0x3f);
            *t++ = 0;
        }
        else if (val <= 0x1fffff) {
            *t++ = 0xf0 | ((val >> 18) & 0x7);
            *t++ = 0x80 | ((val >> 12) & 0x3f);
            *t++ = 0x80 | ((val >> 6) & 0x3f);
            *t++ = 0x80 | (val & 0x3f);
            *t++ = 0;
        }
        else if (val <= 0x3ffffff) {
            *t++ = 0xf8 | ((val >> 24) & 0x3);
            *t++ = 0x80 | ((val >> 18) & 0x3f);
            *t++ = 0x80 | ((val >> 12) & 0x3f);
            *t++ = 0x80 | ((val >> 6) & 0x3f);
            *t++ = 0x80 | (val & 0x3f);
            *t++ = 0;
        }
        else if (val <= 0x7fffffff) {
            *t++ = 0xfc | ((val >> 30) & 0x1);
            *t++ = 0x80 | ((val >> 24) & 0x3f);
            *t++ = 0x80 | ((val >> 18) & 0x3f);
            *t++ = 0x80 | ((val >> 12) & 0x3f);
            *t++ = 0x80 | ((val >> 6) & 0x3f);
            *t++ = 0x80 | (val & 0x3f);
            *t++ = 0;
        }
        return (buf);
    }

#define UNKNOWN_ESCAPE encode_chars("#9587")

    const char *
    tokenToEscape(const char **string, bool warn, htmWidget *html)
    {
        // First check if this is indeed an escape sequence.  It's much
        // more cost-effective to do this test here instead of in the
        // calling routine.

        if (*(*string+1) != '#' && !(isalpha(*(*string+1))))
            return (0);

        // Grab the name.
        const char *t = *string + 1;
        if (*t == '#') {
            t++;
            while (isdigit(*t))
                t++;
        }
        else {
            // For an alpha escape, the terminating ';' must be
            // present.

            while (isalnum(*t))
                t++;
            if (*t != ';')
                return (0);
        }

        int len = (t - (*string + 1));
        char *name = new char[len+1];
        strncpy(name, *string + 1, len);
        name[len] = 0;

        setup_escape_tab();
        const htmEscEnt *ent = (htmEscEnt*)escape_tab->get(name);
        if (ent) {
            delete [] name;
            (*string) += len+1;
            if (**string == ';')
                (*string)++;
            return (ent->escape());
        }

        if (*(*string + 1) == '#') {
            delete [] name;
            const char *enc = encode_chars(*string + 1);
            (*string) += len+1;
            if (**string == ';')
                (*string)++;
            return (enc);
        }

        // bad escape, spit out a warning and continue
        if (warn) {
            if (html)
                html->warning("tokenToEscape",
                    "Unknown escape sequence \"&%s;\".", name);
            else
                fprintf(stderr,
                    "tokenToEscape: unknown escape sequence \"&%s;\".\n",
                    name);
        }
        delete [] name;
        (*string) += len+1;
        return (UNKNOWN_ESCAPE);
    }
#else

    // Convert the HTML & escape sequence to the appropriate char.  This
    // function uses a sorted table defined in the header file escapes.h
    // and uses a binary search to locate the appropriate character for
    // the given escape sequence.  This table contains the hashed escapes
    // as well as the named escapes.  The number of elements is
    // NUM_ESCAPES (currently 197), so a match is always found in less
    // than 8 iterations (2^8=256).  If an escape sequence is not matched
    // and it is a hash escape, the value is assumed to be below 160 and
    // converted to a char using the ASCII representation of the given
    // number.  For other, non-matched characters, 0 is returned and the
    // return pointer is updated to point right after the ampersand sign.
    //
    char
    tokenToEscape(const char **escape, bool warn, htmWidget *html)
    {
        // First check if this is indeed an escape sequence.  It's much
        // more cost-effective to do this test here instead of in the
        // calling routine.

        if (*(*escape+1) != '#' && !(isalpha(*(*escape+1)))) {
            // skip and return
            *escape += 1;
            return ('&');
        }

        // Run this loop twice:  one time with a ; assumed present and one
        // time with ; present.

        for (int skip = 0; skip != 2; skip++) {
            int lo = 0;
            int hi = NUM_ESCAPES - 1;
            while (lo <= hi) {
                int mid = (lo + hi)/2;
                int cmp;
                if ((cmp = strncmp(*escape+1, escapes[mid].escape,
                        escapes[mid].len - skip)) == 0) {
                    // update ptr to point right after the escape sequence
                    *escape += escapes[mid].len + (1 - skip);
                    return (escapes[mid].token);
                }
                else {
                    if (cmp < 0)            // in lower end of array
                        hi = mid - 1;
                    else                    // in higher end of array
                        lo = mid + 1;
                }
            }
        }

        // If we get here, the escape sequence wasn't matched:  big chance
        // it uses a &# escape below 160.  To deal with this, we pick up
        // the numeric code and convert to a plain ASCII value which is
        // returned to the caller

        if (*(*escape+1) == '#') {
            char *chPtr, ret_char;
            int len = 0;

            *escape += 2;   // skip past the &# sequence
            chPtr = *escape;
            while (isdigit(*chPtr)) {
                chPtr++;
                len++;
            }
            if (*chPtr == ';') {
                *chPtr = '\0';  // null out the ;
                len++;
            }
            ret_char = (char)atoi(*escape);  // get corresponding char
            // move past the escape sequence
            if (*(*escape + len) == ';')
                *escape += len + 1;
            else
                *escape += len;
            if ((unsigned char)ret_char == 149)
                // hack for a bullet (asterisk)
                ret_char = 42;
            return (ret_char);
        }

        // bad escape, spit out a warning and continue
        if (warn) {
            char tmp[8];
            strncpy(tmp, *escape, 7);
            tmp[7] = '\0';
            if (html)
                html->warning("tokenToEscape", "Invalid escape sequence %s...",
                    tmp);
            else
                fprintf(stderr,
                    "tokenToEscape: invalid escape sequence %s...\n", tmp);
        }
        *escape += 1;
        return ('&');
    }
#endif
}

