
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
 * WRspice Circuit Simulation and Analysis Tool                           *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Wayne A. Christopher
         1992 Stephen R. Whiteley
****************************************************************************/

#include "frontend.h"
#include "ftedata.h"
#include "ttyio.h"
#include "graphics.h"

// Functions to do complex mathematical functions.  These functions
// require the -lm libraries.  We sacrifice a lot of space to be able
// to avoid having to do a seperate call for every vector element, but
// it pays off in time savings.  These functions should never allow
// FPE's to happen.


#define rcheck(cond, name) if (!(cond)) { \
    GRpkgIf()->ErrPrintf(ET_WARN, "argument out of range for %s.\n", name); \
    delete res; return (0); }


// Calling methods for these functions are:
//  result = data1->v_something(data2)
// The length of the two data vectors is always the same, and is the length
// of the result. The result type is complex iff one of the args is
// complex.
//
sDataVec *
sDataVec::v_plus(sDataVec *data2)
{
    sDataVec *res = new sDataVec(0, v_flags | data2->v_flags, v_length,
        &v_units);
    if (!(res->v_units == data2->v_units))
        res->v_units.set(UU_NOTYPE);
    if (iscomplex()) {
        complex *out = res->v_data.comp;
        if (data2->iscomplex()) {
            complex *c1 = v_data.comp;
            complex *c2 = data2->v_data.comp;
            for (int i = 0; i < v_length; i++) {
                out->real = c1->real + c2->real;
                out->imag = c1->imag + c2->imag;
                out++;
                c1++;
                c2++;
            }
        }
        else {
            complex *c1 = v_data.comp;
            double *d2 = data2->v_data.real;
            for (int i = 0; i < v_length; i++) {
                out->real = c1->real + *d2++;
                out->imag = c1->imag;
                out++;
                c1++;
            }
        }
    }
    else {
        if (data2->iscomplex()) {
            complex *out = res->v_data.comp;
            double *d1 = v_data.real;
            complex *c2 = data2->v_data.comp;
            for (int i = 0; i < v_length; i++) {
                out->real = *d1++ + c2->real;
                out->imag = c2->imag;
                out++;
                c2++;
            }
        }
        else {
            double *out = res->v_data.real;
            double *d1 = v_data.real;
            double *d2 = data2->v_data.real;
            for (int i = 0; i < v_length; i++)
                *out++ = *d1++ + *d2++;
        }
    }
    return (res);
}


sDataVec *
sDataVec::v_minus(sDataVec *data2)
{
    sDataVec *res = new sDataVec(0, v_flags | data2->v_flags, v_length,
        &v_units);
    if (!(res->v_units == data2->v_units))
        res->v_units.set(UU_NOTYPE);
    if (iscomplex()) {
        complex *out = res->v_data.comp;
        if (data2->iscomplex()) {
            complex *c1 = v_data.comp;
            complex *c2 = data2->v_data.comp;
            for (int i = 0; i < v_length; i++) {
                out->real = c1->real - c2->real;
                out->imag = c1->imag - c2->imag;
                out++;
                c1++;
                c2++;
            }
        }
        else {
            complex *c1 = v_data.comp;
            double *d2 = data2->v_data.real;
            for (int i = 0; i < v_length; i++) {
                out->real = c1->real - *d2++;
                out->imag = c1->imag;
                out++;
                c1++;
            }
        }
    }
    else {
        if (data2->iscomplex()) {
            complex *out = res->v_data.comp;
            double *d1 = v_data.real;
            complex *c2 = data2->v_data.comp;
            for (int i = 0; i < v_length; i++) {
                out->real = *d1++ - c2->real;
                out->imag = -c2->imag;
                out++;
                c2++;
            }
        }
        else {
            double *out = res->v_data.real;
            double *d1 = v_data.real;
            double *d2 = data2->v_data.real;
            for (int i = 0; i < v_length; i++)
                *out++ = *d1++ - *d2++;
        }
    }
    return (res);
}


