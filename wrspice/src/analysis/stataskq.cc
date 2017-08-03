
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
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/

#include "statdefs.h"
#include "errors.h"


int
STATanalysis::askQuest(const sCKT *ckt, const sJOB*, int which,
    IFdata *data) const
{
    if (!ckt)
        return (E_NOCKT);
    const sSTATS *stat = ckt->CKTstat;
    IFvalue *value = &data->v;

    switch (which) {
    case ST_ACCEPT:
        value->iValue = stat->STATaccepted;
        data->type = IF_INTEGER;
        break;
    case ST_CVCHKTIME:
        value->rValue = stat->STATcvChkTime;
        data->type = IF_REAL;
        break;
    case ST_EQUATIONS:
        value->iValue = ckt->CKTnodeTab.numNodes();
        data->type = IF_INTEGER;
        break;
    case ST_FILLIN:
        value->iValue = stat->STATfillIn;
        data->type = IF_INTEGER;
        break;
    case ST_INVOLCXSWITCH:
        value->iValue = stat->STATinvolCxSwitch;
        data->type = IF_INTEGER;
        break;
    case ST_LOADTIME:
        value->rValue = stat->STATloadTime;
        data->type = IF_REAL;
        break;
#ifdef WITH_THREADS
    case ST_LOADTHRDS:
        value->iValue = stat->STATloadThreads;
        data->type = IF_INTEGER;
        break;
    case ST_LOOPTHRDS:
        value->iValue = stat->STATloopThreads;
        data->type = IF_INTEGER;
        break;
#endif
    case ST_LUTIME:
        value->rValue = stat->STATdecompTime;
        data->type = IF_REAL;
        break;
    case ST_MATSIZE:
        value->iValue = stat->STATmatSize;
        data->type = IF_INTEGER;
        break;
    case ST_NONZERO:
        value->iValue = stat->STATnonZero;
        data->type = IF_INTEGER;
        break;
    case ST_PGFAULTS:
        value->iValue = stat->STATpageFaults;
        data->type = IF_INTEGER;
        break;
    case ST_REJECTED:
        value->iValue = stat->STATrejected;
        data->type = IF_INTEGER;
        break;
    case ST_REORDERTIME:
        value->rValue = stat->STATreorderTime;
        data->type = IF_REAL;
        break;
    case ST_SOLVETIME:
        value->rValue = stat->STATsolveTime;
        data->type = IF_REAL;
        break;
    case ST_TIME:
        value->rValue = stat->STATtotAnalTime;
        data->type = IF_REAL;
        break;
    case ST_TOTITER:
        value->iValue = stat->STATnumIter;
        data->type = IF_INTEGER;
        break;
    case ST_TRANCURITERS:
        value->iValue = stat->STATtranLastIter;
        data->type = IF_INTEGER;
        break;
    case ST_TRANITER:
        value->iValue = stat->STATtranIter;
        data->type = IF_INTEGER;
        break;
    case ST_TRANITERCUT:
        value->iValue = stat->STATtranIterCut;
        data->type = IF_INTEGER;
        break;
    case ST_TRANLUTIME:
        value->rValue = stat->STATtranDecompTime;
        data->type = IF_REAL;
        break;
    case ST_TRANOUTTIME:
        value->rValue = stat->STATtranOutTime;
        data->type = IF_REAL;
        break;
    case ST_TRANPOINTS:
        value->iValue = stat->STATtimePts;
        data->type = IF_INTEGER;
        break;
    case ST_TRANSOLVETIME:
        value->rValue = stat->STATtranSolveTime;
        data->type = IF_REAL;
        break;
    case ST_TRANTIME:
        value->rValue = stat->STATtranTime;
        data->type = IF_REAL;
        break;
    case ST_TRANTRAPCUT:
        value->iValue = stat->STATtranTrapCut;
        data->type = IF_INTEGER;
        break;
    case ST_TRANTSTIME:
        value->rValue = stat->STATtranTsTime;
        data->type = IF_REAL;
        break;
    case ST_VOLCXSWITCH:
        value->iValue = stat->STATvolCxSwitch;
        data->type = IF_INTEGER;
        break;
    default:
        return (E_BADPARM);
    }
    return (OK);
}

