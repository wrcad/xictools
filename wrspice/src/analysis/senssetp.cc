
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

#include "sensdefs.h"
#include "spmatrix.h"
#include "outdata.h"
#include "kwords_analysis.h"


SENSanalysis SENSinfo;

// Sens keywords
const char *snkw_deftol     = "deftol";
const char *snkw_defperturb = "defperturb";
const char *snkw_outpos     = "outpos";
const char *snkw_outneg     = "outneg";
const char *snkw_outsrc     = "outsrc";
const char *snkw_outname    = "outname";

namespace {
    IFparm SENSparms[] = {
        IFparm(snkw_outpos,     SENS_POS,    IF_IO|IF_NODE,
            "output positive node"),
        IFparm(snkw_outneg,     SENS_NEG,    IF_IO|IF_NODE,
            "output negative node"),
        IFparm(snkw_outsrc,     SENS_SRC,    IF_IO|IF_INSTANCE,
            "output current"),
        IFparm(snkw_outname,    SENS_NAME,   IF_IO|IF_STRING,
            "output variable name"),
        IFparm(ackw_start,      AC_START,    IF_IO|IF_REAL,
            "starting frequency"),
        IFparm(ackw_stop,       AC_STOP,     IF_IO|IF_REAL,
            "ending frequency"),
        IFparm(ackw_numsteps,   AC_STEPS,    IF_IO|IF_INTEGER,
            "number of frequencies"),
        IFparm(ackw_dec,        AC_DEC,      IF_IO|IF_FLAG,
            "step by decades"),
        IFparm(ackw_oct,        AC_OCT,      IF_IO|IF_FLAG,
            "step by octaves"),
        IFparm(ackw_lin,        AC_LIN,      IF_IO|IF_FLAG,
            "step linearly"),
        IFparm(dckw_name1,      DC_NAME1,    IF_IO|IF_INSTANCE,
            "name of source to step"),
        IFparm(dckw_start1,     DC_START1,   IF_IO|IF_REAL,
            "starting voltage/current"),
        IFparm(dckw_stop1,      DC_STOP1,    IF_IO|IF_REAL,
            "ending voltage/current"),
        IFparm(dckw_step1,      DC_STEP1,    IF_IO|IF_REAL,
            "voltage/current step"),
        IFparm(dckw_name2,      DC_NAME2,    IF_IO|IF_INSTANCE,
            "name of source to step"),
        IFparm(dckw_start2,     DC_START2,   IF_IO|IF_REAL,
            "starting voltage/current"),
        IFparm(dckw_stop2,      DC_STOP2,    IF_IO|IF_REAL,
            "ending voltage/current"),
        IFparm(dckw_step2,      DC_STEP2,    IF_IO|IF_REAL,
            "voltage/current step")
    };
}


SENSanalysis::SENSanalysis()
{
    name = "SENS";
    description = "Sensitivity analysis";
    numParms = sizeof(SENSparms)/sizeof(IFparm);
    analysisParms = SENSparms;
    domain = FREQUENCYDOMAIN;
};


int 
SENSanalysis::setParm(sJOB *anal, int which, IFdata *data)
{
    sSENSAN *job = static_cast<sSENSAN*>(anal);
    if (!job)
        return (E_PANIC);
    IFvalue *value = &data->v;

    switch (which) {
    case SENS_POS:
        job->SENSoutPos = (sCKTnode*)value->nValue;
        break;

    case SENS_NEG:
        job->SENSoutNeg = (sCKTnode*)value->nValue;
        break;

    case SENS_SRC:
        job->SENSoutSrc = value->uValue;
        break;

    case SENS_NAME:
        job->SENSoutName = value->sValue;
        break;

    default:
        if (job->JOBac.setp(which, data) == OK)
            return (OK);
        if (job->JOBdc.setp(which, data) == OK)
            return (OK);
        return (E_BADPARM);
    }
    return (OK);
}
// End of SENSanalysis functions.


int
sSENSint::setup(int sz, bool is_dc)
{
    size = sz;

    // Create the perturbation matrix.
    dY = new spMatrixFrame(size, SP_COMPLEX | SP_NO_KLU | SP_NO_SORT);
    int error = dY->spError();
    if (error)
        return (error);

    // Create extra rhs.
    dIr   = new double[size + 1];
    dIdYr = new double[size + 1];
    if (!is_dc) {
        dIi   = new double[size + 1];
        dIdYi = new double[size + 1];
    }
    return (OK);
}


void
sSENSint::clear()
{
    delete dY;              dY = 0;
    delete [] dIr;          dIr = 0;
    delete [] dIi;          dIi = 0;
    delete [] dIdYr;        dIdYr = 0;
    delete [] dIdYi;        dIdYi = 0;
    delete [] o_values;     o_values = 0;
    delete [] o_cvalues;    o_cvalues = 0;
}

