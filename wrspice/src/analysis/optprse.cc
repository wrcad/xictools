
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
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1987 Thomas L. Quarles
         1992 Stephen R. Whiteley
****************************************************************************/

#include "optdefs.h"
#include "input.h"
#include "simulator.h"
#include "misc.h"

// .options parm=xxx ...


namespace {
    IFparm *find_param(const char *name)
    {
        IFparm *prm = OPTinfo.analysisParms;
        for (int i = 0; i < OPTinfo.numParms; i++) {
            if (lstring::cieq(name, prm[i].keyword))
                return (prm + i);
        }
        return (0);
    }
}


int
OPTanalysis::parse(sLine *current, sCKT*, int, const char **line, sTASK *tsk)
{
    char *pname, *value;
    while (IP.getOption(line, &pname, &value)) {
        IFparm *prm = find_param(pname);
        if (prm && (prm->dataType & IF_SET)) {
            IFdata data;
            data.type = prm->dataType;
            const char *t = value;
            if (IP.getValue(&t, &data, 0)) {
                // The tsk is actually a pointer to the sOPTIONS.
                int error = OPTinfo.setParm((sJOB*)tsk, prm->id, &data);
                if (error)
                    IP.logError(current, "Can't set option %s", pname);
            }
            else
                IP.logError(current, "Bad value %s for option %s",
                    value ? value : "(null)", pname);
        }
        delete [] pname;
        delete [] value;
    }
    return (OK);
}

