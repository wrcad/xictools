
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

#include "config.h"
#include "simulator.h"
#include "datavec.h"
#include "output.h"
#include "kwords_fte.h"
#include "runop.h"
#include "ttyio.h"
#include "miscutil/random.h"
#include "ginterf/graphics.h"


#ifndef M_PI
#define M_PI            3.14159265358979323846
#endif
#ifndef M_PI_2
#define M_PI_2          1.57079632679489661923
#endif
#ifndef M_LOG10E
#define M_LOG10E        0.43429448190325182765
#endif

#ifndef HAVE_CBRT
extern double cbrt(double);
#endif
#ifndef HAVE_LGAMMA
extern double lgamma(double);
#endif
#if !defined(HAVE_ERF) || !defined(HAVE_ERFC)
extern double erf(double);
extern double erfc(double);
#endif

// Value returned when out of range.  This allows a little slop for
// processing, unlike MAXDOUBLE
//
#define HUGENUM  1.0e+300

// Functions to do complex mathematical functions.  These require the
// -lm libraries.  We sacrifice a lot of space to be able to avoid
// having to do a seperate call for every vector element, but it pays
// off in time savings.  These functions should never allow FPE's to
// happen.

#define rcheck(cond, name) if (!(cond)) { \
  GRpkg::self()->ErrPrintf(ET_WARN, "argument out of range for %s.\n", name); \
   delete res; return (0); }

#define cxabs(d) (((d) < 0.0) ? - (d) : (d))

namespace {
    // If true, random generation is turned on.
    //
    inline bool return_random()
    {
        if (Sp.GetFlag(FT_MONTE)) {
            // Monte Carlo analysis is in progress.
            return (true);
        }
        if (Sp.GetVar(kw_random, VTYP_BOOL, 0, Sp.CurCircuit()))
            return (true);
        return (false);
    }
}


// Compute sqrt of sum of squares of entries
//
sDataVec *
sDataVec::v_rms()
{
    sDataVec *os = v_scale;
    if (!os && v_plot)
        os = v_plot->scale();
    if (!os || os->v_length <= 1) {
        // compute sqrt(sum of squares)
        sDataVec *res = new sDataVec(0, 0, 1, &v_units);
        double d = 0.0;
        if (iscomplex()) {
            complex *in = v_data.comp;
            for (int i = 0; i < v_length; i++, in++)
                d += in->mag();
        }
        else {
            double *in = v_data.real;
            for (int i = 0; i < v_length; i++, in++)
                d += (*in)*(*in);
        }
        res->v_data.real[0] = sqrt(d);
        return (res);
    }
    if (os->v_length != v_length) {
        GRpkg::self()->ErrPrintf(ET_ERROR,
            "cx_rms: vector/scale length mismatch.\n");
        return (0);
    }
    sDataVec *res = new sDataVec(0, 0, 1, &v_units);
    double T = 0.0, sum = 0.0;
    if (iscomplex()) {
        complex *in = v_data.comp;
        for (int i = 1; i < v_length; i++, in++) {
            double delt = os->realval(i) - os->realval(i-1);
            sum += 0.5*delt*(in->mag() + (in-1)->mag());
            T += delt;
        }
    }
    else {
        double *in = v_data.real;
        for (int i = 1; i < v_length; i++, in++) {
            double delt = os->realval(i) - os->realval(i-1);
            sum += 0.5*delt*((*in)*(*in) + (*(in-1))*(*(in-1)));
            T += delt;
        }
    }
    if (T == 0.0)
        T = 1.0;
    sum /= T;
    res->v_data.real[0] = sqrt(sum);
    return (res);
}


// Compute sum of entries
//
sDataVec *
sDataVec::v_sum()
{
    sDataVec *res;
    if (iscomplex()) {
        res = new sDataVec(0, VF_COMPLEX, 1, &v_units);
        complex *in = v_data.comp;
        complex c(0,0);
        for (int i = 0; i < v_length; i++, in++) {
            c.real += in->real;
            c.imag += in->imag;
        }
        res->v_data.comp[0] = c;
    }
    else {
        res = new sDataVec(0, 0, 1, &v_units);
        double *in = v_data.real;
        double d = 0.0;
        for (int i = 0; i < v_length; i++)
            d += *in++;
        res->v_data.real[0] = d;
    }
    return (res);
}


sDataVec *
sDataVec::v_mag()
{
    sDataVec *res = new sDataVec(0, 0, v_length, &v_units);
    double *out = res->v_data.real;
    if (iscomplex()) {
        complex *in = v_data.comp;
        for (int i = 0; i < v_length; i++, in++)
            *out++ = in->mag();
    }
    else {
        double *in = v_data.real;
        for (int i = 0; i < v_length; i++, in++)
            *out++ = cxabs(*in);
    }
    return (res);
}


sDataVec *
sDataVec::v_ph()
{
    sDataVec *res = new sDataVec(0, 0, v_length);
    double *out = res->v_data.real;
    if (iscomplex()) {
        complex *in = v_data.comp;
        for (int i = 0; i < v_length; i++, in++)
            *out++ = radtodeg(in->phs());
    }
    // Otherwise it is 0, but tmalloc zeros the stuff already
    return (res);
}

// If this is pure imaginary we might get real, but never mind...

sDataVec *
sDataVec::v_j()
{
    sDataVec *res = new sDataVec(0, VF_COMPLEX, v_length, &v_units);
    complex *out = res->v_data.comp;
    if (iscomplex()) {
        complex *in = v_data.comp;
        for (int i = 0; i < v_length; i++) {
            out->real = -in->imag;
            out->imag = in->real;
            in++;
            out++;
        }
    }
    else {
        double *in = v_data.real;
        for (int i = 0; i < v_length; i++, out++)
            out->imag = *in++;
    }
    return (res);
}


sDataVec *
sDataVec::v_real()
{
    sDataVec *res = new sDataVec(0, 0, v_length, &v_units);
    double *out = res->v_data.real;
    if (iscomplex()) {
        complex *in = v_data.comp;
        for (int i = 0; i < v_length; i++, in++)
            *out++ = in->real;
    }
    else {
        double *in = v_data.real;
        for (int i = 0; i < v_length; i++)
            *out++ = *in++;
    }
    return (res);
}


sDataVec *
sDataVec::v_imag()
{
    sDataVec *res = new sDataVec(0, 0, v_length, &v_units);
    double *out = res->v_data.real;
    if (iscomplex()) {
        complex *in = v_data.comp;
        for (int i = 0; i < v_length; i++, in++)
            *out++ = in->imag;
    }
    return (res);
}


sDataVec *
sDataVec::v_pos()
{
    sDataVec *res = new sDataVec(0, 0, v_length);
    double *out = res->v_data.real;
    if (iscomplex()) {
        complex *in = v_data.comp;
        for (int i = 0; i < v_length; i++, in++)
            *out++ = ((in->real > 0.0) ? 1.0 : 0.0);
    }
    else {
        double *in = v_data.real;
        for (int i = 0; i < v_length; i++)
            *out++ = ((*in++ > 0.0) ? 1.0 : 0.0);
    }
    return (res);
}


sDataVec *
sDataVec::v_db()
{
    sDataVec *res = new sDataVec(0, 0, v_length);
    double *out = res->v_data.real;
    if (iscomplex()) {
        complex *in = v_data.comp;
        for (int i = 0; i < v_length; i++, in++) {
            double tt = in->mag();
            rcheck(tt >= 0, "db");
            if (tt == 0.0)
                *out++ = 20.0 * -log(HUGENUM);
            else
                *out++ = 20.0 * log10(tt);
        }
    }
    else {
        double *in = v_data.real;
        for (int i = 0; i < v_length; i++) {
            double tt = *in++;
            rcheck(tt >= 0, "db");
            if (tt == 0.0)
                *out++ = 20.0 * -log(HUGENUM);
            else
                *out++ = 20.0 * log10(tt);
        }
    }
    return (res);
}


sDataVec *
sDataVec::v_log10()
{
    sDataVec *res;
    if (iscomplex()) {
        res = new sDataVec(0, VF_COMPLEX, v_length);
        complex *out = res->v_data.comp;
        complex *in = v_data.comp;
        for (int i = 0; i < v_length; i++) {
            double d = in->mag();
            rcheck(d >= 0, "log");
            if (d == 0.0)
                out->real = -log10(HUGENUM);
            else {
                out->real = log10(d);
                out->imag = M_LOG10E*atan2(in->imag, in->real);
            }
            in++;
            out++;
        }
    }
    else {
        res = new sDataVec(0, 0, v_length);
        double *out = res->v_data.real;
        double *in = v_data.real;
        for (int i = 0; i < v_length; i++) {
            rcheck(*in >= 0, "log");
            if (*in == 0.0)
                *out = -log10(HUGENUM);
            else 
                *out = log10(*in);
            in++;
            out++;
        }
    }
    return (res);
}


// The log() function used to return log10 prior to 3.2.15.  Switched to
// natural log for Hspice compatibility.
//
sDataVec *
sDataVec::v_log()
{
    return (v_ln());
}


sDataVec *
sDataVec::v_ln()
{
    sDataVec *res;
    if (iscomplex()) {
        res = new sDataVec(0, VF_COMPLEX, v_length);
        complex *out = res->v_data.comp;
        complex *in = v_data.comp;
        for (int i = 0; i < v_length; i++) {
            double d = in->mag();
            rcheck(d >= 0, "ln");
            if (d == 0.0)
                out->real = -log(HUGENUM);
            else {
                out->real = log(d);
                out->imag = atan2(in->imag, in->real);
            }
            in++;
            out++;
        }
    }
    else {
        res = new sDataVec(0, 0, v_length);
        double *out = res->v_data.real;
        double *in = v_data.real;
        for (int i = 0; i < v_length; i++) {
            rcheck(*in >= 0, "ln");
            if (*in == 0.0)
                *out = -log(HUGENUM);
            else
                *out = log(*in);
            in++;
            out++;
        }
    }
    return (res);
}


sDataVec *
sDataVec::v_exp()
{
    sDataVec *res;
    if (iscomplex()) {
        res = new sDataVec(0, VF_COMPLEX, v_length);
        complex *out = res->v_data.comp;
        complex *in = v_data.comp;
        for (int i = 0; i < v_length; i++) {
            double d = exp(in->real);
            out->real = d*cos(in->imag);
            out->imag = d*sin(in->imag);
            in++;
            out++;
        }
    }
    else {
        res = new sDataVec(0, 0, v_length);
        double *out = res->v_data.real;
        double *in = v_data.real;
        for (int i = 0; i < v_length; i++)
            *out++ = exp(*in++);
    }
    return (res);
}


sDataVec *
sDataVec::v_sqrt()
{
    sDataVec *res;
    if (iscomplex()) {
        res = new sDataVec(0, VF_COMPLEX, v_length);
        complex *out = res->v_data.comp;
        complex *in = v_data.comp;
        for (int i = 0; i < v_length; i++) {
            double d = in->mag();
            if (in->real >= 0) {
                out->real = sqrt(0.5*(d + in->real));
                if (out->real)
                    out->imag = in->imag/(2.0*out->real);
            }
            else {
                out->imag = sqrt(0.5*(d - in->real));
                if (out->imag)
                    out->real = in->imag/(2.0*out->imag);
            }
            if (out->imag < 0.0) {
                out->real = -out->real;
                out->imag = -out->imag;
            }
            out++;
            in++;
        }
    }
    else {
        bool cres = false;
        double *in = v_data.real;
        for (int i = 0; i < v_length; i++) {
            if (*in++ < 0.0) {
                cres = true;
                break;
            }
        }
        if (cres) {
            res = new sDataVec(0, VF_COMPLEX, v_length);
            complex *out = res->v_data.comp;
            in = v_data.real;
            for (int i = 0; i < v_length; i++) {
                if (*in < 0.0)
                    out->imag = sqrt(-*in);
                else
                    out->real = sqrt(*in);
                in++;
                out++;
            }
        }
        else {
            res = new sDataVec(0, 0, v_length);
            double *out = res->v_data.real;
            in = v_data.real;
            for (int i = 0; i < v_length; i++)
                *out++ = sqrt(*in++);
        }
    }
    return (res);
}


