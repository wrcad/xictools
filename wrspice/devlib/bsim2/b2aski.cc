
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
 * Device Library                                                         *
 *                                                                        *
 *========================================================================*
 $Id: b2aski.cc,v 1.4 2015/07/26 01:09:10 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1988 Hong J. Park
         1993 Stephen R. Whiteley
****************************************************************************/

#include "b2defs.h"

#define MSC(xx) inst->B2m*(xx)


int
B2dev::askInst(const sCKT *ckt, const sGENinstance *geninst, int which,
    IFdata *data)
{
    const sB2instance *inst = static_cast<const sB2instance*>(geninst);

    // Need to override this for non-real returns.
    data->type = IF_REAL;

    switch (which) {
    case BSIM2_M:
        data->v.rValue = inst->B2m;
        break;
    case BSIM2_L:
        data->v.rValue = inst->B2l;
        break;
    case BSIM2_W:
        data->v.rValue = inst->B2w;
        break;
    case BSIM2_AS:
        data->v.rValue = inst->B2sourceArea;
        break;
    case BSIM2_AD:
        data->v.rValue = inst->B2drainArea;
        break;
    case BSIM2_PS:
        data->v.rValue = inst->B2sourcePerimeter;
        break;
    case BSIM2_PD:
        data->v.rValue = inst->B2drainPerimeter;
        break;
    case BSIM2_NRS:
        data->v.rValue = inst->B2sourceSquares;
        break;
    case BSIM2_NRD:
        data->v.rValue = inst->B2drainSquares;
        break;
    case BSIM2_OFF:
        data->type = IF_FLAG;
        data->v.iValue = inst->B2off;
        break;
    case BSIM2_IC_VBS:
        data->v.rValue = inst->B2icVBS;
        break;
    case BSIM2_IC_VDS:
        data->v.rValue = inst->B2icVDS;
        break;
    case BSIM2_IC_VGS:
        data->v.rValue = inst->B2icVGS;
        break;
    case BSIM2_VBD:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            data->v.cValue.real = ckt->rhsOld(inst->B2bNode) -
                ckt->rhsOld(inst->B2dNode);
            data->v.cValue.imag = ckt->irhsOld(inst->B2bNode) -
                ckt->irhsOld(inst->B2dNode);
        }
        else
            data->v.rValue = ckt->interp(inst->B2vbd);
        break;
    case BSIM2_VBS:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            data->v.cValue.real = ckt->rhsOld(inst->B2bNode) -
                ckt->rhsOld(inst->B2sNode);
            data->v.cValue.imag = ckt->irhsOld(inst->B2bNode) -
                ckt->irhsOld(inst->B2sNode);
        }
        else
            data->v.rValue = ckt->interp(inst->B2vbs);
        break;
    case BSIM2_VGS:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            data->v.cValue.real = ckt->rhsOld(inst->B2gNode) -
                ckt->rhsOld(inst->B2sNode);
            data->v.cValue.imag = ckt->irhsOld(inst->B2gNode) -
                ckt->irhsOld(inst->B2sNode);
        }
        else
            data->v.rValue = ckt->interp(inst->B2vgs);
        break;
    case BSIM2_VDS:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            data->v.cValue.real = ckt->rhsOld(inst->B2dNode) -
                ckt->rhsOld(inst->B2sNode);
            data->v.cValue.imag = ckt->irhsOld(inst->B2dNode) -
                ckt->irhsOld(inst->B2sNode);
        }
        else
            data->v.rValue = ckt->interp(inst->B2vds);
        break;
    case BSIM2_VON:
        data->v.rValue = ckt->interp(inst->B2vono); 
        break;
    case BSIM2_CD:
        data->v.rValue = MSC(ckt->interp(inst->B2cd)); 
        break;
    case BSIM2_CBS:
        data->v.rValue = MSC(ckt->interp(inst->B2cbs)); 
        break;
    case BSIM2_CBD:
        data->v.rValue = MSC(ckt->interp(inst->B2cbd)); 
        break;
    case BSIM2_SOURCECOND:
        data->v.rValue = MSC(inst->B2sourceConductance);
        break;
    case BSIM2_DRAINCOND:
        data->v.rValue = MSC(inst->B2drainConductance);
        break;
    case BSIM2_GM:
        data->v.rValue = MSC(ckt->interp(inst->B2gm)); 
        break;
    case BSIM2_GDS:
        data->v.rValue = MSC(ckt->interp(inst->B2gds)); 
        break;
    case BSIM2_GMBS:
        data->v.rValue = MSC(ckt->interp(inst->B2gmbs)); 
        break;
    case BSIM2_GBD:
        data->v.rValue = MSC(ckt->interp(inst->B2gbd)); 
        break;
    case BSIM2_GBS:
        data->v.rValue = MSC(ckt->interp(inst->B2gbs)); 
        break;
    case BSIM2_QB:
        data->v.rValue = MSC(ckt->interp(inst->B2qb)); 
        break;
    case BSIM2_QG:
        data->v.rValue = MSC(ckt->interp(inst->B2qg)); 
        break;
    case BSIM2_QD:
        data->v.rValue = MSC(ckt->interp(inst->B2qd)); 
        break;
    case BSIM2_QBS:
        data->v.rValue = MSC(ckt->interp(inst->B2qbs)); 
        break;
    case BSIM2_QBD:
        data->v.rValue = MSC(ckt->interp(inst->B2qbd)); 
        break;
    case BSIM2_CQB:
        data->v.rValue = MSC(ckt->interp(inst->B2cqb)); 
        break;
    case BSIM2_CQG:
        data->v.rValue = MSC(ckt->interp(inst->B2cqg)); 
        break;
    case BSIM2_CQD:
        data->v.rValue = MSC(ckt->interp(inst->B2cqd)); 
        break;
    case BSIM2_CGG:
        data->v.rValue = MSC(ckt->interp(inst->B2cggb)); 
        break;
    case BSIM2_CGD:
        data->v.rValue = MSC(ckt->interp(inst->B2cgdb)); 
        break;
    case BSIM2_CGS:
        data->v.rValue = MSC(ckt->interp(inst->B2cgsb)); 
        break;
    case BSIM2_CBG:
        data->v.rValue = MSC(ckt->interp(inst->B2cbgb)); 
        break;
    case BSIM2_CAPBD:
        data->v.rValue = MSC(ckt->interp(inst->B2capbd)); 
        break;
    case BSIM2_CQBD:
        data->v.rValue = MSC(ckt->interp(inst->B2cqbd)); 
        break;
    case BSIM2_CAPBS:
        data->v.rValue = MSC(ckt->interp(inst->B2capbs)); 
        break;
    case BSIM2_CQBS:
        data->v.rValue = MSC(ckt->interp(inst->B2cqbs)); 
        break;
    case BSIM2_CDG:
        data->v.rValue = MSC(ckt->interp(inst->B2cdgb)); 
        break;
    case BSIM2_CDD:
        data->v.rValue = MSC(ckt->interp(inst->B2cddb)); 
        break;
    case BSIM2_CDS:
        data->v.rValue = MSC(ckt->interp(inst->B2cdsb)); 
        break;
    case BSIM2_DNODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->B2dNode;
        break;
    case BSIM2_GNODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->B2gNode;
        break;
    case BSIM2_SNODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->B2sNode;
        break;
    case BSIM2_BNODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->B2bNode;
        break;
    case BSIM2_DNODEPRIME:
        data->type = IF_INTEGER;
        data->v.iValue = inst->B2dNodePrime;
        break;
    case BSIM2_SNODEPRIME:
        data->type = IF_INTEGER;
        data->v.iValue = inst->B2sNodePrime;
        break;
    default:
        return (E_BADPARM);
    }
    return (OK);
}