sDataVec *
sDataVec::v_times(sDataVec *data2)
{
    sDataVec *res = new sDataVec(0, v_flags | data2->v_flags, v_length,
        &v_units);
    res->v_units*data2->v_units;
    if (iscomplex()) {
        complex *out = res->v_data.comp;
        if (data2->iscomplex()) {
            complex *c1 = v_data.comp;
            complex *c2 = data2->v_data.comp;
            for (int i = 0; i < v_length; i++) {
                out->real = c1->real*c2->real - c1->imag*c2->imag;
                out->imag = c1->imag*c2->real + c1->real*c2->imag;
                out++;
                c1++;
                c2++;
            }
        }
        else {
            complex *c1 = v_data.comp;
            double *d2 = data2->v_data.real;
            for (int i = 0; i < v_length; i++) {
                out->real = c1->real* *d2;
                out->imag = c1->imag* *d2++;
                out++;
                c1++;
            }
        }
    }
    else {
        if (data2->iscomplex()) {
            complex *out = res->v_data.comp;
            double *d1 = v_data.real;
            complex *c2 = data2->v_data.comp;
            for (int i = 0; i < v_length; i++) {
                out->real = *d1 *c2->real;
                out->imag = *d1++ *c2->imag;
                out++;
                c2++;
            }
        }
        else {
            double *out = res->v_data.real;
            double *d1 = v_data.real;
            double *d2 = data2->v_data.real;
            for (int i = 0; i < v_length; i++)
                *out++ = *d1++ * *d2++;
        }
    }
    return (res);
}


sDataVec *
sDataVec::v_divide(sDataVec *data2)
{
    sDataVec *res = new sDataVec(0, v_flags | data2->v_flags, v_length,
        &v_units);
    res->v_units/data2->v_units;
    if (iscomplex()) {
        complex *out = res->v_data.comp;
        if (data2->iscomplex()) {
            complex *c1 = v_data.comp;
            complex *c2 = data2->v_data.comp;
            for (int i = 0; i < v_length; i++) {
                double d = c2->real*c2->real + c2->imag*c2->imag;
                rcheck(d != 0, "divide");
                out->real = (c1->real*c2->real + c1->imag*c2->imag)/d;
                out->imag = (c1->imag*c2->real - c2->imag*c1->real)/d;
                out++;
                c1++;
                c2++;
            }
        }
        else {
            complex *c1 = v_data.comp;
            double *d2 = data2->v_data.real;
            for (int i = 0; i < v_length; i++) {
                rcheck(*d2 != 0, "divide");
                out->real = c1->real/ *d2;
                out->imag = c1->imag/ *d2++;
                out++;
                c1++;
            }
        }
    }
    else {
        if (data2->iscomplex()) {
            complex *out = res->v_data.comp;
            double *d1 = v_data.real;
            complex *c2 = data2->v_data.comp;
            for (int i = 0; i < v_length; i++) {
                double d = c2->real*c2->real + c2->imag*c2->imag;
                rcheck(d != 0, "divide");
                d = *d1++/d;
                out->real = c2->real*d;
                out->imag = -c2->imag*d;
                out++;
                c2++;
            }
        }
        else {
            double *out = res->v_data.real;
            double *d1 = v_data.real;
            double *d2 = data2->v_data.real;
            for (int i = 0; i < v_length; i++) {
                rcheck(*d2 != 0, "divide");
                *out++ = *d1++ / *d2++;
            }
        }
    }
    return (res);
}


sDataVec *
sDataVec::v_mod(sDataVec *data2)
{
    sDataVec *res = new sDataVec(0, v_flags | data2->v_flags, v_length,
        &v_units);
    res->v_units/data2->v_units;
    if (iscomplex()) {
        complex *out = res->v_data.comp;
        if (data2->iscomplex()) {
            complex *c1 = v_data.comp;
            complex *c2 = data2->v_data.comp;
            for (int i = 0; i < v_length; i++) {
                double r1 = c1->mag();
                double r2 = c2->mag();
                rcheck(r2 != 0.0, "mod");
                double r3 = floor(r1/r2);
                out->real = c1->real - r3*c2->real;
                out->imag = c1->imag - r3*c2->imag;
                out++;
                c1++;
                c2++;
            }
        }
        else {
            complex *c1 = v_data.comp;
            double *d2 = data2->v_data.real;
            for (int i = 0; i < v_length; i++) {
                rcheck(*d2 > 0, "mod");
                double r1 = c1->mag();
                double r3 = floor(r1/ *d2);
                out->real = c1->real - r3* *d2++;
                out->imag = c1->imag;
                out++;
                c1++;
            }
        }
    }
    else {
        if (data2->iscomplex()) {
            complex *out = res->v_data.comp;
            double *d1 = v_data.real;
            complex *c2 = data2->v_data.comp;
            for (int i = 0; i < v_length; i++) {
                double r2 = c2->mag();
                rcheck(r2 != 0.0, "mod");
                double r3 = floor(*d1/r2);
                out->real = *d1++ - r3*c2->real;
                out->imag = -r3*c2->imag;
                out++;
                c2++;
            }
        }
        else {
            double *out = res->v_data.real;
            double *d1 = v_data.real;
            double *d2 = data2->v_data.real;
            for (int i = 0; i < v_length; i++) {
                rcheck(*d2 != 0, "mod");
                *out++ = fmod(*d1++, *d2++);
            }
        }
    }
    return (res);
}