sDataVec *
sDataVec::v_sin()
{
    sDataVec *res;
    if (iscomplex()) {
        res = new sDataVec(0, VF_COMPLEX, v_length);
        complex *out = res->v_data.comp;
        complex *in = v_data.comp;
        for (int i = 0; i < v_length; i++) {
            out->real = sin(degtorad(in->real)) * cosh(degtorad(in->imag));
            out->imag = cos(degtorad(in->real)) * sinh(degtorad(in->imag));
            in++;
            out++;
        }
    }
    else {
        res = new sDataVec(0, 0, v_length);
        double *out = res->v_data.real;
        double *in = v_data.real;
        for (int i = 0; i < v_length; i++)
            *out++ = sin(degtorad(*in++));
    }
    return (res);
}


sDataVec *
sDataVec::v_cos()
{
    sDataVec *res;
    if (iscomplex()) {
        res = new sDataVec(0, VF_COMPLEX, v_length);
        complex *out = res->v_data.comp;
        complex *in = v_data.comp;
        for (int i = 0; i < v_length; i++) {
            out->real = cos(degtorad(in->real)) * cosh(degtorad(in->imag));
            out->imag = -sin(degtorad(in->real)) * sinh(degtorad(in->imag));
            in++;
            out++;
        }
    }
    else {
        res = new sDataVec(0, 0, v_length);
        double *out = res->v_data.real;
        double *in = v_data.real;
        for (int i = 0; i < v_length; i++)
            *out++ = cos(degtorad(*in++));
    }
    return (res);
}


sDataVec *
sDataVec::v_tan()
{
    sDataVec *res;
    if (iscomplex()) {
        res = new sDataVec(0, VF_COMPLEX, v_length);
        complex *out = res->v_data.comp;
        complex *in = v_data.comp;
        for (int i = 0; i < v_length; i++) {
            double u = degtorad(in->real);
            double v = degtorad(in->imag);
            double su = sin(u);
            double cu = cos(u);
            double shv = sinh(v);
            double chv = cosh(v);
            complex c1(su*chv, cu*shv);
            complex c2(cu*chv, -su*shv);
            double d = c2.real*c2.real + c2.imag*c2.imag;
            rcheck(d > 0, "tan");
            out->real = (c1.real*c2.real + c1.imag*c2.imag)/d;
            out->imag = (c1.imag*c2.real - c2.imag*c1.real)/d;
            in++;
            out++;
        }
    }
    else {
        res = new sDataVec(0, 0, v_length);
        double *out = res->v_data.real;
        double *in = v_data.real;
        for (int i = 0; i < v_length; i++) {
            rcheck(cos(degtorad(*in)) != 0, "tan");
            *out++ = tan(degtorad(*in++));
        }
    }
    return (res);
}


sDataVec *
sDataVec::v_asin()
{
    sDataVec *res;
    if (iscomplex()) {
        // pi/2 - i*ln(z + sqrt(z*z-1))
        res = new sDataVec(0, VF_COMPLEX, v_length);
        complex *out = res->v_data.comp;
        complex *in = v_data.comp;
        for (int i = 0; i < v_length; i++) {
            complex c1(in->real*in->real - in->imag*in->imag - 1,
                2*in->real*in->imag);
            c1.csqrt();
            c1.real += in->real;
            c1.imag += in->imag;
            rcheck((c1.real != 0) || (c1.imag != 0), "asin");
            out->imag = radtodeg(-log(c1.mag()));
            out->real = radtodeg(M_PI_2 + atan2(c1.imag, c1.real));
            in++;
            out++;
        }
    }
    else {
        bool cres = false;
        double *in = v_data.real;
        for (int i = 0; i < v_length; i++, in++) {
            if (*in < -1.0 || *in > 1.0) {
                cres = true;
                break;
            }
        }
        if (cres) {
            res = new sDataVec(0, VF_COMPLEX, v_length);
            complex *out = res->v_data.comp;
            in = v_data.real;
            for (int i = 0; i < v_length; i++) {
                complex c1((*in)*(*in) - 1, 0);
                c1.csqrt();
                c1.real += *in;
                rcheck((c1.real != 0) || (c1.imag != 0), "asin");
                out->imag = radtodeg(-log(c1.mag()));
                out->real = radtodeg(M_PI_2 + atan2(c1.imag, c1.real));
                in++;
                out++;
            }
        }
        else {
            res = new sDataVec(0, 0, v_length);
            double *out = res->v_data.real;
            in = v_data.real;
            for (int i = 0; i < v_length; i++)
                *out++ = radtodeg(asin(*in++));
        }
    }
    return (res);
}


sDataVec *
sDataVec::v_acos()
{
    sDataVec *res;
    if (iscomplex()) {
        // -i*ln(z + sqrt(z*z-1))
        res = new sDataVec(0, VF_COMPLEX, v_length);
        complex *out = res->v_data.comp;
        complex *in = v_data.comp;
        for (int i = 0; i < v_length; i++) {
            complex c1(in->real*in->real - in->imag*in->imag - 1,
                2*in->real*in->imag);
            c1.csqrt();
            c1.real += in->real;
            c1.imag += in->imag;
            rcheck((c1.real != 0) || (c1.imag != 0), "acos");
            out->imag = radtodeg(-log(c1.mag()));
            out->real = radtodeg(atan2(c1.imag, c1.real));
            in++;
            out++;
        }
    }
    else {
        bool cres = false;
        double *in = v_data.real;
        for (int i = 0; i < v_length; i++, in++) {
            if (*in < -1.0 || *in > 1.0) {
                cres = true;
                break;
            }
        }
        if (cres) {
            res = new sDataVec(0, VF_COMPLEX, v_length);
            complex *out = res->v_data.comp;
            in = v_data.real;
            for (int i = 0; i < v_length; i++) {
                complex c1((*in)*(*in) - 1, 0);
                c1.csqrt();
                c1.real += *in;
                rcheck((c1.real != 0) || (c1.imag != 0), "acos");
                out->imag = radtodeg(-log(c1.mag()));
                out->real = radtodeg(atan2(c1.imag, c1.real));
                in++;
                out++;
            }
        }
        else {
            res = new sDataVec(0, 0, v_length);
            double *out = res->v_data.real;
            in = v_data.real;
            for (int i = 0; i < v_length; i++)
                *out++ = radtodeg(acos(*in++));
        }
    }
    return (res);
}


sDataVec *
sDataVec::v_atan()
{
    sDataVec *res;
    if (iscomplex()) {
        // 1/2i * ln( (1+iz)/(1-iz) )
        res = new sDataVec(0, VF_COMPLEX, v_length);
        complex *out = res->v_data.comp;
        complex *in = v_data.comp;
        for (int i = 0; i < v_length; i++) {
            complex c2(1.0 + in->imag, -in->real);
            double d = c2.real*c2.real + c2.imag*c2.imag;
            rcheck(d > 0, "atan");
            complex c1(1.0 - in->imag, in->real);
            complex c3(c1.real*c2.real + c1.imag*c2.imag,
                c1.imag*c2.real - c2.imag*c1.real);
            d = c3.mag()/d;
            rcheck(d > 0, "atan");
            out->imag = radtodeg(-0.5*log(d));
            out->real = radtodeg(0.5*atan2(c3.imag, c3.real));
            in++;
            out++;
        }
    }
    else {
        res = new sDataVec(0, 0, v_length);
        double *out = res->v_data.real;
        double *in = v_data.real;
        for (int i = 0; i < v_length; i++)
            *out++ = radtodeg(atan(*in++));
    }
    return (res);
}


sDataVec *
sDataVec::v_sinh()
{
    sDataVec *res;
    if (iscomplex()) {
        res = new sDataVec(0, VF_COMPLEX, v_length);
        complex *out = res->v_data.comp;
        complex *in = v_data.comp;
        for (int i = 0; i < v_length; i++) {
            out->real = sinh(degtorad(in->real)) * cos(degtorad(in->imag));
            out->imag = cosh(degtorad(in->real)) * sin(degtorad(in->imag));
            in++;
            out++;
        }
    }
    else {
        res = new sDataVec(0, 0, v_length);
        double *out = res->v_data.real;
        double *in = v_data.real;
        for (int i = 0; i < v_length; i++)
            *out++ = sinh(degtorad(*in++));
    }
    return (res);
}


sDataVec *
sDataVec::v_cosh()
{
    sDataVec *res;
    if (iscomplex()) {
        res = new sDataVec(0, VF_COMPLEX, v_length);
        complex *out = res->v_data.comp;
        complex *in = v_data.comp;
        for (int i = 0; i < v_length; i++) {
            out->real = cosh(degtorad(in->real)) * cos(degtorad(in->imag));
            out->imag = sinh(degtorad(in->real)) * sin(degtorad(in->imag));
            in++;
            out++;
        }
    }
    else {
        res = new sDataVec(0, 0, v_length);
        double *out = res->v_data.real;
        double *in = v_data.real;
        for (int i = 0; i < v_length; i++)
            *out++ = cosh(degtorad(*in++));
    }
    return (res);
}


sDataVec *
sDataVec::v_tanh()
{
    sDataVec *res;
    if (iscomplex()) {
        res = new sDataVec(0, VF_COMPLEX, v_length);
        complex *out = res->v_data.comp;
        complex *in = v_data.comp;
        for (int i = 0; i < v_length; i++) {
            double u = degtorad(in->real);
            double v = degtorad(in->imag);
            double shu = sinh(u);
            double chu = cosh(u);
            double sv = sin(v);
            double cv = cos(v);
            complex c1(shu*cv, chu*sv);
            complex c2(chu*cv, shu*sv);
            double d = c2.real*c2.real + c2.imag*c2.imag;
            rcheck(d > 0, "tanh");
            out->real = (c1.real*c2.real + c1.imag*c2.imag)/d;
            out->imag = (c1.imag*c2.real - c2.imag*c1.real)/d;
            in++;
            out++;
        }
    }
    else {
        res = new sDataVec(0, 0, v_length);
        double *out = res->v_data.real;
        double *in = v_data.real;
        for (int i = 0; i < v_length; i++)
            *out++ = tanh(degtorad(*in++));
    }
    return (res);
}


sDataVec *
sDataVec::v_asinh()
{
    sDataVec *res;
    if (iscomplex()) {
        // -i * asin(i*z)
        res = new sDataVec(0, VF_COMPLEX, v_length);
        complex *out = res->v_data.comp;
        complex *in = v_data.comp;
        for (int i = 0; i < v_length; i++) {
            complex c0(-in->imag, in->real);
            complex c1(c0.real*c0.real - c0.imag*c0.imag - 1,
                2*c0.real*c0.imag);
            c1.csqrt();
            c1.real += c0.real;
            c1.imag += c0.imag;
            rcheck((c1.real != 0) || (c1.imag != 0), "asinh");
            out->real = radtodeg(-log(c1.mag()));
            out->imag = -radtodeg(M_PI_2 + atan2(c1.imag, c1.real));
            in++;
            out++;
        }
    }
    else {
        res = new sDataVec(0, 0, v_length);
        double *out = res->v_data.real;
        double *in = v_data.real;
        for (int i = 0; i < v_length; i++) {
            double s = *in;
            s += sqrt(s*s + 1);
            *out = radtodeg(log(s));
            in++;
            out++;
        }
    }
    return (res);
}


