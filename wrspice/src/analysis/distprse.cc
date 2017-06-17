
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
 $Id: distprse.cc,v 2.43 2015/07/29 04:50:20 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1987 Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/

#include "distdefs.h"
#include "input.h"
#include "errors.h"

// .disto {DEC OCT LIN} NP FSTART FSTOP <F2OVERF1>


int
DISTOanalysis::parse(sLine *current, sCKT*, int which, const char **line,
    sTASK *task)
{
    sJOB *job;
    int error = task->newAnal(which, &job);
    IP.logError(current, error);
    error = parseAC(current, line, job);
    IP.logError(current, error);

    if (**line) {
        IFdata data;
        data.type = IF_REAL;
        IP.getValue(line, &data, 0); // f1phase
        error = job->setParam("f2overf1", &data);
        IP.logError(current, error);
    }
    if (**line)
        IP.logError(current,
            "Warning: unknown parameter in disto line, ignored");
    return (0);
}

