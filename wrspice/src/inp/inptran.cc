
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
Authors: 1987 Wayne A. Christopher
         1992 Stephen R. Whiteley
****************************************************************************/

#include <math.h>
#include "input.h"
#include "inpptree.h"
#include "circuit.h"
#include "misc.h"
#include "miscutil/random.h"


#ifndef MAXFLOAT
#define MAXFLOAT 3.40282346638528860e+38
#endif

#ifndef M_2_SQRTPI
#define M_2_SQRTPI 1.12837916709551257390  // 2/sqrt(pi)
#endif

#ifndef M_LN2
#define M_LN2		0.69314718055994530942	/* log_e 2 */
#endif

namespace {
double TWOSQRTLN2 = 2.0*sqrt(M_LN2);
}


//
// "Tran" function setup/evaluation (IFtranData methods).
//

//#define DEBUG

IFtranData::IFtranData(PTftType tp, double *list, int num, double *list2,
    bool hasR, int Rix, double TDval)
{
    td_coeffs = 0;
    td_parms = 0;
    td_cache = 0;
    td_numcoeffs = 0;
    td_type = tp;
    td_pwlRgiven = hasR;
    td_pwlRstart = Rix;
    td_pwlindex = 0;
    td_pwldelay = TDval;

    switch (tp) {
    case PTF_tNIL:
    case PTF_tTABLE:
        return;

    case PTF_tPULSE:
        td_parms = new double[8];
        td_cache = 0;
        td_coeffs = list;
        td_numcoeffs = num;

        set_V1(list[0]);
        set_V2(num >= 2 ? list[1] : list[0]);
        set_TD(num >= 3 ? list[2] : 0.0);
        set_TR(num >= 4 ? list[3] : 0.0);
        set_TF(num >= 5 ? list[4] : 0.0);
        set_PW(num >= 6 ? list[5] : 0.0);
        set_PER(num >= 7 ? list[6] : 0.0);
        return;

    case PTF_tGPULSE:
        td_parms = new double[8];
        td_coeffs = list;
        td_numcoeffs = num;

        set_V1(list[0]);
        set_V2(num >= 2 ? list[1] : list[0]);
        set_TD(num >= 3 ? list[2] : 0.0);

        // New in 4.3.3, default is to take pulse width as FWHM, used
        // to just take this as the "variance".  However, if the value
        // is negative, use the old setup with the absolute value.
        set_GPW(num >= 4 ?
            (list[3] >= 0.0 ? list[3]/TWOSQRTLN2 : -list[3]) : 0.0);

        set_RPT(num >= 5 ? list[4] : 0.0);
        return;

    case PTF_tPWL:
        {
            // Add the delay, if any.
            if (td_pwldelay > 0.0) {
                for (int i = 0; i < num; i += 2)
                    list[i] += td_pwldelay;
            }

            // If the first ordinate is larger than 0, add a 0,v1
            // pair.  If the length is odd, add a value at the end.

            bool add0 = (list[0] > 0.0);
            bool add1 = (num & 1);
            if (add0 || add1) {
                int nnum = num + 2*add0 + add1;
                double *nlst = new double[nnum];
                double *d = nlst;
                if (add0) {
                    *d++ = 0.0;
                    *d++ = num > 1 ? list[1] : 0.0;
                }
                for (int i = 0; i < num; i++)
                    *d++ = list[i];
                if (add1) {
                    if (num > 1)
                        *d = *(d-2);
                    else
                        *d = 0.0;
                }
                num = nnum;
                delete [] list;
                list = nlst;
            }
            td_coeffs = list;

            if (add0 && td_pwlRgiven)
                td_pwlRstart++;

            // Throw out pairs that are not monotonic in first value.
            bool foo = false;
            for (int i = 2; i < num; ) {
                if (td_coeffs[i] <= td_coeffs[i-2]) {
                    // monotonicity error
                    for (int k = i; k+2 < num; k += 2) {
                        td_coeffs[k] = td_coeffs[k+2];
                        td_coeffs[k+1] = td_coeffs[k+3];
                    }
                    num -= 2;
                    foo = true;
                    continue;
                }
                i += 2;
            }
            if (foo) {
                IP.logError(IP_CUR_LINE,
                    "PWL list not monotonic, some points were rejected.");
            }
            td_numcoeffs = num;

            // Cache the slopes.
            int i = num/2 - 1;
            if (i > 0) {
                td_cache = new double[i + td_pwlRgiven];
                for (int k = 0; k < i; k++) {
                    td_cache[k] =
                        (td_coeffs[2*k+3] - td_coeffs[2*k+1])/
                        (td_coeffs[2*k+2] - td_coeffs[2*k]);
                }
                if (td_pwlRgiven) {
                    // We need one more slope value.  In our scheme,
                    // the Rstart point is "mapped" to the final list
                    // point, so that the next value is the one
                    // following Rstart.
                    
                    int k = td_pwlRstart;
                    td_cache[i] =
                        (td_coeffs[2*k+3] - td_coeffs[2*i+1])/
                        (td_coeffs[2*k+2] - td_coeffs[2*k]);
                }
            }
            else {
                td_cache = new double;
                *td_cache = 0.0;
            }
        }
        return;

    case PTF_tSIN:
        td_parms = new double[8];
        td_coeffs = list;
        td_numcoeffs = num;

        set_VO(list[0]);
        set_VA(num >= 2 ? list[1] : 0.0);
        set_FREQ(num >= 3 ? list[2] : 0.0);
        set_TDL(num >= 4 ? list[3] : 0.0);
        set_THETA(num >= 5 ? list[4] : 0.0);
        set_PHI(num >= 6 ? list[5] : 0.0);
        return;

    case PTF_tSPULSE:
        td_parms = new double[8];
        td_coeffs = list;
        td_numcoeffs = num;

        set_V1(list[0]);
        set_V2(num >= 2 ? list[1] : list[0]);
        set_SPER(num >= 3 ? list[2] : 0.0);
        set_SDEL(num >= 4 ? list[3] : 0.0);
        set_THETA(num >= 5 ? list[4] : 0.0);
        return;

    case PTF_tEXP:
        td_parms = new double[8];
        td_coeffs = list;
        td_numcoeffs = num;

        set_V1(list[0]);
        set_V2(num >= 2 ? list[1] : list[0]);
        set_TD1(num >= 3 ? list[2] : 0.0);
        set_TAU1(num >= 4 ? list[3] : 0.0);
        set_TD2(num >= 5 ? list[4] : 0.0);
        set_TAU2(num >= 6 ? list[5] : 0.0);
        return;

    case PTF_tSFFM:
        td_parms = new double[8];
        td_coeffs = list;
        td_numcoeffs = num;

        set_VO(list[0]);
        set_VA(num >= 2 ? list[1] : 0.0);
        set_FC(num >= 3 ? list[2] : 0.0);
        set_MDI(num >= 4 ? list[3] : 0.0);
        set_FS(num >= 5 ? list[4] : 0.0);
        return;

    case PTF_tAM:
        td_parms = new double[8];
        td_coeffs = list;
        td_numcoeffs = num;

        set_SA(list[0]);
        set_OC(num >= 2 ? list[1] : 0.0);
        set_MF(num >= 3 ? list[2] : 0.0);
        set_CF(num >= 4 ? list[3] : 0.0);
        set_DL(num >= 5 ? list[4] : 0.0);
        return;

    case PTF_tGAUSS:
        td_parms = new double[8];
        td_coeffs = list;
        td_numcoeffs = num;

        set_SD(list[0]);
        if (SD() == 0.0)
            set_SD(1.0);
        if (num > 1)
            set_MEAN(list[1]);
        else
            set_MEAN(0.0);
        if (num > 2) {
            set_LATTICE(list[2]);
            if (LATTICE() < 0.0)
                set_LATTICE(0.0);
        }
        else
            set_LATTICE(0.0);
        if (num > 3) {
            set_ILEVEL(list[3]);
            if (ILEVEL() != 0.0)
                set_ILEVEL(1.0);
        }
        else
            set_ILEVEL(1.0);
        set_LVAL(MEAN());
        set_VAL(MEAN());
        if (LATTICE() > 0.0)
            set_NVAL(SD()*Rnd.gauss() + MEAN());
        else
            set_NVAL(MEAN());
        set_TIME(0.0);
        return;

    case PTF_tINTERP:
        if (num > 0) {
            td_parms = new double[num];
            td_coeffs = new double[num];
            if (list)
                memcpy(td_parms, list, num*sizeof(double));
            else
                memset(td_parms, 0, num*sizeof(double));
            if (list2)
                memcpy(td_coeffs, list2, num*sizeof(double));
            else
                memset(td_coeffs, 0, num*sizeof(double));
            td_numcoeffs = num;
        }
        return;
    }
}


