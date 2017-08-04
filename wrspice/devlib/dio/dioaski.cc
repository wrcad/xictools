
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

/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1985 Thomas L. Quarles
Modified by Dietmar Warning 2003 and Paolo Nenzi 2003
**********/

#include "diodefs.h"
#include "gencurrent.h"

// SRW - this function was rather extensively modified
//  1) call ckt->interp() for state table variables
//  2) use ac analysis functions for ac currents


int
DIOdev::askInst(const sCKT *ckt, const sGENinstance *geninst, int which,
    IFdata *data)
{
#ifdef WITH_CMP_GOTO
    // Order MUST match parameter definition enum!
    static void *array[] = {
        0, // notused
        &&L_DIO_AREA, 
        &&L_DIO_IC,
        &&L_DIO_OFF,
        &&L_DIO_TEMP,
        &&L_DIO_DTEMP,
        &&L_DIO_PJ,
        &&L_DIO_M,
        &&L_DIO_CAP,
        &&L_DIO_CURRENT,
        &&L_DIO_VOLTAGE,
        &&L_DIO_CHARGE,
        &&L_DIO_CAPCUR,
        &&L_DIO_CONDUCT,
        &&L_DIO_POWER,

        &&L_DIO_POSNODE,
        &&L_DIO_NEGNODE,
        &&L_DIO_INTNODE};

    if ((unsigned int)which > DIO_INTNODE)
        return (E_BADPARM);
#endif

    const sDIOinstance *inst = static_cast<const sDIOinstance*>(geninst);

    // Need to override this for non-real returns.
    data->type = IF_REAL;

#ifdef WITH_CMP_GOTO
    void *jmp = array[which];
    if (!jmp)
        return (E_BADPARM);
    goto *jmp;
    L_DIO_AREA:
        data->v.rValue = inst->DIOarea;
        return (OK);
    L_DIO_IC:
        data->v.rValue = inst->DIOinitCond;
        return (OK);
    L_DIO_OFF:
        data->type = IF_FLAG;
        data->v.iValue = inst->DIOoff;
        return (OK);
    L_DIO_TEMP:
        data->v.rValue = inst->DIOtemp - CONSTCtoK;
        return (OK);
    L_DIO_DTEMP:
        data->v.rValue = inst->DIOdtemp;
        return (OK);    
    L_DIO_PJ:
        data->v.rValue = inst->DIOpj;
        return (OK);
    L_DIO_M:
        data->v.rValue = inst->DIOm;
        return (OK);
    L_DIO_CAP: 
        if (ckt->CKTcurrentAnalysis & DOING_AC)
            data->v.rValue = inst->DIOcap;
        else
            data->v.rValue = ckt->interp(inst->DIOa_cap);
        return (OK);
    L_DIO_CURRENT:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            inst->ac_cd(ckt, &data->v.cValue.real, &data->v.cValue.imag);
        }
        else
            data->v.rValue = ckt->interp(inst->DIOcurrent);
        return (OK);
    L_DIO_VOLTAGE:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            data->v.cValue.real = ckt->rhsOld(inst->DIOposNode) -
                ckt->rhsOld(inst->DIOnegNode);
            data->v.cValue.imag = ckt->irhsOld(inst->DIOposNode) -
                ckt->irhsOld(inst->DIOnegNode);
        }
        else
            data->v.rValue = ckt->interp(inst->DIOvoltage);
        return (OK);
    L_DIO_CHARGE: 
        data->v.rValue = ckt->interp(inst->DIOcapCharge);
        return (OK);
    L_DIO_CAPCUR:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            double xceq = inst->DIOcap*ckt->CKTomega;
            data->v.cValue.real = -xceq* (ckt->irhsOld(inst->DIOposPrimeNode) -
                ckt->irhsOld(inst->DIOnegNode));
            data->v.cValue.imag = xceq* (ckt->rhsOld(inst->DIOposPrimeNode) -
                ckt->rhsOld(inst->DIOnegNode));
        }
        else
            data->v.rValue = ckt->interp(inst->DIOcapCurrent);
        return (OK);
    L_DIO_CONDUCT:
        if (ckt->CKTcurrentAnalysis & DOING_AC)
            data->v.rValue = inst->DIOgd;
        else
            data->v.rValue = ckt->interp(inst->DIOconduct);
        return (OK);
    L_DIO_POWER :
        if (ckt->CKTcurrentAnalysis & DOING_AC)
            data->v.rValue = 0.0;
        else
            data->v.rValue = ckt->interp(inst->DIOcurrent) *
                (ckt->rhsOld(inst->DIOposNode) - 
                ckt->rhsOld(inst->DIOnegNode));
        return (OK);
    L_DIO_POSNODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->DIOposNode;
        return (OK);
    L_DIO_NEGNODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->DIOnegNode;
        return (OK);
    L_DIO_INTNODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->DIOposPrimeNode;
        return (OK);
