
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
Authors: 1988 Jaijeet S Roychowdhury
         1993 Stephen R. Whiteley
****************************************************************************/

#include "distdefs.h"
#include "errors.h"


int 
DISTOanalysis::askQuest(const sCKT*, const sJOB *anal, int which,
    IFdata *data) const
{
    const sDISTOAN *job = static_cast<const sDISTOAN*>(anal);
    if (!job)
        return (E_NOANAL);
    IFvalue *value = &data->v;

    switch (which) {
    case D_START:
        value->rValue = job->DstartF1;
        data->type = IF_REAL;
        break;

    case D_STOP:
        value->rValue = job->DstopF1 ;
        data->type = IF_REAL;
        break;

    case D_STEPS:
        value->iValue = job->DnumSteps;
        data->type = IF_INTEGER;
        break;

    case D_DEC:
        if (job->DstepType == DECADE)
            value->iValue=1;
        else
            value->iValue=0;
        data->type = IF_FLAG;
        break;

    case D_OCT:
        if (job->DstepType == OCTAVE)
            value->iValue=1;
        else
            value->iValue=0;
        data->type = IF_FLAG;
        break;

    case D_LIN:
        if (job->DstepType == LINEAR)
            value->iValue=1;
        else
            value->iValue=0;
        data->type = IF_FLAG;
        break;

    case D_F2OVRF1:
        value->rValue = job->Df2ovrF1;
        data->type = IF_REAL;
        break;

    default:
        return (E_BADPARM);
    }
    return (OK);
}