// Setup function, final setup and assign default values.
//
void
IFtranData::setup(sCKT *ckt, double step, double finaltime, bool skipbr)
{
    if (td_type == PTF_tPULSE) {
        if (TR() <= 0.0)
            set_TR(step);
        if (TF() > 0.0 && PW() <= 0.0)
            set_PW(0.0);
        else {
            if (TF() <= 0.0)
                set_TF(step);
            if (PW() <= 0.0)
                set_PW(MAXFLOAT);
        }
        if (PER() <= 0.0)
            set_PER(MAXFLOAT);
        else if (PER() < TR() + PW() + TF())
            set_PER(TR() + PW() + TF());
        if (!td_cache)
            td_cache = new double[2];
        td_cache[0] = (V2() - V1())/TR();
        td_cache[1] = (V1() - V2())/TF();

        if (!skipbr && ckt) {
            if (PER() < finaltime) {
                double time = TD();
                ckt->breakSetLattice(time, PER());
                time += TR();
                ckt->breakSetLattice(time, PER());
                if (PW() != 0) {
                    time += PW();
                    ckt->breakSetLattice(time, PER());
                }
                if (TF() != 0) {
                    time += TF();
                    ckt->breakSetLattice(time, PER());
                }
            }
            else {
                double time = TD();
                ckt->breakSet(time);
                time += TR();
                ckt->breakSet(time);
                if (PW() != 0) {
                    time += PW();
                    ckt->breakSet(time);
                }
                if (TF() != 0) {
                    time += TF();
                    ckt->breakSet(time);
                }
            }
            // set for additional offsets
            for (int i = 7; i < td_numcoeffs; i++) {
                if (PER() < finaltime) {
                    double time = td_coeffs[i];
                    ckt->breakSetLattice(time, PER());
                    time += TR();
                    ckt->breakSetLattice(time, PER());
                    if (PW() != 0) {
                        time += PW();
                        ckt->breakSetLattice(time, PER());
                    }
                    if (TF() != 0) {
                        time += TF();
                        ckt->breakSetLattice(time, PER());
                    }
                }
                else {
                    double time = td_coeffs[i];
                    ckt->breakSet(time);
                    time += TR();
                    ckt->breakSet(time);
                    if (PW() != 0) {
                        time += PW();
                        ckt->breakSet(time);
                    }
                    if (TF() != 0) {
                        time += TF();
                        ckt->breakSet(time);
                    }
                }
            }
        }
    }
    else if (td_type == PTF_tGPULSE) {
        if (GPW() == 0.0) {

            // A = phi0*fwhm/(2*sqrt(pi*ln(2)))
            // fwhm = A*(2*sqrt(pi*ln(2)))/phi0
            // If no pulse width was given, new default in 4.3.3 is to
            // generate an SFQ pulse with given amplitude, or if the
            // amplitude is zero, use TSTEP as FWHM for SFQ.

            if (V2() != V1()) {
                double A = fabs(V2() - V1());
                set_GPW(TWOSQRTLN2*wrsCONSTphi0/(A*sqrt(M_PI)));
            }
            else {
                set_GPW(step/TWOSQRTLN2);
            }
}

        // If the pulse has zero amplitude, take it to be a single
        // flux quantum (SFQ) pulse.  This is a pulse that when
        // applied to an inductor will induce one quantum of flux (I
        // * L = the physical constant PHI0).  Such pulses are
        // encountered in superconducting electronics.

        if (V2() == V1())
            set_V2(V1() + 0.5*wrsCONSTphi0*M_2_SQRTPI/GPW());
    }
    else if (td_type == PTF_tPWL) {
        if (!skipbr && ckt) {
            // Only call this when time is independent variable.

            int n = td_numcoeffs/2;
            double ta = 0.0;
            int istart = 0;
            for (;;) {
                for (int i = istart; i < n; i++) {
                    double t = td_coeffs[2*i] + ta;
                    if (t > finaltime)
                        return;
                    ckt->breakSet(t);
#ifdef DEBUG
                    printf("B %g\n", t);
#endif
                }
                if (!td_pwlRgiven)
                    break;
                ta += td_coeffs[2*(n-1)] - td_coeffs[2*td_pwlRstart];
                istart = td_pwlRstart + 1;
            }
        }
    }
    else if (td_type == PTF_tSIN) {
        if (!FREQ())
            set_FREQ(1.0/finaltime);
        if (!skipbr) {
            if (TDL() > 0.0)
                ckt->breakSet(TDL());
        }
    }
    else if (td_type == PTF_tSPULSE) {
        if (!SPER())
            set_SPER(finaltime);
        if (!skipbr && ckt) {
            if (SDEL() > 0.0)
                ckt->breakSet(SDEL());
        }
    }
    else if (td_type == PTF_tEXP) {
        if (!TAU2())
            set_TAU2(step);
        if (!TD1())
            set_TD1(step);
        if (!TAU1())
            set_TAU1(step);
        if (!TD2())
            set_TD2(TD1() + step);
        if (!skipbr && ckt) {
            if (TD1() > 0.0)
                ckt->breakSet(TD1());
            if (TD2() > TD1())
                ckt->breakSet(TD2());
        }
    }
    else if (td_type == PTF_tSFFM) {
        if (!FC())
            set_FC(1.0/finaltime);
        if (!FS())
            set_FS(1.0/finaltime);
        // no breakpoints
    }
    else if (td_type == PTF_tAM) {
        if (!MF())
            set_MF(1.0/finaltime);
    }
    else if (td_type == PTF_tGAUSS) {
        if (!skipbr && ckt) {
            if (LATTICE() != 0.0)
                ckt->breakSetLattice(0.0, LATTICE());
        }
    }
}


