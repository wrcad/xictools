
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
 $Id: pzaskq.cc,v 2.25 2015/07/29 04:50:20 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/

#include "pzdefs.h"
#include "errors.h"


int 
PZanalysis::askQuest(const sCKT *ckt, const sJOB *anal, int which,
    IFdata *data) const
{
    if (!ckt)
        return (E_NOCKT);
    const sPZAN *job = static_cast<const sPZAN*>(anal);
    if (!job)
        return (E_NOANAL);
    IFvalue *value = &data->v;

    switch (which) {
    case PZ_NODEI:
        value->nValue = (IFnode)ckt->num2node(job->PZin_pos);
        data->type = IF_NODE;
        break;

    case PZ_NODEG:
        value->nValue = (IFnode)ckt->num2node(job->PZin_neg);
        data->type = IF_NODE;
        break;

    case PZ_NODEJ:
        value->nValue = (IFnode)ckt->num2node(job->PZout_pos);
        data->type = IF_NODE;
        break;

    case PZ_NODEK:
        value->nValue = (IFnode)ckt->num2node(job->PZout_neg);
        data->type = IF_NODE;
        break;

    case PZ_V:
        if (job->PZinput_type == PZ_IN_VOL)
            value->iValue=1;
        else
            value->iValue=0;
        data->type = IF_FLAG;
        break;

    case PZ_I:
        if (job->PZinput_type == PZ_IN_CUR)
            value->iValue=1;
        else
            value->iValue=0;
        data->type = IF_FLAG;
        break;

    case PZ_POL:
        if (job->PZwhich == PZ_DO_POLES)
            value->iValue=1;
        else
            value->iValue=0;
        data->type = IF_FLAG;
        break;

    case PZ_ZER:
        if (job->PZwhich == PZ_DO_ZEROS)
            value->iValue=1;
        else
            value->iValue=0;
        data->type = IF_FLAG;
        break;

    case PZ_PZ:
        if (job->PZwhich == (PZ_DO_POLES | PZ_DO_ZEROS))
            value->iValue=1;
        else
            value->iValue=0;
        data->type = IF_FLAG;
        break;

    default:
        if (job->JOBdc.query(which, data) == OK)
            return (OK);
        return (E_BADPARM);
    }
    return (OK);
}

