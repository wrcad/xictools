
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
 $Id: miscmath.h,v 1.9 2015/10/03 01:28:26 stevew Exp $
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

