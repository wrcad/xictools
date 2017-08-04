
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

#include "resdefs.h"


// Perform the temperature update to the resistors, calculate the
// conductance as a function of the given nominal and current
// temperatures - the resistance given in the struct is the nominal
// temperature resistance.


int
RESdev::temperature(sGENmodel *genmod, sCKT *ckt)
{
    double gmax = ckt->CKTcurTask->TSKgmax;
    if (gmax <= 0.0)
        gmax = RES_GMAX;

    sRESmodel *model = static_cast<sRESmodel*>(genmod);
    for ( ; model; model = model->next()) {

        // Default Value Processing for Resistor Models
        if (!model->REStnomGiven)
            model->REStnom = ckt->CKTcurTask->TSKnomTemp;
        if (!model->REStempGiven)
            model->REStemp = ckt->CKTcurTask->TSKtemp;
        if (!model->RESsheetResGiven)
            model->RESsheetRes = 0.0;
        if (!model->RESdefWidthGiven)
            model->RESdefWidth = 0.0;
        if (!model->REStc1Given)
            model->REStempCoeff1 = 0.0;
        if (!model->REStc2Given)
            model->REStempCoeff2 = 0.0;
        if (!model->RESnarrowGiven)
            model->RESnarrow = 0.0;
        if (!model->RESshortenGiven)
            model->RESshorten = model->RESnarrow;
        if (!model->RESdefLengthGiven)
            model->RESdefLength = 0.0;
        if (!model->RESkfGiven)
            model->RESkf = 0.0;
        else if (model->RESkf < 0.0) {
            DVO.textOut(OUT_FATAL, "%s: KF must be >= 0", model->GENmodName);
            return (E_BADPARM);
        }
        if (!model->RESafGiven)
            model->RESaf = 2.0;
        else if (model->RESaf <= 0.0) {
            DVO.textOut(OUT_FATAL, "%s: AF must be > 0", model->GENmodName);
            return (E_BADPARM);
        }
        if (!model->RESefGiven)
            model->RESef = 1.0;
        else if (model->RESef <= 0.0) {
            DVO.textOut(OUT_FATAL, "%s: EF must be > 0", model->GENmodName);
            return (E_BADPARM);
        }
        if (!model->RESwfGiven)
            model->RESwf = 1.0;
        else if (model->RESwf <= 0.0) {
            DVO.textOut(OUT_FATAL, "%s: WF must be > 0", model->GENmodName);
            return (E_BADPARM);
        }
        if (!model->RESlfGiven)
            model->RESlf = 1.0;
        else if (model->RESlf <= 0.0) {
            DVO.textOut(OUT_FATAL, "%s: LF must be > 0", model->GENmodName);
            return (E_BADPARM);
        }
        if (!model->RESnoiseGiven)
            model->RESnoise = 1.0;
        else if (model->RESnoise < 0.0) {
            DVO.textOut(OUT_FATAL, "%s: NOISE must be >= 0",model->GENmodName);
            return (E_BADPARM);
        }

        sRESinstance *inst;
#ifdef USE_PRELOAD
        // We will move the instances with a tree and variables to the
        // front of the list, so we can easily skip the rest when
        // loading.

        sRESinstance *prev_inst = 0, *next_inst;
        for (inst = model->inst(); inst; inst = next_inst) {
            next_inst = inst->next();
#else
        for (inst = model->inst(); inst; inst = inst->next()) {
#endif
            
            // Default Value Processing for Resistor Instance
            if (!inst->REStempGiven)
                inst->REStemp = model->REStemp;
            if (!inst->RESwidthGiven)
                inst->RESwidth = model->RESdefWidth;
            if (!inst->RESlengthGiven)
                inst->RESlength = model->RESdefLength;
            if (!inst->RESnoiseGiven)
                inst->RESnoise = model->RESnoise;
            else if (inst->RESnoise < 0) {
                DVO.textOut(OUT_FATAL,
                    "%s: NOISE must be >= 0", inst->GENname);
                return (E_BADPARM);
            }

            bool res_given = inst->RESresGiven;

            if (!inst->RESresGiven && !inst->RESpolyCoeffs && !inst->REStree)  {
                if (model->RESsheetResGiven && model->RESsheetRes != 0) {
                    double el = inst->RESlength - model->RESshorten;
                    if (el <= 0.0) {
                        DVO.textOut(OUT_FATAL,
                            "%s: effective length is <= 0", inst->GENname);
                        return (E_BADPARM);
                    }
                    double ew = inst->RESwidth - model->RESnarrow;
                    if (ew <= 0.0) {
                        DVO.textOut(OUT_FATAL,
                            "%s: effective width is <= 0", inst->GENname);
                        return (E_BADPARM);
                    }
                    inst->RESresist = model->RESsheetRes*el/ew;
                    res_given = true;
                }
                else {
                    DVO.textOut(OUT_FATAL,
                        "%s: resistance not defined", inst->GENname);
                    return (E_BADPARM);
                }
            }

            double difference = inst->REStemp - model->REStnom;
            double factor = 1.0;
            if (difference != 0.0) {
                double tc1 = inst->REStc1Given ?
                    inst->REStc1 : model->REStempCoeff1;
                double tc2 = inst->REStc2Given ?
                    inst->REStc2 : model->REStempCoeff2;
                factor += (tc1 + tc2*difference)*difference;
            }
            inst->REStcFactor = factor;

            if (inst->REStree && inst->REStree->num_vars() == 0) {
                // Constant expression.
                double R = 0;
                if (inst->REStree->eval(&R, 0, 0) == OK) {
                    inst->RESresist = R;
                    res_given = true;
                }
                else
                    return (E_SYNTAX);
            }
            if (res_given) {
                double G = 1.0/(inst->RESresist * factor);
                if (G > gmax) {
                    G = gmax;
                    DVO.textOut(OUT_WARNING,
                        "%s: resistance reset to %g by gmax limiting.",
                        (const char*)inst->GENname, 1.0/gmax);
                }
                else if (G < -gmax) {
                    G = -gmax;
                    DVO.textOut(OUT_WARNING,
                        "%s: resistance reset to %g by gmax limiting.",
                        (const char*)inst->GENname, -1.0/gmax);
                }
                inst->RESconduct = G;
            }

#ifdef USE_PRELOAD
            if (ckt->CKTpreload) {
                // preload constants
                if (inst->RESconduct != 0.0) {
                    ckt->preldadd(inst->RESposPosptr, inst->RESconduct);
                    ckt->preldadd(inst->RESnegNegptr, inst->RESconduct);
                    ckt->preldadd(inst->RESposNegptr, -inst->RESconduct);
                    ckt->preldadd(inst->RESnegPosptr, -inst->RESconduct);
                }
            }

            if (inst->RESpolyCoeffs || (inst->REStree &&
                    inst->REStree->num_vars() > 0)) {
                // Move these to the top of the list.
                if (prev_inst) {
                    prev_inst->GENnextInstance = next_inst;
                    inst->GENnextInstance = model->GENinstances;
                    model->GENinstances = inst;
                    continue;
                }
            }
            prev_inst = inst;
#endif
        }
    }
    return (OK);
}

