
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
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1992 Stephen R. Whiteley
****************************************************************************/

#include "capdefs.h"


int
CAPdev::temperature(sGENmodel *genmod, sCKT *ckt)
{
    (void)ckt;
    sCAPmodel *model = static_cast<sCAPmodel*>(genmod);
    for ( ; model; model = model->next()) {

        if (!model->CAPtnomGiven)
            model->CAPtnom = ckt->CKTcurTask->TSKnomTemp;

        sCAPinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {

            if (!inst->CAPtempGiven)
                inst->CAPtemp = ckt->CKTcurTask->TSKtemp;
            if (!inst->CAPwidthGiven)
                inst->CAPwidth = model->CAPdefWidth;
            if (!inst->CAPcapGiven && !inst->CAPtree) {
                inst->CAPnomCapac = 
                    model->CAPcj * 
                        (inst->CAPwidth - model->CAPnarrow) * 
                        (inst->CAPlength - model->CAPnarrow) + 
                    model->CAPcjsw * 2 * (
                        (inst->CAPlength - model->CAPnarrow) +
                        (inst->CAPwidth - model->CAPnarrow) );
            }

            double difference = inst->CAPtemp - model->CAPtnom;
            double factor = 1.0;
            if (difference != 0.0) {
                double tc1 = inst->CAPtc1Given ?
                    inst->CAPtc1 : model->CAPtempCoeff1;
                double tc2 = inst->CAPtc2Given ?
                    inst->CAPtc2 : model->CAPtempCoeff2;
                factor += (tc1 + tc2*difference)*difference;
                inst->CAPcapac *= factor;
            }
            inst->CAPtcFactor = factor;

            if (inst->CAPtree && inst->CAPtree->num_vars() == 0) {
                // Constant expression.
                double C = 0;
                if (inst->CAPtree->eval(&C, 0, 0) == OK)
                    inst->CAPnomCapac = C;
                else
                    return (E_SYNTAX);
            }
            inst->CAPcapac = inst->CAPnomCapac * inst->CAPtcFactor;
        }
    }
    return (OK);
}