// Evaluation functions.

double
IFtranData::eval_tPULSE(double t)
{
    double value;
    double pw = TR() + PW();
    double time = t - TD();
    if (time > PER()) {
        // Repeating signal - figure out where we are in period.
        double basetime = PER() * (int)(time/PER());
        time -= basetime;
    }
    if (time < TR() || time >= pw + TF()) {
        value = V1();
        if (time > 0 && time < TR())
            value += time* *td_cache;
    }
    else {
        value = V2();
        if (time > pw)
            value += (time - pw)* *(td_cache+1);
    }

    // New feature: additional entries are added  as in
    // pulse(V1 V2 TD ...) + pulse(0 V2-V1 TD1 ...) + ...
    // extra entries have offset subtracted.
    //
    for (int i = 7; i < td_numcoeffs; i++) {
        time = t - td_coeffs[i];
        if (time > PER()) {
            // Repeating signal - figure out where we are in period.
            double basetime = PER() * (int)(time/PER());
            time -= basetime;
        }
        if (time < TR() || time >= pw + TF()) {
            if (time > 0 && time < TR())
                value += time* *td_cache;
        }
        else {
            value += (V2()-V1());
            if (time > pw)
                value += (time - pw)* *(td_cache+1);
        }
    }
    return (value);
}


double
IFtranData::eval_tPULSE_D(double t)
{
    double value = 0.0;
    double pw = TR() + PW();
    double time = t - TD();
    if (time > PER()) {
        // Repeating signal - figure out where we are in period.
        double basetime = PER() * (int)(time/PER());
        time -= basetime;
    }
    if (time < TR() || time >= pw + TF()) {
        if (time > 0 && time < TR())
            value = *td_cache;
    }
    else {
        if (time > pw)
            value = *(td_cache+1);
    }

    // New feature: additional entries are added  as in
    // pulse(V1 V2 TD ...) + pulse(0 V2-V1 TD1 ...) + ...
    // extra entries have offset subtracted.
    //
    for (int i = 7; i < td_numcoeffs; i++) {
        time = t - td_coeffs[i];
        if (time > PER()) {
            // Repeating signal - figure out where we are in period.
            double basetime = PER() * (int)(time/PER());
            time -= basetime;
        }
        if (time < TR() || time >= pw + TF()) {
            if (time > 0 && time < TR())
                value = *td_cache;
        }
        else {
            if (time > pw)
                value = *(td_cache+1);
        }
    }
    return (value);
}


