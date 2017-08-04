
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
 * Misc. Utilities Library                                                *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "miscmath.h"
#include <stdio.h>
#include <string.h>


/*  Ye olde slow version
// Return the greatest common divisor.
//
unsigned int
mmGCD(unsigned int u, unsigned int v)
{
    if (u == 0)
        return (v);
    if (v == 0)
        return (u);
    while (u > 0) {
        if (u < v) {
            int t = u;
            u = v;
            v = t;
        }
        u -= v;
    }
    return (v);
}
*/

// Binary GCD algorithm, from Wikipedia
unsigned int
mmGCD(unsigned int u, unsigned int v)
{
    int shift;

    // GCD(0,x) := x
    if (u == 0 || v == 0)
        return (u | v);

    // Let shift := lg K, where K is the greatest power of 2
    // dividing both u and v.
    for (shift = 0; ((u | v) & 1) == 0; ++shift) {
        u >>= 1;
        v >>= 1;
    }

    while ((u & 1) == 0)
        u >>= 1;

    // From here on, u is always odd.
    do {
        while ((v & 1) == 0)  // Loop X
            v >>= 1;

        // Now u and v are both odd, so diff(u, v) is even.
        // Let u = min(u, v), v = diff(u, v)/2.
        if (u <= v) {
            v -= u;
        }
        else {
            int diff = u - v;
            u = v;
            v = diff;
        }
        v >>= 1;
    } while (v != 0);

    return (u << shift);
}


// Functions to directly print integer and double values into a string.
// These were derived from the Google modp_numtoa functions from
// stringencoders-v3.7.0.  They are almost certainly faster than sprintf.

/*
 * Copyright 2005, 2006, 2007
 * Nick Galbreath -- nickg [at] modp [dot] com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 *   Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 *   Neither the name of the modp.com nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This is the standard "new" BSD license:
 * http://www.opensource.org/licenses/bsd-license.php
 */

namespace {
    inline void
    strreverse(char *begin, char *end)
    {
        char aux;
        while (end > begin)
            aux = *end, *end-- = *begin, *begin++ = aux;
    }

    const double mmPow10[] = {
        1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000,
        1000000000
    };
}


// Print value in buf with prec, return a pointer to the string terminator.
// If gstrip, strip trailing 0s and maybe the decimal point.
//
char *
mmDtoA(char *buf, double value, int prec, bool gstrip)
{
    // If input is larger than thres_max, revert to exponential.
    const double thres_max = (double)(0x7FFFFFFF);

    // For very large numbers switch back to native sprintf for
    // exponentials.  Anyone want to write code to replace this?

    if (fabs(value) > thres_max) {
        sprintf(buf, "%.*e", prec, value);
        return (strchr(buf, 0));
    }

    if (prec < 0)
        prec = 0;
    else if (prec > 9)
        // Precision of >= 10 can lead to overflow errors.
        prec = 9;

    // We'll work in positive values and deal with the negative sign
    // issue later.
    int neg = 0;
    if (value < 0) {
        neg = 1;
        value = -value;
    }

    int whole = (int)value;
    double tmp = (value - whole) * mmPow10[prec];
    unsigned int frac = (unsigned int)(tmp);
    double diff = tmp - frac;

    if (diff > 0.5) {
        ++frac;
        // Handle rollover, e.g.  case 0.99 with prec 1 is 1.0
        if (frac >= mmPow10[prec]) {
            frac = 0;
            ++whole;
        }
    }
    else if (diff == 0.5 && ((frac == 0) || (frac & 1)))
        // If halfway, round up if odd, OR if last digit is 0.  That
        // last part is strange.
        ++frac;

    char* wstr = buf;
    if (prec == 0) {
        diff = value - whole;
        if (diff > 0.5)
            // Greater than 0.5, round up, e.g. 1.6 -> 2
            ++whole;
        else if (diff == 0.5 && (whole & 1))
            // Exactly 0.5 and ODD, then round up 1.5 -> 2, but 2.5 -> 2
            ++whole;

    }
    else {
        int count = prec;
        // Now do fractional part, as an unsigned number.
        do {
            --count;
            *wstr++ = '0' + (frac % 10);
        } while (frac /= 10);
        // Add extra 0s.
        while (count-- > 0) *wstr++ = '0';
        // Add decimal.
        *wstr++ = '.';
    }

    // Do whole part.
    // Take care of sign.
    // Conversion. Number is reversed.
    do *wstr++ = '0' + (whole % 10); while (whole /= 10);
    if (neg)
        *wstr++ = '-';
    *wstr = 0;
    strreverse(buf, wstr-1);
    if (gstrip) {
        wstr--;
        while (*wstr == '0')
            *wstr-- = 0;
        if (*wstr == '.')
            *wstr-- = 0;
        if (!*wstr) {
            *wstr++ = '0';
            *wstr = 0;
        }
        else
            wstr++;
    }
    return (wstr);
}


// The Google functions have been extended to long ints.
#define USE_GOOGLE

#ifdef USE_GOOGLE

// Print d in buf, return a pointer to the string terminator.
//
char *
mmItoA(char *buf, long d)
{
    char *wstr = buf;
    int sign = d;
    if (d < 0)
        d = -d;
    do *wstr++ = ('0' + (d % 10)); while (d /= 10);
    if (sign < 0) *wstr++ = '-';
    *wstr = 0;

    // Reverse string
    strreverse(buf, wstr-1);
    return (wstr);
}


