
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
 $Id: sensaskq.cc,v 2.24 2015/07/26 17:54:52 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: UCB CAD Group
         1993 Stephen R. Whiteley
****************************************************************************/

#include "sensdefs.h"
#include "errors.h"


int 
SENSanalysis::askQuest(const sCKT*, const sJOB *anal, int which,
    IFdata *data) const
{
    const sSENSAN *job = static_cast<const sSENSAN*>(anal);
    if (!job)
        return (E_NOANAL);
    IFvalue *value = &data->v;

    switch (which) {
    case SENS_POS:
        value->nValue = (IFnode)job->SENSoutPos;
        data->type = IF_NODE;
        break;

    case SENS_NEG:
        value->nValue = (IFnode)job->SENSoutNeg;
        data->type = IF_NODE;
        break;

    case SENS_SRC:
        value->uValue = job->SENSoutSrc;
        data->type = IF_INSTANCE;
        break;

    case SENS_NAME:
        value->sValue = job->SENSoutName;
        data->type = IF_STRING;
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

