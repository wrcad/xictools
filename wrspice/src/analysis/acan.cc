
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
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1992 Stephen R. Whiteley
****************************************************************************/

#include "acdefs.h"
#include "device.h"
#include "outdata.h"


int
ACanalysis::anFunc(sCKT *ckt, int restart)
{
    sACAN *job = static_cast<sACAN*>(ckt->CKTcurJob);
    sCKTmodGen mgen(ckt->CKTmodels);
    for (sGENmodel *m = mgen.next(); m; m = mgen.next()) {
        if (DEV.device(m->GENmodType)->flags() & DV_NOAC) {
            OP.error(ERR_FATAL,
                "AC analysis not possible with device %s.",
                DEV.device(m->GENmodType)->name());
            return (OK);
        }
    }
    ckt->CKTcurrentAnalysis |= DOING_AC;

    if (restart) {
        if (!job->JOBoutdata)
            job->JOBoutdata = new sOUTdata;
        int error = ac_init(ckt);
        if (error) {
            ckt->CKTcurrentAnalysis &= ~DOING_AC;
            return (error);
        }
    }
    int error = job->JOBdc.loop(ac_dcoperation, ckt, restart);
    if (error < 0) {
        // pause
        ckt->CKTcurrentAnalysis &= ~DOING_AC;
        return (error);
    }

    OP.endPlot(job->JOBrun, false);
    ckt->CKTcurrentAnalysis &= ~DOING_AC;
    return (error);
}


// Static private function.
//
int
ACanalysis::ac_dcoperation(sCKT *ckt, int restart)
{
    int error = ckt->ic();
    if (error)
        return (error);

    error = ckt->op(MODEDCOP | MODEINITJCT, 
        MODEDCOP | MODEINITFLOAT, ckt->CKTcurTask->TSKdcMaxIter);
    if (error)
        return (error);

    ckt->CKTmode = MODEDCOP | MODEINITSMSIG;
    error = ckt->load();
    if (error)
        return (error);

    ckt->CKTniState |= NIACSHOULDREORDER;  // KLU requires this.
    error = ((sACAN*)ckt->CKTcurJob)->JOBac.loop(ac_operation, ckt, restart);
    if (error)
        return (error);

    return (OK);
}


// Static private function.
//
int
ACanalysis::ac_init(sCKT *ckt)
{
    sACAN *job = static_cast<sACAN*>(ckt->CKTcurJob);
    sOUTdata *outd = job->JOBoutdata;

    int error = job->JOBdc.init(ckt);
    if (error)
        return (error);
    int multip = job->JOBdc.points(ckt);

    error = ckt->names(&outd->numNames, &outd->dataNames);
    if (error)
        return (error);

    ckt->newUid(&outd->refName, 0, "frequency", UID_OTHER);

    outd->numPts = job->points(ckt);

    outd->circuitPtr = ckt;
    outd->analysisPtr = ckt->CKTcurJob;
    outd->analName = ckt->CKTcurJob->JOBname;
    outd->refType = IF_REAL;
    outd->dataType = IF_COMPLEX;
    outd->initValue = ((sACAN*)ckt->CKTcurJob)->JOBac.fstart();
    outd->finalValue = ((sACAN*)ckt->CKTcurJob)->JOBac.fstop();
    outd->step = 0;
    outd->count = 0;
    job->JOBrun = OP.beginPlot(outd, multip);
    if (!job->JOBrun)
        return (E_TOOMUCH);
    return (OK);
}


// Static private function.
//
int
ACanalysis::ac_operation(sCKT *ckt, int)
{
    int error = ckt->NIacIter();
    if (error)
        return (error);

    ckt->acDump(ckt->CKTomega/(2*M_PI), ckt->CKTcurJob->JOBrun);
    return (OK);
}