sDataVec *
sDataVec::v_acosh()
{
    sDataVec *res;
    if (iscomplex()) {
        // -i * acos(z)
        res = new sDataVec(0, VF_COMPLEX, v_length);
        complex *out = res->v_data.comp;
        complex *in = v_data.comp;
        for (int i = 0; i < v_length; i++) {
            complex c1(in->real*in->real - in->imag*in->imag - 1,
                2*in->real*in->imag);
            c1.csqrt();
            c1.real += in->real;
            c1.imag += in->imag;
            rcheck((c1.real != 0) || (c1.imag != 0), "acosh");
            out->real = radtodeg(log(c1.mag()));
            out->imag = radtodeg(atan2(c1.imag, c1.real));
            in++;
            out++;
        }
    }
    else {
        bool cres = false;
        double *in = v_data.real;
        for (int i = 0; i < v_length; i++, in++) {
            if (*in < 1.0) {
                cres = true;
                break;
            }
        }
        if (cres) {
            res = new sDataVec(0, VF_COMPLEX, v_length);
            complex *out = res->v_data.comp;
            in = v_data.real;
            for (int i = 0; i < v_length; i++) {
                complex c1((*in)*(*in) - 1, 0);
                c1.csqrt();
                c1.real += *in;
                rcheck((c1.real != 0) || (c1.imag != 0), "acosh");
                out->real = radtodeg(log(c1.mag()));
                out->imag = radtodeg(atan2(c1.imag, c1.real));
                in++;
                out++;
            }
        }
        else {
            res = new sDataVec(0, 0, v_length);
            double *out = res->v_data.real;
            in = v_data.real;
            for (int i = 0; i < v_length; i++) {
                double s = *in;
                s += sqrt(s*s - 1);
                rcheck(s > 0, "acosh");
                *out = radtodeg(log(s));
                in++;
                out++;
            }
        }
    }
    return (res);
}


sDataVec *
sDataVec::v_atanh()
{
    sDataVec *res;
    if (iscomplex()) {
        // -i * atan(i*z) -> -1/2 ln( (1-z)/(1+z) )
        res = new sDataVec(0, VF_COMPLEX, v_length);
        complex *out = res->v_data.comp;
        complex *in = v_data.comp;
        for (int i = 0; i < v_length; i++) {
            complex c2(1.0 + in->real, in->imag);
            double d = c2.real*c2.real + c2.imag*c2.imag;
            rcheck(d > 0, "atanh");
            complex c1(1.0 - in->real, -in->imag);
            complex c3(c1.real*c2.real + c1.imag*c2.imag,
                c1.imag*c2.real - c2.imag*c1.real);
            d = c3.mag()/d;
            out->real = -radtodeg(0.5*log(d));
            out->imag = -radtodeg(0.5*atan2(c3.imag, c3.real));
            in++;
            out++;
        }
    }
    else {
        bool cres = false;
        double *in = v_data.real;
        for (int i = 0; i < v_length; i++, in++) {
            if (*in < -1.0 || *in > 1.0) {
                cres = true;
                break;
            }
        }
        if (cres) {
            res = new sDataVec(0, VF_COMPLEX, v_length);
            complex *out = res->v_data.comp;
            in = v_data.real;
            for (int i = 0; i < v_length; i++) {
                rcheck(*in != -1.0 && *in != 1.0, "atanh");
                double d = (1.0 - *in)/(1.0 + *in);
                if (d > 0)
                    out->real = -radtodeg(0.5*log(d));
                else {
                    out->real = -radtodeg(0.5*log(-d));
                    out->imag = -M_PI_2;
                }
                in++;
                out++;
            }
        }
        else {
            res = new sDataVec(0, 0, v_length);
            double *out = res->v_data.real;
            in = v_data.real;
            for (int i = 0; i < v_length; i++) {
                double s = *in;
                rcheck(s != 1.0, "atanh");
                s = (1.0 + s)/(1.0 - s);
                rcheck(s > 0, "atanh");
                *out = radtodeg(0.5*log(s));
                in++;
                out++;
            }
        }
    }
    return (res);
}


sDataVec *
sDataVec::v_j0()
{
    sDataVec *res = new sDataVec(0, 0, v_length);
    double *out = res->v_data.real;
    if (iscomplex()) {
        complex *in = v_data.comp;
        for (int i = 0; i < v_length; i++, in++)
            *out++ = j0(in->real);
    }
    else {
        double *in = v_data.real;
        for (int i = 0; i < v_length; i++)
            *out++ = j0(*in++);
    }
    return (res);
}


sDataVec *
sDataVec::v_j1()
{
    sDataVec *res = new sDataVec(0, 0, v_length);
    double *out = res->v_data.real;
    if (iscomplex()) {
        complex *in = v_data.comp;
        for (int i = 0; i < v_length; i++, in++)
            *out++ = j1(in->real);
    }
    else {
        double *in = v_data.real;
        for (int i = 0; i < v_length; i++)
            *out++ = j1(*in++);
    }
    return (res);
}


sDataVec *
sDataVec::v_jn()
{
    sDataVec *res = new sDataVec(0, 0, v_length);
    double *out = res->v_data.real;
    if (iscomplex()) {
        complex *in = v_data.comp;
        for (int i = 0; i < v_length; i++, in++)
            *out++ = jn((int)in->real, in->imag);
    }
    else {
        double *in = v_data.real;
        for (int i = 0; i < v_length; i++)
            *out++ = j0(*in++);
    }
    return (res);
}


sDataVec *
sDataVec::v_y0()
{
    sDataVec *res = new sDataVec(0, 0, v_length);
    double *out = res->v_data.real;
    if (iscomplex()) {
        complex *in = v_data.comp;
        for (int i = 0; i < v_length; i++, in++) {
            rcheck(in->real > 0, "y0");
            *out++ = y0(in->real);
        }
    }
    else {
        double *in = v_data.real;
        for (int i = 0; i < v_length; i++) {
            rcheck(*in > 0, "y0");
            *out++ = y0(*in++);
        }
    }
    return (res);
}


sDataVec *
sDataVec::v_y1()
{
    sDataVec *res = new sDataVec(0, 0, v_length);
    double *out = res->v_data.real;
    if (iscomplex()) {
        complex *in = v_data.comp;
        for (int i = 0; i < v_length; i++, in++) {
            rcheck(in->real > 0, "y1");
            *out++ = y1(in->real);
        }
    }
    else {
        double *in = v_data.real;
        for (int i = 0; i < v_length; i++) {
            rcheck(*in > 0, "y1");
            *out++ = y1(*in++);
        }
    }
    return (res);
}


sDataVec *
sDataVec::v_yn()
{
    sDataVec *res = new sDataVec(0, 0, v_length);
    double *out = res->v_data.real;
    if (iscomplex()) {
        complex *in = v_data.comp;
        for (int i = 0; i < v_length; i++, in++) {
            rcheck(in->imag > 0, "yn");
            *out++ = yn((int)in->real, in->imag);
        }
    }
    else {
        double *in = v_data.real;
        for (int i = 0; i < v_length; i++) {
            rcheck(*in > 0, "yn");
            *out++ = y0(*in++);
        }
    }
    return (res);
}


sDataVec *
sDataVec::v_cbrt()
{
    sDataVec *res;
    if (iscomplex()) {
        res = new sDataVec(0, VF_COMPLEX, v_length);
        complex *out = res->v_data.comp;
        complex *in = v_data.comp;
        for (int i = 0; i < v_length; i++, in++) {
            double d = in->mag();
            if (d > 0) {
                complex r(log(d)/3.0,
                    in->imag != 0.0 ? atan2(in->imag, in->real)/3.0 : 0.0);
                out->real = exp(r.real);
                if (r.imag != 0.0) {
                    out->imag = out->real*sin(r.imag);
                    out->real *= cos(r.imag);
                }
            }
            out++;
            in++;
        }
    }
    else {
        res = new sDataVec(0, 0, v_length);
        double *out = res->v_data.real;
        double *in = v_data.real;
        for (int i = 0; i < v_length; i++)
            *out++ = cbrt(*in++);
    }
    return (res);
}


sDataVec *
sDataVec::v_erf()
{
    sDataVec *res = new sDataVec(0, 0, v_length);
    double *out = res->v_data.real;
    if (iscomplex()) {
        complex *in = v_data.comp;
        for (int i = 0; i < v_length; i++, in++)
            *out++ = erf(in->real);
    }
    else {
        double *in = v_data.real;
        for (int i = 0; i < v_length; i++)
            *out++ = erf(*in++);
    }
    return (res);
}


sDataVec *
sDataVec::v_erfc()
{
    sDataVec *res = new sDataVec(0, 0, v_length);
    double *out = res->v_data.real;
    if (iscomplex()) {
        complex *in = v_data.comp;
        for (int i = 0; i < v_length; i++, in++)
            *out++ = erfc(in->real);
    }
    else {
        double *in = v_data.real;
        for (int i = 0; i < v_length; i++)
            *out++ = erfc(*in++);
    }
    return (res);
}


sDataVec *
sDataVec::v_gamma()
{
    sDataVec *res = new sDataVec(0, 0, v_length);
    double *out = res->v_data.real;
    if (iscomplex()) {
        complex *in = v_data.comp;
        for (int i = 0; i < v_length; i++, in++) {
            rcheck(in->imag != 0, "gamma");
            *out++ = exp(lgamma(in->real));
        }
    }
    else {
        double *in = v_data.real;
        for (int i = 0; i < v_length; i++) {
            rcheck(*in != 0, "gamma");
            *out++ = exp(lgamma(*in++));
        }
    }
    return (res);
}


// Normalize the data so that the magnitude of the greatest value is 1.
//
sDataVec *
sDataVec::v_norm()
{
    double largest = 0.0;
    if (iscomplex()) {
        complex *in = v_data.comp;
        for (int i = 0; i < v_length; i++, in++) {
            double t = in->mag();
            if (t > largest)
                largest = t;
        }
    }
    else {
        double *in = v_data.real;
        for (int i = 0; i < v_length; i++, in++) {
            double t = cxabs(*in);
            if (t > largest)
                largest = t;
        }
    }
    if (largest == 0.0) {
        GRpkg::self()->ErrPrintf(ET_ERROR, "can't normalize a 0 vector.\n");
        return (0);
    }

    sDataVec *res;
    if (iscomplex()) {
        res = new sDataVec(0, VF_COMPLEX, v_length);
        complex *out = res->v_data.comp;
        complex *in = v_data.comp;
        for (int i = 0; i < v_length; i++) {
            out->real = in->real/largest;
            out->imag = in->imag/largest;
            in++;
            out++;
        }
    }
    else {
        res = new sDataVec(0, 0, v_length);
        double *out = res->v_data.real;
        double *in = v_data.real;
        for (int i = 0; i < v_length; i++)
            *out++ = *in++/largest;
    }
    return (res);
}


sDataVec *
sDataVec::v_uminus()
{
    sDataVec *res;
    if (iscomplex()) {
        res = new sDataVec(0, VF_COMPLEX, v_length, &v_units);
        complex *out = res->v_data.comp;
        complex *in = v_data.comp;
        for (int i = 0; i < v_length; i++) {
            out->real = -in->real;
            out->imag = -in->imag;
            in++;
            out++;
        }
    }
    else {
        res = new sDataVec(0, 0, v_length, &v_units);
        double *out = res->v_data.real;
        double *in = v_data.real;
        for (int i = 0; i < v_length; i++)
            *out++ = -*in++;
    }
    return (res);
}


// Compute the mean of a vector.
//
sDataVec *
sDataVec::v_mean()
{
    sDataVec *res = 0;
    rcheck(v_length > 0, "mean");
    if (iscomplex()) {
        res = new sDataVec(0, VF_COMPLEX, 1, &v_units);
        complex *in = v_data.comp;
        complex c(0, 0);
        for (int i = 0; i < v_length; i++, in++) {
            c.real += in->real;
            c.imag += in->imag;
        }
        c.real /= v_length;
        c.imag /= v_length;
        res->v_data.comp[0] = c;
    }
    else {
        res = new sDataVec(0, 0, 1, &v_units);
        double *in = v_data.real;
        double d = 0;
        for (int i = 0; i < v_length; i++)
            d += *in++;
        d /= v_length;
        res->v_data.real[0] = d;
    }
    return (res);
}


sDataVec *
sDataVec::v_size()
{
    sDataVec *res = new sDataVec(0, 0, 1);
    res->v_data.real[0] = v_length;
    return (res);
}


