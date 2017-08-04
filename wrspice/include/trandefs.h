
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
Authors: 1985 Thomas L. Quarles
         1992 Stephen R. Whiteley
****************************************************************************/

#ifndef TRANDEFS_H
#define TRANDEFS_H

#include "dctdefs.h"


//
// TRANdefs.h - defs for transient analyses.
//

// Specification of output points, this is variable length.
//
struct transpec_t
{
    double step(int i)
        {
            if (i < 0 || i >= nparts)
                return (0.0);
            return (vals[2*i]);
        }

    double end(int i)
        {
            if (i < 0 || i >= nparts)
                return (0.0);
            return (vals[2*i + 1]);
        }

    int points(const sCKT*);
    transpec_t *dup();
    int part(double);

    int nparts;         // number of ranges
    double tstart;      // start time

    static transpec_t *new_transpec(double, int, const double*);

private:
    double vals[2];     // tstep, tend for each range
};


// Structure to hold private parameters and functions for transient
// analysis.
//
struct sTRANint
{
    sTRANint()
        {
            t_check = 0.0;
            t_start = 0.0;
            t_step = 0.0;
            t_stop = 0.0;
            t_delmax = 0.0;
            t_delmin = 0.0;
            t_fixed_step = 0.0;

            t_polydegree = 0;
            t_count = 0;
            t_ordcnt = 0;

            t_nointerp = false;
            t_hitusertp = false;
            t_dumpit = false;
            t_firsttime = false;
            t_nocut = false;
            t_spice3 = false;
            t_delmin_given = false;
            t_minbreak_given = false;
        }

    inline void inc_check(sCKT*);

    int  accept(sCKT*, sSTATS*, int*, int*);
    void setbp(sCKT*);
    int  step(sCKT*, sSTATS*);
    int  interpolate(sCKT*);

    double  t_check;        // running variable for plot points
    double  t_start;
    double  t_step;
    double  t_stop;
    double  t_delmax;       // maximum time delta
    double  t_delmin;       // minimum time delta
    double  t_fixed_step;   // fixed time step if > 0
    int     t_polydegree;   // interpolation degree
    int     t_count;        // output point count
    int     t_ordcnt;       // count order=1 steps
    bool    t_nointerp;     // don't interpolate
    bool    t_hitusertp;    // hit plot points like breakpoints
    bool    t_dumpit;       // output the next point
    bool    t_firsttime;    // first time through
    bool    t_nocut;        // no timestep setting devices in ckt
    bool    t_spice3;       // Spice3 compatibility
    bool    t_delmin_given; // User specified delmin
    bool    t_minbreak_given; // Used specified minbreak
};

struct sTRANAN : public sDCTAN
{
    sTRANAN()
        {
            TRANspec = 0;
            TRANmaxStep = 0.0;
            TRANsegDelta = 0.0;
            TRANmode = 0;
            TRANsegBaseName = 0;
        }

    ~sTRANAN()
        {
            delete TRANspec;
        }

    sJOB *dup()
        {
            sTRANAN *tran = new sTRANAN;
            tran->b_name = b_name;
            tran->b_type = b_type;
            tran->JOBoutdata = new sOUTdata(*JOBoutdata);
            tran->JOBrun = JOBrun;
            tran->JOBdc = JOBdc;
            tran->JOBdc.uninit();
            tran->TRANspec = TRANspec->dup();
            tran->TRANmaxStep = TRANmaxStep;
            tran->TRANsegDelta = TRANsegDelta;
            tran->TRANmode = TRANmode;
            tran->TRANsegBaseName = TRANsegBaseName;
            tran->TS = TS;
            return (tran);
        }

    bool threadable();
    int init(sCKT*);

    int points(const sCKT *ckt)
        {
            if (TS.t_nointerp)
                return (1);
            return (TRANspec->points(ckt));
        }

    transpec_t *TRANspec;   // parameters
    double TRANmaxStep;     // maximum internal timestep
    double TRANsegDelta;    // interval to use for segment
    long TRANmode;          // MODEUIC, MODESCROLL?
    const char *TRANsegBaseName; // base name for segment file
    sTRANint TS;            // pass this to subroutines
};

struct TRANanalysis : public IFanalysis
{
    // transetp.cc
    TRANanalysis();
    int setParm(sJOB*, int, IFdata*);

    // tranaskq.cc
    int askQuest(const sCKT*, const sJOB*, int, IFdata*) const;

    // tranprse.cc
    int parse(sLine*, sCKT*, int, const char**, sTASK*);

    // tranan.cc
    int anFunc(sCKT*, int);

    sJOB *newAnal() { return (new sTRANAN); }

private:
    static int  tran_dcoperation(sCKT*, int);
};

extern TRANanalysis TRANinfo;

#define TRAN_PARTS     1
#define TRAN_TSTART    2
#define TRAN_TMAX      3
#define TRAN_UIC       4
#define TRAN_SCROLL    5
#define TRAN_SEGMENT   6
#define TRAN_SEGWIDTH  7

#endif // TRANDEFS_H

