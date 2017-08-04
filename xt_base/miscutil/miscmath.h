
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

#ifndef MISCMATH_H
#define MISCMATH_H

#include <math.h>
#include <limits.h>

//
// Some general math utilities.
//

#define mmMax(x,y) ((x) > (y) ? (x) : (y))
#define mmMin(x,y) ((x) < (y) ? (x) : (y))
#define mmAbs(x) ((x) >= 0 ? (x) : (-(x)))
#define mmSwapInts(x,y) { int z__tmp = x; x = y; y = z__tmp; }

// Return the greatest common divisor of the two arguments.
extern unsigned int mmGCD(unsigned int, unsigned int);

// Fast number-to-string functions.  The first argument is a buffer,
// return a pointer to the string terminator.
//
// long decimal, unsigned long decimal, unsigned long hex.
extern char *mmItoA(char*, long);
extern char *mmUtoA(char*, unsigned long);
extern char *mmHtoA(char*, unsigned long);
//
// Print real number, given precision.  If the final argument is
// true, eliminate trailing zeroes and possibly decimal point.
extern char *mmDtoA(char*, double, int=6, bool=false);


// General purpose double->int rounding function.  Using fabs timed
// faster than two comparisons, probably not true when fabs is not
// inlined (gcc inlines fabs).
//
// 0.4999999999999999 is two ticks below 0.5
//
// In order for a+mmRnd(b) == mmRnd(a+b) where a is an arbitrary int,
// -x.5 should round to -x, thus the asymmetry below.  The two-tick
// difference overcomes the internal rounding.
//
inline int mmRnd(double a)
{
    static const double zmax = (double)INT_MAX;
    if (fabs(a) >= zmax)
        return (a > 0.0 ? INT_MAX : -INT_MAX);
    return (a >= 0.0 ? (int)(a + 0.5) : (int)(a - 0.4999999999999999));
}

inline long mmRndL(double a)
{
    static const double zmax = (double)LONG_MAX;
    if (fabs(a) >= zmax)
        return (a > 0.0 ? LONG_MAX : -LONG_MAX);
    return (a >= 0.0 ? (long)(a + 0.5) : (long)(a - 0.4999999999999999));
}

#endif

