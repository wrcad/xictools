
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

/*========================================================================*
 *                                                                        *
 * This file is a heavily modified derivative of random.c from            *
 * FreeBSD-6.2.   3/16/08                                                 *
 *                                                                        *
 * S. R. Whiteley, Whiteley Research Inc., Sunnyvale, CA                  *
 *                                                                        *
 *========================================================================*/

/*
 * Copyright (c) 1983, 1993
 *  The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *  This product includes software developed by the University of
 *  California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "randval.h"


// For each of the currently supported random number generators, we
// have a break value on the amount of state information (you need at
// least this many bytes of state info to support this random number
// generator), a degree for the polynomial (actually a trinomial) that
// the R.N.G.  is based on, and the separation between the two lower
// order coefficients of the trinomial.
//
#define TYPE_0      0       // linear congruential
#define BREAK_0     8
#define DEG_0       0
#define SEP_0       0

#define TYPE_1      1       // x**7 + x**3 + 1
#define BREAK_1     32
#define DEG_1       7
#define SEP_1       3

#define TYPE_2      2       // x**15 + x + 1
#define BREAK_2     64
#define DEG_2       15
#define SEP_2       1

#define TYPE_3      3       // x**31 + x**3 + 1
#define BREAK_3     128
#define DEG_3       31
#define SEP_3       3

#define TYPE_4      4       // x**63 + x + 1
#define BREAK_4     256
#define DEG_4       63
#define SEP_4       1

// Array versions of the above information to make code run faster --
// relies on fact that TYPE_i == i.
//
#define MAX_TYPES   5       // max number of types above

#ifdef  USE_WEAK_SEEDING
#define NSHUFF 0
#else
#define NSHUFF 50       // to drop some "seed -> 1st value" linearity
#endif


// Initially, everything is set up as if from:
//
//  initstate(1, randtbl, 128);
//
// Note that this initialization takes advantage of the fact that srandom()
// advances the front and rear pointers 10*rand_deg times, and hence the
// rear pointer which starts at 0 will also end up at zero; thus the zeroeth
// element of the state information, which contains info about the current
// position of the rear pointer is just
//
//  MAX_TYPES * (rptr - state) + TYPE_3 == TYPE_3.

namespace {
    unsigned int randtbl[DEG_3 + 1] = {
        TYPE_3,
#ifdef  USE_WEAK_SEEDING
        // Historic implementation compatibility
        // The random sequences do not vary much with the seed
        0x9a319039, 0x32d9c024, 0x9b663182, 0x5da1f342, 0xde3b81e0, 0xdf0a6fb5,
        0xf103bc02, 0x48f340fb, 0x7449e56b, 0xbeb1dbb0, 0xab5c5918, 0x946554fd,
        0x8c2e680f, 0xeb3d799f, 0xb11ee0b7, 0x2d436b86, 0xda672e2a, 0x1588ca88,
        0xe369735d, 0x904f35f7, 0xd7158fd6, 0x6fa6f051, 0x616e6b96, 0xac94efdc,
        0x36413f93, 0xc622c298, 0xf5a42ab8, 0x8a88d77b, 0xf5ad9d0e, 0x8999220b,
        0x27fb47b9,
#else
        0x991539b1, 0x16a5bce3, 0x6774a4cd, 0x3e01511e, 0x4e508aaa, 0x61048c05,
        0xf5500617, 0x846b7115, 0x6a19892c, 0x896a97af, 0xdb48f936, 0x14898454,
        0x37ffd106, 0xb58bff9c, 0x59e17104, 0xcf918a49, 0x09378c83, 0x52c7a471,
        0x8d293ea9, 0x1f4fc301, 0xc3db71be, 0x39b44e1c, 0xf8a44ef9, 0x4c8b80b1,
        0x19edc328, 0x87bf4bdd, 0xc9b240e5, 0xe9ee4b1b, 0x4382aee7, 0x535b6b41,
        0xf3bec5da
#endif
    };
}

// fptr and rptr are two pointers into the state info, a front and a
// rear pointer.  These two pointers are always rand_sep places
// aparts, as they cycle cyclically through the state information. 
// (Yes, this does mean we could get away with just one pointer, but
// the code for random() is more efficient this way).  The pointers
// are left positioned as they would be from the call
//
//  initstate(1, randtbl, 128);
//
// (The position of the rear pointer, rptr, is really 0 (as explained
// above in the initialization of randtbl) because the state table
// pointer is set to point to randtbl[1] (as explained below).

// The following things are the pointer to the state information
// table, the type of the current generator, the degree of the current
// polynomial being used, and the separation between the two pointers. 
// Note that for efficiency of random(), we remember the first
// location of the state information, not the zeroeth.  Hence it is
// valid to access state[-1], which is used to store the type of the
// R.N.G.  Also, we remember the last location, since this is more
// efficient than indexing every time to find the address of the last
// element to see if the front and rear pointers have wrapped.

randval::randval()
{
    fptr = &randtbl[SEP_3 + 1];
    rptr = &randtbl[1];
    state = &randtbl[1];
    end_ptr = &randtbl[DEG_3 + 1];
    rand_type = TYPE_3;
    rand_deg = DEG_3;
    rand_sep = SEP_3;
}


namespace {
    inline unsigned int good_rand(int x)
    {
#ifdef  USE_WEAK_SEEDING
    // Historic implementation compatibility.
    // The random sequences do not vary much with the seed,
    // even with overflowing.
        return (1103515245 * x + 12345);
#else
    // Compute x = (7^5 * x) mod (2^31 - 1)
    // wihout overflowing 31 bits:
    //      (2^31 - 1) = 127773 * (7^5) + 2836
    // From "Random number generators: good ones are hard to find",
    // Park and Miller, Communications of the ACM, vol. 31, no. 10,
    // October 1988, p. 1195.

        // Can't be initialized with 0, so use another value.
        if (x == 0)
            x = 123459876;
        int hi = x / 127773;
        int lo = x % 127773;
        x = 16807 * lo - 2836 * hi;
        if (x < 0)
            x += 0x7fffffff;
        return (x);
#endif
    }
}


// Initialize the random number generator based on the given seed.  If
// the type is the trivial no-state-information type, just remember
// the seed.  Otherwise, initializes state[] based on the given "seed"
// via a linear congruential generator.  Then, the pointers are set to
// known locations that are exactly rand_sep places apart.  Lastly, it
// cycles the state information a given number of times to get rid of
// any initial dependencies introduced by the L.C.R.N.G.  Note that
// the initialization of randtbl[] for default usage relies on values
// produced by this routine.
//
void
randval::rand_seed(unsigned int x)
{
    state[0] = x;
    int lim;
    if (rand_type == TYPE_0)
        lim = NSHUFF;
    else {
        for (int i = 1; i < rand_deg; i++)
            state[i] = good_rand(state[i - 1]);
        fptr = &state[rand_sep];
        rptr = &state[0];
        lim = 10 * rand_deg;
    }
    for (int i = 0; i < lim; i++)
        rand_value();
}


// If we are using the trivial TYPE_0 R.N.G., just do the old linear
// congruential bit.  Otherwise, we do our fancy trinomial stuff,
// which is the same in all the other cases due to all the global
// variables that have been set up.  The basic operation is to add the
// number at the rear pointer into the one at the front pointer.  Then
// both pointers are advanced to the next location cyclically in the
// table.  The value returned is the sum generated, reduced to 31 bits
// by throwing away the "least random" low bit.
//
// Note:  the code takes advantage of the fact that both the front and
// rear pointers can't wrap on the same call by not testing the rear
// pointer if the front one has wrapped.
//
// Returns a 31-bit random number.
//
unsigned int
randval::rand_value()
{
    unsigned int i;
    if (rand_type == TYPE_0) {
        i = state[0];
        state[0] = i = (good_rand(i)) & 0x7fffffff;
    } else {
        unsigned int *f = fptr;
        unsigned int *r = rptr;
        *f += *r;
        i = (*f >> 1) & 0x7fffffff;  // chucking least random bit
        if (++f >= end_ptr) {
            f = state;
            ++r;
        }
        else if (++r >= end_ptr) {
            r = state;
        }

        fptr = f;
        rptr = r;
    }
    return (i);
}

