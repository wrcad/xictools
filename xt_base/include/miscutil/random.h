
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

#ifndef RANDOM_H
#define RANDOM_H

#include "randval.h"


// Random number generating utility struct.
//
struct sRnd
{
    sRnd()
        {
            rnd_seed = 0;
            rnd_set = false;
            rnd_val = 0.0;
        }

    void seed(int sd)
        {
            rnd_seed = sd;
            rnd_set = false;
            rnd.rand_seed(sd);
        }

    int cur_seed() const
        {
            return (rnd_seed);
        }

    unsigned int random_int()
        {
            return (rnd.rand_value());
        }

    // Return uniform random number between 0 and 1.
    //
    double random()
        {
            return ((rnd.rand_value() & 0x7fffffff)/(0x7fffffff + 1.0));
        }

    double gauss();

    double dst_exponential(double mean)
        {
            if (mean <= 0)
                return (0);
            return (-mean * log1p(-random()));
        }

    double dst_gamma(double, double);
    double dst_chisq(double);
    double dst_erlang(double, double);
    double dst_tdist(double);
    double dst_beta(double, double);
    unsigned int dst_binomial(double, unsigned int);
    unsigned int dst_poisson(double);

private:
    double gamma_large(double);
    double gamma_frac(double);
    double gamma_int(unsigned int);

    int rnd_seed;
    bool rnd_set;   // deviate selector for gauss
    double rnd_val; // second normal deviate for gauss
    randval rnd;
};

#endif
