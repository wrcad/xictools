
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA, http://wrcad.com            *
 *  Copyright (C) 2017 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
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

#ifndef TFDEFS_H
#define TFDEFS_H

#include "acdefs.h"


//
// Defs for transfer function analyses.
//

struct sTFAN : public sACAN
{
    sTFAN()
        {
            TFoutPos = 0;
            TFoutNeg = 0;
            TFoutSrc = 0;
            TFinSrc = 0;
            TFinSrcDev = 0;
            TFoutSrcDev = 0;
            TFoutName = 0;
            TFoutIsV = 0;
            TFoutIsI = 0;
            TFinIsV = 0;
            TFinIsI = 0;
        }

    ~sTFAN() { }

    sJOB *dup()
        {
            sTFAN *tf = new sTFAN;
            tf->b_name = b_name;
            tf->b_type = b_type;
            tf->JOBoutdata = new sOUTdata(*JOBoutdata);
            tf->JOBrun = JOBrun;
            tf->JOBdc = JOBdc;
            tf->JOBdc.uninit();
            tf->JOBac = JOBac;
            tf->TFoutPos = TFoutPos;
            tf->TFoutNeg = TFoutNeg;
            tf->TFoutSrc = TFoutSrc;
            tf->TFinSrc = TFinSrc;
            tf->TFinSrcDev = TFinSrcDev;
            tf->TFoutSrcDev = TFoutSrcDev;
            tf->TFoutName = TFoutName;
            tf->TFoutIsV = TFoutIsV;
            tf->TFoutIsI = TFoutIsI;
            tf->TFinIsV = TFinIsV;
            tf->TFinIsI = TFinIsI;
            return (tf);
        }

    bool threadable()   { return (true); }
    int init(sCKT*);

    int points(const sCKT *ckt)
        {
            return (JOBac.points(ckt));
        }

    sCKTnode *TFoutPos;   // output nodes
    sCKTnode *TFoutNeg;
    IFuid TFoutSrc;       // device names
    IFuid TFinSrc;
    sGENSRCinstance *TFinSrcDev;  // pointers to devices
    sGENSRCinstance *TFoutSrcDev;
    const char *TFoutName;     // a printable name for an output v(x,y)
    unsigned int TFoutIsV :1;
    unsigned int TFoutIsI :1;
    unsigned int TFinIsV :1;
    unsigned int TFinIsI :1;
};

struct TFanalysis : public IFanalysis
{
    // tfsetp.cc
    TFanalysis();
    int setParm(sJOB*, int, IFdata*);

    // tfaskq.cc
    int askQuest(const sCKT*, const sJOB*, int, IFdata*) const;

    // tfprse.cc
    int parse(sLine*, sCKT*, int, const char**, sTASK*);

    // tfan.cc
    int anFunc(sCKT*, int);

    sJOB *newAnal() { return (new sTFAN); }

private:
    static int tf_acoperation(sCKT*, int);
    static int tf_dcoperation(sCKT*, int);
};

extern TFanalysis TFinfo;

#define TF_OUTPOS  1
#define TF_OUTNEG  2
#define TF_OUTSRC  3
#define TF_INSRC   4
#define TF_OUTNAME 5

#endif // TFDEFS_H

