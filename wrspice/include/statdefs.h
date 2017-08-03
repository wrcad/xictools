
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
         1992 Stephen R. Whiteley
****************************************************************************/

#ifndef STATDEFS_H
#define STATDEFS_H

#include "circuit.h"


enum {
    ST_ACCEPT       = 100,
    ST_CVCHKTIME,
    ST_EQUATIONS,
    ST_FILLIN,
    ST_INVOLCXSWITCH,
    ST_LOADTIME,
#ifdef WITH_THREADS
    ST_LOADTHRDS,
    ST_LOOPTHRDS,
#endif
    ST_LUTIME,
    ST_MATSIZE,
    ST_NONZERO,
    ST_PGFAULTS,
    ST_REJECTED,
    ST_REORDERTIME,
    ST_SOLVETIME,
    ST_TIME,
    ST_TOTITER,
    ST_TRANCURITERS,
    ST_TRANITER,
    ST_TRANITERCUT,
    ST_TRANLUTIME,
    ST_TRANOUTTIME,
    ST_TRANPOINTS,
    ST_TRANSOLVETIME,
    ST_TRANTIME,
    ST_TRANTRAPCUT,
    ST_TRANTSTIME,
    ST_VOLCXSWITCH
};

struct STATanalysis : public IFanalysis
{
    STATanalysis();
    sJOB *newAnal() { return (0); }
    int parse(sLine*, sCKT*, int, const char**, sTASK*) { return (0); }
    int setParm(sJOB*, int, IFdata*) { return (0); }
    int askQuest(const sCKT*, const sJOB*, int, IFdata*) const;
    int anFunc(sCKT*, int) { return (0); }
    virtual int delJob(sJOB*) { return (0); }
};

extern STATanalysis STATinfo;

#endif