#else
    switch (which) {
    case DIO_AREA:
        data->v.rValue = inst->DIOarea;
        break;
    case DIO_IC:
        data->v.rValue = inst->DIOinitCond;
        break;
    case DIO_OFF:
        data->type = IF_FLAG;
        data->v.iValue = inst->DIOoff;
        break;
    case DIO_TEMP:
        data->v.rValue = inst->DIOtemp - CONSTCtoK;
        break;
    case DIO_DTEMP:
        data->v.rValue = inst->DIOdtemp;
        return(OK);    
    case DIO_PJ:
        data->v.rValue = inst->DIOpj;
        return(OK);
    case DIO_M:
        data->v.rValue = inst->DIOm;
        return(OK);
    case DIO_CAP: 
        if (ckt->CKTcurrentAnalysis & DOING_AC)
            data->v.rValue = inst->DIOcap;
        else
            data->v.rValue = ckt->interp(inst->DIOa_cap);
        break;
    case DIO_CURRENT:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            inst->ac_cd(ckt, &data->v.cValue.real, &data->v.cValue.imag);
        }
        else
            data->v.rValue = ckt->interp(inst->DIOcurrent);
        break;
    case DIO_VOLTAGE:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            data->v.cValue.real = ckt->rhsOld(inst->DIOposNode) -
                ckt->rhsOld(inst->DIOnegNode);
            data->v.cValue.imag = ckt->irhsOld(inst->DIOposNode) -
                ckt->irhsOld(inst->DIOnegNode);
        }
        else
            data->v.rValue = ckt->interp(inst->DIOvoltage);
        break;
    case DIO_CHARGE: 
        data->v.rValue = ckt->interp(inst->DIOcapCharge);
        break;
    case DIO_CAPCUR:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            double xceq = inst->DIOcap*ckt->CKTomega;
            data->v.cValue.real = -xceq* (ckt->irhsOld(inst->DIOposPrimeNode) -
                ckt->irhsOld(inst->DIOnegNode));
            data->v.cValue.imag = xceq* (ckt->rhsOld(inst->DIOposPrimeNode) -
                ckt->rhsOld(inst->DIOnegNode));
        }
        else
            data->v.rValue = ckt->interp(inst->DIOcapCurrent);
        break;
    case DIO_CONDUCT:
        if (ckt->CKTcurrentAnalysis & DOING_AC)
            data->v.rValue = inst->DIOgd;
        else
            data->v.rValue = ckt->interp(inst->DIOconduct);
        break;
    case DIO_POWER :
        if (ckt->CKTcurrentAnalysis & DOING_AC)
            data->v.rValue = 0.0;
        else
            data->v.rValue = ckt->interp(inst->DIOcurrent) *
                (ckt->rhsOld(inst->DIOposNode) - 
                ckt->rhsOld(inst->DIOnegNode));
        break;
    case DIO_POSNODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->DIOposNode;
        break;
    case DIO_NEGNODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->DIOnegNode;
        break;
    case DIO_INTNODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->DIOposPrimeNode;
        break;
    default:
        return (E_BADPARM);
    }
#endif
    return (OK);
}  


void
sDIOinstance::ac_cd(const sCKT *ckt, double *cdr, double *cdi) const
{
    if (DIOposNode != DIOposPrimeNode) {
        double gspr =
            static_cast<const sDIOmodel*>(GENmodPtr)->DIOconductance*DIOarea;
        *cdr = gspr* (ckt->rhsOld(DIOposNode) -
            ckt->rhsOld(DIOposPrimeNode));
        *cdi = gspr* (ckt->irhsOld(DIOposNode) -
            ckt->irhsOld(DIOposPrimeNode));
        return;
    }
    double geq  = DIOgd;
    double xceq = DIOcap*ckt->CKTomega;
 
    *cdr = geq* (ckt->rhsOld(DIOposPrimeNode) - ckt->rhsOld(DIOnegNode)) -
        xceq* (ckt->irhsOld(DIOposPrimeNode) - ckt->irhsOld(DIOnegNode));

    *cdi = geq* (ckt->irhsOld(DIOposPrimeNode) - ckt->irhsOld(DIOnegNode)) +
        xceq* (ckt->rhsOld(DIOposPrimeNode) - ckt->rhsOld(DIOnegNode));
}

