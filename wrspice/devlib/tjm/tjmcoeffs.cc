
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
 * WRspice Circuit Simulation and Analysis Tool:  Device Library          *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "tjmdefs.h"
#include "stab.h"
#include <stdio.h>

// WRspice provides this, defined in ckt.cc.  This not defined in the
// headers to avoid having to include stdio there.  WRspice is
// sensitive to a variable named "tjm_path" which is list of locations
// to search for fit files.  This effectively defaults to
// "( . ~/.mmjco )".
//
extern FILE *tjm_fopen(const char*);


//
// A database of tunneling amplitude tables for the TJM.
//

// Static function.
// Return the coefficient set file name for the parameters.
//
char *
TJMcoeffSet::fit_fname(double temp, double d1, double d2, double sm,
    int numxpts, int numterms, double thr)
{
    char tbuf[80];
    sprintf(tbuf, "tca%03ld%03ld%03ld%02ld%04d",
        lround(temp*100), lround(d1*100000), lround(d2*100000),
        lround(sm*1000), numxpts);
    sprintf(tbuf+strlen(tbuf), "-%02d%03ld.fit", numterms,
        lround(thr*1000));
    char *tr = new char[strlen(tbuf)+1];
    strcpy(tr, tbuf);
    return (tr);
}


namespace {
    // Built-in default TJM models, where p, A, and B are Dirichlet
    // coefficients.  Size of Dirichlet series should be even and less
    // or equal to 20.  Taken from MiTMoJCo
    // (https://github.com/drgulevich/mitmojco) repository.

    // Mitmojco  BCS42_008 and NbNb_4K2_008
    IFcomplex P_1[] =
        { cIFcomplex(-4.373312, -0.114845),
        cIFcomplex(-0.411779, 0.898335),
        cIFcomplex(-0.139858, 0.991853),
        cIFcomplex(-0.013048, 1.000349),
        cIFcomplex(-0.043182, 1.000747),
        cIFcomplex(-1.105852, -0.000000),
        cIFcomplex(-0.651673, 0.123645),
        cIFcomplex(-0.073309, 0.000067)};
    IFcomplex A_1[] = 
        { cIFcomplex(1.057436, -18.941930),
        cIFcomplex(0.219525, 0.072188),
        cIFcomplex(0.077953, 0.007944),
        cIFcomplex(0.006675, -0.000475),
        cIFcomplex(0.025091, 0.000289),
        cIFcomplex(1.376607, -0.000002),
        cIFcomplex(-0.459333, -0.152676),
        cIFcomplex(-0.000889, -0.041516)};
    IFcomplex B_1[] =
        { cIFcomplex(0.108712, 7.209623),
        cIFcomplex(0.109049, 0.238592),
        cIFcomplex(0.072237, 0.029470),
        cIFcomplex(0.006656, -0.000208),
        cIFcomplex(0.025316, 0.002275),
        cIFcomplex(-0.217956, 0.000001),
        cIFcomplex(-0.401859, -0.545156),
        cIFcomplex(0.000643, -0.098458)};

    // Mitmojco  BCS42_001 and NbNb_4K2_001
    IFcomplex P_2[] = 
        { cIFcomplex(-0.090721, -0.000036),
        cIFcomplex(-0.004370, 1.000126),
        cIFcomplex(-0.813405, -0.043201),
        cIFcomplex(-0.299741, 0.941648),
        cIFcomplex(-0.013468, 1.000303),
        cIFcomplex(-0.001497, 1.000022),
        cIFcomplex(-0.039953, 0.999920),
        cIFcomplex(-0.113673, 0.993161),
        cIFcomplex(-6.766647, -0.000001),
        cIFcomplex(-0.646220, 0.637507)};
    IFcomplex A_2[] = 
        { cIFcomplex(-0.000935, -0.344952),
        cIFcomplex(0.002376, -0.000079),
        cIFcomplex(0.701978, -3.433012),
        cIFcomplex(0.141990, 0.034241),
        cIFcomplex(0.007165, -0.000087),
        cIFcomplex(0.000650, -0.000029),
        cIFcomplex(0.020416, 0.000395),
        cIFcomplex(0.056332, 0.006446),
        cIFcomplex(1.266591, 0.000000),
        cIFcomplex(0.187313, 0.279494)};
    IFcomplex B_2[] = 
        { cIFcomplex(0.001392, -0.100648),
        cIFcomplex(0.002382, -0.000055),
        cIFcomplex(-0.258742, 0.553749),
        cIFcomplex(0.095523, 0.119127),
        cIFcomplex(0.007150, 0.000084),
        cIFcomplex(0.000649, -0.000026),
        cIFcomplex(0.020445, 0.002159),
        cIFcomplex(0.053536, 0.018137),
        cIFcomplex(-0.017427, 0.000001),
        cIFcomplex(-0.161605, 0.336628)};
}


sTab<TJMcoeffSet> *TJMcoeffSet::TJMcoeffsTab = 0;