// Return a vector from 0 to the magnitude of the argument. Length of the
// argument is irrelevent.
//
sDataVec *
sDataVec::v_vector()
{
    int len;
    if (isreal())
        len = (int)cxabs(*v_data.real);
    else
        len = (int)v_data.comp[0].mag();
    if (len == 0)
        len = 1;
    sDataVec *res = new sDataVec(0, 0, len);
    double *out = res->v_data.real;
    for (int i = 0; i < len; i++)
        *out++ = i;
    return (res);
}


// Create a vector of the given length composed of all ones.
//
sDataVec *
sDataVec::v_unitvec()
{
    int len;
    if (isreal())
        len = (int)cxabs(*v_data.real);
    else
        len = (int)v_data.comp[0].mag();
    if (len == 0)
        len = 1;
    sDataVec *res = new sDataVec(0, 0, len);
    double *out = res->v_data.real;
    for (int i = 0; i < len; i++)
        *out++ = 1;
    return (res);
}


sDataVec *
sDataVec::v_not()
{
    sDataVec *res = new sDataVec(0, 0, v_length);
    double *out = res->v_data.real;
    if (iscomplex()) {
        complex *in = v_data.comp;
        for (int i = 0; i < v_length; i++, in++)
            *out++ = (in->real == 0.0 && in->imag == 0.0 ? 1.0 : 0.0);
    }
    else {
        double *in = v_data.real;
        for (int i = 0; i < v_length; i++, in++)
            *out++ = (*in == 0.0 ? 1.0 : 0.0);
    }
    return (res);
}


sDataVec *
sDataVec::v_sgn()
{
    sDataVec *res;
    if (iscomplex()) {
        res = new sDataVec(0, VF_COMPLEX, v_length, &v_units);
        complex *out = res->v_data.comp;
        complex *in = v_data.comp;
        for (int i = 0; i < v_length; i++) {
            out->real = in->real > 0.0 ? 1.0 : (in->real < 0.0 ? -1.0 : 0.0);
            out->imag = in->imag > 0.0 ? 1.0 : (in->imag < 0.0 ? -1.0 : 0.0);
            out++;
            in++;
        }
    }
    else {
        res = new sDataVec(0, 0, v_length, &v_units);
        double *out = res->v_data.real;
        double *in = v_data.real;
        for (int i = 0; i < v_length; i++) {
            *out++ = *in > 0.0 ? 1.0 : (*in < 0.0 ? -1.0 : 0.0);
            in++;
        }
    }
    return (res);
}


sDataVec *
sDataVec::v_floor()
{
    sDataVec *res;
    if (iscomplex()) {
        res = new sDataVec(0, VF_COMPLEX, v_length, &v_units);
        complex *out = res->v_data.comp;
        complex *in = v_data.comp;
        for (int i = 0; i < v_length; i++) {
            out->real = floor(in->real);
            out->imag = floor(in->imag);
            in++;
            out++;
        }
    }
    else {
        res = new sDataVec(0, 0, v_length, &v_units);
        double *out = res->v_data.real;
        double *in = v_data.real;
        for (int i = 0; i < v_length; i++)
            *out++ = floor(*in++);
    }
    return (res);
}


sDataVec *
sDataVec::v_ceil()
{
    sDataVec *res;
    if (iscomplex()) {
        res = new sDataVec(0, VF_COMPLEX, v_length, &v_units);
        complex *out = res->v_data.comp;
        complex *in = v_data.comp;
        for (int i = 0; i < v_length; i++) {
            out->real = ceil(in->real);
            out->imag = ceil(in->imag);
            in++;
            out++;
        }
    }
    else {
        res = new sDataVec(0, 0, v_length, &v_units);
        double *out = res->v_data.real;
        double *in = v_data.real;
        for (int i = 0; i < v_length; i++)
            *out++ = ceil(*in++);
    }
    return (res);
}


#if defined(hpux) || defined(WIN32)
#define rint(x) (x > 0.0 ? floor(x + 0.5) : ceil(x - 0.5))
#endif

sDataVec *
sDataVec::v_rint()
{
    sDataVec *res;
    if (iscomplex()) {
        res = new sDataVec(0, VF_COMPLEX, v_length, &v_units);
        complex *out = res->v_data.comp;
        complex *in = v_data.comp;
        for (int i = 0; i < v_length; i++) {
            out->real = rint(in->real);
            out->imag = rint(in->imag);
            in++;
            out++;
        }
    }
    else {
        res = new sDataVec(0, 0, v_length, &v_units);
        double *out = res->v_data.real;
        double *in = v_data.real;
        for (int i = 0; i < v_length; i++)
            *out++ = rint(*in++);
    }
    return (res);
}


namespace {
    // Return true if both arrays are monotonically increasing or
    // decreasing, in the same direction.
    //
    bool
    check_mono(double *d1, double *d2, int len)
    {
        if (len <= 1)
            return (true);
        len--;
        bool inc1 = (d1[1] > d1[0]);
        for (int i = 0; i < len; i++) {
            if (inc1 != (d1[1] > d1[0]) || inc1 != (d2[1] > d2[0]))
                return (false);
            d1++;
            d2++;
        }
        return (true);
    }

    // Return true if the two arrays match term-by-term.
    //
    bool
    check_same(double *d1, double *d2, int len)
    {
        if (len <= 0)
            return (true);
        for (int i = 0; i < len; i++) {
            if (d1[i] != d2[i]) {
                if (fabs(d1[i] - d2[i]) > 1e-12*(fabs(d1[i]) + fabs(d2[i])))
                    return (false);
            }
        }
        return (true);
    }
}


// This is a strange function. What we do is fit a polynomial to the
// curve, of degree $polydegree, and then evaluate it at the points
// in the time scale.  What we do is this: for every set of points that
// we fit a polynomial to, fill in as much of the new vector as we can
// (i.e, between the last value of the old scale we went from to this
// one). At the ends we just use what we have...  We have to detect
// badness here too...
// Note that we pass arguments differently for this one cx_ function...
//
sDataVec *
sDataVec::v_interpolate(sDataVec *ns, bool silent)
{
    const char *msg1 = "%s has complex data.\n";
    const char *msg2 = "%s not monotonic.\n";
    const char *newscale = "new scale";
    const char *oldscale = "old scale";

    sDataVec *os = v_scale;
    if (!os && v_plot)
        os = v_plot->scale();
    if (!ns && OP.curPlot())
        ns = OP.curPlot()->scale();
    if (!os) {
        if (!silent)
            GRpkg::self()->ErrPrintf(ET_INTERR, "%s is null.\n", oldscale);
        return (0);
    }
    if (!ns) {
        if (!silent)
            GRpkg::self()->ErrPrintf(ET_INTERR, "%s is null.\n", newscale);
        return (0);
    }
    if (os->iscomplex()) {
        if (!silent)
            GRpkg::self()->ErrPrintf(ET_ERROR, msg1, oldscale);
        return (0);
    }
    if (ns->iscomplex()) {
        if (!silent)
            GRpkg::self()->ErrPrintf(ET_ERROR, msg1, newscale);
        return (0);
    }
    if (iscomplex()) {
        if (!silent)
            GRpkg::self()->ErrPrintf(ET_ERROR, msg1, "vector");
        return (0);
    }
    if (v_length != os->v_length) {
        if (!silent)
            GRpkg::self()->ErrPrintf(ET_ERROR,
                "old scale and vector lengths don't match.\n");
        return (0);
    }

    // Check the dimensionality.  If the plots have higher dimensions,
    // the scales must match exactly (for now).

    int ond = os->v_numdims > 1 ? os->v_numdims : 1;
    int nnd = ns->v_numdims > 1 ? ns->v_numdims : 1;
    if (ond != nnd) {
        if (!silent) {
            GRpkg::self()->ErrPrintf(ET_ERROR,
                "scale dimensions don't match.\n");
        }
        return (0);
    }
    int dnd = v_numdims > 1 ? v_numdims : 1;
    if (dnd != ond) {
        if (!silent) {
            GRpkg::self()->ErrPrintf(ET_ERROR,
                "old scale and vector dimensions don't match.\n");
        }
        return (0);
    }
    if (ond > 1) {
        // Higher dimensionality, can't interpolate, just copy data
        // if all is well.

        if (os->v_length != ns->v_length) {
            if (!silent) {
                GRpkg::self()->ErrPrintf(ET_ERROR,
                    "incompatible scale, old and new lengths differ.\n");
            }
            return (0);
        }
        if (os->v_dims[ond-1] != ns->v_dims[ond-1]) {
            if (!silent) {
                GRpkg::self()->ErrPrintf(ET_ERROR,
                    "scale blocksize mismatch.\n");
            }
            return (0);
        }
        if (v_dims[ond-1] != os->v_dims[ond-1]) {
            if (!silent) {
                GRpkg::self()->ErrPrintf(ET_ERROR,
                    "old scale and vector blocksize mismatch.\n");
            }
            return (0);
        }
        if (!check_same(os->v_data.real, ns->v_data.real, v_length)) {
            if (!silent) {
                GRpkg::self()->ErrPrintf(ET_ERROR,
                    "incompatible scale, old and new values differ.\n");
            }
            return (0);
        }
        sDataVec *res = new sDataVec(0, 0, ns->v_length, &v_units);
        memcpy(res->v_data.real, v_data.real, res->v_length*sizeof(double));
        res->v_numdims = v_numdims;
        memcpy(res->v_dims, v_dims, MAXDIMS*sizeof(int));
        res->v_scale = ns;
        return (res);
    }

    // Vectors are one-dimensional.  Now make sure that either both
    // scales are strictly increasing or both are strictly decreasing.
    if (os->v_length == ns->v_length) {
        if (!check_mono(os->v_data.real, ns->v_data.real, os->v_length)) {
            if (!check_same(os->v_data.real, ns->v_data.real, os->v_length)) {
                if (!silent) {
                    GRpkg::self()->ErrPrintf(ET_ERROR,
                       "incompatible scale, non-monotonic and old/new "
                       "values differ.\n");
                }
                return (0);
            }
            // Scales aren't monotonic, but the same.  Return copied data.
            sDataVec *res = new sDataVec(0, 0, ns->v_length, &v_units);
            memcpy(res->v_data.real, v_data.real, res->v_length*sizeof(double));
            res->v_numdims = v_numdims;
            memcpy(res->v_dims, v_dims, MAXDIMS*sizeof(int));
            res->v_scale = ns;
            return (res);
        }
    }
    else {
        bool oinc = (os->v_data.real[0] < os->v_data.real[1]);
        int len = os->v_length - 1;
        double *d = os->v_data.real;
        for (int i = 0; i < len; i++) {
            if ((d[0] < d[1]) != oinc) {
                if (!silent)
                    GRpkg::self()->ErrPrintf(ET_ERROR, msg2, oldscale);
                return (0);
            }
            d++;
        }
        bool ninc = (ns->v_data.real[0] < ns->v_data.real[1]);
        len = ns->v_length - 1;
        d = ns->v_data.real;
        for (int i = 0; i < len; i++) {
            if ((d[0] < d[1]) != ninc) {
                if (!silent)
                    GRpkg::self()->ErrPrintf(ET_ERROR, msg2, newscale);
                return (0);
            }
            d++;
        }
        if (oinc != ninc) {
            if (!silent) {
                GRpkg::self()->ErrPrintf(ET_ERROR,
                    "incompatible scale, old and new monotinicity differ.\n");
            }
            return (0);
        }
    }

    sDataVec *res = new sDataVec(0, 0, ns->v_length, &v_units);
    res->v_scale = ns;

    int degree = DEF_polydegree;
    VTvalue vv;
    if (Sp.GetVar(kw_polydegree, VTYP_NUM, &vv))
        degree = vv.get_int();

    sPoly po(degree);
    if (!po.interp(v_data.real, res->v_data.real, os->v_data.real,
            os->v_length, ns->v_data.real, ns->v_length)) {
        delete res;
        return (0);
    }

    return (res);
}


