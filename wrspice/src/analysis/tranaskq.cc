
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

#include "trandefs.h"
#include "errors.h"


int 
TRANanalysis::askQuest(const sCKT*, const sJOB *anal, int which,
    IFdata *data) const
{
    const sTRANAN *job = static_cast<const sTRANAN*>(anal);
    if (!job)
        return (E_NOANAL);
    IFvalue *value = &data->v;

    switch (which) {
    case TRAN_PARTS:
        {
            if (!job->TRANspec) {
                double *v = new double[2];
                v[0] = v[1] = 0.0;
                value->v.vec.rVec = v;
                value->v.numValue = 2;
            }
            else {
                double *v = new double[job->TRANspec->nparts * 2];
                for (int i = 0; i < job->TRANspec->nparts; i++) {
                    v[2*i] = job->TRANspec->step(i);
                    v[2*i + 1] = job->TRANspec->end(i);
                }
                value->v.vec.rVec = v;
                value->v.numValue = 2*job->TRANspec->nparts;
            }
            value->v.freeVec = true;
            data->type = IF_REALVEC;
        }
        break;

    case TRAN_TSTART:
        if (!job->TRANspec)
            value->rValue = 0.0;
        else
            value->rValue = job->TRANspec->tstart;
        data->type = IF_REAL;
        break;

    case TRAN_TMAX:
        value->rValue = job->TRANmaxStep;
        data->type = IF_REAL;
        break;

    case TRAN_UIC:
        if (job->TRANmode & MODEUIC)
            value->iValue = 1;
        else
            value->iValue = 0;
        data->type = IF_FLAG;
        break;

    case TRAN_SCROLL:
        if (job->TRANmode & MODESCROLL)
            value->iValue = 1;
        else
            value->iValue = 0;
        data->type = IF_FLAG;
        break;

    default:
        if (job->JOBdc.query(which, data) == OK)
            return (OK);
        return (E_BADPARM);
    }
    return (OK);
}

