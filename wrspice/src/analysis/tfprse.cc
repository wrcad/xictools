
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

#include "tfdefs.h"
#include "input.h"
#include "misc.h"
#include "kwords_analysis.h"

// .tf {vsrc v(node1[, node2])} src [ac {dec oct lin} pts fstart fstop]
//        [ dc|sweep SRC1NAME Vstart1 [Vstop1 [Vinc1]]
//        [SRC2NAME Vstart2 [Vstop2 [Vinc2]]] ]


int
TFanalysis::parse(sLine *current, sCKT *ckt, int which, const char **line,
    sTASK *task)
{
    sJOB *job;
    int error = sTASK::newAnal(task, which, &job);
    IP.logError(current, error);

    IFdata ptemp;
    char *token = IP.getTok(line, true);
    if (!token) {
        IP.logError(current, "Syntax error: empty .tf specification");
        return (OK);
    }
    // token is now either V or I or a serious error
    if ((*token == 'v' || *token == 'V') && strlen(token) == 1) {
        delete [] token;
        if (**line != '(') { 
            IP.logError(current, "Syntax error: '(' expected after 'v'");
            return (OK);
        }
        char *nname1 = IP.getTok(line, false);
        if (!nname1) {
            IP.logError(current, "Syntax error: missing node name");
            return (OK);
        }
        sCKTnode *node;
        ckt->termInsert(&nname1, &node);
        ptemp.v.nValue = node;
        ptemp.type = IF_NODE;
        error = job->setParam("outpos", &ptemp);
        IP.logError(current, error);

        if (**line != ')') {
            char *nname2 = IP.getTok(line, true);
            if (!nname2) {
                IP.logError(current,
                    "Syntax error: missing reference node name");
                return (OK);
            }
            ckt->termInsert(&nname2, &node);
            ptemp.v.nValue = node;
            error = job->setParam("outneg", &ptemp);
            IP.logError(current, error);

            int len = 5 + strlen(nname1) + strlen(nname2);
            char *cp = new char[len];
            snprintf(cp, len, "v(%s, %s)", nname1, nname2);
            ptemp.v.sValue = cp;
            ptemp.type = IF_STRING;
            error = job->setParam("outname", &ptemp);
            IP.logError(current, error);
        }
        else {
            // obtain ground node
            char *tt = lstring::copy("0");
            ckt->gndInsert(&tt, &ptemp.v.nValue);
            error = job->setParam("outneg", &ptemp);
            IP.logError(current, error);

            int len = 4 + strlen(nname1);
            char *cp = new char[len];
            snprintf(cp, len, "v(%s)", nname1);
            ptemp.v.sValue = cp;
            ptemp.type = IF_STRING;
            error = job->setParam("outname", &ptemp);
            IP.logError(current, error);
        }
    }
    else if ((*token == 'i' || *token == 'I') && strlen(token) == 1) {
        delete [] token;
        token = IP.getTok(line, true);
        if (!token) {
            IP.logError(current, "Syntax error: missing output source name");
            return (OK);
        }
        ckt->insert(&token);
        ptemp.v.uValue = token;
        ptemp.type = IF_INSTANCE;
        error = job->setParam("outsrc", &ptemp);
        IP.logError(current, error);
    }
    else {
        ckt->insert(&token);
        ptemp.v.uValue = token;
        ptemp.type = IF_INSTANCE;
        error = job->setParam("outsrc", &ptemp);
        IP.logError(current, error);
    }
    token = IP.getTok(line, true);
    ckt->insert(&token);
    ptemp.v.uValue = token;
    ptemp.type = IF_INSTANCE;
    error = job->setParam("insrc", &ptemp);
    IP.logError(current, error);
    bool had_ac = false;
    if (**line) {
        token = IP.getTok(line, true);
        if (token && lstring::cieq(token, "ac")) {
            error = parseAC(current, line, job);
            IP.logError(current, error);
            had_ac = true;
        }
        else if (token && lstring::cieq(token, kw_dc)) {
            error = parseDC(current, ckt, line, job, 1);
            IP.logError(current, error);
            if (**line) {
                error = parseDC(current, ckt, line, job, 2);
                IP.logError(current, error);
            }
        }
        else
            IP.logError(current, "Syntax error: 'ac' or 'dc' expected");
        delete [] token;
    }
    if (**line && had_ac) {
        token = IP.getTok(line, true);
        if (token) {
            if (lstring::cieq(token, kw_dc) ||
                    lstring::cieq(token, kw_sweep)) {
                error = parseDC(current, ckt, line, job, 1);
                IP.logError(current, error);
                if (**line) {
                    error = parseDC(current, ckt, line, job, 2);
                    IP.logError(current, error);
                }
            }
            else
                IP.logError(current, "Syntax error: 'dc' or 'sweep' expected");
            delete [] token;
        }
    }
    if (**line)
        IP.logError(current, "Warning: unknown parameter in tf line, ignored");
    return (OK);
}

