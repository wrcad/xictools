
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
 * WRspice Circuit Simulation and Analysis Tool:  Device Library          *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1987 Kanwar Jit Singh
         1992 Stephen R. Whiteley
****************************************************************************/

#include "srcdefs.h"


int
SRCdev::convTest(sGENmodel *genmod, sCKT *ckt)
{
    sSRCmodel *model = static_cast<sSRCmodel*>(genmod);
    for ( ; model; model = model->next()) {
        sSRCinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {

            if (!inst->SRCtree || !inst->SRCtree->num_vars())
                continue;

            for (int i = 0; i < inst->SRCtree->num_vars(); i++)
                inst->SRCvalues[i] = *(ckt->CKTrhsOld + inst->SRCeqns[i]);

            double rhs;
            if (inst->SRCtree->eval(&rhs, inst->SRCvalues,
                    inst->SRCderivs) == OK) {

                double prev = inst->SRCprev;
                double diff = FABS(prev - rhs);
                double tol;
                if (inst->SRCtype == SRC_V) {
                    tol = ckt->CKTcurTask->TSKreltol * 
                        SPMAX(FABS(rhs), FABS(prev)) +
                            ckt->CKTcurTask->TSKvoltTol;
                }
                else {
                    tol = ckt->CKTcurTask->TSKreltol * 
                        SPMAX(FABS(rhs), FABS(prev)) +
                            ckt->CKTcurTask->TSKabstol;
                }

                if (diff > tol) {
                    ckt->CKTnoncon++;
                    ckt->CKTtroubleElt = inst;
                    return (OK);
                }
            }
            else
                return (E_BADPARM);
        }
    }
    return (OK);
}

