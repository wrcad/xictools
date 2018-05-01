
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
 * WRspice Circuit Simulation and Analysis Tool:  Device Library          *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

//-------------------------------------------------------------------------
// This is a general transmission line model derived from:
//  1) the spice3 TRA (lossless) model
//  2) the spice3 LTRA (lossy, convolution) model
//  3) the kspice TXL (lossy, Pade approximation convolution) model
// Authors:
//  1985 Thomas L. Quarles
//  1990 Jaijeet S. Roychowdhury
//  1990 Shen Lin
//  1992 Charles Hough
//  2002 Stephen R. Whiteley
// Copyright Regents of the University of California.  All rights reserved.
//-------------------------------------------------------------------------

#include "tradefs.h"

namespace {
    double intlinfunc(double, double, double, double, double, double);
    double twiceintlinfunc(double, double, double, double, double,
        double, double);
    double thriceintlinfunc(double, double, double, double, double,
        double, double, double);
    double bessI0(double);
    double bessXX(double);
    double bessI1xOverX(double);
    double bessYY(double, double);
    double bessZZ(double, double);
    double ltra_rlcH1dashTwiceIntFunc(double, double);
    double ltra_rlcH3dashIntFunc(double, double, double);
    double ltra_rcH1dashTwiceIntFunc(double, double);
    double ltra_rcH2TwiceIntFunc(double, double);
}


//
// Miscellaneous functions to do with lossy lines.
//


#ifdef notdef
namespace {
    // Returns the complementary error function with fractional error
    // everywhere less that 1.2e-7.  From Numerical Recipes in C.
    //
    double
    erfc(double x)
    {
        double z = fabs(x);
        double t = 1.0/(1.0 + 0.5*x);
        double ans =
            t*exp(-z*z-1.26551223+t*(1.00002368+t*(0.37409196+t*(0.09678418+
            t*(-0.18628806+t*(0.27886807+t*(-1.13520398+t*(1.48851587+
            t*(-0.82215223+t*0.17087277)))))))));
        return (x >= 0 ? ans : 2.0 - ans);
    }
}
#endif


