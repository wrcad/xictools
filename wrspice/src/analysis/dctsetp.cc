
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

#include "dctdefs.h"
#include "errors.h"
#include "output.h"
#include "kwords_analysis.h"


DCTanalysis DCTinfo;

// DC keywords
const char *dckw_name1  = "name1";
const char *dckw_start1 = "start1";
const char *dckw_stop1  = "stop1";
const char *dckw_step1  = "step1";
const char *dckw_name2  = "name2";
const char *dckw_start2 = "start2";
const char *dckw_stop2  = "stop2";
const char *dckw_step2  = "step2";

// Sweep keywords
const char *kw_dc       = "dc";
const char *kw_sweep    = "sweep";

namespace {
    IFparm DCTparms[] = {
        IFparm(dckw_name1,      DC_NAME1,   IF_IO|IF_INSTANCE,
#ifdef ALLPRMS
            "device[param] to step"),
#else
            "name of source to step"),
#endif
        IFparm(dckw_start1,     DC_START1,  IF_IO|IF_REAL,
            "starting value"),
        IFparm(dckw_stop1,      DC_STOP1,   IF_IO|IF_REAL,
            "ending value"),
        IFparm(dckw_step1,      DC_STEP1,   IF_IO|IF_REAL,
            "value step"),
        IFparm(dckw_name2,      DC_NAME2,   IF_IO|IF_INSTANCE,
#ifdef ALLPRMS
            "device[param] to step"),
#else
            "name of source to step"),
#endif
        IFparm(dckw_start2,     DC_START2,  IF_IO|IF_REAL,
            "starting value"),
        IFparm(dckw_stop2,      DC_STOP2,   IF_IO|IF_REAL,
            "ending value"),
        IFparm(dckw_step2,      DC_STEP2,   IF_IO|IF_REAL,
            "value step")
    };
}


DCTanalysis::DCTanalysis()
{
    name = "DC";
    description = "D.C. Transfer curve analysis";
    numParms = sizeof(DCTparms)/sizeof(IFparm);
    analysisParms = DCTparms;
    domain = SWEEPDOMAIN;
};


int 
DCTanalysis::setParm(sJOB *anal, int which, IFdata *data)
{
    sDCTAN *job = static_cast<sDCTAN*>(anal);
    if (!job)
        return (E_PANIC);

    if (job->JOBdc.setp(which, data) == OK)
        return (OK);
    return (E_BADPARM);
}

