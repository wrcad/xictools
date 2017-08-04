
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
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: UCB CAD Group
         1993 Stephen R. Whiteley
****************************************************************************/

#ifndef SENSDEFS_H
#define SENSDEFS_H

#include "acdefs.h"


//
// Structures used to describe an Adjoint Sensitivity analysis.
//

// internal data
struct sSENSint
{
    sSENSint()
        {
            dY = 0;
            dIr = 0;
            dIi = 0;
            dIdYr = 0;
            dIdYi = 0;
            o_cvalues = 0;
            o_values = 0;
            size = 0;
        }

    ~sSENSint()
        {
            clear();
        }

    // senssetp.cc
    int setup(int, bool);
    void clear();

    spMatrixFrame *dY;
    double *dIr;
    double *dIi;
    double *dIdYr;
    double *dIdYi;
    IFcomplex *o_cvalues;
    double *o_values;
    int size;
};


struct sSENSAN : public sACAN
{
    sSENSAN()
        {
            SENSoutPos = 0;
            SENSoutNeg = 0;
            SENSoutSrc = 0;
            SENSoutSrcDev = 0;
            SENSoutName = 0;
        }

    ~sSENSAN()
        {
            ST.clear();
        }

    sJOB *dup()
        {
            return (0);
            /* XXX no mt yet
            sSENSAN *sens = new sSENSAN;
            sens->b_name = b_name;
            sens->b_type = b_type;
            sens->JOBoutdata = new sOUTdata(*JOBoutdata);
            sens->JOBrun = JOBrun;
            sens->JOBdc = JOBdc;
            sens->JOBdc.uninit();
            sens->JOBac = JOBac;
            sens->SENSoutPos = SENSoutPos;
            sens->SENSoutNeg = SENSoutNeg;
            sens->SENSoutSrc = SENSoutSrc;
            sens->SENSoutSrcDev = SENSoutSrcDev;
            sens->SENSoutName = SENSoutName;
            sens->ST = ST;
            */
        }

    /* XXX no mt yet
    virtual bool threadable()   { return (true); }

    virtual int points(const sCKT *ckt)
        {
            return (JOBac.points(ckt));
        }
    */

    sCKTnode *SENSoutPos;    // output positive node
    sCKTnode *SENSoutNeg;    // output reference node
    IFuid SENSoutSrc;        // output source UID
    sGENSRCinstance *SENSoutSrcDev;  // pointer to output device
    const char *SENSoutName; // name of output, e.g. v(1,2)
    sSENSint ST;             // internal variables, pass to subroutines
};

struct SENSanalysis : public IFanalysis
{
    // senssetp.cc
    SENSanalysis();
    int setParm(sJOB*, int, IFdata*);

    // sensaskq.cc
    int askQuest(const sCKT*, const sJOB*, int, IFdata*) const;

    // sensprse.cc
    int parse(sLine*, sCKT*, int, const char**, sTASK*);

    // sensan.cc
    int anFunc(sCKT*, int);

    sJOB *newAnal() { return (new sSENSAN); }

private:
    static int sens_acoperation(sCKT*, int);
    static int sens_dcoperation(sCKT*, int);
    static double Sens_Delta;
    static double Sens_Abs_Delta;
};

extern SENSanalysis SENSinfo;

#define    SENS_POS         2
#define    SENS_NEG         3
#define    SENS_SRC         4
#define    SENS_NAME        5

#endif // SENSDEFS_H