// TRArcCoeffsSetup sets up the all coefficient lists for 
// the special case where L=G=0
//
void
sTRAconvModel::rcCoeffsSetup(sCKT *ckt)
{
    // the first coefficients
#ifdef NEWTL
    sTRAconval *ca = TRAcvdb->head();
    double delta1   = ckt->CKTtime - ca->time;
#else
    int auxindex = ckt->CKTtimeIndex;
    double delta1   = ckt->CKTtime - *(ckt->CKTtimePoints + auxindex);
#endif
    double hilimit1 = delta1;

    double h1lovalue1 = 0.0;
    double h1hivalue1 = sqrt(4*TRAcByR*hilimit1/M_PI);
    double h1dummy1   = h1hivalue1/delta1;
//XXX double h1relval = fabs(h1dummy1 * ckt->CKTcurTask->TSKreltol);
    TRAh1dashFirstCoeff = h1dummy1;

    double temp  = TRArclsqr/(4*hilimit1);
    double temp2 = (temp >= 100.0 ? 0.0 : erfc(sqrt(temp)));
    double temp3 = exp(-temp);
    double temp4 = sqrt(TRArclsqr);
    double temp5 = sqrt(TRAcByR);

    double h2lovalue1 = 0.0;
    double h2hivalue1;
    if (hilimit1 != 0.0)
        h2hivalue1 = (hilimit1 + TRArclsqr*0.5)*temp2 -
            sqrt(hilimit1*TRArclsqr/M_PI)*temp3;
    else
        h2hivalue1 = 0.0;
        
    double h2dummy1 = h2hivalue1/delta1;
//XXX double h2relval = fabs(h2dummy1 * ckt->CKTcurTask->TSKreltol);
    TRAh2FirstCoeff = h2dummy1;

    double h3lovalue1 = 0.0;
    double h3hivalue1;
    if (hilimit1 != 0.0) {
        temp = 2*sqrt(hilimit1/M_PI)*temp3 - temp4*temp2;
        h3hivalue1 = temp5*temp;
    }
    else
        h3hivalue1 = 0.0;

    double h3dummy1 = h3hivalue1/delta1;
    TRAh3dashFirstCoeff = h3dummy1;
//XXX double h3relval = fabs(h3dummy1 * ckt->CKTcurTask->TSKreltol);

    // the coefficients for the rest of the timepoints

    int doh1 = 1, doh2 = 1, doh3 = 1;
#ifdef NEWTL
    for (sTRAconval *cv = ca; cv->prev; cv = cv->prev) {
#else
    for (int i = auxindex; i > 0; i--) {
#endif

#ifdef NEWTL
        delta1   = cv->time - cv->prev->time;
        hilimit1 = ckt->CKTtime - cv->prev->time;
#else
        delta1   = *(ckt->CKTtimePoints + i) - *(ckt->CKTtimePoints + i - 1);
        hilimit1 = ckt->CKTtime - *(ckt->CKTtimePoints + i - 1);
#endif


        if (doh1) {
            double h1hivalue2 = h1hivalue1; // previous hivalue1
            double h1dummy2   = h1dummy1;   // previous dummy1

            h1lovalue1 = h1hivalue2;
            h1hivalue1 = sqrt(4*TRAcByR*hilimit1/M_PI);
            h1dummy1 = (h1hivalue1 - h1lovalue1)/delta1;
#ifdef NEWTL
            cv->h1dashCoeff = h1dummy1 - h1dummy2;
#else
            *(TRAh1dashCoeffs + i) = h1dummy1 - h1dummy2;
#endif
/*XXX
            if (fabs(h1dummy1 - h1dummy2) < h1relval)
                doh1 = 0;
*/
        }
        else
#ifdef NEWTL
            cv->h1dashCoeff = 0.0;
#else
            *(TRAh1dashCoeffs + i) = 0.0;
#endif

        if (doh2 || doh3) {
            temp  = TRArclsqr/(4*hilimit1);
            temp2 = (temp >= 100.0 ? 0.0 : erfc(sqrt(temp)));
            temp3 = exp(-temp);
        }

        if (doh2) {
            double h2hivalue2 = h2hivalue1; // previous hivalue1
            double h2dummy2   = h2dummy1;   // previous dummy1

            h2lovalue1 = h2hivalue2;
            if (hilimit1 != 0.0)
                h2hivalue1 = (hilimit1 + TRArclsqr*0.5)*temp2 -
                    sqrt(hilimit1*TRArclsqr/M_PI)*temp3;
            else
                h2hivalue1 = 0.0;
            h2dummy1 = (h2hivalue1 - h2lovalue1)/delta1;
#ifdef NEWTL
            cv->h2Coeff = h2dummy1 - h2dummy2;
#else
            *(TRAh2Coeffs + i) = h2dummy1 - h2dummy2;
#endif
/*XXX
            if (fabs(h2dummy1 - h2dummy2) < h2relval)
                doh2 = 0;
*/
        }
        else
#ifdef NEWTL
            cv->h2Coeff = 0.0;
#else
            *(TRAh2Coeffs + i) = 0.0;
#endif

        if (doh3) {
            double h3hivalue2 = h3hivalue1; // previous hivalue1
            double h3dummy2   = h3dummy1;   // previous dummy1

            h3lovalue1 = h3hivalue2;
            if (hilimit1 != 0.0) {
                temp = 2*sqrt(hilimit1/M_PI)*temp3 - temp4*temp2;
                h3hivalue1 = temp5*temp;
            }
            else
                h3hivalue1 = 0.0;
            h3dummy1 = (h3hivalue1 - h3lovalue1)/delta1;
#ifdef NEWTL
            cv->h3dashCoeff = h3dummy1 - h3dummy2;
#else
            *(TRAh3dashCoeffs + i) = h3dummy1 - h3dummy2;
#endif
/*XXX
            if (fabs(h3dummy1 - h3dummy2) < h3relval)
                doh3 = 0;
*/
        }
        else
#ifdef NEWTL
            cv->h3dashCoeff = 0.0;
#else
            *(TRAh3dashCoeffs + i) = 0.0;
#endif
    }
}


void
sTRAconvModel::rlcCoeffsSetup(sCKT *ckt)
{
    // we assume a piecewise linear function, and we calculate the
    // coefficients using this assumption in the integration of the
    // function

#ifdef NEWTL
    sTRAconval *ca = 0;
#else
    int auxindex;
#endif
    if (TRAtd == 0.0) {
#ifdef NEWTL
        ca = TRAcvdb->head();
#else
        auxindex = ckt->CKTtimeIndex;
#endif
    }
    else {

        if (ckt->CKTtime - TRAtd <= 0.0) {
#ifdef NEWTL
            ca = TRAcvdb->tail();
#else
            auxindex = 0;
#endif
        }
        else {
#ifdef NEWTL
            double tt = ckt->CKTtime - TRAtd;
            for (ca = TRAcvdb->tail(); ca->next; ca = ca->next) {
                if (ca->next->time > tt)
                    break;
            }

#else
            int i, exact = 0;
            for (i = ckt->CKTtimeIndex; i >= 0; i--) {
                if (ckt->CKTtime - *(ckt->CKTtimePoints + i) ==
                        TRAtd) {
                    exact = 1;
                    break;
                } 
                if (ckt->CKTtime - *(ckt->CKTtimePoints + i) >
                        TRAtd)
                    break;
            }

#ifdef TRADEBUG
            if ((i < 0) || ((i == 0) && (exact == 1)))
                printf("TRAcoeffSetup: i <= 0: some mistake!\n");
#endif

            if (!exact)
                auxindex = i;
            else
                auxindex = i-1;
#endif
        }
    }
    // the first coefficient

    double alphasqTterm;
    double expbetaTterm;
    double h3dummy1;
    double h2dummy1;
    double h2lovalue1;
    double h2hivalue1;
    double h3hivalue1;
#ifdef NEWTL
    if (ca && ca->next) {
#else
    if (auxindex != 0) {
#endif
        double lolimit1 = TRAtd;
#ifdef NEWTL
        double hilimit1 = ckt->CKTtime - ca->time;
#else
        double hilimit1 = ckt->CKTtime - *(ckt->CKTtimePoints + auxindex);
#endif
        double delta1   = hilimit1 - lolimit1;

        h2lovalue1 = rlcH2Func(TRAtd);
        double besselarg  = (hilimit1 > TRAtd) ?
            TRAalpha*sqrt(hilimit1*hilimit1 - TRAtd*TRAtd) : 0.0;
        double exparg  = -TRAbeta*hilimit1;
        double bessi1overxterm = bessYY(besselarg, exparg);

        alphasqTterm = TRAalpha*TRAalpha*TRAtd;
        h2hivalue1 =
            ((TRAalpha == 0.0) || (hilimit1 < TRAtd)) ? 0.0:
            alphasqTterm*bessi1overxterm;

        h2dummy1 = twiceintlinfunc(lolimit1, hilimit1, lolimit1, h2lovalue1,
            h2hivalue1, lolimit1, hilimit1)/delta1;
//XXX double h2relval = fabs(h2dummy1 * ckt->CKTcurTask->TSKreltol);
        TRAh2FirstCoeff = h2dummy1;

        double h3lovalue1 = 0.0; // E3dash should be consistent with this
        double bessi0term = bessZZ(besselarg, exparg);

        expbetaTterm = exp(-TRAbeta*TRAtd);
        h3hivalue1 =
            ((hilimit1 <= TRAtd) || (TRAbeta == 0.0)) ? 0.0:
            bessi0term - expbetaTterm;

        h3dummy1 = intlinfunc(lolimit1, hilimit1, h3lovalue1,
            h3hivalue1, lolimit1, hilimit1)/delta1;
//XXX double h3relval = fabs(h3dummy1 * ckt->CKTcurTask->TSKreltol);
        TRAh3dashFirstCoeff = h3dummy1;
    }
    else {
        TRAh2FirstCoeff = TRAh3dashFirstCoeff = 0.0;
        // really don't need to initialize
        alphasqTterm = 0.0;
        expbetaTterm = 0.0;
        h3dummy1 = 0.0;
        h2dummy1 = 0.0;
        h2lovalue1 = 0.0;
        h2hivalue1 = 0.0;
        h3hivalue1 = 0.0;
    }

    double lolimit2 = 0.0;
    double hilimit2 = 0.0;
    double lolimit1 = 0.0;
#ifdef NEWTL
    double hilimit1 = ckt->CKTtime - TRAcvdb->head()->time;
#else
    double hilimit1 = ckt->CKTtime - *(ckt->CKTtimePoints + ckt->CKTtimeIndex);
#endif
    double delta1 = hilimit1 - lolimit1;
    double exparg = -TRAbeta*hilimit1;
    double h1lovalue1 = 0.0;
    double h1hivalue1 = (TRAbeta == 0.0)? hilimit1:
        ((hilimit1 == 0.0) ? 0.0 : bessXX(-exparg)*hilimit1 - hilimit1);
    double h1dummy1 = h1hivalue1/delta1;
//XXX double h1relval = fabs(h1dummy1 * ckt->CKTcurTask->TSKreltol);
    TRAh1dashFirstCoeff = h1dummy1;

    // the coefficients for the rest of the timepoints

    int doh1 = 1, doh2 = 1, doh3 = 1;
#ifdef NEWTL
    for (sTRAconval *cv = TRAcvdb->head(); cv->prev; cv = cv->prev) { 
#else
    for (int i = ckt->CKTtimeIndex; i > 0; i--) {
#endif

        if (doh1 || doh2 || doh3) {
            lolimit2 = lolimit1; // previous lolimit1
            hilimit2 = hilimit1; // previous hilimit1

            lolimit1 = hilimit2;
#ifdef NEWTL
            hilimit1 = ckt->CKTtime - cv->prev->time;
            delta1   = cv->time - cv->prev->time;
#else
            hilimit1 = ckt->CKTtime - *(ckt->CKTtimePoints + i - 1);
            delta1   =
                *(ckt->CKTtimePoints + i) - *(ckt->CKTtimePoints + i - 1);
#endif

            exparg = -TRAbeta*hilimit1;
        }

        if (doh1) {
            double h1hivalue2 = h1hivalue1; // previous hivalue1
            double h1dummy2   = h1dummy1;   // previous dummy1

            h1lovalue1 = h1hivalue2;
            h1hivalue1 = (TRAbeta == 0.0)? hilimit1:
                ((hilimit1 == 0.0) ? 0.0:
                bessXX(-exparg)*hilimit1 - hilimit1);
            h1dummy1 = (h1hivalue1 - h1lovalue1)/delta1;

#ifdef NEWTL
            cv->h1dashCoeff = h1dummy1 - h1dummy2;
#else
            *(TRAh1dashCoeffs + i) = h1dummy1 - h1dummy2;
#endif
/*XXX
            if (h1dummy1 - h1dummy2) <= h1relval)
                doh1 = 0;
*/
        }
        else
#ifdef NEWTL
            cv->h1dashCoeff = 0.0;
#else
            *(TRAh1dashCoeffs + i) = 0.0;
#endif

#ifdef NEWTL
        if (cv->time <= ca->time) {
#else
        if (i <= auxindex) {
#endif
    
            double besselarg = 0;
            if (doh2 || doh3)
                besselarg = (hilimit1 > TRAtd) ?
                    TRAalpha*sqrt(hilimit1*hilimit1 - TRAtd*TRAtd) : 0.0;

            if (doh2) {
                double h2lovalue2 = h2lovalue1; // previous lovalue1
                double h2hivalue2 = h2hivalue1; // previous hivalue1
                double h2dummy2   = h2dummy1;   // previous dummy1

                h2lovalue1 = h2hivalue2;
                double bessi1overxterm = bessYY(besselarg, exparg);

                h2hivalue1 = ((TRAalpha == 0.0) || (hilimit1 < TRAtd)) ?
                    0.0 : alphasqTterm*bessi1overxterm;

                h2dummy1 = twiceintlinfunc(lolimit1, hilimit1, lolimit1,
                    h2lovalue1, h2hivalue1, lolimit1, hilimit1)/delta1;

#ifdef NEWTL
                cv->h2Coeff = h2dummy1 - h2dummy2 + intlinfunc(lolimit2,
                    hilimit2, h2lovalue2, h2hivalue2, lolimit2, hilimit2);
#else
                *(TRAh2Coeffs + i) = h2dummy1 - h2dummy2 +
                    intlinfunc(lolimit2, hilimit2,
                        h2lovalue2, h2hivalue2, lolimit2, hilimit2);
#endif
/*XXX
                if (*(TRAh2Corffs + i) <= h2relval)
                    doh2 = 0;
*/
            }
            else
#ifdef NEWTL
                cv->h2Coeff = 0.0;
#else
                *(TRAh2Coeffs + i) = 0.0;
#endif

            if (doh3) {
                double h3hivalue2 = h3hivalue1; // previous hivalue1
                double h3dummy2   = h3dummy1;   // previous dummy1
    
                double h3lovalue1 = h3hivalue2;
                double bessi0term = bessZZ(besselarg, exparg);

                h3hivalue1 = ((hilimit1 <= TRAtd) || (TRAbeta == 0.0)) ?
                    0.0 : bessi0term - expbetaTterm;

                h3dummy1 = intlinfunc(lolimit1, hilimit1, h3lovalue1,
                    h3hivalue1, lolimit1, hilimit1)/delta1;

#ifdef NEWTL
                cv->h3dashCoeff = h3dummy1 - h3dummy2;
#else
                *(TRAh3dashCoeffs + i) = h3dummy1 - h3dummy2;
#endif
/*XXX
                if (h3dummy1 - h3dummy2) <= h3relval)
                    doh3 = 0;
*/
            }
            else
#ifdef NEWTL
                cv->h3dashCoeff = 0.0;
#else
                *(TRAh3dashCoeffs + i) = 0.0;
#endif
        } 
    }
#ifdef NEWTL
#else
    TRAauxIndex = auxindex;
#endif
}


double
sTRAconvModel::rlcH2Func(double time)
{
    //
    // result = 0, time < T
    //        = (alpha*T*e^{-beta*time})/sqrt(t^2 - T^2) *
    //                           I_1(alpha*sqrt(t^2 - T^2))
    //
    if (TRAalpha == 0.0 || time < TRAtd)
        return (0.0);
    double besselarg;
    if (time != TRAtd)
        besselarg = TRAalpha*sqrt(time*time - TRAtd*TRAtd); 
    else
        besselarg = 0.0;
    double exparg = -TRAbeta*time;
    double returnval = TRAalpha*TRAalpha*TRAtd*
        exp(exparg)*bessI1xOverX(besselarg);
    return (returnval);
}


double
sTRAconvModel::rlcH3dashFunc(double time, double T, double alpha, double beta)
{
    //
    // result = 0, time < T
    //        = alpha*e^{-beta*time}*(t/sqrt(t^2-T^2)*
    //             I_1(alpha*sqrt(t^2-T^2)) - I_0(alpha*sqrt(t^2-T^2)))
    //
    if (alpha == 0.0 || time < T)
        return (0.0);
    double besselarg, exparg = - beta*time;
    if (time != T)
        besselarg = alpha*sqrt(time*time - T*T); 
    else
        besselarg = 0.0;
    double returnval = alpha*time*bessI1xOverX(besselarg) - bessI0(besselarg);
    returnval *= alpha*exp(exparg);
    return (returnval);
}


// i is the index of the latest value, 
// a,b,c values correspond to values at t_{i-2}, t{i-1} and t_i
//

#define SECONDDERIV(i,a,b,c) (oof = (i==ckt->CKTtimeIndex+1?curtime:\
*(ckt->CKTtimePoints+i)),\
(( c - b )/(oof-*(ckt->CKTtimePoints+i-1)) -\
( b - a )/(*(ckt->CKTtimePoints+i-1)-\
*(ckt->CKTtimePoints+i-2)))/(oof - \
*(ckt->CKTtimePoints+i-2)))

#ifdef NEWTL
#define SECONDDERIV1(a,b,c) (oof = curtime),\
(( c - b )/(oof-*(ckt->CKTtimePoints+i-1)) -\
( b - a )/(*(ckt->CKTtimePoints+i-1)-\
*(ckt->CKTtimePoints+i-2)))/(oof - \
*(ckt->CKTtimePoints+i-2)))

#define SECONDDERIV2(t3,t2,t1,a,b,c) (((c-b)/(t1-t2) - (b-a)/(t2-t3))/(t1-t3))
#endif

// TRAlteCalculate - returns sum of the absolute values of the total
// local truncation error of the 2 equations for the TRAline
// Call before present time point is saved in CKTtimePoints.
//

double
sTRAconvModel::lteCalculate(sCKT *ckt, sTRAinstance *inst, double curtime)
{
    double eq1LTE = 0.0, eq2LTE = 0.0;
    if (inst->TRAcase == TRA_LC || inst->TRAcase == TRA_RG)
        return (0.0);

    else if (inst->TRAcase == TRA_RLC) {

        double hilimit1 = curtime - *(ckt->CKTtimePoints + ckt->CKTtimeIndex);
        double lolimit1 = 0.0;
        double hivalue1 =
            ltra_rlcH1dashTwiceIntFunc(hilimit1, TRAbeta);
        double lovalue1 = 0.0;

        double f1i = hivalue1;
        double g1i = intlinfunc(lolimit1, hilimit1, lovalue1, hivalue1,
            lolimit1, hilimit1);
        double h1dashTfirstCoeff = 0.5 * f1i *
            (curtime - *(ckt->CKTtimePoints+ckt->CKTtimeIndex)) - g1i;

        if (curtime > TRAtd) {
#ifdef NEWTL
            sTRAtimeval *ta = inst->TRAtvdb->tail();
            double atime = curtime - TRAtd;
            while (ta && ta->time <= atime) {
                if (ta->time == atime)
                    break;
                if (ta->next) {
                    if (ta->next->time > atime)
                        break;
                }
                ta = ta->next;
            }
#else
            int i, exact = 0;
            for (i = ckt->CKTtimeIndex ; i >= 0; i--) {
                if (curtime - *(ckt->CKTtimePoints + i) == TRAtd) {
                    exact = 1;
                    break;
                } 
                if (curtime - *(ckt->CKTtimePoints + i) > TRAtd)
                    break;
            }
#endif

#ifdef TRADEBUG
            if ((i < 0) || ((i==0) && (exact==1)))
            printf("TRAlteCalculate: i <= 0: some mistake!\n");
#endif

#ifdef NEWTL
            hilimit1 = curtime - ta->time;
            lolimit1 = inst->TRAtvdb->head()->time - ta->time;
#else
            int auxindex;
            if (!exact)
                auxindex = i;
            else
                auxindex = i-1;

            hilimit1 = curtime - *(ckt->CKTtimePoints + auxindex);
            lolimit1 = *(ckt->CKTtimePoints + ckt->CKTtimeIndex) -
                *(ckt->CKTtimePoints + auxindex);
#endif
            lolimit1 = SPMAX(TRAtd, lolimit1);

            // are the following really doing the operations in the
            //  write-up?
            hivalue1 = rlcH2Func(hilimit1);
            lovalue1 = rlcH2Func(lolimit1);
            f1i = twiceintlinfunc(lolimit1, hilimit1, lolimit1, lovalue1,
                hivalue1, lolimit1, hilimit1);
            g1i = thriceintlinfunc(lolimit1, hilimit1, lolimit1, lolimit1,
                lovalue1, hivalue1, lolimit1, hilimit1);
#ifdef NEWTL
            double h2TfirstCoeff = 0.5*f1i*(curtime-TRAtd -
                ta->time) - g1i;
#else
            double h2TfirstCoeff = 0.5*f1i*(curtime-TRAtd -
                *(ckt->CKTtimePoints+auxindex)) - g1i;
#endif

            hivalue1 = ltra_rlcH3dashIntFunc(hilimit1, TRAtd, TRAbeta);
            lovalue1 = ltra_rlcH3dashIntFunc(lolimit1, TRAtd, TRAbeta);
            f1i = intlinfunc(lolimit1, hilimit1, lovalue1, hivalue1,
                lolimit1, hilimit1);
            g1i = twiceintlinfunc(lolimit1, hilimit1, lolimit1, lovalue1,
                hivalue1, lolimit1, hilimit1);
#ifdef NEWTL
            double h3dashTfirstCoeff = 0.5*f1i*(curtime - TRAtd -
                ta->time) - g1i;
#else
            double h3dashTfirstCoeff = 0.5*f1i*(curtime - TRAtd -
                *(ckt->CKTtimePoints+auxindex)) - g1i;
#endif

            // LTEs for convolution with v1
            // Get divided differences for v1 (2nd derivative estimates).
            // No need to subtract operating point values because
            // taking differences.
            // Not bothering to interpolate since everything is
            // approximate
            //
#ifdef NEWTL
            sTRAtimeval *tv = inst->TRAtvdb->head();
            double dashdash = SECONDDERIV2(tv->prev->time, tv->time, curtime,
                tv->prev->v_i, tv->v_i,
                *(ckt->CKTrhsOld + inst->TRAposNode1) -
                *(ckt->CKTrhsOld + inst->TRAnegNode1));
#else
            double oof; // in macro
            double dashdash = SECONDDERIV(ckt->CKTtimeIndex+1,
                inst->TRAvalues[ckt->CKTtimeIndex-1].v_i,
                inst->TRAvalues[ckt->CKTtimeIndex].v_i,
                *(ckt->CKTrhsOld + inst->TRAposNode1) -
                *(ckt->CKTrhsOld + inst->TRAnegNode1));
#endif
            eq1LTE += TRAadmit*FABS(dashdash*h1dashTfirstCoeff);
#ifdef NEWTL
            if (ta) {
                dashdash = SECONDDERIV2(ta->prev->time, ta->time,
                    ta->next->time, ta->prev->v_i, ta->v_i, ta->next->v_i);
#else
            if (auxindex) {
                dashdash = SECONDDERIV(auxindex+1,
                    inst->TRAvalues[auxindex - 1].v_i,
                    inst->TRAvalues[auxindex].v_i,
                    inst->TRAvalues[auxindex + 1].v_i);
#endif
                eq2LTE += TRAadmit*FABS(dashdash*h3dashTfirstCoeff);
            }
            // end LTEs for convolution with v1

            // LTEs for convolution with v2
            // Get divided differences for v2 (2nd derivative estimates).
            //
#ifdef NEWTL
            dashdash = SECONDDERIV2(tv->prev->time, tv->time, curtime,
                tv->prev->v_o, tv->v_o,
                *(ckt->CKTrhsOld + inst->TRAposNode2) -
                *(ckt->CKTrhsOld + inst->TRAnegNode2));
#else
            dashdash = SECONDDERIV(ckt->CKTtimeIndex+1,
                inst->TRAvalues[ckt->CKTtimeIndex-1].v_o,
                inst->TRAvalues[ckt->CKTtimeIndex].v_o,
                *(ckt->CKTrhsOld + inst->TRAposNode2) -
                *(ckt->CKTrhsOld + inst->TRAnegNode2));
#endif
            eq2LTE += TRAadmit*FABS(dashdash*h1dashTfirstCoeff);
#ifdef NEWTL
            if (ta) {
                dashdash = SECONDDERIV2(ta->prev->time, ta->time,
                    ta->next->time, ta->prev->v_o, ta->v_o, ta->next->v_o);
#else
            if (auxindex) {
                dashdash = SECONDDERIV(auxindex+1,
                    inst->TRAvalues[auxindex - 1].v_o,
                    inst->TRAvalues[auxindex].v_o,
                    inst->TRAvalues[auxindex + 1].v_o) ;
#endif
                eq1LTE += TRAadmit*FABS(dashdash*h3dashTfirstCoeff);
            }
            // end LTEs for convolution with v2

#ifdef NEWTL
            if (ta) {
#else
            if (auxindex) {
#endif
                // LTE for convolution with i1
                // Get divided differences for i1 (2nd deriv estimates).
                //
#ifdef NEWTL
                dashdash = SECONDDERIV2(ta->prev->time, ta->time,
                    ta->next->time, ta->prev->i_i, ta->i_i, ta->next->i_i);
#else
                dashdash = SECONDDERIV(auxindex+1,
                    inst->TRAvalues[auxindex - 1].i_i,
                    inst->TRAvalues[auxindex].i_i,
                    inst->TRAvalues[auxindex + 1].i_i) ;
#endif
                eq2LTE += FABS(dashdash * h2TfirstCoeff);
                // end LTE for convolution with i1

                // LTE for convolution with i2
                // Get divided differences for i2 (2nd deriv estimates).
                //
#ifdef NEWTL
                dashdash = SECONDDERIV2(ta->prev->time, ta->time,
                    ta->next->time, ta->prev->i_o, ta->i_o, ta->next->i_o);
#else
                dashdash = SECONDDERIV(auxindex+1,
                    inst->TRAvalues[auxindex - 1].i_o,
                    inst->TRAvalues[auxindex].i_o,
                    inst->TRAvalues[auxindex + 1].i_o) ;
#endif
                eq1LTE += FABS(dashdash * h2TfirstCoeff);
            }
            // end LTE for convolution with i1
        }
        else {
            // LTEs for convolution with v1
#ifdef NEWTL
            sTRAtimeval *tv = inst->TRAtvdb->head();
            double dashdash = SECONDDERIV2(tv->prev->time, tv->time, curtime,
                tv->prev->v_i, tv->v_i,
                *(ckt->CKTrhsOld + inst->TRAposNode1) -
                *(ckt->CKTrhsOld + inst->TRAnegNode1));
#else
            double oof;  // n macro
            double dashdash = SECONDDERIV(ckt->CKTtimeIndex+1,
                inst->TRAvalues[ckt->CKTtimeIndex-1].v_i,
                inst->TRAvalues[ckt->CKTtimeIndex].v_i,
                *(ckt->CKTrhsOld + inst->TRAposNode1) -
                *(ckt->CKTrhsOld + inst->TRAnegNode1));
#endif
            eq1LTE += TRAadmit*FABS(dashdash*h1dashTfirstCoeff);

            // LTEs for convolution with v2
#ifdef NEWTL
            dashdash = SECONDDERIV2(tv->prev->time, tv->time, curtime,
                tv->prev->v_o, tv->v_o,
                *(ckt->CKTrhsOld + inst->TRAposNode2) -
                *(ckt->CKTrhsOld + inst->TRAnegNode2));
#else
            dashdash = SECONDDERIV(ckt->CKTtimeIndex+1,
                inst->TRAvalues[ckt->CKTtimeIndex-1].v_o,
                inst->TRAvalues[ckt->CKTtimeIndex].v_o,
                *(ckt->CKTrhsOld + inst->TRAposNode2) -
                *(ckt->CKTrhsOld + inst->TRAnegNode2));
#endif
            eq2LTE += TRAadmit*FABS(dashdash*h1dashTfirstCoeff);
        }
    }
    else if (inst->TRAcase == TRA_RC) {

        double hilimit1 = curtime - *(ckt->CKTtimePoints + ckt->CKTtimeIndex);
        double lolimit1 = 0.0;

        double hivalue1 = ltra_rcH1dashTwiceIntFunc(hilimit1, TRAcByR);
        double lovalue1 = 0.0;

        double f1i = hivalue1;
        double g1i = intlinfunc(lolimit1, hilimit1, lovalue1, hivalue1,
            lolimit1, hilimit1);
#ifdef NEWTL
        double h1dashTfirstCoeff = 0.5*f1i*(curtime -
            inst->TRAtvdb->head()->time) - g1i;
#else
        double h1dashTfirstCoeff = 0.5*f1i*(curtime -
            *(ckt->CKTtimePoints+ckt->CKTtimeIndex)) - g1i;
#endif

        hivalue1 = ltra_rcH2TwiceIntFunc(hilimit1, TRArclsqr);
        lovalue1 = 0.0;

        f1i = hivalue1;
        g1i = intlinfunc(lolimit1, hilimit1, lovalue1, hivalue1, lolimit1,
            hilimit1);

// SRW - looks like a bug below, fixed (comments are original code)
/*
        h1dashTfirstCoeff = 0.5*f1i*(curtime -
            *(ckt->CKTtimePoints+ckt->CKTtimeIndex)) - g1i;
*/
#ifdef NEWTL
        double h2TfirstCoeff = 0.5*f1i*(curtime -
            inst->TRAtvdb->head()->time) - g1i;
#else
        double h2TfirstCoeff = 0.5*f1i*(curtime -
            *(ckt->CKTtimePoints+ckt->CKTtimeIndex)) - g1i;
#endif

        hivalue1 = ltra_rcH2TwiceIntFunc(hilimit1, TRArclsqr);
        lovalue1 = 0.0;

        f1i = hivalue1;
        g1i = intlinfunc(lolimit1, hilimit1, lovalue1,
            hivalue1, lolimit1, hilimit1);
/*
        h1dashTfirstCoeff = 0.5*f1i*(curtime -
            *(ckt->CKTtimePoints+ckt->CKTtimeIndex)) - g1i;
*/
#ifdef NEWTL
        double h3dashTfirstCoeff = 0.5*f1i*(curtime -
            inst->TRAtvdb->head()->time) - g1i;
#else
        double h3dashTfirstCoeff = 0.5*f1i*(curtime -
            *(ckt->CKTtimePoints+ckt->CKTtimeIndex)) - g1i;
#endif

        // LTEs for convolution with v1
        // get divided differences for v1 (2nd derivative estimates)

        // no need to subtract operating point values because
        // taking differences anyway

#ifdef NEWTL
        sTRAtimeval *tv = inst->TRAtvdb->head();
        double dashdash = SECONDDERIV2(tv->prev->time, tv->time, curtime,
            tv->prev->v_i, tv->v_i,
            *(ckt->CKTrhsOld + inst->TRAposNode1) -
            *(ckt->CKTrhsOld + inst->TRAnegNode1));
#else
        double oof; // in macro
        double dashdash = SECONDDERIV(ckt->CKTtimeIndex+1,
            inst->TRAvalues[ckt->CKTtimeIndex-1].v_i,
            inst->TRAvalues[ckt->CKTtimeIndex].v_i,
            *(ckt->CKTrhsOld + inst->TRAposNode1) -
            *(ckt->CKTrhsOld + inst->TRAnegNode1));
#endif
        eq1LTE += FABS(dashdash * h1dashTfirstCoeff);
        eq2LTE += FABS(dashdash * h3dashTfirstCoeff);
        
        // end LTEs for convolution with v1

        // LTEs for convolution with v2
        // get divided differences for v2 (2nd derivative estimates)

#ifdef NEWTL
        dashdash = SECONDDERIV2(tv->prev->time, tv->time, curtime,
            tv->prev->v_o, tv->v_o,
            *(ckt->CKTrhsOld + inst->TRAposNode2) -
            *(ckt->CKTrhsOld + inst->TRAnegNode2));
#else
        dashdash = SECONDDERIV(ckt->CKTtimeIndex+1,
            inst->TRAvalues[ckt->CKTtimeIndex-1].v_o,
            inst->TRAvalues[ckt->CKTtimeIndex].v_o,
            *(ckt->CKTrhsOld + inst->TRAposNode2) -
            *(ckt->CKTrhsOld + inst->TRAnegNode2));
#endif

        eq2LTE += FABS(dashdash * h1dashTfirstCoeff);
        eq1LTE += FABS(dashdash * h3dashTfirstCoeff);
        
        // end LTEs for convolution with v2

        // LTE for convolution with i1
        // get divided differences for i1 (2nd derivative estimates)

#ifdef NEWTL
        dashdash = SECONDDERIV2(tv->prev->time, tv->time, curtime,
            tv->prev->i_i, tv->i_i,
            *(ckt->CKTrhsOld + inst->TRAbrEq1));
#else
        dashdash = SECONDDERIV(ckt->CKTtimeIndex+1,
            inst->TRAvalues[ckt->CKTtimeIndex - 1].i_i,
            inst->TRAvalues[ckt->CKTtimeIndex].i_i,
            *(ckt->CKTrhsOld + inst->TRAbrEq1));
#endif

        eq2LTE += FABS(dashdash * h2TfirstCoeff);

        // end LTE for convolution with i1

        // LTE for convolution with i2
        // get divided differences for i2 (2nd derivative estimates)

#ifdef NEWTL
        dashdash = SECONDDERIV2(tv->prev->time, tv->time, curtime,
            tv->prev->i_o, tv->i_o,
            *(ckt->CKTrhsOld + inst->TRAbrEq2));
#else
        dashdash = SECONDDERIV(ckt->CKTtimeIndex+1,
            inst->TRAvalues[ckt->CKTtimeIndex - 1].i_o,
            inst->TRAvalues[ckt->CKTtimeIndex].i_o,
            *(ckt->CKTrhsOld + inst->TRAbrEq2));
#endif

        eq1LTE += FABS(dashdash * h2TfirstCoeff);
        
        // end LTE for convolution with i1

    }
    else
        return (1);

#ifdef TRADEBUG
    fprintf(stdout, "%s: LTE/input for Eq1 at time %g is: %g\n",
        inst->TRAname, curtime, eq1LTE/inst->TRAinput1);
    fprintf(stdout,"%s: LTE/input for Eq2 at time %g is: %g\n",
        inst->TRAname, curtime, eq2LTE/inst->TRAinput2);
    fprintf(stdout,"\n");
#endif

    return (FABS(eq1LTE) + FABS(eq2LTE));
}


namespace {
    // intlinfunc returns int lolimit-hilimit h(tau) dtau, where
    // h(tau) is assumed to be linear, with values lovalue and hivalue
    // tau = t1 and t2 respectively
    // this is used only locally
    //
    double
    intlinfunc(double lolimit, double hilimit, double lovalue, double hivalue,
        double t1, double t2)
    {
        double width = t2 - t1;
        if (width == 0.0)
            return (0.0);
        double m = (hivalue - lovalue)/width;

        return ((hilimit-lolimit)*lovalue + 0.5*m*((hilimit-t1)*(hilimit-t1)
            - (lolimit - t1)*(lolimit - t1)));
    }


    // twiceintlinfunc returns int lolimit-hilimit int otherlolimit-tau 
    // h(tau') d tau' d tau , where
    // h(tau') is assumed to be linear, with values lovalue and hivalue
    // tau = t1 and t2 respectively
    // this is used only locally
    //
    double
    twiceintlinfunc(double lolimit, double hilimit, double otherlolimit,
        double lovalue, double hivalue, double t1, double t2)
    {
        double width = t2 - t1;
        if (width == 0.0)
            return (0.0);
        double m = (hivalue - lovalue)/width;

        double temp1 = hilimit - t1;
        double temp2 = lolimit - t1;
        double temp3 = otherlolimit - t1;
        double dummy =
            lovalue*((hilimit - otherlolimit)*(hilimit - otherlolimit) -
            (lolimit - otherlolimit)*(lolimit - otherlolimit));
        dummy += m*((temp1*temp1*temp1 - temp2*temp2*temp2)/3.0 -
            temp3*temp3*(hilimit - lolimit));
        return(dummy*0.5);
    }


    // thriceintlinfunc returns int lolimit-hilimit int secondlolimit-tau
    // int thirdlolimit-tau' h(tau'') d tau'' d tau' d tau , where
    // h(tau'') is assumed to be linear, with values lovalue and hivalue
    // tau = t1 and t2 respectively
    // this is used only locally
    //
    double
    thriceintlinfunc(double lolimit, double hilimit, double secondlolimit,
        double thirdlolimit, double lovalue, double hivalue, double t1,
        double t2)
    {
        double width = t2 - t1;
        if (width == 0.0)
            return (0.0);
        double m = (hivalue - lovalue)/width;

        double temp1 = hilimit - t1;
        double temp2 = lolimit - t1;
        double temp3 = secondlolimit - t1;
        double temp4 = thirdlolimit - t1;
        double temp5 = hilimit - thirdlolimit;
        double temp6 = lolimit - thirdlolimit;
        double temp7 = secondlolimit - thirdlolimit;
        double temp8 = hilimit - lolimit;
        double temp9 = hilimit - secondlolimit;
        double temp10 = lolimit - secondlolimit;
        double dummy = lovalue*((temp5*temp5*temp5 - temp6*temp6*temp6)/3 -
            temp7*temp5*temp8);
        dummy += m*(((temp1*temp1*temp1*temp1 - temp2*temp2*temp2*temp2)*0.25 -
            temp3*temp3*temp3*temp8)/3 - temp4*temp4*0.5*(temp9*temp9 -
            temp10*temp10));
        return (dummy*0.5);
    }

    
    // These are from the book Numerical Recipes in C
    //

    double
    bessI0(double x)
    {
        double ans, ax = fabs(x);
        if (ax < 3.75) {
            double y = x/3.75;
            y *= y;
            ans = 1.0+y*(3.5156229+y*(3.0899424+y*(1.2067492
                +y*(0.2659732+y*(0.360768e-1+y*0.45813e-2)))));
        }
        else {
            double y = 3.75/ax;
            ans = (exp(ax)/sqrt(ax))*(0.39894228+y*(0.1328592e-1
                +y*(0.225319e-2+y*(-0.157565e-2+y*(0.916281e-2
                +y*(-0.2057706e-1+y*(0.2635537e-1+y*(-0.1647633e-1
                +y*0.392377e-2))))))));
        }
        return (ans);
    }
        

    /*
    double
    bessI1(double x)
    {
        double ans, ax = fabs(x);
        if (ax < 3.75) {
            double y = x/3.75;
            y *= y;
            ans = ax*(0.5+y*(0.87890594+y*(0.51498869+y*(0.15084934
                +y*(0.2658733e-1+y*(0.301532e-2+y*0.32411e-3))))));
        }
        else {
            double y = 3.75/ax;
            ans = 0.2282967e-1+y*(-0.2895312e-1+y*(0.1787654e-1
                -y*0.420059e-2));
            ans = 0.39894228+y*(-0.3988024e-1+y*(-0.362018e-2
                +y*(0.163801e-2+y*(-0.1031555e-1+y*ans))));
            ans *= (exp(ax)/sqrt(ax));
        }
        return (x < 0.0 ? -ans : ans);
    }
    */


    // return (bessI1(x) + bessI0(x))*exp(-x) assuming x > 0
    //
    double
    bessXX(double x)
    {
        double ax = fabs(x);
        if (ax < 3.75) {
            double y = x/3.75;
            y *= y;
            double ans1 = 1.0+y*(3.5156229+y*(3.0899424+y*(1.2067492
                +y*(0.2659732+y*(0.360768e-1+y*0.45813e-2)))));
            double ans2 = ax*(0.5+y*(0.87890594+y*(0.51498869+y*(0.15084934
                +y*(0.2658733e-1+y*(0.301532e-2+y*0.32411e-3))))));
            if (x > 0)
                ans1 += ans2;
            else
                ans1 -= ans2;
            return (ans1 * exp(-x));
        }
        else {
            double y = 3.75/ax;
            double ans1 = (0.39894228+y*(0.1328592e-1
                +y*(0.225319e-2+y*(-0.157565e-2+y*(0.916281e-2
                +y*(-0.2057706e-1+y*(0.2635537e-1+y*(-0.1647633e-1
                +y*0.392377e-2))))))));
            double ans2 = 0.2282967e-1+y*(-0.2895312e-1+y*(0.1787654e-1
                -y*0.420059e-2));
            ans2 = 0.39894228+y*(-0.3988024e-1+y*(-0.362018e-2
                +y*(0.163801e-2+y*(-0.1031555e-1+y*ans2))));
            if (x > 0)
                ans1 += ans2;
            else
                ans1 -= ans2;
            ans1 /= sqrt(ax);
            return (ans1);
        }
    }


    double
    bessI1xOverX(double x)
    {
        double ans, ax = fabs(x);
        if (ax < 3.75) {
            double y = x/3.75;
            y *= y;
            ans = 0.5+y*(0.87890594+y*(0.51498869+y*(0.15084934
                +y*(0.2658733e-1+y*(0.301532e-2+y*0.32411e-3)))));
        }
        else {
            double y = 3.75/ax;
            ans = 0.2282967e-1+y*(-0.2895312e-1+y*(0.1787654e-1
                -y*0.420059e-2));
            ans = 0.39894228+y*(-0.3988024e-1+y*(-0.362018e-2
                +y*(0.163801e-2+y*(-0.1031555e-1+y*ans))));
            ans *= (exp(ax)/(ax*sqrt(ax)));
        }
        return (ans);
    }


    double
    bessYY(double x, double z)
    {
        double ans, ax = fabs(x);
        if (ax < 3.75) {
            double y = x/3.75;
            y *= y;
            ans = 0.5+y*(0.87890594+y*(0.51498869+y*(0.15084934
                +y*(0.2658733e-1+y*(0.301532e-2+y*0.32411e-3)))));
            ans *= exp(z);
        }
        else {
            double y = 3.75/ax;
            ans = 0.2282967e-1+y*(-0.2895312e-1+y*(0.1787654e-1
                -y*0.420059e-2));
            ans = 0.39894228+y*(-0.3988024e-1+y*(-0.362018e-2
                +y*(0.163801e-2+y*(-0.1031555e-1+y*ans))));
            ans *= (exp(ax + z)/(ax*sqrt(ax)));
        }
        return (ans);
    }


    double
    bessZZ(double x, double z)
    {
        double ans, ax = fabs(x);
        if (ax < 3.75) {
            double y = x/3.75;
            y *= y;
            ans = 1.0+y*(3.5156229+y*(3.0899424+y*(1.2067492
                +y*(0.2659732+y*(0.360768e-1+y*0.45813e-2)))));
            ans *= exp(z);
        }
        else {
            double y = 3.75/ax;
            ans = (exp(ax + z)/sqrt(ax))*(0.39894228+y*(0.1328592e-1
                +y*(0.225319e-2+y*(-0.157565e-2+y*(0.916281e-2
                +y*(-0.2057706e-1+y*(0.2635537e-1+y*(-0.1647633e-1
                +y*0.392377e-2))))))));
        }
        return (ans);
    }
        

    // ltra_rlcH1dashTwiceIntFunc - twice repeated integral of h1dash
    // for the special case of G = 0
    //
    double
    ltra_rlcH1dashTwiceIntFunc(double time, double beta)
    {
        // result = time * e^{- beta*time} * {I_0(beta*time) +
        // I_1(beta*time)} - time
        //
        if (beta == 0.0)
            return (time);
        double arg = beta*time;
        if (arg == 0.0)
            return (0.0);
        return (bessXX(arg)*time - time);
    }


    // ltra_rlcH3dashIntFunc - twice repeated integral of h1dash for the
    // special case of G = 0
    //
    double
    ltra_rlcH3dashIntFunc(double time, double T, double beta)
    {
        if (time <= T || beta == 0.0)
            return (0.0);
        double exparg = -beta*time;
        double besselarg = beta*sqrt(time*time - T*T);
        return (bessZZ(besselarg, exparg) - exp(-beta*T));
    }


    double 
    ltra_rcH1dashTwiceIntFunc(double time, double cbyr)
    {
        return (sqrt(4*cbyr*time/M_PI));
    }


    double
    ltra_rcH2TwiceIntFunc(double time,double rclsqr)
    {
        if (time != 0.0) {
            double temp = rclsqr/(4*time);
            return ((time + rclsqr*0.5)*erfc(sqrt(temp)) -
                sqrt(time*rclsqr/M_PI)*exp(-temp));
        }
        return (0.0);
    }
}

