
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
 $Id: rangetest.h,v 1.2 2009/07/09 05:54:04 stevew Exp $
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