double
IFtranData::eval_tGPULSE(double t)
{
    double V = 0;
    const double rchk = 5.0;
    if (RPT() > 0.0) {
        double per = RPT();
        double minper = GPW() + GPW();
        if (per >= minper) {

            int ifirst, ilast;
            if (t < TD()) {
                ifirst = 0;
                ilast = 0;
            }
            else {
                double del = rchk*GPW();
                ifirst = (int)rint((t - TD() - del)/per);
                ilast = (int)rint((t - TD() + del)/per);
            }
            for (int i = ifirst; i <= ilast; i++) {
                double time = t - i*per;
                double a = (time - TD())/GPW();
                V += exp(-a*a);
            }
        }
    }
    else {
        double a = (t - TD())/GPW();
        if (fabs(a) < rchk)
            V += exp(-a*a);

        for (int i = 5; i < td_numcoeffs; i++) {
            a = (t - td_coeffs[i])/GPW();
            if (fabs(a) < rchk)
                V += exp(-a*a);
        }
    }
    return (V1() + (V2() - V1())*V);
}


double
IFtranData::eval_tGPULSE_D(double t)
{
    double V = 0;
    const double rchk = 5.0;
    if (RPT() > 0.0) {
        double per = RPT();
        double minper = GPW() + GPW();
        if (per >= minper) {

            int ifirst, ilast;
            if (t < TD()) {
                ifirst = 0;
                ilast = 0;
            }
            else {
                double del = rchk*GPW();
                ifirst = (int)rint((t - TD() - del)/per);
                ilast = (int)rint((t - TD() + del)/per);
            }
            for (int i = ifirst; i <= ilast; i++) {
                double time = t - i*per;
                double a = (time - TD())/GPW();
                V += exp(-a*a)*(-2*a/GPW());
            }
        }
    }
    else {
        double a = (t - TD())/GPW();
        if (fabs(a) < rchk)
            V += exp(-a*a)*(-2*a/GPW());

        for (int i = 5; i < td_numcoeffs; i++) {
            a = (t - td_coeffs[i])/GPW();
            if (fabs(a) < rchk)
                V += exp(-a*a)*(-2*a/GPW());
        }
    }
    return ((V2() - V1())*V);
}