// The function pointer version, for parse tree evaluation.
//
sDataVec *
sDataVec::v_interpolate()
{
    return (v_interpolate(0));
}


sDataVec *
sDataVec::v_deriv()
{
    const char *msg1 = "polyfit failed at %d.\n";

    sDataVec *os = v_scale;
    if (!os && v_plot)
        os = v_plot->scale();
    if (!os) {
        GRpkg::self()->ErrPrintf(ET_ERROR, "cx_deriv: vector has no scale.\n");
        return (0);
    }

    int degree = DEF_dpolydegree; // default quadratic
    VTvalue vv;
    if (Sp.GetVar(kw_dpolydegree, VTYP_NUM, &vv))
        degree = vv.get_int();

    int n = degree + 1;
    sPoly po(degree);
    sDataVec *res;

    if (iscomplex()) {
        res = new sDataVec(0, VF_COMPLEX, v_length, &v_units);
        res->v_units/os->v_units;
        complex *c_outdata = res->v_data.comp;

        double *r_vec = new double[v_length];
        double *i_vec = new double[v_length];
        int i;
        for (i = 0; i < v_length; i++) {
            r_vec[i] = v_data.comp[i].real;
            i_vec[i] = v_data.comp[i].imag;
        }

        bool scnew = false;
        double *tscale;
        if (os->isreal())
            tscale = os->v_data.real;
        else {
            tscale = new double[v_length];
            scnew = true;
            for (i = 0; i < v_length; i++)
                tscale[i] = os->v_data.comp[i].real;
        }
        double *r_coefs = new double[n];
        double *i_coefs = new double[n];
        int k = 0;
        for (i = degree; i < v_length; i++) {

            // real
            int j;
            if (!po.polyfit(
                    tscale + i - degree, r_vec + i - degree, r_coefs)) {
                GRpkg::self()->ErrPrintf(ET_ERROR, msg1, i);
                delete [] i_vec;
                delete [] r_vec;
                delete [] i_coefs;
                delete [] r_coefs;
                if (scnew)
                    delete [] tscale;
                delete res;
                return (0);
            }
            po.pderiv(r_coefs);

            // for loop gets the beginning part
            for (j = k; j <= i + degree / 2; j++)
                c_outdata[j].real = po.peval(tscale[j], r_coefs, degree - 1);

            // imag
            if (!po.polyfit(
                    tscale + i - degree, i_vec + i - degree, i_coefs)) {
                GRpkg::self()->ErrPrintf(ET_ERROR, msg1, i);
                delete [] i_vec;
                delete [] r_vec;
                delete [] i_coefs;
                delete [] r_coefs;
                if (scnew)
                    delete [] tscale;
                delete res;
                return (0);
            }
            po.pderiv(i_coefs);

            // for loop gets the beginning part
            for (j = k; j <= i - degree / 2; j++)
                c_outdata[j].imag = po.peval(tscale[j], i_coefs, degree - 1);
            k = j;
        }

        // get the tail
        for (i = k; i < v_length; i++) {
            c_outdata[i].real = po.peval(tscale[i], r_coefs, degree - 1);
            c_outdata[i].imag = po.peval(tscale[i], i_coefs, degree - 1);
        }

        delete [] i_vec;
        delete [] r_vec;
        delete [] i_coefs;
        delete [] r_coefs;
        if (scnew)
            delete [] tscale;
    }

    else {
        // all-real case
        res = new sDataVec(0, 0, v_length, &v_units);
        res->v_units/os->v_units;
        double *outdata = res->v_data.real;

        bool scnew = false;
        double *tscale;
        if (os->isreal())
            tscale = os->v_data.real;
        else {
            tscale = new double[v_length];
            scnew = true;
            for (int i = 0; i < v_length; i++)
                tscale[i] = os->v_data.real[i];
        }

        double *coefs = new double[n];
        double *indata = v_data.real;
        int i, k = 0;
        for (i = degree; i < v_length; i++) {
            if (!po.polyfit(tscale + i - degree, indata + i - degree, coefs)) {
                GRpkg::self()->ErrPrintf(ET_ERROR, msg1, i);
                delete [] coefs;
                if (scnew)
                    delete [] tscale;
                delete res;
                return (0);
            }
            po.pderiv(coefs);

            // for loop gets the beginning part
            int j;
            for (j = k; j <= i - degree / 2; j++)
                outdata[j] = po.peval(tscale[j], coefs, degree - 1);
            k = j;
        }
        for (i = k; i < v_length; i++)
            outdata[i] = po.peval(tscale[i], coefs, degree - 1);
        delete [] coefs;
        if (scnew)
            delete [] tscale;
    }
    return (res);
}


sDataVec *
sDataVec::v_integ()
{
    sDataVec *os = v_scale;
    if (!os && v_plot)
        os = v_plot->scale();
    if (!os) {
        GRpkg::self()->ErrPrintf(ET_ERROR, "cx_integ: vector has no scale.\n");
        return (0);
    }
    if (os->v_length != v_length) {
        GRpkg::self()->ErrPrintf(ET_ERROR,
            "cx_integ: vector/scale length mismatch.\n");
        return (0);
    }
    if (os->v_length <= 1) {
        GRpkg::self()->ErrPrintf(ET_ERROR,
            "cx_integ: can't integrate scalar data.\n");
        return (0);
    }
    sDataVec *res;
    if (iscomplex()) {
        res = new sDataVec(0, VF_COMPLEX, v_length, &v_units);
        res->v_units*os->v_units;
        res->v_data.comp[0].real = 0.0;
        res->v_data.comp[0].imag = 0.0;
        for (int i = 1; i < v_length; i++) {
            double delt = os->realval(i) - os->realval(i-1);
            double d = 0.5*delt*(v_data.comp[i-1].real + v_data.comp[i].real);
            res->v_data.comp[i].real = res->v_data.comp[i-1].real + d;
            d = 0.5*delt*(v_data.comp[i-1].imag + v_data.comp[i].imag);
            res->v_data.comp[i].imag = res->v_data.comp[i-1].imag + d;
        }
    }
    else {
        res = new sDataVec(0, 0, v_length, &v_units);
        res->v_units*os->v_units;
        res->v_data.real[0] = 0.0;
        for (int i = 1; i < v_length; i++) {
            double delt = os->realval(i) - os->realval(i-1);
            double d = 0.5*delt*(v_data.real[i-1] + v_data.real[i]);
            res->v_data.real[i] = res->v_data.real[i-1] + d;
        }
    }
    return (res);
}


// FFT function from "Numerical Recipies in C", W. H. Press, et al.

#define SWAP(a, b) temp=(a); (a)=(b); (b)=temp;

namespace {
    void four1(double *data, int nn, int isign)
    {
        int n = nn << 1;
        int i, j = 1;
        for (i = 1; i < n; i += 2) {
            if (j > i) {
                double temp;
                SWAP(data[i-1], data[j-1]);
                SWAP(data[j], data[i]);
            }
            int m = n >> 1;
            while (m >= 2 && j > m) {
                j -= m;
                m >>= 1;
            }
            j += m;
        }
        int mmax = 2;
        while (n > mmax) {
            int istep = 2*mmax;
            double theta = 2*M_PI/(isign*mmax);
            double wtemp = sin(0.5*theta);
            double wpr = -2.0*wtemp*wtemp;
            double wpi = sin(theta);
            double wr = 1.0;
            double wi = 0.0;
            for (int m = 1; m < mmax; m += 2) {
                for (i = m; i <= n; i += istep) {
                    j = i + mmax;
                    double tempr = wr*data[j-1] - wi*data[j];
                    double tempi = wr*data[j] + wi*data[j-1];
                    data[j-1] = data[i-1] - tempr;
                    data[j] = data[i] - tempi;
                    data[i-1] += tempr;
                    data[i] += tempi;
                }
                wr = (wtemp = wr)*wpr - wi*wpi + wr;
                wi = wi*wpr + wtemp*wpi + wi;
            }
            mmax = istep;
        }
    }
}


sDataVec *
sDataVec::v_fft()
{
    // The vector *must* have equally-spaced points 
    int j;
    for (j = 1; j < v_length; j <<= 1) ;
    sDataVec *res = new sDataVec(0, VF_COMPLEX, j, &v_units);
    complex *c = res->v_data.comp;
    int i;
    for (i = 0; i < v_length; i++) {
        if (isreal()) {
            c[i].real = v_data.real[i];
            c[i].imag = 0.0;
        }
        else
            c[i] = v_data.comp[i];
    }
    for ( ; i < j; i++) {
        c[i].real = 0.0;
        c[i].imag = 0.0;
    }
    four1((double*)c, j, 1);

    // Since the time function is assumed real, get rid of the negative
    // frequency terms (complex conjugates)
    int len = j/2 + 1;
    res->v_data.comp = new complex[len];
    memcpy(res->v_data.comp, c, len*sizeof(complex));
    delete [] c;
    res->v_length = res->v_rlength = len;
    return (res);
}


sDataVec *
sDataVec::v_ifft()
{
    // The vector *must* have equally-spaced points 
    // The vector is assumed to have non-negative frequency components only.
    // Have to generate the complex conjugates for ifft
    int j;
    for (j = 1; j < v_length - 1; j <<= 1) ;
    int n = j;
    j *= 2;

    double fct = 1/(double)j;
    complex *c = new complex[j];

    if (isreal()) {
        c[0].real = fct*v_data.real[0];
        c[0].imag = 0.0;
    }
    else {
        c[0].real = fct*v_data.comp[0].real;
        c[0].imag = fct*v_data.comp[0].imag;
    }

    int i;
    for (i = 1; i < n; i++) {
        if (isreal()) {
            c[i].imag = 0.0;
            c[j - i].imag = 0.0;
            if (i >= v_length) {
                c[i].real = 0.0;
                c[j - i].real = 0.0;
            }
            else {
                c[i].real = fct*v_data.real[i];
                c[j - i].real = fct*v_data.real[i];
            }
        }
        else {
            if (i >= v_length) {
                c[i].real = 0;
                c[i].imag = 0;
                c[j - i].real = 0;
                c[j - i].imag = 0;
            }
            else {
                c[i].real = fct*v_data.comp[i].real;
                c[i].imag = fct*v_data.comp[i].imag;
                c[j - i].real = fct*v_data.comp[i].real;
                c[j - i].imag = -fct*v_data.comp[i].imag;
            }
        }
    }
    if (n < v_length) {
        if (isreal()) {
            c[n].real = fct*v_data.real[n];
            c[n].imag = 0;
        }
        else {
            c[n].real = fct*v_data.comp[n].real;
            c[n].imag = fct*v_data.comp[n].imag;
        }
    }
    else {
        c[n].real = 0;
        c[n].imag = 0;
    }

    four1((double*)c, j, -1);

    // return the real part
    sDataVec *res = new sDataVec(0, 0, j, &v_units);
    for (i = 0; i < j; i++)
        res->v_data.real[i] = c[i].real;
    delete [] c;
    return (res);
}


// Dummy function, will resolve to a user-defined function at eval time.
//
sDataVec *
sDataVec::v_undefined()
{
    return (0);
}



#ifndef HAVE_CBRT

// Cube root
//
double
cbrt(double x)
{
    if (x == 0.0)
        return (0.0);
    if (x < 0.0)
        x = -x;
    return (exp(log(x)/3.0));
}

#endif


#ifndef HAVE_LGAMMA

// Returns ln(gamma(xx)) for xx > 0
//
double
lgamma(double xx)
{
    static double cof[6] = {76.18009173, -86.50532033, 24.01409822,
        -1.231739516, 0.120858003e-2, -0.536382e-5 };

    double x = xx - 1.0;
    double tmp = x + 5.5;
    tmp -= (x + 0.5)*log(tmp);
    double ser = 1.0;
    for (int j = 0; j <= 5; j++) {
        x += 1.0;
        ser += cof[j]/x;
    }
    return (-tmp + log(2.50662827465*ser));
}

