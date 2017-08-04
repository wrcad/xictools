
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
Authors: 1988 Jaijeet S Roychowdhury
         1992 Stephen R. Whiteley
****************************************************************************/

#ifndef DISTDEFS_H
#define DISTDEFS_H

#ifdef D_DBG_ALLTIMES
#define D_DBG_BLOCKTIMES
#define D_DBG_SMALLTIMES
#endif

#include "circuit.h"

// Structure for passing a large number of values.
// No constructor, always created on stack.
//
struct DpassStr
{
    double cxx;
    double cyy;
    double czz;
    double cxy;
    double cyz;
    double cxz;
    double cxxx;
    double cyyy;
    double czzz;
    double cxxy;
    double cxxz;
    double cxyy;
    double cyyz;
    double cxzz;
    double cyzz;
    double cxyz;
    double r1h1x;
    double i1h1x;
    double r1h1y;
    double i1h1y;
    double r1h1z;
    double i1h1z;
    double r1h2x;
    double i1h2x;
    double r1h2y;
    double i1h2y;
    double r1h2z;
    double i1h2z;
    double r2h11x;
    double i2h11x;
    double r2h11y;
    double i2h11y;
    double r2h11z;
    double i2h11z;
    double h2f1f2x;
    double ih2f1f2x;
    double h2f1f2y;
    double ih2f1f2y;
    double h2f1f2z;
    double ih2f1f2z;
};


// Structure to keep derivatives of upto 3rd order w.r.t 3 variables.
// No constructor, always created on stack.
//
struct Dderivs
{
    double value;
    double d1_p;
    double d1_q;
    double d1_r;
    double d2_p2;
    double d2_q2;
    double d2_r2;
    double d2_pq;
    double d2_qr;
    double d2_pr;
    double d3_p3;
    double d3_q3;
    double d3_r3;
    double d3_p2q;
    double d3_p2r;
    double d3_pq2;
    double d3_q2r;
    double d3_pr2;
    double d3_qr2;
    double d3_pqr;
};


// Structure used to describe an DISTO analysis to be performed.
//
struct sDISTOAN : public sJOB
{
    sDISTOAN()
        {
            DstartF1 = 0.0;
            DstopF1 = 0.0;
            DfreqDelta = 0.0;
            DsaveF1 = 0.0;
            Freq = 0.0;
            FreqTol = 0.0;
            NumPoints = 0;
            displacement = 0;
            size = 0;
            DstepType = 0;
            DnumSteps = 0;
            Df2wanted = 0;
            Df2given = 0;
            Df2ovrF1  = 0.0;
            Domega1  = 0.0;
            Domega2  = 0.0;

            r1H1ptr = 0;
            i1H1ptr = 0;
            r2H11ptr = 0;
            i2H11ptr = 0;
            r3H11ptr = 0;
            i3H11ptr = 0;
            r1H2ptr = 0;
            i1H2ptr = 0;
            r2H12ptr = 0;
            i2H12ptr = 0;
            r2H1m2ptr = 0;
            i2H1m2ptr = 0;
            r3H1m2ptr = 0;
            i3H1m2ptr = 0;

            r1H1stor = 0;
            i1H1stor = 0;
            r2H11stor = 0;
            i2H11stor = 0;
            r3H11stor = 0;
            i3H11stor = 0;
            r1H2stor = 0;
            i1H2stor = 0;
            r2H12stor = 0;
            i2H12stor = 0;
            r2H1m2stor = 0;
            i2H1m2stor = 0;
            r3H1m2stor = 0;
            i3H1m2stor = 0;
        }

    ~sDISTOAN();

    sJOB *dup() { return (0); }  // XXX fixme

    double DstartF1;   // the start value of the higher frequency for
                       //  distortion analysis
    double DstopF1;    // the stop value ove above
    double DfreqDelta; // multiplier for decade/octave stepping,
                       //  step for linear steps
    double DsaveF1;    // frequency at which we left off last time
    double Freq;
    double FreqTol;    // tolerence for finding final frequency
    int NumPoints;
    int displacement;
    int size;
    int DstepType;     // values described below
    int DnumSteps;
    int Df2wanted;     // set if f2overf1 is given in the disto command
    int Df2given;      // set if at least 1 source has an f2 input
    double Df2ovrF1;   // ratio of f2 over f1 if 2 frequencies given
                       //  should be < 1
    double Domega1;    // current omega1
    double Domega2;    // current omega2

