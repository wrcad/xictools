
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

#ifndef RANGETEST_H
#define RANGETEST_H

#include "lstring.h"


// A class for parsing a range list in the form
//
//    min[-max][[,] ...]
//
// i.e., a comma or space-separated list of unsigned integers or
// ranges of unsigned integers.
//
// A check function is provided for testing whether an arbitrary integer is
// in one or the ranges (or matches a single value).
//
// Parsing will stop at ac character that is not '-', ',', or an integer.
// If no ranges or values are found, "all" is assumed.

class cRangeTest
{
    struct rngelt_t
    {
        rngelt_t(int mn, int mx, rngelt_t *n)
            {
                r_min = mmMin(mn, mx);
                r_max = mmMax(mn, mx);
                next = n;
            }

        int r_min;
        int r_max;
        rngelt_t *next;
    };

public:
    cRangeTest(const char *str)
        {
            rt_elts = 0;
            while (str && *str) {
                while (isspace(*str))
                    str++;
                const char *t = str;
                while (isdigit(*str))
                    str++;
                if (str == t)
                    break;
                int n1 = atoi(t);
                int n2 = n1;
                while (isspace(*str))
                    str++;
                if (*str == '-') {
                    str++;
                    while (isspace(*str))
                        str++;
                    t = str;
                    while (isdigit(*str))
                        str++;
                    if (str != t)
                        n2 = atoi(t);
                    while (isspace(*str))
                        str++;
                }
                rt_elts = new rngelt_t(n1, n2, rt_elts);
                if (!*str)
                    break;
                while (isspace(*str) || *str == ',')
                    str++;
            }
        }

    ~cRangeTest()
        {
            while (rt_elts) {
                rngelt_t *rx = rt_elts;
                rt_elts = rt_elts->next;
                delete rx;
            }
        }

    bool check(int n)
        {
            if (!rt_elts)
                return (true);
            for (rngelt_t *r = rt_elts; r; r = r->next) {
                if (n >= r->r_min && n <= r->r_max)
                    return (true);
            }
            return (false);
        }

private:
    rngelt_t *rt_elts;
};

#endif