// Print u in buf, return a pointer to the string terminator.
//
char *
mmUtoA(char *buf, unsigned long u)
{
    char *wstr = buf;
    do *wstr++ = '0' + (u % 10); while (u /= 10);
    *wstr = 0;
    // Reverse string
    strreverse(buf, wstr-1);
    return (wstr);
}


// Print u in buf, return a pointer to the string terminator.
//
char *
mmHtoA(char *buf, unsigned long u)
{
    const char *hchars = "0123456789abcdef";

    char *wstr = buf;
    do *wstr++ = hchars[u % 16]; while (u /= 16);
    *wstr = 0;
    // Reverse string
    strreverse(buf, wstr-1);
    return (wstr);
}

#else

// The following alternatives are faster in some cases.
// This is Whiteley Research code, not subject to Google copyright.
// WARNING: these are for 32-bit ints only.

namespace {
    const unsigned int mmIvalues[10][10] =
    {
        { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 },
        { 0, 10, 20, 30, 40, 50, 60, 70, 80, 90 },
        { 0, 100, 200, 300, 400, 500, 600, 700, 800, 900 },
        { 0, 1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000 },
        { 0, 10000, 20000, 30000, 40000, 50000, 60000, 70000, 80000, 90000 },
        { 0, 100000, 200000, 300000, 400000, 500000, 600000, 700000, 800000,
          900000 },
        { 0, 1000000, 2000000, 3000000, 4000000, 5000000, 6000000, 7000000,
          8000000, 9000000 },
        { 0, 10000000, 20000000, 30000000, 40000000, 50000000, 60000000,
          70000000, 80000000, 90000000 },
        { 0, 100000000, 200000000, 300000000, 400000000, 500000000, 600000000,
          700000000, 800000000, 900000000 },
        { 0, 1000000000, 2000000000, 300000000U, 4000000000U, 0xffffffff,
          0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }
    };


    int
    find_main(unsigned int xx)
    {
        if (mmIvalues[5][1] > xx) {
            if (mmIvalues[2][1] > xx)
                return (mmIvalues[1][1] > xx ? 0 : 1);
            if (mmIvalues[4][1] > xx)
                return (mmIvalues[3][1] > xx ? 2 : 3);
            return (4);
        }
        if (mmIvalues[7][1] > xx)
            return (mmIvalues[6][1] > xx ? 5 : 6);
        if (mmIvalues[9][1] > xx)
            return (mmIvalues[8][1] > xx ? 7 : 8);
        return (9);
    }


    inline unsigned int
    find_sub(const unsigned int *vs, unsigned int xx)
    {
        if (vs[4] > xx) {
            if (vs[2] > xx)
                return (vs[1] > xx ? 0 : 1);
            return (vs[3] > xx ? 2 : 3);
        }
        if (vs[8] > xx) {
            if (vs[6] > xx)
                return (vs[5] > xx ? 4 : 5);
            return (vs[7] > xx ? 6 : 7);
        }
        if (vs[9] > xx)
            return (8);
        if (xx == 0xffffffff)
            return (4);
        return (9);
    }
}


char *
mmUtoA(char *s, unsigned int xx)
{
    for (int pn = find_main(xx); pn; pn--) {
        const unsigned int *v = mmIvalues[pn];
        unsigned int qn = find_sub(v, xx);
        xx -= v[qn];
        *s++ = '0' + qn;
    }
    *s++ = '0' + xx;
    *s = 0;
    return (s);
}


char *
mmItoA(char *s, int xx)
{
    if (xx < 0) {
        xx = -xx;
        *s++ = '-';
    }
    for (int pn = find_main(xx); pn; pn--) {
        const unsigned int *v = mmIvalues[pn];
        unsigned int qn = find_sub(v, xx);
        xx -= v[qn];
        *s++ = '0' + qn;
    }
    *s++ = '0' + xx;
    *s = 0;
    return (s);
}

#endif

#if 0

// Compile: g++ -O3 miscmath.cc

#include <stdlib.h>

int
main(int argc, char **argv)
{
    int bs = 0;
    if (argc > 1)
        bs = atoi(argv[1]);
    char buf[64];
    for (int i = 0; i < 10000000; i++) {
//    for (int i = 0; i < 100; i++) {
//        sprintf(buf, "%d", i+bs);
        mmItoA(buf, i + bs);
//        printf("%s\n", buf);
    }
    /*
    mmItoA(buf, 0xfffffffe);
    printf("%s\n", buf);
    mmItoA(buf, 0xffffffff);
    printf("%s\n", buf);
    mmUtoA(buf, 0xfffffffe);
    printf("%s\n", buf);
    mmUtoA(buf, 0xffffffff);
    printf("%s\n", buf);
    */
    return (0);
}

// The newer functions should be faster, since they don't use division
// or modulus.  There is a big advantage for FreeBSD-6.2, however the
// Google versions are faster on Red Hat Enterprise Linux 3, both 32
// and 64 bits.  I presume that the compiler special-cases divide by
// 10.

// -O3 FreeBSD-6.2 i686
// sprintf
// 5.544u 0.000s 0:05.57 99.4%     10+176k 0+0io 0pf+0w
// Google mmItoA
// 1.693u 0.000s 0:01.70 99.4%     10+176k 0+0io 0pf+0w
// mmItoA
// 0.464u 0.000s 0:00.46 100.0%    5+175k 0+0io 0pf+0w

// -03 RHEL3 x86_64
// sprintf
// real    0m2.450s
// Google mmItoA
// real    0m0.512s
// mmItoA
// real    0m0.692s

// -O3 Mingw i386
// sprintf
// real    0m5.169s
// Google mmItoA
// real    0m1.831s
// mmItoA
// real    0m0.585s

#endif

