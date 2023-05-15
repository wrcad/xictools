
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
Authors: 1987 Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/

#include <stdio.h>
#include "dctdefs.h"
#include "input.h"


#ifdef ALLPRMS
// .dc dev[param] Vstart1 [Vstop1 [Vinc1]]
//        [dev[param] Vstart2 [Vstop2 [Vinc2]]]
#else
// .dc SRC1NAME Vstart1 [Vstop1 [Vinc1]]
//        [SRC2NAME Vstart2 [Vstop2 [Vinc2]]]
#endif


int
DCTanalysis::parse(sLine *current, sCKT *ckt, int which, const char **line,
    sTASK *task)
{
    sJOB *job;
    int error = sTASK::newAnal(task, which, &job);
    IP.logError(current, error);

    error = parseDC(current, ckt, line, job, 1);
    IP.logError(current, error);

    if (**line) {
        error = parseDC(current, ckt, line, job, 2);
        IP.logError(current, error);
    }
    if (**line)
        IP.logError(current, "Warning: unknown parameter in dc line, ignored");
    return (OK);
}


int
IFanalysis::parseDC(sLine *current, sCKT *ckt, const char **line, sJOB *job,
    int index)
{
    char *nm = IP.getTok(line, true);
    if (!nm) {
        IP.logError(current, "Syntax error: empty DC specification");
        return (OK);
    }
    IFdata ptemp;
#ifdef ALLPRMS
    (void)ckt;
    ptemp.v.sValue = nm;
    ptemp.type = IF_STRING;
#else
    ckt->insert(&nm);
    ptemp.v.uValue = nm;
    ptemp.type = IF_INSTANCE;
#endif
    char buf[32];
    snprintf(buf, sizeof(buf), "name%d", index);
    int error = job->setParam(buf, &ptemp);
    IP.logError(current, error);
#ifdef ALLPRMS
    delete [] nm;
#endif

    double startval = 0.0;
    IFdata data;
    data.type = IF_REAL;
    if (IP.getValue(line, &data, 0)) {
        // vstart
        startval = data.v.rValue;
        snprintf(buf, sizeof(buf), "start%d", index);
        error = job->setParam(buf, &data);
        IP.logError(current, error);
    }
    else {
        IP.logError(current, "Syntax error: expecting start value");
        if (!**line)
            return (OK);
    }

    if (IP.getValue(line, &data, 0)) {
        // vstop
        snprintf(buf, sizeof(buf), "stop%d", index);
        error = job->setParam(buf, &data);
        IP.logError(current, error);
    }
    else {
        data.v.rValue = startval;
        snprintf(buf, sizeof(buf), "stop%d", index);
        error = job->setParam(buf, &data);
        IP.logError(current, error);
    }

    if (IP.getValue(line, &data, 0)) {
        // vstep
        snprintf(buf, sizeof(buf), "step%d", index);
        error = job->setParam(buf, &data);
        IP.logError(current, error);
    }

    return (OK);
}

