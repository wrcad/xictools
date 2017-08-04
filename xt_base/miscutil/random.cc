
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

#include <math.h>
#include <limits.h>
#include "random.h"


// Based on gasdev() from Numerical Recipes in C.  Use the Box-Muller
// transformation to get two normal deviates, return one and store the
// other for the next call.
//
double
sRnd::gauss()
{
    if (!rnd_set) {
        double r, v1, v2;
        do {
            v1 = 2.0*random() - 1.0;
            v2 = 2.0*random() - 1.0;
            r = v1*v1 + v2*v2;
        } while (r >= 1.0);
        double fac = sqrt(-2.0*log(r)/r);
        rnd_val = v1*fac;
        rnd_set = true;
        return (v2*fac); 
    }
    rnd_set = false;
    return (rnd_val);
}


//The Gamma distribution of order a>0 is defined by:
//
// p(x) dx = {1 / \Gamma(a) b^a } x^{a-1} e^{-x/b} dx
//   
// for x>0.  If X and Y are independent gamma-distributed random
// variables of order a1 and a2 with the same scale parameter b, then
// X+Y has gamma distribution of order a1+a2.
//  
// The algorithms below are from Knuth, vol 2, 2nd ed, p. 129.
//
double
sRnd::dst_gamma(double a, double b)
{
    if (a <= 0.0)
        return (0);
    unsigned int na = (unsigned int)floor(a);

    if (a >= UINT_MAX)
        return (b*(gamma_large(floor(a)) + gamma_frac(a - floor (a))));
    if (a == na)
        return (b*gamma_int(na));
    if (na == 0)
        return (b*gamma_frac(a));
    return (b*(gamma_int(na) + gamma_frac(a - na)));
}


// The chisq distribution has the form
//
// p(x) dx = (1/(2*Gamma(nu/2))) (x/2)^(nu/2 - 1) exp(-x/2) dx
//
// for x = 0 ... +infty
//
double
sRnd::dst_chisq(double nu)
{
    return (2.0*dst_gamma(nu/2.0, 1.0));
}


// The sum of N samples from an exponential distribution gives an
// Erlang distribution.
//
// p(x) dx = x^(n-1) exp(-x/a) / ((n-1)!a^n) dx
//
// for x = 0 ... +infty
//
double
sRnd::dst_erlang(double a, double n)
{
    return (dst_gamma(n, a));
}


// The t-distribution has the form
//
// p(x) dx = (Gamma((nu + 1)/2)/(sqrt(pi nu) Gamma(nu/2))
// * (1 + (x^2)/nu)^-((nu + 1)/2) dx
//
// The method used here is the one described in Knuth
//
double
sRnd::dst_tdist(double nu)
{
    if (nu <= 2) {
        double Y1 = gauss();
        double Y2 = dst_chisq(nu);
        return (Y1/sqrt(Y2/nu));
    }
    double Y1, Y2, Z;
    do {
        Y1 = gauss();
        Y2 = -(1/(nu/2 - 1))*log1p(-random());

        Z = Y1*Y1 / (nu - 2);
    }
    while (1 - Z < 0 || exp (-Y2 - Z) > (1 - Z));

    // Note that there is a typo in Knuth's formula, the line below
    // is taken from the original paper of Marsaglia, Mathematics of
    // Computation, 34 (1980), p 234-256 */

    return (Y1/sqrt((1 - 2/nu)*(1 - Z)));
}


// The beta distribution has the form
//
// p(x) dx = (Gamma(a + b)/(Gamma(a) Gamma(b))) x^(a-1) (1-x)^(b-1) dx
//
// The method used here is the one described in Knuth.
//
double
sRnd::dst_beta(const double a, const double b)
{
    double x1 = dst_gamma(a, 1.0);
    double x2 = dst_gamma(b, 1.0);

    return (x1/(x1 + x2));
}


// The binomial distribution has the form,
//
// prob(k) =  n!/(k!(n-k)!) *  p^k (1-p)^(n-k) for k = 0, 1, ..., n
//
// This is the algorithm from Knuth.
//
unsigned int
sRnd::dst_binomial(double p, unsigned int n)
{
    unsigned int k = 0;
    while (n > 10) {
        unsigned int a = 1 + (n/2);
        unsigned int b = 1 + n - a;

        double X = dst_beta((double)a, (double)b);

        if (X >= p) {
            n = a - 1;
            p /= X;
        }
        else {
            k += a;
            n = b - 1;
            p = (p - X)/(1 - X);
        }
    }
    for (unsigned int i = 0; i < n; i++) {
        double u = random();
        if (u < p)
            k++;
    }
    return (k);
}


// The poisson distribution has the form
//
// p(n) = (mu^n / n!) exp(-mu)
//
// for n = 0, 1, 2, ... . The method used here is the one from Knuth.
//
unsigned int
sRnd::dst_poisson(double mu)
{
    double prod = 1.0;
    unsigned int k = 0;
    while (mu > 10) {
        unsigned int m = (unsigned int)(mu * (7.0 / 8.0));

        double X = gamma_int(m);

        if (X >= mu)
            return (k + dst_binomial(mu/X, m - 1));
        k += m;
        mu -= X;
    }

    // The following method works well when mu is small.
    double emu = exp(-mu);
    do {
        prod *= random();
        k++;
    }
    while (prod > emu);

    return (k - 1);
}


double
sRnd::gamma_large(double a)
{
    // Works only if a > 1, and is most efficient if a is large.
    //
    // This algorithm, reported in Knuth, is attributed to Ahrens.  A
    // faster one, we are told, can be found in:  J.  H.  Ahrens and U. 
    // Dieter, Computing 12 (1974) 223-246.

    double x, y, v;
    double sqa = sqrt(a+a - 1);
    do {
        do {
            y = tan(M_PI * random());
            x = sqa*y + a - 1;
        }
        while (x <= 0);
        v = random();
    }
    while (v > (1 + y*y)*exp((a - 1)*log(x/(a - 1)) - sqa*y));
    return (x);
}


double
sRnd::gamma_frac(double a)
{
    // This is exercise 16 from Knuth; see page 135, and the solution
    // is on page 551.

    if (a == 0)
        return (0);

    double q, x;
    double p = M_E/(a + M_E);
    do {
        double u = random();
        double v = random();
        while (v == 0.0)
            v = random();

        if (u < p) {
            x = exp((1/a)*log(v));
            q = exp(-x);
        }
        else {
            x = 1 - log(v);
            q = exp((a - 1)*log(x));
        }
    }
    while (random() >= q);
    return (x);
}


double
sRnd::gamma_int(unsigned int a)
{
    if (a < 12) {
        double prod = 1;
        for (unsigned int i = 0; i < a; i++)
            prod *= random();

        // Note: for 12 iterations we are safe against underflow, since
        // the smallest positive random number is O(2^-32). This means
        // the smallest possible product is 2^(-12*32) = 10^-116 which
        // is within the range of double precision.

        return (-log(prod));
    }
    return (gamma_large((double)a));
}

