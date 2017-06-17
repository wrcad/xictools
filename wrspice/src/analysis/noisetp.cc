
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 1996 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL STEPHEN R. WHITELEY BE LIABLE       *
 *   FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION      *
 *   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN           *
 *   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN         *
 *   THE SOFTWARE.                                                        *
 *                                                                        *
 *========================================================================*
 *                                                                        *
 * Whiteley Research Circuit Simulation and Analysis Tool                 *
 *                                                                        *
 *========================================================================*
 $Id: noisetp.cc,v 2.47 2015/08/06 00:57:02 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1987 Gary W. Ng
         1992 Stephen R. Whiteley
****************************************************************************/

#include "noisdefs.h"
#include "errors.h"
#include "outdata.h"
#include "kwords_analysis.h"


NOISEanalysis NOISEinfo;

// Noise keywords
const char *nokw_output    = "output";
const char *nokw_outputref = "outputref";
const char *nokw_input     = "input";
const char *nokw_ptspersum = "ptspersum";

namespace {
    IFparm Nparms[] = {
        IFparm(nokw_output,     N_OUTPUT,       IF_SET|IF_STRING,
            "output noise summation node"),
        IFparm(nokw_outputref,  N_OUTREF,       IF_SET|IF_STRING,
            "output noise reference node"),
        IFparm(nokw_input,      N_INPUT,        IF_SET|IF_STRING,
            "input noise source"),
        IFparm(nokw_ptspersum,  N_PTSPERSUM,    IF_SET|IF_INTEGER,
            "frequency points per summary report"),
        IFparm(ackw_dec,        AC_DEC,         IF_SET|IF_FLAG,
            "step by decades"),
        IFparm(ackw_oct,        AC_OCT,         IF_SET|IF_FLAG,
            "step by octaves"),
        IFparm(ackw_lin,        AC_LIN,         IF_SET|IF_FLAG,
            "step linearly"),
        IFparm(ackw_numsteps,   AC_STEPS,       IF_SET|IF_INTEGER,
            "number of frequencies"),
        IFparm(ackw_start,      AC_START,       IF_SET|IF_REAL,
            "starting frequency"),
        IFparm(ackw_stop,       AC_STOP,        IF_SET|IF_REAL,
            "ending frequency"),
        IFparm(dckw_name1,      DC_NAME1,       IF_SET|IF_INSTANCE,
            "name of source to step"),
        IFparm(dckw_start1,     DC_START1,      IF_SET|IF_REAL,
            "starting voltage/current"),
        IFparm(dckw_stop1,      DC_STOP1,       IF_SET|IF_REAL,
            "ending voltage/current"),
        IFparm(dckw_step1,      DC_STEP1,       IF_SET|IF_REAL,
            "voltage/current step"),
        IFparm(dckw_name2,      DC_NAME2,       IF_SET|IF_INSTANCE,
            "name of source to step"),
        IFparm(dckw_start2,     DC_START2,      IF_SET|IF_REAL,
            "starting voltage/current"),
        IFparm(dckw_stop2,      DC_STOP2,       IF_SET|IF_REAL,
            "ending voltage/current"),
        IFparm(dckw_step2,      DC_STEP2,       IF_SET|IF_REAL,
            "voltage/current step")
    };
}


NOISEanalysis::NOISEanalysis()
{
    name = "NOISE";
    description = "Noise analysis";
    numParms = sizeof(Nparms)/sizeof(IFparm);
    analysisParms = Nparms;
    domain = FREQUENCYDOMAIN;
};


int 
NOISEanalysis::setParm(sJOB *anal, int which, IFdata *data)
{
    sNOISEAN *job = static_cast<sNOISEAN*>(anal);
    if (!job)
        return (E_PANIC);
    IFvalue *value = &data->v;

    switch (which) {
    case N_OUTPUT:
        job->Noutput = value->sValue;
        break;

    case N_OUTREF:
        job->NoutputRef = value->sValue;
        break;

    case N_INPUT:
        job->Ninput = value->sValue;
        break;

    case N_PTSPERSUM:
        job->NStpsSm = value->iValue;
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