namespace {
    struct twovals { double x; double y; };
}

double
IFtranData::eval_tPWL(double t)
{
    twovals *tv = (twovals*)td_coeffs;
    if (!tv)
        return (0.0);
    int nvals = td_numcoeffs/2;

    if (t >= tv[nvals-1].x) {
        if (!td_pwlRgiven)
            return (tv[nvals-1].y);
        t -= tv[nvals-1].x;
        double ts = tv[td_pwlRstart].x;
        double dt = tv[nvals-1].x - ts;
        double tx;
        while ((tx = t - dt) > 0.0)
            t = tx;
        t += ts;
        if (t <= tv[td_pwlRstart+1].x)
            return (tv[nvals-1].y + td_cache[nvals-1]*(t - ts));
    }

    int i = td_pwlindex;
    while (i + 1 < nvals) {
        if (t <= tv[i+1].x)
            break;
        i++;
    }
    while (t < tv[i].x && i)
            i--;
    td_pwlindex = i;
    if (!i && t <= tv[0].x)
        return (tv[0].y);
    return (tv[i].y + td_cache[i]*(t - tv[i].x));
}


double
IFtranData::eval_tPWL_D(double t)
{
    twovals *tv = (twovals*)td_coeffs;
    if (!tv)
        return (0.0);
    int nvals = td_numcoeffs/2;

    if (t >= tv[nvals-1].x) {
        if (!td_pwlRgiven)
            return (0.0);
        t -= tv[nvals-1].x;
        double ts = tv[td_pwlRstart].x;
        double dt = tv[nvals-1].x - ts;
        double tx;
        while ((tx = t - dt) > 0.0)
            t = tx;
        t += ts;
        if (t <= tv[td_pwlRstart+1].x)
            return (td_cache[nvals-1]);
    }

    int i = td_pwlindex;
    while (i + 1 < nvals) {
        if (t <= tv[i+1].x)
            break;
        i++;
    }
    while (t < tv[i].x && i)
            i--;
    td_pwlindex = i;
    if (!i && t <= tv[0].x)
        return (0.0);
    return (td_cache[i]);
}


