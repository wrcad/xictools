
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
 $Id: pzprse.cc,v 2.46 2015/07/29 04:50:20 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1987 Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/

#include "pzdefs.h"
#include "input.h"
#include "misc.h"


// .pz nodeI nodeG nodeJ nodeK {V I} {POL ZER PZ}
//        [ dc SRC1NAME Vstart1 [Vstop1 [Vinc1]]
//        [SRC2NAME Vstart2 [Vstop2 [Vinc2]]] ]

int
PZanalysis::parse(sLine *current, sCKT *ckt, int which, const char **line,
    sTASK *task)
{
    sJOB *job;
    int error = task->newAnal(which, &job);
    IP.logError(current, error);

    IFdata data;
    data.type = IF_NODE;
    IP.getValue(line, &data, ckt); 
    error = job->setParam("nodei", &data);
    IP.logError(current, error);

    IP.getValue(line, &data, ckt); 
    error = job->setParam("nodeg", &data);
    IP.logError(current, error);

    IP.getValue(line, &data, ckt); 
    error = job->setParam("nodej", &data);
    IP.logError(current, error);

    IP.getValue(line, &data, ckt); 
    error = job->setParam("nodek", &data);
    IP.logError(current, error);

    char *steptype = IP.getTok(line, true); // get V or I
    if (!steptype) {
        IP.logError(current, "missing step type");
        return (OK);
    }
    steptype[1] = '\0';
    if (*steptype == 'c' || *steptype == 'C')  // recognize "cur"
        *steptype = 'i';
    IFdata ptemp;
    ptemp.v.iValue = 1;
    ptemp.type = IF_FLAG;
    error = job->setParam(steptype, &ptemp);
    IP.logError(current, error);
    delete [] steptype;

    steptype = IP.getTok(line, true); // get POL, ZER, or PZ
    if (!steptype) {
        IP.logError(current, "missing POL/ZER");
        return (OK);
    }
    steptype[3] = '\0';  // "pole", "zero" OK
    ptemp.v.iValue = 1;
    error = job->setParam(steptype, &ptemp);
    IP.logError(current, error);
    delete [] steptype;

    // dc sweep params
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
        IP.logError(current, "Warning: unknown parameter in pz line, ignored");
    return (OK);
}

