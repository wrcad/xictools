
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
Authors: 1988 Jaijeet S Roychowdhury
         1993 Stephen R. Whiteley
****************************************************************************/

#include "distdefs.h"
#include "errors.h"
#include "kwords_analysis.h"


DISTOanalysis DISTOinfo;

// Distortion keywords
const char *dokw_start    = "start";
const char *dokw_stop     = "stop";
const char *dokw_numsteps = "numsteps";
const char *dokw_dec      = "dec";
const char *dokw_oct      = "oct";
const char *dokw_lin      = "lin";
const char *dokw_f2overf1 = "f2overf1";

namespace {
    IFparm Dparms[] = {
        IFparm(dokw_start,      D_START,   IF_SET|IF_REAL,
            "starting frequency"),
        IFparm(dokw_stop,       D_STOP,    IF_SET|IF_REAL,
            "ending frequency"),
        IFparm(dokw_numsteps,   D_STEPS,   IF_SET|IF_INTEGER,
            "number of frequencies"),
        IFparm(dokw_dec,        D_DEC,     IF_SET|IF_FLAG,
            "step by decades"),
        IFparm(dokw_oct,        D_OCT,     IF_SET|IF_FLAG,
            "step by octaves"),
        IFparm(dokw_lin,        D_LIN,     IF_SET|IF_FLAG,
            "step linearly"),
        IFparm(dokw_f2overf1,   D_F2OVRF1, IF_SET|IF_REAL,
            "ratio of F2 to F1")
    };
}


DISTOanalysis::DISTOanalysis()
{
    name = "DISTO";
    description = "Small signal distortion analysis";
    numParms = sizeof(Dparms)/sizeof(IFparm);
    analysisParms = Dparms;
    domain = FREQUENCYDOMAIN;
};


int 
DISTOanalysis::setParm(sJOB *anal, int which, IFdata *data)
{
    sDISTOAN *job = static_cast<sDISTOAN*>(anal);
    if (!job)
        return (E_PANIC);
    IFvalue *value = &data->v;

    switch (which) {
    case D_START:
        job->DstartF1 = value->rValue;
        break;

    case D_STOP:
        job->DstopF1 = value->rValue;
        break;

    case D_STEPS:
        job->DnumSteps = value->iValue;
        break;

    case D_DEC:
        job->DstepType = DECADE;
        break;

    case D_OCT:
        job->DstepType = OCTAVE;
        break;

    case D_LIN:
        job->DstepType = LINEAR;
        break;

    case D_F2OVRF1:
        job->Df2ovrF1 = value->rValue;
        job->Df2wanted = 1;
        break;

    default:
        return (E_BADPARM);
    }
    return (OK);
}

