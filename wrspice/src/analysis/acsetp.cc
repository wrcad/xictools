
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
Authors: 1985 Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/

#include "acdefs.h"
#include "errors.h"
#include "outdata.h"
#include "kwords_analysis.h"


ACanalysis ACinfo;

// AC analysis keywords
const char *ackw_start    = "start";
const char *ackw_stop     = "stop";
const char *ackw_numsteps = "numsteps";
const char *ackw_dec      = "dec";
const char *ackw_oct      = "oct";
const char *ackw_lin      = "lin";

namespace {
    IFparm ACparms[] = {
        IFparm(ackw_start,      AC_START,   IF_IO|IF_REAL,
            "starting frequency"),
        IFparm(ackw_stop,       AC_STOP,    IF_IO|IF_REAL,
            "ending frequency"),
        IFparm(ackw_numsteps,   AC_STEPS,   IF_IO|IF_INTEGER,
            "number of frequencies"),
        IFparm(ackw_dec,        AC_DEC,     IF_IO|IF_FLAG,
            "step by decades"),
        IFparm(ackw_oct,        AC_OCT,     IF_IO|IF_FLAG,
            "step by octaves"),
        IFparm(ackw_lin,        AC_LIN,     IF_IO|IF_FLAG,
            "step linearly"),
        IFparm(dckw_name1,      DC_NAME1,   IF_IO|IF_INSTANCE,
            "name of source to step"),
        IFparm(dckw_start1,     DC_START1,  IF_IO|IF_REAL,
            "starting voltage/current"),
        IFparm(dckw_stop1,      DC_STOP1,   IF_IO|IF_REAL,
            "ending voltage/current"),
        IFparm(dckw_step1,      DC_STEP1,   IF_IO|IF_REAL,
            "voltage/current step"),
        IFparm(dckw_name2,      DC_NAME2,   IF_IO|IF_INSTANCE,
            "name of source to step"),
        IFparm(dckw_start2,     DC_START2,  IF_IO|IF_REAL,
            "starting voltage/current"),
        IFparm(dckw_stop2,      DC_STOP2,   IF_IO|IF_REAL,
            "ending voltage/current"),
        IFparm(dckw_step2,      DC_STEP2,   IF_IO|IF_REAL,
            "voltage/current step")
    };
}


ACanalysis::ACanalysis()
{
    name = "AC";
    description = "A.C. Small signal analysis";
    numParms = sizeof(ACparms)/sizeof(IFparm);
    analysisParms = ACparms;
    domain = FREQUENCYDOMAIN;
};


int 
ACanalysis::setParm(sJOB *anal, int which, IFdata *data)
{
    sACAN *job = static_cast<sACAN*>(anal);
    if (!job)
        return (E_PANIC);

    if (job->JOBac.setp(which, data) == OK)
        return (OK);
    if (job->JOBdc.setp(which, data) == OK)
        return (OK);
    return (E_BADPARM);
}