// The comma operator. What this does (unless it is part of the argument
// list of a user-defined function) is arg1 + j(arg2).
//
sDataVec *
sDataVec::v_comma(sDataVec *data2)
{
    sDataVec *res = new sDataVec(0, VF_COMPLEX | v_flags | data2->v_flags,
        v_length, &v_units);
    if (!(res->v_units == data2->v_units))
        res->v_units.set(UU_NOTYPE);
    complex *out = res->v_data.comp;
    if (iscomplex()) {
        if (data2->iscomplex()) {
            complex *c1 = v_data.comp;
            complex *c2 = data2->v_data.comp;
            for (int i = 0; i < v_length; i++) {
                out->real = c1->real - c2->imag; 
                out->imag = c1->imag + c2->real;
                out++;
                c1++;
                c2++;
            }
        }
        else {
            complex *c1 = v_data.comp;
            double *d2 = data2->v_data.real;
            for (int i = 0; i < v_length; i++) {
                out->real = c1->real;
                out->imag = c1->imag + *d2++;
                out++;
                c1++;
            }
        }
    }
    else {
        if (data2->iscomplex()) {
            double *d1 = v_data.real;
            complex *c2 = data2->v_data.comp;
            for (int i = 0; i < v_length; i++) {
                out->real = *d1++ - c2->imag; 
                out->imag = c2->real;
                out++;
                c2++;
            }
        }
        else {
            double *d1 = v_data.real;
            double *d2 = data2->v_data.real;
            for (int i = 0; i < v_length; i++, out++) {
                out->real = *d1++; 
                out->imag = *d2++;
            }
        }
    }
    return (res);
}


sDataVec *
sDataVec::v_power(sDataVec *data2)
{
    sDataVec *res = new sDataVec(0, v_flags | data2->v_flags, v_length);
    if (iscomplex()) {
        complex *out = res->v_data.comp;
        if (data2->iscomplex()) {
            complex *c1 = v_data.comp;
            complex *c2 = data2->v_data.comp;
            for (int i = 0; i < v_length; i++) {

                double d = c1->mag();
                rcheck(d > 0.0, "power");
                // r = ln(c1)
                complex r(log(d),
                    c1->imag != 0.0 ? atan2(c1->imag, c1->real) : 0.0);
                // s = c2*ln(c1)
                complex s(r.real*c2->real - r.imag*c2->imag,
                    r.imag*c2->real + r.real*c2->imag);
                // out = exp(s)
                out->real = exp(s.real);
                if (s.imag != 0.0) {
                    out->imag = out->real*sin(s.imag);
                    out->real *= cos(s.imag);
                }

                out++;
                c1++;
                c2++;
            }
        }
        else {
            complex *c1 = v_data.comp;
            double *d2 = data2->v_data.real;
            for (int i = 0; i < v_length; i++) {

                double d = c1->mag();
                rcheck(d > 0.0, "power");
                // r = ln(c1)
                complex r(log(d),
                    c1->imag != 0.0 ? atan2(c1->imag, c1->real) : 0.0);
                // s = c2*ln(c1)
                complex s(r.real* *d2, r.imag* *d2);
                // out = exp(s)
                out->real = exp(s.real);
                if (s.imag != 0.0) {
                    out->imag = out->real*sin(s.imag);
                    out->real *= cos(s.imag);
                }

                out++;
                c1++;
                d2++;
            }
        }
    }
    else {
        if (data2->iscomplex()) {
            complex *out = res->v_data.comp;
            double *d1 = v_data.real;
            complex *c2 = data2->v_data.comp;
            for (int i = 0; i < v_length; i++) {

                double d = fabs(*d1);
                rcheck(d > 0.0, "power");
                // r = ln(c1)
                complex r(log(d), 0.0);
                // s = c2*ln(c1)
                complex s(r.real*c2->real, r.real*c2->imag);
                // out = exp(s)
                out->real = exp(s.real);
                if (s.imag != 0.0) {
                    out->imag = out->real*sin(s.imag);
                    out->real *= cos(s.imag);
                }

                out++;
                d1++;
                c2++;
            }
        }
        else {
            double *out = res->v_data.real;
            double *d1 = v_data.real;
            double *d2 = data2->v_data.real;
            for (int i = 0; i < v_length; i++) {
                rcheck((*d1 >= 0) || (floor(*d2) == ceil(*d2)), "power");
                *out++ = pow(*d1++, *d2++);
            }
        }
    }
    return (res);
}


