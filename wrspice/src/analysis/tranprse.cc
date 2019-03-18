
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

#include "trandefs.h"
#include "input.h"
#include "misc.h"
#include "kwords_analysis.h"


// .tran Tstep Tstop [[START] Tstart] [Tmax] [UIC]
//        [ dc|sweep SRC1NAME Vstart1 [Vstop1 [Vinc1]]
//        [SRC2NAME Vstart2 [Vstop2 [Vinc2]]] ]

int
TRANanalysis::parse(sLine *current, sCKT *ckt, int which, const char **line,
    sTASK *task)
{
    sJOB *job;
    int error = sTASK::newAnal(task, which, &job);
    IP.logError(current, error);
    error = parseTRAN(current, line, job);
    IP.logError(current, error);
    IFdata ptemp;
    while (**line) {
        char *token = IP.getTok(line, true);
        if (!token)
            break;
        if (lstring::cieq(token, kw_dc) || lstring::cieq(token, kw_sweep)) {
            delete [] token;
            error = parseDC(current, ckt, line, job, 1);
            IP.logError(current, error);
            if (**line) {
                error = parseDC(current, ckt, line, job, 2);
                IP.logError(current, error);
            }
            continue;
        }
        if (lstring::cieq(token, trkw_uic)) {
            delete [] token;
            ptemp.v.iValue = 1;
            ptemp.type = IF_FLAG;
            error = job->setParam(trkw_uic, &ptemp);
            IP.logError(current, error);
            continue;
        }
        if (lstring::cieq(token, trkw_scroll)) {
            delete [] token;
            ptemp.v.iValue = 1;
            ptemp.type = IF_FLAG;
            error = job->setParam(trkw_scroll, &ptemp);
            IP.logError(current, error);
            continue;
        }
        if (lstring::cieq(token, trkw_segment)) {
            delete [] token;
            if (!*line) {
                IP.logError(current, E_PARMVAL, trkw_segment);
                continue;
            }
            char *fb = IP.getTok(line, true);
            if (!fb || !*line) {
                IP.logError(current, E_PARMVAL, trkw_segment);
                continue;
            }
            double dtemp = IP.getFloat(line, &error, true);
            if (error) {
                IP.logError(current, E_PARMVAL, trkw_segment);
                continue;
            }
            ptemp.v.sValue = fb;
            ptemp.type = IF_STRING;
            error = job->setParam(trkw_segment, &ptemp);
            if (error) {
                IP.logError(current, error, trkw_segment);
                continue;
            }
            ptemp.v.rValue = dtemp;
            ptemp.type = IF_REAL;
            error = job->setParam(trkw_segwidth, &ptemp);
            if (error) {
                IP.logError(current, error, trkw_segment);
                continue;
            }
            continue;
        }
        if (lstring::cieq(token, trkw_tstart)) {
            // The optional "start" keyword ahead of the tstart value,
            // for HSPICE compatability.  Note that more than one step
            // value range is not supported at this time.
            delete [] token;
            double dtemp = IP.getFloat(line, &error, true);
            if (error) {
                IP.logError(current, error, trkw_tstart);
                continue;
            }
            ptemp.v.rValue = dtemp;
            ptemp.type = IF_REAL;
            error = job->setParam(trkw_tstart, &ptemp);
            if (error) {
                IP.logError(current, error, trkw_tstart);
                continue;
            }
            continue;
        }
        IP.logError(current,
            "Warning: unknown parameter in tran line, ignored");
        delete [] token;
    }
    return (0);
}


int
IFanalysis::parseTRAN(sLine *current, const char **line, sJOB *job)
{
    double vals[50];  // Should be enough, not worth the overhead of
                      // making this dynamic.
    int vcnt = 0, error;
    for (;;) {
        if (!**line)
            break;
        double dtemp = IP.getFloat(line, &error, true);
        if (error)
            break;
        vals[vcnt++] = dtemp;
        if (vcnt == 50)
            break;
    }
    if (vcnt < 2) {
        if (vcnt == 0)
            IP.logError(current, E_PARMVAL, "tstep");
        else
            IP.logError(current, E_PARMVAL, "tstop");
    }
    double tstart = 0.0, tmax = 0.0;
    if (vcnt > 2) {
        tstart = vals[2];
        vcnt--;
        for (int i = 2; i < vcnt; i++)
            vals[i] = vals[i+1];
    }
    if (vcnt > 2) {
        tmax = vals[2];
        vcnt--;
        for (int i = 2; i < vcnt; i++)
            vals[i] = vals[i+1];
    }
    IFdata ptemp;
    ptemp.v.v.vec.rVec = vals;
    ptemp.v.v.numValue = vcnt;
    ptemp.type = IF_REALVEC;
    error = job->setParam(trkw_part, &ptemp);
    IP.logError(current, error, "");

    if (tstart != 0.0) {
        ptemp.v.rValue = tstart;
        ptemp.type = IF_REAL;
        error = job->setParam(trkw_tstart, &ptemp);
        IP.logError(current, error, trkw_tstart);
    }
    if (tmax != 0.0) {
        ptemp.v.rValue = tmax;
        ptemp.type = IF_REAL;
        error = job->setParam(trkw_tmax, &ptemp);
        IP.logError(current, error, trkw_tmax);
    }

    return (0);
}

