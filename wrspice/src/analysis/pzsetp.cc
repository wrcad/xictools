
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
Authors: 1985 Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/

#include "pzdefs.h"
#include "errors.h"
#include "output.h"
#include "kwords_analysis.h"


PZanalysis PZinfo;

// PZ keywords
const char *pzkw_nodei = "nodei";
const char *pzkw_nodeg = "nodeg";
const char *pzkw_nodej = "nodej";
const char *pzkw_nodek = "nodek";
const char *pzkw_v     = "v";
const char *pzkw_i     = "i";
const char *pzkw_pol   = "pol";
const char *pzkw_zer   = "zer";
const char *pzkw_pz    = "pz";

namespace {
    IFparm PZparms[] = {
        IFparm(pzkw_nodei,      PZ_NODEI,   IF_IO|IF_NODE,
            ""),
        IFparm(pzkw_nodeg,      PZ_NODEG,   IF_IO|IF_NODE,
            ""),
        IFparm(pzkw_nodej,      PZ_NODEJ,   IF_IO|IF_NODE,
            ""),
        IFparm(pzkw_nodek,      PZ_NODEK,   IF_IO|IF_NODE,
            ""),
        IFparm(pzkw_v,          PZ_V,       IF_IO|IF_FLAG,
            ""),
        IFparm(pzkw_i,          PZ_I,       IF_IO|IF_FLAG,
            ""),
        IFparm(pzkw_pol,        PZ_POL,     IF_IO|IF_FLAG,
            ""),
        IFparm(pzkw_zer,        PZ_ZER,     IF_IO|IF_FLAG,
            ""),
        IFparm(pzkw_pz,         PZ_PZ,      IF_IO|IF_FLAG,
            ""),
        IFparm(dckw_name1,      DC_NAME1,   IF_IO|IF_INSTANCE,
            "name of source to step"),
        IFparm(dckw_start1,     DC_START1,  IF_IO|IF_REAL,
            "starting voltage/current"),
        IFparm(dckw_stop1,      DC_STOP1,   IF_IO|IF_REAL,
            "ending voltage/current"),
        IFparm(dckw_step1,      DC_STEP1,   IF_IO|IF_REAL,
            "voltage/current step"),
        IFparm(dckw_name2,      DC_NAME2,   IF_IO|IF_INSTANCE,
            "name of source to step"),
        IFparm(dckw_start2,     DC_START2,  IF_IO|IF_REAL,
            "starting voltage/current"),
        IFparm(dckw_stop2,      DC_STOP2,   IF_IO|IF_REAL,
            "ending voltage/current"),
        IFparm(dckw_step2,      DC_STEP2,   IF_IO|IF_REAL,
            "voltage/current step")
    };
}


PZanalysis::PZanalysis()
{
    name = "PZ";
    description = "pole-zero analysis";
    numParms = sizeof(PZparms)/sizeof(IFparm);
    analysisParms = PZparms;
    domain = NODOMAIN;
};


int 
PZanalysis::setParm(sJOB *anal, int which, IFdata *data)
{
    sPZAN *job = static_cast<sPZAN*>(anal);
    if (!job)
        return (E_PANIC);
    IFvalue *value = &data->v;

    switch (which) {
    case PZ_NODEI:
        job->PZin_pos = ((sCKTnode*)value->nValue)->number();
        break;

    case PZ_NODEG:
        job->PZin_neg = ((sCKTnode*)value->nValue)->number();
        break;

    case PZ_NODEJ:
        job->PZout_pos = ((sCKTnode*)value->nValue)->number();
        break;

    case PZ_NODEK:
        job->PZout_neg = ((sCKTnode*)value->nValue)->number();
        break;

    case PZ_V:
        if (value->iValue)
            job->PZinput_type = PZ_IN_VOL;
        break;

    case PZ_I:
        if (value->iValue)
            job->PZinput_type = PZ_IN_CUR;
        break;

    case PZ_POL:
        if (value->iValue)
            job->PZwhich = PZ_DO_POLES;
        break;

    case PZ_ZER:
        if (value->iValue)
            job->PZwhich = PZ_DO_ZEROS;
        break;

    case PZ_PZ:
        if (value->iValue)
            job->PZwhich = PZ_DO_POLES | PZ_DO_ZEROS;
        break;

    default:
        if (job->JOBdc.setp(which, data) == OK)
            return (OK);
        return (E_BADPARM);
    }
    return (OK);
}