// Now come all the relational and logical functions. It's overkill to put
// them here, but... Note that they always return a real value, with the
// result the same length as the arguments.
//
sDataVec *
sDataVec::v_eq(sDataVec *data2)
{
    sDataVec *res = new sDataVec(0, 0, v_length);
    double *out = res->v_data.real;
    if (iscomplex()) {
        if (data2->iscomplex()) {
            complex *c1 = v_data.comp;
            complex *c2 = data2->v_data.comp;
            for (int i = 0; i < v_length; i++) {
                *out++ = (c1->real == c2->real && c1->imag == c2->imag ?
                    1.0 : 0.0);
                c1++;
                c2++;
            }
        }
        else {
            complex *c1 = v_data.comp;
            double *d2 = data2->v_data.real;
            for (int i = 0; i < v_length; i++) {
                *out++ = (c1->real == *d2++ && c1->imag == 0.0 ? 1.0 : 0.0);
                c1++;
            }
        }
    }
    else {
        if (data2->iscomplex()) {
            double *d1 = v_data.real;
            complex *c2 = data2->v_data.comp;
            for (int i = 0; i < v_length; i++) {
                *out++ = (*d1++ == c2->real && 0.0 == c2->imag ? 1.0 : 0.0);
                c2++;
            }
        }
        else {
            double *d1 = v_data.real;
            double *d2 = data2->v_data.real;
            for (int i = 0; i < v_length; i++)
                *out++ = (*d1++ == *d2++ ? 1.0 : 0.0);
        }
    }
    return (res);
}


sDataVec *
sDataVec::v_gt(sDataVec *data2)
{
    sDataVec *res = new sDataVec(0, 0, v_length);
    double *out = res->v_data.real;
    if (iscomplex()) {
        if (data2->iscomplex()) {
            complex *c1 = v_data.comp;
            complex *c2 = data2->v_data.comp;
            for (int i = 0; i < v_length; i++) {
                *out++ = (c1->real > c2->real && c1->imag > c2->imag ?
                    1.0 : 0.0);
                c1++;
                c2++;
            }
        }
        else {
            complex *c1 = v_data.comp;
            double *d2 = data2->v_data.real;
            for (int i = 0; i < v_length; i++) {
                *out++ = (c1->real > *d2++ && c1->imag > 0.0 ? 1.0 : 0.0);
                c1++;
            }
        }
    }
    else {
        if (data2->iscomplex()) {
            double *d1 = v_data.real;
            complex *c2 = data2->v_data.comp;
            for (int i = 0; i < v_length; i++) {
                *out++ = (*d1++ > c2->real && 0.0 > c2->imag ? 1.0 : 0.0);
                c2++;
            }
        }
        else {
            double *d1 = v_data.real;
            double *d2 = data2->v_data.real;
            for (int i = 0; i < v_length; i++)
                *out++ = (*d1++ > *d2++ ? 1.0 : 0.0);
        }
    }
    return (res);
}