double
IFtranData::eval_tSIN(double t)
{
    double time = t - TDL();
    if (time <= 0) {
        if (PHI() != 0.0)
            return VO() + VA()*sin(M_PI*(PHI()/180));
        return (VO());
    }
    double a;
    if (PHI() != 0.0)
        a = VA()*sin(2*M_PI*(FREQ()*time + (PHI()/360)));
    else
        a = VA()*sin(2*M_PI*FREQ()*time);
    if (THETA() != 0.0)
        a *= exp(-time*THETA());
    return (VO() + a);
}


double
IFtranData::eval_tSIN_D(double t)
{
    double time = t - TDL();
    if (time <= 0)
        return (0);
    double w = FREQ()*2*M_PI;
    if (PHI() != 0.0) {
        double phi = M_PI*(PHI()/180);
        if (THETA() != 0.0) {
            return (VA()*(w*cos(w*time + phi) -
                THETA()*sin(w*time + phi))*exp(-time*THETA()));
        }
        return (VA()*w*cos(w*time + phi));
    }
    if (THETA() != 0.0)
        return (VA()*(w*cos(w*time) - THETA()*sin(w*time))*exp(-time*THETA()));
    return (VA()*w*cos(w*time));
}


double
IFtranData::eval_tSPULSE(double t)
{
    double time = t - SDEL();
    if (time <= 0)
        return (V1());
    if (THETA() != 0.0) {
        return (V1() + (V2()-V1())*( 1 -
            cos(2*M_PI*time/SPER())*exp(-time*THETA()) )/2);
    }
    return (V1() + (V2()-V1())*( 1 - cos(2*M_PI*time/SPER()) )/2);
}


double
IFtranData::eval_tSPULSE_D(double t)
{
    double time = t - SDEL();
    if (time <= 0)
        return (0);
    double w = 2*M_PI/SPER();
    if (THETA() != 0.0) {
        return ((V2()-V1())*(w*sin(w*time) +
            THETA()*cos(w*time))*exp(-time*THETA())/2);
    }
    return ((V2()-V1())*w*sin(w*time)/2);
}


double
IFtranData::eval_tEXP(double t)
{
    double value = V1();
    if (t <= TD1())
        return (value);
    value += (V2()-V1())*(1-exp(-(t-TD1())/TAU1()));
    if (t <= TD2())
        return (value);
    value += (V1()-V2())*(1-exp(-(t-TD2())/TAU2()));
    return (value);
}


