
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
Authors: 1987 Gary W. Ng
         1992 Stephen R. Whiteley
****************************************************************************/

#include "noisdefs.h"
#include "errors.h"


int 
NOISEanalysis::askQuest(const sCKT*, const sJOB *anal, int which,
    IFdata *data) const
{
    const sNOISEAN *job = static_cast<const sNOISEAN*>(anal);
    if (!job)
        return (E_NOANAL);
    IFvalue *value = &data->v;

    switch (which) {
    case N_OUTPUT:
        value->sValue = job->Noutput;
        data->type = IF_STRING;
        break;

    case N_OUTREF:
        value->sValue = job->NoutputRef;
        data->type = IF_STRING;
        break;

    case N_INPUT:
        value->sValue = job->Ninput;
        data->type = IF_STRING;
        break;

    case N_PTSPERSUM:
        value->iValue = job->NStpsSm;
        data->type = IF_INTEGER;
        break;

    default:
        if (job->JOBac.query(which, data) == OK)
            return (OK);
        if (job->JOBdc.query(which, data) == OK)
            return (OK);
        return (E_BADPARM);
    }
    return (OK);
}

