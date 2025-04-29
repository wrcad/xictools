
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
Authors: 1988 Hong J. Park
         1993 Stephen R. Whiteley
****************************************************************************/

#include "b1defs.h"

#define MSC(xx) inst->B1m*(xx)


int
B1dev::askInst(const sCKT *ckt, const sGENinstance *geninst, int which,
    IFdata *data)
{
    const sB1instance *inst = static_cast<const sB1instance*>(geninst);

    // Need to override this for non-real returns.
    data->type = IF_REAL;

    switch (which) {
    case BSIM1_M:
        data->v.rValue = inst->B1m;
        break;
    case BSIM1_L:
        data->v.rValue = inst->B1l;
        break;
    case BSIM1_W:
        data->v.rValue = inst->B1w;
        break;
    case BSIM1_AS:
        data->v.rValue = inst->B1sourceArea;
        break;
    case BSIM1_AD:
        data->v.rValue = inst->B1drainArea;
        break;
    case BSIM1_PS:
        data->v.rValue = inst->B1sourcePerimeter;
        break;
    case BSIM1_PD:
        data->v.rValue = inst->B1drainPerimeter;
        break;
    case BSIM1_NRS:
        data->v.rValue = inst->B1sourceSquares;
        break;
    case BSIM1_NRD:
        data->v.rValue = inst->B1drainSquares;
        break;
    case BSIM1_OFF:
        data->type = IF_FLAG;
        data->v.iValue = inst->B1off;
        break;
    case BSIM1_IC_VBS:
        data->v.rValue = inst->B1icVBS;
        break;
    case BSIM1_IC_VDS:
        data->v.rValue = inst->B1icVDS;
        break;
    case BSIM1_IC_VGS:
        data->v.rValue = inst->B1icVGS;
        break;
    case BSIM1_VBD:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            data->v.cValue.real = ckt->rhsOld(inst->B1bNode) -
                ckt->rhsOld(inst->B1dNode);
            data->v.cValue.imag = ckt->irhsOld(inst->B1bNode) -
                ckt->irhsOld(inst->B1dNode);
        }
        else
            data->v.rValue = ckt->interp(inst->B1vbd);
        break;
    case BSIM1_VBS:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            data->v.cValue.real = ckt->rhsOld(inst->B1bNode) -
                ckt->rhsOld(inst->B1sNode);
            data->v.cValue.imag = ckt->irhsOld(inst->B1bNode) -
                ckt->irhsOld(inst->B1sNode);
        }
        else
            data->v.rValue = ckt->interp(inst->B1vbs);
        break;
    case BSIM1_VGS:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            data->v.cValue.real = ckt->rhsOld(inst->B1gNode) -
                ckt->rhsOld(inst->B1sNode);
            data->v.cValue.imag = ckt->irhsOld(inst->B1gNode) -
                ckt->irhsOld(inst->B1sNode);
        }
        else
            data->v.rValue = ckt->interp(inst->B1vgs);
        break;
    case BSIM1_VDS:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            data->v.cValue.real = ckt->rhsOld(inst->B1dNode) -
                ckt->rhsOld(inst->B1sNode);
            data->v.cValue.imag = ckt->irhsOld(inst->B1dNode) -
                ckt->irhsOld(inst->B1sNode);
        }
        else
            data->v.rValue = ckt->interp(inst->B1vds);
        break;
    case BSIM1_VON:
        data->v.rValue = ckt->interp(inst->B1vono); 
        break;
    case BSIM1_CD:
        data->v.rValue = MSC(ckt->interp(inst->B1cd)); 
        break;
    case BSIM1_CBS:
        data->v.rValue = MSC(ckt->interp(inst->B1cbs)); 
        break;
    case BSIM1_CBD:
        data->v.rValue = MSC(ckt->interp(inst->B1cbd)); 
        break;
    case BSIM1_SOURCECOND:
        data->v.rValue = MSC(inst->B1sourceConductance);
        break;
    case BSIM1_DRAINCOND:
        data->v.rValue = MSC(inst->B1drainConductance);
        break;
    case BSIM1_GM:
        data->v.rValue = MSC(ckt->interp(inst->B1gm));
        break;
    case BSIM1_GDS:
        data->v.rValue = MSC(ckt->interp(inst->B1gds));
        break;
    case BSIM1_GMBS:
        data->v.rValue = MSC(ckt->interp(inst->B1gmbs));
        break;
    case BSIM1_GBD:
        data->v.rValue = MSC(ckt->interp(inst->B1gbd));
        break;
    case BSIM1_GBS:
        data->v.rValue = MSC(ckt->interp(inst->B1gbs));
        break;
    case BSIM1_QB:
        data->v.rValue = MSC(ckt->interp(inst->B1qb));
        break;
    case BSIM1_QG:
        data->v.rValue = MSC(ckt->interp(inst->B1qg));
        break;
    case BSIM1_QD:
        data->v.rValue = MSC(ckt->interp(inst->B1qd));
        break;
    case BSIM1_QBS:
        data->v.rValue = MSC(ckt->interp(inst->B1qbs));
        break;
    case BSIM1_QBD:
        data->v.rValue = MSC(ckt->interp(inst->B1qbd));
        break;
    case BSIM1_CQB:
        data->v.rValue = MSC(ckt->interp(inst->B1cqb));
        break;
    case BSIM1_CQG:
        data->v.rValue = MSC(ckt->interp(inst->B1cqg));
        break;
    case BSIM1_CQD:
        data->v.rValue = MSC(ckt->interp(inst->B1cqd));
        break;
    case BSIM1_CGG:
        data->v.rValue = MSC(ckt->interp(inst->B1cggb));
        break;
    case BSIM1_CGD:
        data->v.rValue = MSC(ckt->interp(inst->B1cgdb));
        break;
    case BSIM1_CGS:
        data->v.rValue = MSC(ckt->interp(inst->B1cgsb));
        break;
    case BSIM1_CBG:
        data->v.rValue = MSC(ckt->interp(inst->B1cbgb));
        break;
    case BSIM1_CAPBD:
        data->v.rValue = MSC(ckt->interp(inst->B1capbd));
        break;
    case BSIM1_CQBD:
        data->v.rValue = MSC(ckt->interp(inst->B1cqbd));
        break;
    case BSIM1_CAPBS:
        data->v.rValue = MSC(ckt->interp(inst->B1capbs));
        break;
    case BSIM1_CQBS:
        data->v.rValue = MSC(ckt->interp(inst->B1cqbs));
        break;
    case BSIM1_CDG:
        data->v.rValue = MSC(ckt->interp(inst->B1cdgb));
        break;
    case BSIM1_CDD:
        data->v.rValue = MSC(ckt->interp(inst->B1cddb));
        break;
    case BSIM1_CDS:
        data->v.rValue = MSC(ckt->interp(inst->B1cdsb));
        break;
    case BSIM1_DNODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->B1dNode;
        break;
    case BSIM1_GNODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->B1gNode;
        break;
    case BSIM1_SNODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->B1sNode;
        break;
    case BSIM1_BNODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->B1bNode;
        break;
    case BSIM1_DNODEPRIME:
        data->type = IF_INTEGER;
        data->v.iValue = inst->B1dNodePrime;
        break;
    case BSIM1_SNODEPRIME:
        data->type = IF_INTEGER;
        data->v.iValue = inst->B1sNodePrime;
        break;
    default:
        return (E_BADPARM);
    }
    return (OK);
}