void
TJMcoeffSet::check_coeffTab()
{
    if (!TJMcoeffsTab) {
        TJMcoeffsTab = new sTab<TJMcoeffSet>(true);
        IFcomplex *p = new IFcomplex[8];
        IFcomplex *A = new IFcomplex[8];
        IFcomplex *B = new IFcomplex[8];
        for (int i = 0; i < 8; i++) {
            p[i] = P_1[i];
            A[i] = A_1[i];
            B[i] = B_1[i];
        }
        TJMcoeffSet *cs = new TJMcoeffSet(strdup("tjm1"), 8, p, A, B);
        TJMcoeffsTab->add(cs);

        p = new IFcomplex[10];
        A = new IFcomplex[10];
        B = new IFcomplex[10];
        for (int i = 0; i < 10; i++) {
            p[i] = P_2[i];
            A[i] = A_2[i];
            B[i] = B_2[i];
        }
        cs = new TJMcoeffSet(strdup("tjm2"), 10, p, A, B);
        TJMcoeffsTab->add(cs);
    }
}


#define MAX_PARAMS 20

// Static Function.
TJMcoeffSet *
TJMcoeffSet::getTJMcoeffSet(double temp, double d1, double d2, double sm,
    int numxpts, int numterms, double thr)
{
    char *nm = fit_fname(temp, d1, d2, sm, numxpts, numterms, thr);
    TJMcoeffSet *cs = getTJMcoeffSet(nm);
    if (cs) {
        delete [] nm;
        return (cs);
    }
    char buf[80];
    sprintf(buf,
        "mmjco cdf -t %.2f -d1 %.2f -d2 %.2f -s %.3f -x %d -n %d -h %.2f",
        temp, d1*1e3, d2*1e3, sm, numxpts, numterms, thr);
    printf("%s\n", buf);
    system(buf);
    cs = getTJMcoeffSet(nm);
    delete [] nm;
    if (cs)
        return (cs);
    return (0);
}


// Static Function.
TJMcoeffSet *
TJMcoeffSet::getTJMcoeffSet(const char *nm)
{
    if (!nm)
        return (0);
    check_coeffTab();

    TJMcoeffSet *cs = TJMcoeffsTab->find(nm);
    if (cs)
        return (cs);

    FILE *fp = tjm_fopen(nm);
    if (fp) {
        cIFcomplex p[MAX_PARAMS], A[MAX_PARAMS], B[MAX_PARAMS];
        char buf[256];
        int cnt = 0;
        while (fgets(buf, 256, fp) != 0) {
            double pr, pi, ar, ai, br, bi;
            if (sscanf(buf, "%lf, %lf, %lf, %lf, %lf, %lf", &pr, &pi, &ar, &ai,
                    &br, &bi) == 6) {
                if (cnt < MAX_PARAMS) {
                    p[cnt].real = pr;
                    p[cnt].imag = pi;
                    A[cnt].real = ar;
                    A[cnt].imag = ai;
                    B[cnt].real = br;
                    B[cnt].imag = bi;
                }
                cnt++;
            }
        }
        if (cnt >= 4 && cnt <= MAX_PARAMS && !(cnt&1)) {
            cIFcomplex *np = new cIFcomplex[cnt];
            cIFcomplex *nA = new cIFcomplex[cnt];
            cIFcomplex *nB = new cIFcomplex[cnt];
            for (int i = 0; i < cnt; i++) {
                np[i] = p[i];
                nA[i] = A[i];
                nB[i] = B[i];
            }
            return (new TJMcoeffSet(strdup(nm), cnt, np, nA, nB));
        }
    }
    return (0);
}


// Static function.
// Jqp model.
//
cIFcomplex *
TJMcoeffSet::modelJqp(const cIFcomplex *cp, const cIFcomplex *cb, int csz,
    const double *w, int lenw)
{
#define rep(z)  (-fabs(z))
#define zeta(n) cp[n].real
#define eta(n)  cp[n].imag
#define reb(n)  cb[n].real
#define imb(n)  cb[n].imag
    cIFcomplex *sum = new cIFcomplex[lenw];
    for (int k = 0; k < lenw; k++) {
        sum[k].real = 0.0;
        sum[k].imag = 0.0;
        for (int n = 0; n < csz; n++) { 
            double numr = reb(n)*rep(zeta(n)) + imb(n)*eta(n);
            double numi = w[k]*reb(n);
            double denr = rep(zeta(n))*rep(zeta(n)) + eta(n)*eta(n) - w[k]*w[k];
            double deni = 2.0*w[k]*rep(zeta(n));
            double d = denr*denr + deni*deni;
            sum[k].real -= (numr*denr + numi*deni)/d;
            sum[k].imag -= (numi*denr - numr*deni)/d;
        }
        sum[k].imag += w[k];
    }
    return (sum);
#undef rep
#undef zeta
#undef eta
#undef reb
#undef imb
}

