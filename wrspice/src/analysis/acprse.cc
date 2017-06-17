
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
 $Id: acprse.cc,v 2.45 2015/07/29 04:50:20 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1987 Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/

#include "acdefs.h"
#include "input.h"
#include "misc.h"

// .ac {DEC OCT LIN} NP FSTART FSTOP [ dc SRC1NAME Vstart1 [Vstop1 [Vinc1]]
//        [SRC2NAME Vstart2 [Vstop2 [Vinc2]]] ]


int
ACanalysis::parse(sLine *current, sCKT *ckt, int which, const char **line,
    sTASK *task)
{
    sJOB *job;
    int error = task->newAnal(which, &job);
    IP.logError(current, error);
    error = parseAC(current, line, job);
    IP.logError(current, error);
    if (**line) {
        char *token = IP.getTok(line, true);
        if (token && lstring::cieq(token, "dc")) {
            error = parseDC(current, ckt, line, job, 1);
            IP.logError(current, error);
            if (**line) {
                error = parseDC(current, ckt, line, job, 2);
                IP.logError(current, error);
            }
        }
        else
            IP.logError(current, "Syntax error: 'dc' expected");
        delete [] token;
    }
    if (**line)
        IP.logError(current, "Warning: unknown parameter in ac line, ignored");
    return (OK);
}


int
IFanalysis::parseAC(sLine *current, const char **line, sJOB *job)
{
    IFdata ptemp;
    ptemp.v.iValue = 1;
    ptemp.type = IF_FLAG;
    char *steptype = IP.getTok(line, true); // get DEC, OCT, or LIN
    if (!steptype || job->setParam(steptype, &ptemp))
        IP.logError(current, "Syntax error: expecting 'dec', 'oct', or 'lin'");
    delete [] steptype;

    IFdata data;
    data.type = IF_INTEGER;
    if (IP.getValue(line, &data, 0)) {
        // number of points
        int error = job->setParam("numsteps", &data);
        IP.logError(current, error);
    }
    else {
        IP.logError(current, "Syntax error: expecting number of points");
        if (!**line)
            return (OK);
    }

    double startfreq = 0.0;
    data.type = IF_REAL;
    if (IP.getValue(line, &data, 0)) {
        // fstart
        startfreq = data.v.rValue;
        int error = job->setParam("start", &data);
        IP.logError(current, error);
    }
    else {
        IP.logError(current, "Syntax error: expecting start frequency");
        if (!**line)
            return (OK);
    }

    if (IP.getValue(line, &data, 0)) {
        // fstop
        int error = job->setParam("stop", &data);
        IP.logError(current, error);
    }
    else {
        data.v.rValue = startfreq;
        int error = job->setParam("stop", &data);
        IP.logError(current, error);
    }
    return (OK);
}

