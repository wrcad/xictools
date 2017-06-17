
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2015 Whiteley Research Inc, all rights reserved.        *
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
 $Id: random.h,v 1.2 2015/04/10 00:54:07 stevew Exp $
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
