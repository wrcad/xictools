
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
Authors: 1985 Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/

#include "tfdefs.h"
#include "errors.h"


int 
TFanalysis::askQuest(const sCKT*, const sJOB *anal, int which,
    IFdata *data) const
{
    const sTFAN *job = static_cast<const sTFAN*>(anal);
    if (!job)
        return (E_NOANAL);
    IFvalue *value = &data->v;

    switch (which) {
    case TF_OUTPOS:
        value->nValue = (IFnode)job->TFoutPos;
        data->type = IF_NODE;
        break;

    case TF_OUTNEG:
        value->nValue = (IFnode)job->TFoutNeg;
        data->type = IF_NODE;
        break;

    case TF_OUTNAME:
        value->sValue = job->TFoutName;
        data->type = IF_STRING;
        break;

    case TF_OUTSRC:
        value->uValue = job->TFoutSrc;
        data->type = IF_INSTANCE;
        break;

    case TF_INSRC:
        value->uValue = job->TFinSrc;
        data->type = IF_INSTANCE;
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