double
IFtranData::eval_tEXP_D(double t)
{
    double value = 0.0;
    if (t <= TD1())
        return (value);
    value += (V2()-V1())*exp(-(t-TD1())/TAU1())/TAU1();
    if (t <= TD2())
        return (value);
    value += (V1()-V2())*exp(-(t-TD2())/TAU2())/TAU2();
    return (value);
}


double
IFtranData::eval_tSFFM(double t)
{
    double w1 = 2*M_PI*FC();
    double w2 = 2*M_PI*FS();
    return (VO() + VA()*sin(w1*t + MDI()*sin(w2*t)));
}


double
IFtranData::eval_tAM(double t)
{
    if (t <= DL())
        return (0.0);
    double w = 2*M_PI*(t - DL());
    return (SA()*(OC() + sin(w*MF()))*sin(w*CF()));
}


double
IFtranData::eval_tAM_D(double t)
{
    if (t <= DL())
        return (0.0);
    double w = 2*M_PI*(t - DL());
    double f1 = OC() + sin(w*MF());
    double df1 = 2*M_PI*MF()*cos(w*MF());
    double f2 = sin(w*CF());
    double df2 = 2*M_PI*CF()*cos(w*CF());
    return (SA()*(f1*df2 + f2*df1));
}


double
IFtranData::eval_tSFFM_D(double t)
{
    double w1 = 2*M_PI*FC();
    double w2 = 2*M_PI*FS();
    return (VA()*cos(w1*t + MDI()*sin(w2*t))*(w1 + MDI()*w2*cos(w2*t)));
}


double
IFtranData::eval_tGAUSS(double t)
{
    if (LATTICE() == 0.0) {
        if (t != TIME()) {
            set_VAL(SD()*Rnd.gauss() + MEAN());
            set_TIME(t);
        }
        return (VAL());
    }
    while (t > TIME() + LATTICE()) {
        set_LVAL(VAL());
        set_VAL(NVAL());
        set_NVAL(SD()*Rnd.gauss() + MEAN());
        unsigned int n = (unsigned int)(TIME()/LATTICE() + 0.5) + 1;
        set_TIME(n*LATTICE());
    }
    if (t < TIME()) {
        if (TIME() - t > LATTICE() || ILEVEL() == 0.0)
            return (LVAL());
        return ((VAL() - LVAL())*(t - TIME())/LATTICE() + VAL());
    }
    else {
        if (ILEVEL() == 0.0)
            return (VAL());
        return ((NVAL() - VAL())*(t - TIME())/LATTICE() + VAL());
    }
}


double
IFtranData::eval_tGAUSS_D(double)
{
    return (0.0);
}


double
IFtranData::eval_tINTERP(double t)
{
    double *tvec = td_parms;   // time values
    double *vec = td_coeffs;   // source values

    if (t >= tvec[td_numcoeffs-1])
        return (vec[td_numcoeffs-1]);
    else if (t <= tvec[0])
        return (vec[0]);
    int i = td_pwlindex;
    while (t > tvec[i+1] && i < td_numcoeffs - 2)
        i++;
    while (t < tvec[i] && i > 0)
        i--;
    td_pwlindex = i;
    return (vec[i] + (vec[i+1] - vec[i])*(t - tvec[i])/(tvec[i+1] - tvec[i]));
}


double
IFtranData::eval_tINTERP_D(double t)
{
    double *tvec = td_parms;   // time values
    double *vec = td_coeffs;   // source values

    if (t >= tvec[td_numcoeffs-1])
        return (0);
    else if (t <= tvec[0])
        return (0);
    int i = td_pwlindex;
    while (t > tvec[i+1] && i < td_numcoeffs - 2)
        i++;
    while (t < tvec[i] && i > 0)
        i--;
    td_pwlindex = i;
    return ((vec[i+1] - vec[i])/(tvec[i+1] - tvec[i]));
}


// Other functions.

