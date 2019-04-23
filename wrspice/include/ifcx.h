
/*========================================================================*
 *                                                                        *
 *  Distributed by Whiteley Research Inc., Sunnyvale, California, USA     *
 *                       http://wrcad.com                                 *
 *  Copyright (C) 2019 Whiteley Research Inc., all rights reserved.       *
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

#ifndef IFCX_H
#define IFCX_H

//
// Math operator inlines for IFcomplex.
//


inline const IFcomplex operator+(const IFcomplex &A, const IFcomplex &B)
{
    IFcomplex ret;
    ret.set(A.real+B.real, A.imag+B.imag);
    return (ret);
}

inline const IFcomplex operator+(double r, const IFcomplex &A)
{
    IFcomplex ret;
    ret.set(A.real+r, A.imag);
    return (ret);
}

inline const IFcomplex operator+(const IFcomplex &A, double r)
{
    IFcomplex ret;
    ret.set(A.real+r, A.imag);
    return (ret);
}


inline const IFcomplex operator-(const IFcomplex &A, const IFcomplex &B)
{
    IFcomplex ret;
    ret.set(A.real-B.real, A.imag-B.imag);
    return (ret);
}

inline const IFcomplex operator-(double r, const IFcomplex &A)
{
    IFcomplex ret;
    ret.set(r-A.real, -A.imag);
    return (ret);
}

inline const IFcomplex operator-(const IFcomplex &A, double r)
{
    IFcomplex ret;
    ret.set(A.real-r, A.imag);
    return (ret);
}

inline const IFcomplex operator-(const IFcomplex &A)
{
    IFcomplex ret;
    ret.set(-A.real, -A.imag);
    return (ret);
}


inline const IFcomplex operator*(const IFcomplex &A, const IFcomplex &B)
{
    IFcomplex ret;
    ret.set(A.real*B.real - A.imag*B.imag, A.imag*B.real + A.real*B.imag);
    return (ret);
}

inline const IFcomplex operator*(double r, const IFcomplex &A)
{
    IFcomplex ret;
    ret.set(A.real*r, A.imag*r);
    return (ret);
}

inline const IFcomplex operator*(const IFcomplex &A, double r)
{
    IFcomplex ret;
    ret.set(A.real*r, A.imag*r);
    return (ret);
}


inline const IFcomplex operator/(const IFcomplex &A, const IFcomplex &B)
{
    double d = B.real*B.real + B.imag*B.imag;
    IFcomplex ret;
    ret.set((A.real*B.real + A.imag*B.imag)/d,
        (A.imag*B.real - A.real*B.imag)/d);
    return (ret);
}

inline const IFcomplex operator/(double r, const IFcomplex &A)
{
    double d = A.real*A.real + A.imag*A.imag;
    r /= d;
    IFcomplex ret;
    ret.set(r*A.real, -r*A.imag);
    return (ret);
}

inline const IFcomplex operator/(const IFcomplex &A, double r)
{
    IFcomplex ret;
    ret.set(A.real/r, A.imag/r);
    return (ret);
}

#endif