#endif


#if !defined(HAVE_ERF) || !defined(HAVE_ERFC)

// Most of this came from Numerical Recipes in C.


namespace {

#define ITMAX 100
#define EPS 3.0e-7

    // Returns the incomplete gamma function P(a,x) evaluated by its
    // series representation as gamser, also return ln(gamma(a)) as gln.
    //
    void gser(double *gamser, double a, double x, double *gln)
    {
        *gln = lgamma(a);
        if (x <= 0.0) {
            *gamser = 0.0;
            return;
        }
        double ap = a;
        double sum = 1.0/a;
        double del = sum;
        for (int n = 1; n <= ITMAX; n++) {
            ap += 1.0;
            del *= x/ap;
            sum += del;
            if (fabs(del) < fabs(sum)*EPS) {
                *gamser = sum*exp(-x + a*log(x) - (*gln));
                return;
            }
        }
        // bad;
        *gamser = 0.0;
        return;
    }


    // Returns the incomplete gamma function Q(a,x) evaluated by its
    // continued fraction representation as gammcf, also returns
    // ln(gamma(a)) as gin.
    //
    void gcf(double *gammcf, double a, double x, double *gln)
    {
        double gold = 0.0, fac = 1.0, b1 = 1.0;
        double b0 = 0.0, a0 = 1.0;
        *gln = lgamma(a);
        double a1 = x;
        for (int n = 1; n <= ITMAX; n++) {
            double an = n;
            double ana = an - a;
            a0 = (a1 + a0*ana)*fac;
            b0 = (b1 + b0*ana)*fac;
            double anf = an*fac;
            a1 = x*a0 + anf*a1;
            b1 = x*b0 + anf*b1;
            if (a1 != 0.0) {
                fac = 1.0/a1;
                double g = b1*fac;
                if (fabs((g - gold)/g) < EPS) {
                    *gammcf = exp(-x + a*log(x) - (*gln))*g;
                    return;
                }
                gold = g;
            }
        }
        // bad
        *gammcf = 0;
    }


    // Compute incomplete gamma function P(a,x).
    //
    double gammp(double a, double x)
    {
        if (x < 0.0 || a <= 0.0)
            // bad
            return (0.0);
        double gamser, gammcf, gln;
        if (x < a + 1.0) {
            gser(&gamser, a, x, &gln);
            return (gamser);
        }
        gcf(&gammcf, a, x, &gln);
        return (1.0 - gammcf);
    }


    // Compute incomplete gamma function Q(a,x) = 1 - P(a,x).
    //
    double gammq(double a, double x)
    {
        if (x < 0.0 || a <= 0.0)
            // bad
            return (0.0);
        double gamser, gammcf, gln;
        if (x < a + 1.0) {
            gser(&gamser, a, x, &gln);
            return (1.0 - gamser);
        }
        gcf(&gammcf, a, x, &gln);
        return (gammcf);
    }
}


// Error function
//
double
erf(double x)
{
    return (x < 0.0 ? -gammp(0.5, x*x) : gammp(0.5, x*x));
}


// Complementary error function
//
double
erfc(double x)
{
    return (x < 0.0 ? 1.0 + gammp(0.5, x*x) : gammq(0.5, x*x));
}

#endif


//
// Statistical functions
//

sDataVec *
sDataVec::v_beta()
{
    sDataVec *res = new sDataVec(0, 0, v_length);
    double *out = res->v_data.real;
    if (iscomplex()) {
        complex *in = v_data.comp;
        for (int i = 0; i < v_length; i++, in++)
            *out++ = Rnd.dst_beta(in->real, in->imag);
    }
    else {
        double *in = v_data.real;
        for (int i = 0; i < v_length; i++)
            *out++ = Rnd.dst_beta(*in++, 1.0);
    }
    return (res);
}


sDataVec *
sDataVec::v_binomial()
{
    sDataVec *res = new sDataVec(0, 0, v_length);
    double *out = res->v_data.real;
    if (iscomplex()) {
        complex *in = v_data.comp;
        for (int i = 0; i < v_length; i++, in++) {
            int n = (int)(in->imag > 0.0 ? in->imag + 0.5 : in->imag - 0.5);
            rcheck(n > 0, "binomial");
            *out++ = Rnd.dst_binomial(in->real, n);
        }
    }
    else {
        double *in = v_data.real;
        for (int i = 0; i < v_length; i++)
            *out++ = Rnd.dst_binomial(*in++, 1);
    }
    return (res);
}


sDataVec *
sDataVec::v_chisq()
{
    sDataVec *res;
    if (iscomplex()) {
        res = new sDataVec(0, VF_COMPLEX, v_length);
        complex *out = res->v_data.comp;
        complex *in = v_data.comp;
        for (int i = 0; i < v_length; i++) {
            out->real = Rnd.dst_chisq(in->real);
            out->imag = Rnd.dst_chisq(in->imag);
            in++;
            out++;
        }
    }
    else {
        res = new sDataVec(0, 0, v_length);
        double *out = res->v_data.real;
        double *in = v_data.real;
        for (int i = 0; i < v_length; i++)
            *out++ = Rnd.dst_chisq(*in++);
    }
    return (res);
}


sDataVec *
sDataVec::v_erlang()
{
    sDataVec *res = new sDataVec(0, 0, v_length);
    double *out = res->v_data.real;
    if (iscomplex()) {
        complex *in = v_data.comp;
        for (int i = 0; i < v_length; i++, in++)
            *out++ = Rnd.dst_erlang(in->real, in->imag);
    }
    else {
        double *in = v_data.real;
        for (int i = 0; i < v_length; i++)
            *out++ = Rnd.dst_erlang(*in++, 10.0);
    }
    return (res);
}


sDataVec *
sDataVec::v_exponential()
{
    sDataVec *res;
    if (iscomplex()) {
        res = new sDataVec(0, VF_COMPLEX, v_length);
        complex *out = res->v_data.comp;
        complex *in = v_data.comp;
        for (int i = 0; i < v_length; i++) {
            out->real = Rnd.dst_exponential(in->real);
            out->imag = Rnd.dst_exponential(in->imag);
            in++;
            out++;
        }
    }
    else {
        res = new sDataVec(0, 0, v_length);
        double *out = res->v_data.real;
        double *in = v_data.real;
        for (int i = 0; i < v_length; i++)
            *out++ = Rnd.dst_exponential(*in++);
    }
    return (res);
}


// Return a real vector with normal distributed coefficients.
// If the given vector is complex, terms are interpreted as (sd,mean).
// Otherwise the returned coefficients have sd given by the term
// in the real vector given, with zero mean.
//
sDataVec *
sDataVec::v_ogauss()
{
    sDataVec *res = new sDataVec(0, 0, v_length);
    double *out = res->v_data.real;
    if (iscomplex()) {
        complex *in = v_data.comp;
        for (int i = 0; i < v_length; i++, in++)
            *out++ = Rnd.gauss()*in->real + in->imag;
    }
    else {
        double *in = v_data.real;
        for (int i = 0; i < v_length; i++)
            *out++ = Rnd.gauss()*(*in++);
    }
    return (res);
}


sDataVec *
sDataVec::v_poisson()
{
    sDataVec *res;
    if (iscomplex()) {
        res = new sDataVec(0, VF_COMPLEX, v_length);
        complex *out = res->v_data.comp;
        complex *in = v_data.comp;
        for (int i = 0; i < v_length; i++) {
            out->real = Rnd.dst_poisson(in->real);
            out->imag = Rnd.dst_poisson(in->imag);
            in++;
            out++;
        }
    }
    else {
        res = new sDataVec(0, 0, v_length);
        double *out = res->v_data.real;
        double *in = v_data.real;
        for (int i = 0; i < v_length; i++)
            *out++ = Rnd.dst_poisson(*in++);
    }
    return (res);
}


// Return a vector of random values between zero and the number given
// in the input vector.  If the input vector is complex, so is the
// vector produced.
//
sDataVec *
sDataVec::v_rnd()
{
    sDataVec *res;
    if (iscomplex()) {
        res = new sDataVec(0, VF_COMPLEX, v_length);
        complex *out = res->v_data.comp;
        complex *in = v_data.comp;
        for (int i = 0; i < v_length; i++) {
            out->real = Rnd.random()*in->real;
            out->imag = Rnd.random()*in->imag;
            in++;
            out++;
        }
    }
    else {
        res = new sDataVec(0, 0, v_length);
        double *out = res->v_data.real;
        double *in = v_data.real;
        for (int i = 0; i < v_length; i++)
            *out++ = Rnd.random()*(*in++);
    }
    return (res);
}


sDataVec *
sDataVec::v_tdist()
{
    sDataVec *res;
    if (iscomplex()) {
        res = new sDataVec(0, VF_COMPLEX, v_length);
        complex *out = res->v_data.comp;
        complex *in = v_data.comp;
        for (int i = 0; i < v_length; i++) {
            out->real = Rnd.dst_tdist(in->real);
            out->imag = Rnd.dst_tdist(in->imag);
            in++;
            out++;
        }
    }
    else {
        res = new sDataVec(0, 0, v_length);
        double *out = res->v_data.real;
        double *in = v_data.real;
        for (int i = 0; i < v_length; i++)
            *out++ = Rnd.dst_tdist(*in++);
    }
    return (res);
}


//
// Funcs exported by the measaurement system.
//

// Utility to return the nearest indices to t1 and t2, or assume
// endpoints if the given values are not covered.
//
void
sDataVec::find_range(double t1, double t2, int *n1, int *n2)
{
    // If multi-dimensional, use the initial period.
    int len = v_numdims > 1 ? v_dims[1] : v_length;
    bool decr = realval(0) > realval(1);
    if ((decr && (t1 < t2)) || (!decr && (t1 > t2))) {
        double t = t1;
        t1 = t2;
        t2 = t;
    }
    bool fnd1 = false, fnd2 = false;
    for (int i = 1; i < len; i++) {
        double dx = fabs(realval(i) - realval(i-1))*1e-3;
        if (!fnd1) {
            if ((decr && (realval(i) < t1+dx)) ||
                    (!decr && (realval(i) > t1-dx))) {
                if (fabs(realval(i-1) - t1) < fabs(realval(i) - t1))
                    *n1 = i-1;
                else
                    *n1 = i;
                fnd1 = true;
            }
        }
        if (!fnd2) {
            if ((decr && (realval(i) < t2+dx)) ||
                    (!decr && (realval(i) > t2-dx))) {
                if (fabs(realval(i-2) - t1) < fabs(realval(i) - t2))
                    *n2 = i-1;
                else
                    *n2 = i;
                fnd2 = true;
            }
        }
        if (fnd1 && fnd2)
            break;
    }
    if (!fnd1)
        *n1 = 0;
    if (!fnd2)
        *n2 = len-1;
}


sDataVec *
sDataVec::v_mmin(sDataVec **v)
{
    sMfunc m(Mmin, 0);
    sDataVec *xs = scale();
    if (xs) {
        int len = xs->numdims() > 1 ? xs->dims(1) : xs->length();
        double t1 = v[0] ? v[0]->realval(0) : xs->realval(0);
        double t2 = v[1] ? v[1]->realval(0) : xs->realval(len-1);
        int n1, n2;
        xs->find_range(t1, t2, &n1, &n2);
        m.mmin(this, n1, n2, 0, 0);
    }
    sDataVec *res = new sDataVec(0, 0, 1, &v_units);
    res->v_data.real[0] = m.val();
    return (res);
}


sDataVec *
sDataVec::v_mmax(sDataVec **v)
{
    sMfunc m(Mmax, 0);
    sDataVec *xs = scale();
    if (xs) {
        int len = xs->numdims() > 1 ? xs->dims(1) : xs->length();
        double t1 = v[0] ? v[0]->realval(0) : xs->realval(0);
        double t2 = v[1] ? v[1]->realval(0) : xs->realval(len-1);
        int n1, n2;
        xs->find_range(t1, t2, &n1, &n2);
        m.mmax(this, n1, n2, 0, 0);
    }
    sDataVec *res = new sDataVec(0, 0, 1, &v_units);
    res->v_data.real[0] = m.val();
    return (res);
}


