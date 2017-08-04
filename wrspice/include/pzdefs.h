
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
Authors: UCB CAD Group
         1992 Stephen R. Whiteley
****************************************************************************/

#ifndef PZDEFS_H
#define PZDEFS_H

#include "acdefs.h"


//
// Structure used to describe an PZ analysis to be performed.
//

struct PZtrial
{
    PZtrial()
        {
            next = prev = 0;
            mag_raw = mag_def = 0;
            multiplicity = 0;
            flags = 0;
            seq_num = 0;
            count = 0;
        }
        
    cIFcomplex s, f_raw, f_def;
    PZtrial *next, *prev;
    int mag_raw, mag_def;
    int multiplicity;
    int flags;
    int seq_num;
    int count;
};

struct sPZAN : public sACAN
{
    sPZAN()
        {
            PZin_pos = 0;
            PZin_neg = 0;
            PZout_pos = 0;
            PZout_neg = 0;
            PZinput_type = 0;
            PZwhich = 0;
            PZnumswaps = 0;
            PZbalance_col = 0;
            PZsolution_col = 0;
            PZnPoles = 0;
            PZnZeros = 0;
            PZpoleList = 0;
            PZzeroList = 0;
            PZdrive_pptr = 0;
            PZdrive_nptr = 0;

            NIpzK = 0.0;
            PZtrapped = 0;
            NIpzK_mag = 0;
            NZeros = 0;
            NFlat = 0;
            Max_Zeros = 0;
            Seq_Num = 0;
            ZeroTrial = 0;
            Trials = 0;
            Guess_Param = 0.0;
            High_Guess = 0.0;
            Low_Guess = 0.0;
            Last_Move = 0;
            Consec_Moves = 0;
            NIter = 0;
            NTrials = 0;
            Aberr_Num = 0;
        }

    ~sPZAN() { }

    sJOB *dup() { return (0); }  // XXX fixme

    int PZsetup(sCKT*, int);
    int PZfindZeros(sCKT*, PZtrial**, int*);

    int PZin_pos;
    int PZin_neg;
    int PZout_pos;
    int PZout_neg;
    int PZinput_type;
    int PZwhich;
    int PZnumswaps;
    int PZbalance_col;
    int PZsolution_col;
    int PZnPoles;
    int PZnZeros;
    PZtrial *PZpoleList;
    PZtrial *PZzeroList;
    double *PZdrive_pptr;
    double *PZdrive_nptr;

private:
    int PZload(sCKT*, IFcomplex*);
    int PZeval(int, PZtrial**, PZtrial**);
    int PZstrat(PZtrial**);
    int PZrunTrial(sCKT*, PZtrial**, PZtrial**);
    int PZverify(PZtrial**, PZtrial*);
    void PZupdateSet(PZtrial**, PZtrial*);
    int PZstep(int, PZtrial**);
    void PZreset(PZtrial**);
    PZtrial *pzseek(PZtrial*, int);
    void clear_trials(int);
    void zaddeq(double*, int*, double, int, double, int);
    void show_trial(PZtrial*, char);
    void check_flat(PZtrial*, PZtrial*);
    int alter(PZtrial*, PZtrial*, double, double);
    int NIpzSym(PZtrial**, PZtrial*);
    int NIpzComplex(PZtrial**, PZtrial*);
    int NIpzMuller(PZtrial**, PZtrial*);
    int NIpzSym2(PZtrial**, PZtrial*);

    double NIpzK;
    int PZtrapped;
    int NIpzK_mag;
    int NZeros;
    int NFlat;
    int Max_Zeros;
    int Seq_Num;
    PZtrial *ZeroTrial;
    PZtrial *Trials;
    double Guess_Param;
    double High_Guess;
    double Low_Guess;
    int Last_Move;
    int Consec_Moves;
    int NIter;
    int NTrials;
    int Aberr_Num;
};

struct PZanalysis : public IFanalysis
{
    // pzsetp.cc
    PZanalysis();
    int setParm(sJOB*, int, IFdata*);

    // pzaskq.cc
    int askQuest(const sCKT*, const sJOB*, int, IFdata*) const;

    // pzprse.cc
    int parse(sLine*, sCKT*, int, const char**, sTASK*);

    // pzan.cc
    int anFunc(sCKT*, int);

    sJOB *newAnal() { return (new sPZAN); }

private:
    static int pzInit(sCKT*);
    static int pz_dcoperation(sCKT*, int);
};

extern PZanalysis PZinfo;

#define PZ_DO_POLES 0x1
#define PZ_DO_ZEROS 0x2
#define PZ_IN_VOL 1
#define PZ_IN_CUR 2

#define PZ_NODEI 1
#define PZ_NODEG 2
#define PZ_NODEJ 3
#define PZ_NODEK 4
#define PZ_V     5
#define PZ_I     6
#define PZ_POL   7
#define PZ_ZER   8
#define PZ_PZ    9

#endif // PZDEFS_H