sDataVec *
sDataVec::v_lt(sDataVec *data2)
{
    sDataVec *res = new sDataVec(0, 0, v_length);
    double *out = res->v_data.real;
    if (iscomplex()) {
        if (data2->iscomplex()) {
            complex *c1 = v_data.comp;
            complex *c2 = data2->v_data.comp;
            for (int i = 0; i < v_length; i++) {
                *out++ = (c1->real < c2->real && c1->imag < c2->imag ?
                    1.0 : 0.0);
                c1++;
                c2++;
            }
        }
        else {
            complex *c1 = v_data.comp;
            double *d2 = data2->v_data.real;
            for (int i = 0; i < v_length; i++) {
                *out++ = (c1->real < *d2++ && c1->imag < 0.0 ? 1.0 : 0.0);
                c1++;
            }
        }
    }
    else {
        if (data2->iscomplex()) {
            double *d1 = v_data.real;
            complex *c2 = data2->v_data.comp;
            for (int i = 0; i < v_length; i++) {
                *out++ = (*d1++ < c2->real && 0.0 < c2->imag ? 1.0 : 0.0);
                c2++;
            }
        }
        else {
            double *d1 = v_data.real;
            double *d2 = data2->v_data.real;
            for (int i = 0; i < v_length; i++)
                *out++ = (*d1++ < *d2++ ? 1.0 : 0.0);
        }
    }
    return (res);
}


sDataVec *
sDataVec::v_ge(sDataVec *data2)
{
    sDataVec *res = new sDataVec(0, 0, v_length);
    double *out = res->v_data.real;
    if (iscomplex()) {
        if (data2->iscomplex()) {
            complex *c1 = v_data.comp;
            complex *c2 = data2->v_data.comp;
            for (int i = 0; i < v_length; i++) {
                *out++ = (c1->real >= c2->real && c1->imag >= c2->imag ?
                    1.0 : 0.0);
                c1++;
                c2++;
            }
        }
        else {
            complex *c1 = v_data.comp;
            double *d2 = data2->v_data.real;
            for (int i = 0; i < v_length; i++) {
                *out++ = (c1->real >= *d2++ && c1->imag >= 0.0 ? 1.0 : 0.0);
                c1++;
            }
        }
    }
    else {
        if (data2->iscomplex()) {
            double *d1 = v_data.real;
            complex *c2 = data2->v_data.comp;
            for (int i = 0; i < v_length; i++) {
                *out++ = (*d1++ >= c2->real && 0.0 >= c2->imag ? 1.0 : 0.0);
                c2++;
            }
        }
        else {
            double *d1 = v_data.real;
            double *d2 = data2->v_data.real;
            for (int i = 0; i < v_length; i++)
                *out++ = (*d1++ >= *d2++ ? 1.0 : 0.0);
        }
    }
    return (res);
}


sDataVec *
sDataVec::v_le(sDataVec *data2)
{
    sDataVec *res = new sDataVec(0, 0, v_length);
    double *out = res->v_data.real;
    if (iscomplex()) {
        if (data2->iscomplex()) {
            complex *c1 = v_data.comp;
            complex *c2 = data2->v_data.comp;
            for (int i = 0; i < v_length; i++) {
                *out++ = (c1->real <= c2->real && c1->imag <= c2->imag ?
                    1.0 : 0.0);
                c1++;
                c2++;
            }
        }
        else {
            complex *c1 = v_data.comp;
            double *d2 = data2->v_data.real;
            for (int i = 0; i < v_length; i++) {
                *out++ = (c1->real <= *d2++ && c1->imag <= 0.0 ? 1.0 : 0.0);
                c1++;
            }
        }
    }
    else {
        if (data2->iscomplex()) {
            double *d1 = v_data.real;
            complex *c2 = data2->v_data.comp;
            for (int i = 0; i < v_length; i++) {
                *out++ = (*d1++ <= c2->real && 0.0 <= c2->imag ? 1.0 : 0.0);
                c2++;
            }
        }
        else {
            double *d1 = v_data.real;
            double *d2 = data2->v_data.real;
            for (int i = 0; i < v_length; i++)
                *out++ = (*d1++ <= *d2++ ? 1.0 : 0.0);
        }
    }
    return (res);
}