IFtranData *
IFtranData::dup() const
{
    IFtranData *td = new IFtranData(*this);
    if (td_coeffs) {
        td->td_coeffs = new double[td->td_numcoeffs];
        memcpy(td->td_coeffs, td_coeffs, td->td_numcoeffs*sizeof(double));
    }
    if (td_type == PTF_tINTERP) {
        td->td_parms = new double[td->td_numcoeffs];
        memcpy(td->td_parms, td_parms, td->td_numcoeffs*sizeof(double));
    }
    else if (td_type != PTF_tPWL) {
        td->td_parms = new double[8];
        memcpy(td->td_parms, td_parms, 8*sizeof(double));
    }
    if (td_type == PTF_tPULSE)
        td->td_cache = new double[2];
    else if (td_type == PTF_tPWL) {
        // Make sure to copy the extra location for R.
        int num = td->td_numcoeffs/2;
        td->td_cache = new double[num];
        memcpy(td->td_cache, td_cache, num*sizeof(double));
    }
    return (td);
}


void
IFtranData::time_limit(const sCKT *ckt, double *plim)
{
    if (!ckt || !plim)
        return;
    if (td_type == PTF_tSIN) {
        if (ckt->CKTtime >= TDL()) {
            double per = ckt->CKTcurTask->TSKdphiMax/(2*M_PI*FREQ());
            if (*plim > per)
                *plim = per;
        }
    }
    else if (td_type == PTF_tSPULSE) {
        if (ckt->CKTtime >= SDEL()) {
            double per = ckt->CKTcurTask->TSKdphiMax*SPER()/(2*M_PI);
            if (*plim > per)
                *plim = per;
        }
    }
    else if (td_type == PTF_tEXP) {
        double t = ckt->CKTtime;
        double f = ckt->CKTcurTask->TSKdphiMax/(2*M_PI);
        if (t >= TD1() && t < TD1() + 5.0*TAU1()) {
            double per = TAU1()*f*exp((t - TD1())/TAU1());
            if (*plim > per)
                *plim = per;
        }
        if (t >= TD2() && t < TD2() + 5.0*TAU2()) {
            double per = TAU2()*f*exp((t - TD2())/TAU2());
            if (*plim > per)
                *plim = per;
        }
    }
    else if (td_type == PTF_tSFFM) {
        double per = ckt->CKTcurTask->TSKdphiMax / (2*M_PI*(FC() + FS()));
        if (*plim > per)
            *plim = per;
    }
    else if (td_type == PTF_tAM) {
        double per = ckt->CKTcurTask->TSKdphiMax / (2*M_PI*(CF() + MF()));
        if (*plim > per)
            *plim = per;
    }
}


void
IFtranData::print(const char *name, sLstr &lstr)
{
    lstr.add(name);
    lstr.add_c('(');
    if (td_type == PTF_tPWL) {
        if (td_pwldelay > 0.0) {
            for (int i = 0; i < td_numcoeffs; i++) {
                if (i == 0 && td_coeffs[i] == 0.0) {
                    i++;
                    continue;
                }
                lstr.add_c(' ');
                lstr.add_g(td_coeffs[i] - td_pwldelay);
            }
        }
        else {
            for (int i = 0; i < td_numcoeffs; i++) {
                lstr.add_c(' ');
                lstr.add_g(td_coeffs[i]);
            }
        }
        if (td_pwlRgiven) {
            lstr.add(" R ");
            double r = 0.0;
            if (td_pwlRstart >= 0) {
                r = td_coeffs[2*td_pwlRstart];
                if (r > 0.0 && td_pwldelay > 0.0)
                    r -= td_pwldelay;
            }
            if (r < 0.0)
                r = 0.0;
            lstr.add_g(r);
        }
        if (td_pwldelay > 0.0) {
            lstr.add(" TD ");
            lstr.add_g(td_pwldelay);
        }
    }
    else {
        for (int i = 0; i < td_numcoeffs; i++) {
            lstr.add_c(' ');
            lstr.add_g(td_coeffs[i]);
        }
    }
    lstr.add_c(' ');
    lstr.add_c(')');
}