    double* r1H1ptr;
    double* i1H1ptr;
    double* r2H11ptr;
    double* i2H11ptr;
    double* r3H11ptr;
    double* i3H11ptr;
    double* r1H2ptr;   // distortion analysis Volterra transforms
    double* i1H2ptr;
    double* r2H12ptr;
    double* i2H12ptr;
    double* r2H1m2ptr;
    double* i2H1m2ptr;
    double* r3H1m2ptr;
    double* i3H1m2ptr;

    double** r1H1stor;
    double** i1H1stor;
    double** r2H11stor;
    double** i2H11stor;
    double** r3H11stor;
    double** i3H11stor; // these store computed values
    double** r1H2stor;  //  for the plots 
    double** i1H2stor;
    double** r2H12stor;
    double** i2H12stor;
    double** r2H1m2stor;
    double** i2H1m2stor;
    double** r3H1m2stor;
    double** i3H1m2stor;
};

struct DISTOanalysis : public IFanalysis
{
    // distsetp.cc
    DISTOanalysis();
    int setParm(sJOB*, int, IFdata*);

    // distaskq.cc
    int askQuest(const sCKT*, const sJOB*, int, IFdata*) const;

    // distprse.cc
    int parse(sLine*, sCKT*, int, const char**, sTASK*);

    // distan.cc
    int anFunc(sCKT*, int);

    sJOB *newAnal() { return (new sDISTOAN); }
};

extern DISTOanalysis DISTOinfo;

// available step types:

#define DECADE 1
#define OCTAVE 2
#define LINEAR 3

// defns. used in DsetParm

#define D_DEC     1
#define D_OCT     2
#define D_LIN     3
#define D_START   4
#define D_STOP    5
#define D_STEPS   6
#define D_F2OVRF1 7

// defns. used by CKTdisto for calling different functions

#define D_SETUP     1
#define D_F1        2
#define D_F2        3
#define D_TWOF1     4
#define D_THRF1     5
#define D_F1PF2     6
#define D_F1MF2     7
#define D_2F1MF2    8
#define D_RHSF1     9
#define D_RHSF2    10

extern double D1i2F1(double, double, double);
extern double D1i3F1(double, double, double, double, double, double);
extern double D1iF12(double, double, double, double, double);
extern double D1i2F12(double, double, double, double, double, double, double,
                double, double, double);
extern double D1n2F1(double, double, double);
extern double D1n3F1(double, double, double, double, double, double);
extern double D1nF12(double, double, double, double, double);
extern double D1n2F12(double, double, double, double, double, double, double,
                double, double, double);
extern double DFn2F1(double, double, double, double, double,
                double, double, double, double, double, double, double);
extern double DFi2F1(double, double, double, double, double,
                double, double, double, double, double, double, double);
extern double DFi3F1(double, double, double, double, 
                double, double, double, double, double, double, double,
                double, double, double, double, double, double, double,
                double, double, double, double, double, double, double,
                double, double, double);
extern double DFn3F1(double, double, double, double, 
                double, double, double, double, double, double, double,
                double, double, double, double, double, double, double,
                double, double, double, double, double, double, double,
                double, double, double);
extern double DFnF12(double, double, double, double,
                double, double, double, double, double, double, double,
                double, double, double, double, double, double, double);
extern double DFiF12(double, double, double, double, 
                double, double, double, double, double, double, double,
                double, double, double, double, double, double, double);
extern double DFn2F12(DpassStr*);
extern double DFi2F12(DpassStr*);

extern void AtanDeriv(Dderivs*, Dderivs*);
extern void CosDeriv(Dderivs*, Dderivs*);
extern void CubeDeriv(Dderivs*, Dderivs*);
extern void DivDeriv(Dderivs*, Dderivs*, Dderivs*);
extern void EqualDeriv(Dderivs*, Dderivs*);
extern void ExpDeriv(Dderivs*, Dderivs*);
extern void InvDeriv(Dderivs*, Dderivs*);
extern void MultDeriv(Dderivs*, Dderivs*, Dderivs*);
extern void PlusDeriv(Dderivs*, Dderivs*, Dderivs*);
extern void PowDeriv(Dderivs*, Dderivs*, double);
extern void SqrtDeriv(Dderivs*, Dderivs*);
extern void TanDeriv(Dderivs*, Dderivs*);
extern void TimesDeriv(Dderivs*, Dderivs*, double);

#endif // DISTDEFS_H