sDataVec *
sDataVec::v_mpp(sDataVec **v)
{
    sMfunc m(Mpp, 0);
    sDataVec *xs = scale();
    if (xs) {
        int len = xs->numdims() > 1 ? xs->dims(1) : xs->length();
        double t1 = v[0] ? v[0]->realval(0) : xs->realval(0);
        double t2 = v[1] ? v[1]->realval(0) : xs->realval(len-1);
        int n1, n2;
        xs->find_range(t1, t2, &n1, &n2);
        m.mpp(this, n1, n2, 0, 0);
    }
    sDataVec *res = new sDataVec(0, 0, 1, &v_units);
    res->v_data.real[0] = m.val();
    return (res);
}


sDataVec *
sDataVec::v_mavg(sDataVec **v)
{
    sMfunc m(Mavg, 0);
    sDataVec *xs = scale();
    if (xs) {
        int len = xs->numdims() > 1 ? xs->dims(1) : xs->length();
        double t1 = v[0] ? v[0]->realval(0) : xs->realval(0);
        double t2 = v[1] ? v[1]->realval(0) : xs->realval(len-1);
        int n1, n2;
        xs->find_range(t1, t2, &n1, &n2);
        m.mavg(this, n1, n2, 0, 0);
    }
    sDataVec *res = new sDataVec(0, 0, 1, &v_units);
    res->v_data.real[0] = m.val();
    return (res);
}


sDataVec *
sDataVec::v_mrms(sDataVec **v)
{
    sMfunc m(Mrms, 0);
    sDataVec *xs = scale();
    if (xs) {
        int len = xs->numdims() > 1 ? xs->dims(1) : xs->length();
        double t1 = v[0] ? v[0]->realval(0) : xs->realval(0);
        double t2 = v[1] ? v[1]->realval(0) : xs->realval(len-1);
        int n1, n2;
        xs->find_range(t1, t2, &n1, &n2);
        m.mrms(this, n1, n2, 0, 0);
    }
    sDataVec *res = new sDataVec(0, 0, 1, &v_units);
    res->v_data.real[0] = m.val();
    return (res);
}


sDataVec *
sDataVec::v_mpw(sDataVec **v)
{
    sMfunc m(Mpw, 0);
    sDataVec *xs = scale();
    if (xs) {
        int len = xs->numdims() > 1 ? xs->dims(1) : xs->length();
        double t1 = v[0] ? v[0]->realval(0) : xs->realval(0);
        double t2 = v[1] ? v[1]->realval(0) : xs->realval(len-1);
        int n1, n2;
        xs->find_range(t1, t2, &n1, &n2);
        m.mpw(this, n1, n2, 0, 0);
    }
    sDataVec *res = new sDataVec(0, 0, 1, xs->units());
    res->v_data.real[0] = m.val();
    return (res);
}


// Rise or fall time.  Y=This can have 3-5 arguments, the two optional
// arguments are the start and end fractions, which default to 0.1
// and 0.9.
//
sDataVec *
sDataVec::v_mrft(sDataVec **v)
{
    sMfunc m(Mrft, 0);
    sDataVec *xs = scale();
    if (xs) {
        int len = xs->numdims() > 1 ? xs->dims(1) : xs->length();
        double t1 = v[0] ? v[0]->realval(0) : xs->realval(0);
        double t2 = v[1] ? v[1]->realval(0) : xs->realval(len-1);
        int n1, n2;
        xs->find_range(t1, t2, &n1, &n2);
        if (v[2]) {
            double pc1 = v[2]->realval(0);
            if (v[3]) {
                double pc2 = v[3]->realval(0);
                m.mrft(this, n1, n2, 0, 0, pc1, pc2);
            }
            else 
                m.mrft(this, n1, n2, 0, 0, pc1);
        }
        else
            m.mrft(this, n1, n2, 0, 0);
    }
    sDataVec *res = new sDataVec(0, 0, 1, xs->units());
    res->v_data.real[0] = m.val();
    return (res);
}


//
// HSPICE Compatibility functions
//

sDataVec *
sDataVec::v_hs_unif(sDataVec **args)
{
    if (!args[0])
        return (0);
    sDataVec *res;
    if (!return_random()) {
        if (iscomplex()) {
            res = new sDataVec(0, VF_COMPLEX, v_length, &v_units);
            complex *out = res->v_data.comp;
            complex *in = v_data.comp;
            for (int i = 0; i < v_length; i++)
                *out++ = *in++;
        }
        else {
            res = new sDataVec(0, 0, v_length, &v_units);
            double *out = res->v_data.real;
            double *in = v_data.real;
            for (int i = 0; i < v_length; i++)
                *out++ = *in++;
        }
    }
    else {
        if (iscomplex()) {
            res = new sDataVec(0, VF_COMPLEX, v_length, &v_units);
            complex *out = res->v_data.comp;
            complex *in = v_data.comp;
            if (args[0]->iscomplex()) {
                double var_r = 2.0*args[0]->v_data.comp[0].real;
                double var_i = 2.0*args[0]->v_data.comp[0].imag;
                for (int i = 0; i < v_length; i++) {
                    out->real = in->real + in->real*var_r*(Rnd.random() - 0.5);
                    out->imag = in->imag + in->imag*var_i*(Rnd.random() - 0.5);
                    out++;
                    in++;
                }
            }
            else {
                double var = 2.0*args[0]->v_data.real[0];
                for (int i = 0; i < v_length; i++) {
                    out->real = in->real + in->real*var*(Rnd.random() - 0.5);
                    out->imag = in->imag + in->imag*var*(Rnd.random() - 0.5);
                    out++;
                    in++;
                }
            }
        }
        else {
            res = new sDataVec(0, 0, v_length, &v_units);
            double *out = res->v_data.real;
            double *in = v_data.real;
            double var = 2.0*args[0]->v_data.real[0];
            for (int i = 0; i < v_length; i++) {
                *out++ = *in + *in*var*(Rnd.random() - 0.5);
                in++;
            }
        }
    }
    return (res);
}


sDataVec *
sDataVec::v_hs_aunif(sDataVec **args)
{
    if (!args[0])
        return (0);
    sDataVec *res;
    if (!return_random()) {
        if (iscomplex()) {
            res = new sDataVec(0, VF_COMPLEX, v_length, &v_units);
            complex *out = res->v_data.comp;
            complex *in = v_data.comp;
            for (int i = 0; i < v_length; i++)
                *out++ = *in++;
        }
        else {
            res = new sDataVec(0, 0, v_length, &v_units);
            double *out = res->v_data.real;
            double *in = v_data.real;
            for (int i = 0; i < v_length; i++)
                *out++ = *in++;
        }
    }
    else {
        if (iscomplex()) {
            res = new sDataVec(0, VF_COMPLEX, v_length, &v_units);
            complex *out = res->v_data.comp;
            complex *in = v_data.comp;
            if (args[0]->iscomplex()) {
                double var_r = 2.0*args[0]->v_data.comp[0].real;
                double var_i = 2.0*args[0]->v_data.comp[0].imag;
                for (int i = 0; i < v_length; i++) {
                    out->real = in->real + var_r*(Rnd.random() - 0.5);
                    out->imag = in->imag + var_i*(Rnd.random() - 0.5);
                    out++;
                    in++;
                }
            }
            else {
                double var = 2.0*args[0]->v_data.real[0];
                for (int i = 0; i < v_length; i++) {
                    out->real = in->real + var*(Rnd.random() - 0.5);
                    out->imag = in->imag + var*(Rnd.random() - 0.5);
                    out++;
                    in++;
                }
            }
        }
        else {
            res = new sDataVec(0, 0, v_length, &v_units);
            double *out = res->v_data.real;
            double *in = v_data.real;
            double var = 2.0*args[0]->v_data.real[0];
            for (int i = 0; i < v_length; i++)
                *out++ = *in++ + var*(Rnd.random() - 0.5);
        }
    }
    return (res);
}


// This is the same as the original internal gauss function if 0 or 1
// arguments.
//
sDataVec *
sDataVec::v_hs_gauss(sDataVec **args)
{
    sDataVec *res;
    if (!return_random()) {
        if (!args[1]) {
            if (!args[0]) {
                res = new sDataVec(0, 0, v_length);
                double *out = res->v_data.real;
                if (iscomplex()) {
                    complex *in = v_data.comp;
                    for (int i = 0; i < v_length; i++, in++)
                        *out++ = in->imag;
                }
                else {
                    for (int i = 0; i < v_length; i++)
                        *out++ = 0.0;
                }
                return (res);
            }
            if (iscomplex()) {
                res = new sDataVec(0, VF_COMPLEX, v_length);
                complex *out = res->v_data.comp;
                if (args[0]->iscomplex()) {
                    complex *y = args[0]->v_data.comp;
                    for (int i = 0; i < v_length; i++) {
                        out->real = y->real;
                        out->imag = y->imag;
                        out++;
                        if (i < args[0]->v_length - 1)
                            y++;
                    }
                }
                else {
                    double *y = args[0]->v_data.real;
                    for (int i = 0; i < v_length; i++) {
                        out->real = *y;
                        out->imag = *y;
                        out++;
                        if (i < args[0]->v_length - 1)
                            y++;
                    }
                }
            }
            else {
                res = new sDataVec(0, 0, v_length);
                double *out = res->v_data.real;
                if (args[0]->iscomplex()) {
                    complex *y = args[0]->v_data.comp;
                    for (int i = 0; i < v_length; i++) {
                        *out++ = y->real;
                        if (i < args[0]->v_length - 1)
                            y++;
                    }
                }
                else {
                    double *y = args[0]->v_data.real;
                    for (int i = 0; i < v_length; i++) {
                        *out++ = *y;
                        if (i < args[0]->v_length - 1)
                            y++;
                    }
                }
            }
            return (res);
        }
        if (iscomplex()) {
            res = new sDataVec(0, VF_COMPLEX, v_length, &v_units);
            complex *out = res->v_data.comp;
            complex *in = v_data.comp;
            for (int i = 0; i < v_length; i++)
                *out++ = *in++;
        }
        else {
            res = new sDataVec(0, 0, v_length, &v_units);
            double *out = res->v_data.real;
            double *in = v_data.real;
            for (int i = 0; i < v_length; i++)
                *out++ = *in++;
        }
        return (res);
    }

    if (!args[1]) {
        if (!args[0]) {
            res = new sDataVec(0, 0, v_length);
            double *out = res->v_data.real;
            if (iscomplex()) {
                complex *in = v_data.comp;
                for (int i = 0; i < v_length; i++, in++)
                    *out++ = Rnd.gauss()*in->real + in->imag;
            }
            else {
                double *in = v_data.real;
                for (int i = 0; i < v_length; i++)
                    *out++ = Rnd.gauss()*(*in++);
            }
            return (res);
        }
        if (iscomplex()) {
            res = new sDataVec(0, VF_COMPLEX, v_length);
            complex *out = res->v_data.comp;
            complex *in = v_data.comp;
            if (args[0]->iscomplex()) {
                complex *y = args[0]->v_data.comp;
                for (int i = 0; i < v_length; i++) {
                    out->real = Rnd.gauss()*in->real + y->real;
                    out->imag = Rnd.gauss()*in->imag + y->imag;
                    out++;
                    in++;
                    if (i < args[0]->v_length - 1)
                        y++;
                }
            }
            else {
                double *y = args[0]->v_data.real;
                for (int i = 0; i < v_length; i++) {
                    out->real = Rnd.gauss()*in->real + *y;
                    out->imag = Rnd.gauss()*in->imag + *y;
                    out++;
                    in++;
                    if (i < args[0]->v_length - 1)
                        y++;
                }
            }
        }
        else {
            res = new sDataVec(0, 0, v_length);
            double *out = res->v_data.real;
            double *in = v_data.real;
            if (args[0]->iscomplex()) {
                complex *y = args[0]->v_data.comp;
                for (int i = 0; i < v_length; i++) {
                    *out++ = Rnd.gauss()*(*in++) + y->real;
                    if (i < args[0]->v_length - 1)
                        y++;
                }
            }
            else {
                double *y = args[0]->v_data.real;
                for (int i = 0; i < v_length; i++) {
                    *out++ = Rnd.gauss()*(*in++) + *y;
                    if (i < args[0]->v_length - 1)
                        y++;
                }
            }
        }
        return (res);
    }

    double sigma = args[1]->v_data.real[0];
    if (sigma == 0.0)
        return (0);
    if (iscomplex()) {
        res = new sDataVec(0, VF_COMPLEX, v_length, &v_units);
        complex *out = res->v_data.comp;
        complex *in = v_data.comp;
        if (args[0]->iscomplex()) {
            complex *y = args[0]->v_data.comp;
            for (int i = 0; i < v_length; i++) {
                out->real = in->real + in->real*y->real*Rnd.gauss()/sigma;
                out->imag = in->imag + in->imag*y->imag*Rnd.gauss()/sigma;
                out++;
                in++;
                if (i < args[0]->v_length - 1)
                    y++;
            }
        }
        else {
            double *y = args[0]->v_data.real;
            for (int i = 0; i < v_length; i++) {
                out->real = in->real + in->real*(*y)*Rnd.gauss()/sigma;
                out->imag = in->imag + in->imag*(*y)*Rnd.gauss()/sigma;
                out++;
                in++;
                if (i < args[0]->v_length - 1)
                    y++;
            }
        }
    }
    else {
        res = new sDataVec(0, 0, v_length, &v_units);
        double *out = res->v_data.real;
        double *in = v_data.real;
        if (args[0]->iscomplex()) {
            complex *y = args[0]->v_data.comp;
            for (int i = 0; i < v_length; i++) {
                *out++ = *in + (*in)*(y->real)*Rnd.gauss()/sigma;
                in++;
                if (i < args[0]->v_length - 1)
                    y++;
            }
        }
        else {
            double *y = args[0]->v_data.real;
            for (int i = 0; i < v_length; i++) {
                *out++ = *in + (*in)*(*y)*Rnd.gauss()/sigma;
                in++;
                if (i < args[0]->v_length - 1)
                    y++;
            }
        }
    }
    return (res);
}