sDataVec *
sDataVec::v_ne(sDataVec *data2)
{
    sDataVec *res = new sDataVec(0, 0, v_length);
    double *out = res->v_data.real;
    if (iscomplex()) {
        if (data2->iscomplex()) {
            complex *c1 = v_data.comp;
            complex *c2 = data2->v_data.comp;
            for (int i = 0; i < v_length; i++) {
                *out++ = (c1->real != c2->real || c1->imag != c2->imag ?
                    1.0 : 0.0);
                c1++;
                c2++;
            }
        }
        else {
            complex *c1 = v_data.comp;
            double *d2 = data2->v_data.real;
            for (int i = 0; i < v_length; i++) {
                *out++ = (c1->real != *d2++ || c1->imag != 0.0 ? 1.0 : 0.0);
                c1++;
            }
        }
    }
    else {
        if (data2->iscomplex()) {
            double *d1 = v_data.real;
            complex *c2 = data2->v_data.comp;
            for (int i = 0; i < v_length; i++) {
                *out++ = (*d1++ != c2->real || 0.0 != c2->imag ? 1.0 : 0.0);
                c2++;
            }
        }
        else {
            double *d1 = v_data.real;
            double *d2 = data2->v_data.real;
            for (int i = 0; i < v_length; i++)
                *out++ = (*d1++ != *d2++ ? 1.0 : 0.0);
        }
    }
    return (res);
}


sDataVec *
sDataVec::v_and(sDataVec *data2)
{
    sDataVec *res = new sDataVec(0, 0, v_length);
    double *out = res->v_data.real;
    if (iscomplex()) {
        if (data2->iscomplex()) {
            complex *c1 = v_data.comp;
            complex *c2 = data2->v_data.comp;
            for (int i = 0; i < v_length; i++) {
                *out++ = ((c1->real != 0.0 || c1->imag != 0.0) &&
                    (c2->real != 0.0 || c2->imag != 0.0) ? 1.0 : 0.0);
                c1++;
                c2++;
            }
        }
        else {
            complex *c1 = v_data.comp;
            double *d2 = data2->v_data.real;
            for (int i = 0; i < v_length; i++) {
                *out++ = ((c1->real != 0.0 || c1->imag != 0.0) && *d2++ != 0.0
                    ? 1.0 : 0.0);
                c1++;
            }
        }
    }
    else {
        if (data2->iscomplex()) {
            double *d1 = v_data.real;
            complex *c2 = data2->v_data.comp;
            for (int i = 0; i < v_length; i++) {
                *out++ = (*d1++ != 0.0 && (c2->real != 0.0 || c2->imag != 0.0)
                    ? 1.0 : 0.0);
                c2++;
            }
        }
        else {
            double *d1 = v_data.real;
            double *d2 = data2->v_data.real;
            for (int i = 0; i < v_length; i++)
                *out++ = (*d1 != 0.0 && *d2 != 0.0 ? 1.0 : 0.0);
        }
    }
    return (res);
}


sDataVec *
sDataVec::v_or(sDataVec *data2)
{
    sDataVec *res = new sDataVec(0, 0, v_length);
    double *out = res->v_data.real;
    if (iscomplex()) {
        if (data2->iscomplex()) {
            complex *c1 = v_data.comp;
            complex *c2 = data2->v_data.comp;
            for (int i = 0; i < v_length; i++) {
                *out++ = (c1->real != 0.0 || c1->imag != 0.0 ||
                    c2->real != 0.0 || c2->imag != 0.0 ? 1.0 : 0.0);
                c1++;
                c2++;
            }
        }
        else {
            complex *c1 = v_data.comp;
            double *d2 = data2->v_data.real;
            for (int i = 0; i < v_length; i++) {
                *out++ = (c1->real != 0.0 || c1->imag != 0.0 || *d2++ != 0.0
                    ? 1.0 : 0.0);
                c1++;
            }
        }
    }
    else {
        if (data2->iscomplex()) {
            double *d1 = v_data.real;
            complex *c2 = data2->v_data.comp;
            for (int i = 0; i < v_length; i++) {
                *out++ = (*d1++ != 0.0 || c2->real != 0.0 || c2->imag != 0.0
                    ? 1.0 : 0.0);
                c2++;
            }
        }
        else {
            double *d1 = v_data.real;
            double *d2 = data2->v_data.real;
            for (int i = 0; i < v_length; i++)
                *out++ = (*d1 != 0.0 || *d2 != 0.0 ? 1.0 : 0.0);
        }
    }
    return (res);
}

