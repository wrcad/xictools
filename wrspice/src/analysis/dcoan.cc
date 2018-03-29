
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
         1992 Stephen R. Whiteley
****************************************************************************/

#include "dcodefs.h"
#include "device.h"
#include "outdata.h"


int
DCOanalysis::anFunc(sCKT *ckt, int restart)
{
    sDCOAN *job = static_cast<sDCOAN*>(ckt->CKTcurJob);
    sCKTmodGen mgen(ckt->CKTmodels);
    for (sGENmodel *m = mgen.next(); m; m = mgen.next()) {
        if (DEV.device(m->GENmodType)->flags() & DV_NODCO) {
            OP.error(ERR_FATAL,
                "DCOP analysis not possible with device %s.",
                DEV.device(m->GENmodType)->name());
            return (OK);
        }
    }
    ckt->CKTcurrentAnalysis |= DOING_DCOP;

    sOUTdata *outd;
    if (restart) {
        if (!job->JOBoutdata) {
            outd = new sOUTdata;
            job->JOBoutdata = outd;
        }
        else
            outd = job->JOBoutdata;
        int error = ckt->names(&outd->numNames, &outd->dataNames);
        if (error) {
            ckt->CKTcurrentAnalysis &= ~DOING_DCOP;
            return (error);
        }
        outd->circuitPtr = ckt;
        outd->analysisPtr = ckt->CKTcurJob;
        outd->analName = ckt->CKTcurJob->JOBname;
        outd->refName = 0;
        outd->refType = IF_REAL;
        outd->dataType = IF_REAL;
        outd->numPts = 1;
        outd->initValue = 0;
        outd->finalValue = 0;
        outd->step = 0;

        // Disable the "tran" functions found in the sources and
        // device expressions.
        ckt->initTranFuncs(0.0, 0.0);

        job->JOBrun = OP.beginPlot(outd);
        if (!job->JOBrun) {
            ckt->CKTcurrentAnalysis &= ~DOING_DCOP;
            return (E_TOOMUCH);
        }
    }
    else
        outd = (struct sOUTdata*)job->JOBoutdata;
    int error = ckt->ic();
    if (error) {
        ckt->CKTcurrentAnalysis &= ~DOING_DCOP;
        return (error);
    }
    int converged = ckt->op(MODEDCOP | MODEINITJCT, 
            MODEDCOP | MODEINITFLOAT, ckt->CKTcurTask->TSKdcMaxIter);
    if (converged != 0 || (converged = OP.pauseTest(job->JOBrun)) < 0) {
        ckt->CKTcurrentAnalysis &= ~DOING_DCOP;
        return (converged);
    }
    ckt->CKTmode = MODEDCOP | MODEINITSMSIG;
    converged = ckt->load();
    ckt->dump(0.0, job->JOBrun);
    OP.endPlot(job->JOBrun, false);

    ckt->CKTcurrentAnalysis &= ~DOING_DCOP;
    return (converged);
}

