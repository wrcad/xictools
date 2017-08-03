
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
Authors: 1987 Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/

#include "noisdefs.h"
#include "input.h"
#include "misc.h"


// .noise OUTPUT SRC {DEC OCT LIN} NP FSTART FSTOP [PTSPRSUM]
//        [ dc SRC1NAME Vstart1 [Vstop1 [Vinc1]]
//        [SRC2NAME Vstart2 [Vstop2 [Vinc2]]] ]
//
// OUTPUT can be:
//  V(x) or V(x,y)
//  voltage source name
//  i(v_source_name)
//
int
NOISEanalysis::parse(sLine *current, sCKT *ckt, int which, const char **line,
    sTASK *task)
{
    sJOB *job;
    int error = task->newAnal(which, &job);
    IP.logError(current, error);
    char *token = IP.getTok(line, true);
    if (!token) {
        IP.logError(current, "Syntax error: incomplete noise specification");
        return (OK);
    }

    IFdata ptemp;
    char buf[128];
    if ((*token == 'v' || *token == 'V') && strlen(token) == 1) {
        delete [] token;
        if (**line != '(') { 
            IP.logError(current, "Syntax error: '(' expected after 'v'");
            return (OK);
        }
        token = IP.getTok(line, false); // don't gobble
        if (!token) {
            IP.logError(current, "Syntax error: missing node name");
            return (OK);
        }
        ptemp.v.sValue = token;
        ptemp.type = IF_STRING;
        error = job->setParam("output", &ptemp);
        IP.logError(current, error);

        // See if an output reference node is specified, i.e., a
        // command of the form V(xxxx, yyyy).

        if (**line != ')') {
            token = IP.getTok(line, true);
            if (!token) {
                IP.logError(current,
                    "Syntax error: missing reference node name");
                return (OK);
            }
            ptemp.v.sValue = token;
            error = job->setParam("outputref", &ptemp);
            IP.logError(current, error);
        }
    }
    else if ((*token == 'i' || *token == 'I') && strlen(token) == 1) {
        delete [] token;
        if (**line != '(') { 
            IP.logError(current, "Syntax error: '(' expected after 'i'");
            return (OK);
        }
        token = IP.getTok(line, false); // don't gobble
        if (!token) {
            IP.logError(current, "Syntax error: missing source name");
            return (OK);
        }
        sprintf(buf, "%s#branch", token);
        delete [] token;
        token = lstring::copy(buf);

        ptemp.v.sValue = token;
        ptemp.type = IF_STRING;
        error = job->setParam("output", &ptemp);
        IP.logError(current, error);
    }
    else {
        if (!lstring::substring("#branch", token)) {
            sprintf(buf, "%s#branch", token);
            delete [] token;
            token = lstring::copy(buf);
        }

        ptemp.v.sValue = token;
        ptemp.type = IF_STRING;
        error = job->setParam("output", &ptemp);
        IP.logError(current, error);
    }

    token = IP.getTok(line, true);
    if (!token) {
        IP.logError(current, "Syntax error: missing input name");
        return (OK);
    }
    ckt->insert(&token);
    ptemp.v.uValue = token;
    ptemp.type = IF_INSTANCE;
    error = job->setParam("input", &ptemp);
    IP.logError(current, error);

    error = parseAC(current, line, job);
    IP.logError(current, error);

    if (**line) {
        ptemp.v.iValue = (int)IP.getFloat(line, &error, true); // ptspersum?
        ptemp.type = IF_INTEGER;
        if (error == 0) {
            error = job->setParam("ptspersum", &ptemp);
            IP.logError(current, error);
        }
        if (**line) {
            token = IP.getTok(line,  true);
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
    }
    if (**line)
        IP.logError(current,
            "Warning: unknown parameter in noise line, ignored");

    return (OK);
}