sDataVec *
sDataVec::v_hs_agauss(sDataVec **args)
{
    if (!args[0] || !args[1])
        return (0);

    sDataVec *res;
    if (!return_random()) {
        if (iscomplex()) {
            res = new sDataVec(0, VF_COMPLEX, v_length, &v_units);
            complex *out = res->v_data.comp;
            complex *in = v_data.comp;
            for (int i = 0; i < v_length; i++)
                *out++ = *in++;
        }
        else {
            res = new sDataVec(0, 0, v_length, &v_units);
            double *out = res->v_data.real;
            double *in = v_data.real;
            for (int i = 0; i < v_length; i++)
                *out++ = *in++;
        }
    }
    else {
        double sigma = args[1]->v_data.real[0];
        if (sigma == 0.0)
            return (0);
        if (iscomplex()) {
            res = new sDataVec(0, VF_COMPLEX, v_length, &v_units);
            complex *out = res->v_data.comp;
            complex *in = v_data.comp;
            if (args[0]->iscomplex()) {
                complex *y = args[0]->v_data.comp;
                for (int i = 0; i < v_length; i++) {
                    out->real = in->real + y->real*Rnd.gauss()/sigma;
                    out->imag = in->imag + y->imag*Rnd.gauss()/sigma;
                    out++;
                    in++;
                    if (i < args[0]->v_length - 1)
                        y++;
                }
            }
            else {
                double *y = args[0]->v_data.real;
                for (int i = 0; i < v_length; i++) {
                    out->real = in->real + (*y)*Rnd.gauss()/sigma;
                    out->imag = in->imag + (*y)*Rnd.gauss()/sigma;
                    out++;
                    in++;
                    if (i < args[0]->v_length - 1)
                        y++;
                }
            }
        }
        else {
            res = new sDataVec(0, 0, v_length, &v_units);
            double *out = res->v_data.real;
            double *in = v_data.real;
            if (args[0]->iscomplex()) {
                complex *y = args[0]->v_data.comp;
                for (int i = 0; i < v_length; i++) {
                    *out++ = *in++ + y->real*Rnd.gauss()/sigma;
                    if (i < args[0]->v_length - 1)
                        y++;
                }
            }
            else {
                double *y = args[0]->v_data.real;
                for (int i = 0; i < v_length; i++) {
                    *out++ = *in++ + (*y)*Rnd.gauss()/sigma;
                    if (i < args[0]->v_length - 1)
                        y++;
                }
            }
        }
    }
    return (res);
}


sDataVec *
sDataVec::v_hs_limit(sDataVec **args)
{
    if (!args[0])
        return (0);
    sDataVec *res;
    if (!return_random()) {
        if (iscomplex()) {
            res = new sDataVec(0, VF_COMPLEX, v_length, &v_units);
            complex *out = res->v_data.comp;
            complex *in = v_data.comp;
            for (int i = 0; i < v_length; i++)
                *out++ = *in++;
        }
        else {
            res = new sDataVec(0, 0, v_length, &v_units);
            double *out = res->v_data.real;
            double *in = v_data.real;
            for (int i = 0; i < v_length; i++)
                *out++ = *in++;
        }
    }
    else {
        if (iscomplex()) {
            res = new sDataVec(0, VF_COMPLEX, v_length, &v_units);
            complex *out = res->v_data.comp;
            complex *in = v_data.comp;
            if (args[0]->iscomplex()) {
                double v_r = args[0]->v_data.comp[0].real;
                double v_i = args[0]->v_data.comp[0].imag;
                for (int i = 0; i < v_length; i++) {
                    out->real =
                        Rnd.random() > 0.5 ? in->real + v_r : in->real - v_r;
                    out->imag =
                        Rnd.random() > 0.5 ? in->imag + v_i : in->real - v_i;
                    out++;
                    in++;
                }
            }
            else {
                double var = args[0]->v_data.real[0];
                for (int i = 0; i < v_length; i++) {
                    out->real =
                        Rnd.random() > 0.5 ? in->real + var : in->real - var;
                    out->imag =
                        Rnd.random() > 0.5 ? in->imag + var : in->imag - var;
                    out++;
                    in++;
                }
            }
        }
        else {
            res = new sDataVec(0, 0, v_length, &v_units);
            double *out = res->v_data.real;
            double *in = v_data.real;
            double var = args[0]->v_data.real[0];
            for (int i = 0; i < v_length; i++) {
                *out++ = Rnd.random() > 0.5 ? *in + var : *in - var;
                in++;
            }
        }
    }
    return (res);
}


sDataVec *
sDataVec::v_hs_pow(sDataVec **args)
{
    // This is like v_power, except when y is real, in which case
    //  x ^ int(y) is computed.

    if (!args[0])
        return (0);
    sDataVec *data2 = args[0];

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
                if (i < data2->v_length - 1)
                    c2++;
            }
        }
        else {
            complex *c1 = v_data.comp;
            double *d2 = data2->v_data.real;
            for (int i = 0; i < v_length; i++) {

                int dd = (int)*d2;
                double d = c1->mag();
                rcheck(d > 0.0, "power");
                // r = ln(c1)
                complex r(log(d),
                    c1->imag != 0.0 ? atan2(c1->imag, c1->real) : 0.0);
                // s = c2*ln(c1)
                complex s(r.real*dd, r.imag*dd);
                // out = exp(s)
                out->real = exp(s.real);
                if (s.imag != 0.0) {
                    out->imag = out->real*sin(s.imag);
                    out->real *= cos(s.imag);
                }

                out++;
                c1++;
                if (i < data2->v_length - 1)
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
                if (i < data2->v_length - 1)
                    c2++;
            }
        }
        else {
            double *out = res->v_data.real;
            double *d1 = v_data.real;
            double *d2 = data2->v_data.real;
            for (int i = 0; i < v_length; i++) {
                int dd = (int)*d2;
                *out++ = pow(*d1++, (double)dd);
                if (i < data2->v_length - 1)
                    d2++;
            }
        }
    }
    return (res);
}


sDataVec *
sDataVec::v_hs_pwr(sDataVec **args)
{
    //  (sign re(x)) * (mag(x) ^ re(y))
    if (!args[0])
        return (0);
    sDataVec *res;
    res = new sDataVec(0, 0, v_length, &v_units);
    if (iscomplex()) {
        complex *out = res->v_data.comp;
        complex *in = v_data.comp;
        if (args[0]->iscomplex()) {
            complex *y = args[0]->v_data.comp;
            for (int i = 0; i < v_length; i++) {
                double dd = sqrt(in->real*in->real + in->imag*in->imag);
                dd = pow(dd, y->real);
                *out++ = in->real >= 0 ? dd : -dd;
                in++;
                if (i < args[0]->v_length - 1)
                    y++;
            }
        }
        else {
            double *y = args[0]->v_data.real;
            for (int i = 0; i < v_length; i++) {
                double dd = sqrt(in->real*in->real + in->imag*in->imag);
                dd = pow(dd, *y);
                *out++ = in->real >= 0 ? dd : -dd;
                in++;
                if (i < args[0]->v_length - 1)
                    y++;
            }
        }
    }
    else {
        double *out = res->v_data.real;
        double *in = v_data.real;
        if (args[0]->iscomplex()) {
            complex *y = args[0]->v_data.comp;
            for (int i = 0; i < v_length; i++) {
                double dd = pow(fabs(*in), y->real);
                *out++ = *in >= 0 ? dd : -dd;
                in++;
                if (i < args[0]->v_length - 1)
                    y++;
            }
        }
        else {
            double *y = args[0]->v_data.real;
            for (int i = 0; i < v_length; i++) {
                double dd = pow(fabs(*in), *y);
                *out++ = *in >= 0 ? dd : -dd;
                in++;
                if (i < args[0]->v_length - 1)
                    y++;
            }
        }
    }
    return (res);
}


sDataVec *
sDataVec::v_hs_sign(sDataVec **args)
{
    if (!args[0])
        return (0);
    sDataVec *res;
    if (iscomplex()) {
        res = new sDataVec(0, VF_COMPLEX, v_length, &v_units);
        complex *out = res->v_data.comp;
        complex *in = v_data.comp;
        if (args[0]->iscomplex()) {
            complex *y = args[0]->v_data.comp;
            for (int i = 0; i < v_length; i++) {
                out->real = y->real >= 0.0 ? fabs(in->real) : -fabs(in->real);
                out->imag = y->imag >= 0.0 ? fabs(in->imag) : -fabs(in->imag);
                out++;
                in++;
                if (i < args[0]->v_length - 1)
                    y++;
            }
        }
        else {
            double *y = args[0]->v_data.real;
            for (int i = 0; i < v_length; i++) {
                out->real = *y >= 0.0 ? fabs(in->real) : -fabs(in->real);
                out->imag = *y >= 0.0 ? fabs(in->imag) : -fabs(in->imag);
                out++;
                in++;
                if (i < args[0]->v_length - 1)
                    y++;
            }
        }
    }
    else {
        res = new sDataVec(0, 0, v_length, &v_units);
        double *out = res->v_data.real;
        double *in = v_data.real;
        if (args[0]->iscomplex()) {
            complex *y = args[0]->v_data.comp;
            for (int i = 0; i < v_length; i++) {
                *out++ = y->real >= 0.0 ? fabs(*in) : -fabs(*in);
                in++;
                if (i < args[0]->v_length - 1)
                    y++;
            }
        }
        else {
            double *y = args[0]->v_data.real;
            for (int i = 0; i < v_length; i++) {
                *out++ = *y >= 0.0 ? fabs(*in) : -fabs(*in);
                in++;
                if (i < args[0]->v_length - 1)
                    y++;
            }
        }
    }
    return (res);
}

