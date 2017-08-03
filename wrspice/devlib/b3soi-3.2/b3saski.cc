
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA, http://wrcad.com            *
 *  Copyright (C) 2017 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
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
Author: 1998 Samuel Fung, Dennis Sinitsky and Stephen Tang
File: b3soiask.c          98/5/01
Modified by Pin Su	99/4/30
Modified by Pin Su      01/2/15
Modified by Hui Wan     02/11/12
Modified by Pin Su	03/07/30
**********/

#include "b3sdefs.h"
#include "gencurrent.h"

// SRW - this function was rather extensively modified
//  1) call ckt->interp() for state table variables
//  2) use ac analysis functions for ac currents
//  3) added CD,CS,CG,CB entries


int
B3SOIdev::askInst(const sCKT *ckt, const sGENinstance *geninst, int which,
    IFdata *data)
{
    const sB3SOIinstance *here = static_cast<const sB3SOIinstance*>(geninst);
    IFvalue *value = &data->v;

    // Need to override this for non-real returns.
    data->type = IF_REAL;

    switch(which)
    {
    case B3SOI_L:
        value->rValue = here->B3SOIl;
        return(OK);
    case B3SOI_W:
        value->rValue = here->B3SOIw;
        return(OK);
    case B3SOI_AS:
        value->rValue = here->B3SOIsourceArea;
        return(OK);
    case B3SOI_AD:
        value->rValue = here->B3SOIdrainArea;
        return(OK);
    case B3SOI_PS:
        value->rValue = here->B3SOIsourcePerimeter;
        return(OK);
    case B3SOI_PD:
        value->rValue = here->B3SOIdrainPerimeter;
        return(OK);
    case B3SOI_NRS:
        value->rValue = here->B3SOIsourceSquares;
        return(OK);
    case B3SOI_NRD:
        value->rValue = here->B3SOIdrainSquares;
        return(OK);
    case B3SOI_OFF:
        data->type = IF_INTEGER;
        value->iValue = here->B3SOIoff;
        return(OK);
    case B3SOI_BJTOFF:
        data->type = IF_INTEGER;
        value->iValue = here->B3SOIbjtoff;
        return(OK);

        // SRW
    case B3SOI_DEBUG:
        data->type = IF_INTEGER;
        value->iValue = here->B3SOIdebugMod;
        return(OK);

    case B3SOI_RTH0:
        value->rValue = here->B3SOIrth0;
        return(OK);
    case B3SOI_CTH0:
        value->rValue = here->B3SOIcth0;
        return(OK);
    case B3SOI_NRB:
        value->rValue = here->B3SOIbodySquares;
        return(OK);
    case B3SOI_FRBODY:
        value->rValue = here->B3SOIfrbody;
        return(OK);

        /* v3.2 */
    case B3SOI_SOIMOD:
        data->type = IF_INTEGER;
        value->iValue = here->B3SOIsoiMod;
        return(OK);

        /* v3.1 added rgate by wanh */
    case B3SOI_RGATEMOD:
        data->type = IF_INTEGER;
        value->iValue = here->B3SOIrgateMod;
        return(OK);
        /* added rgate by wanh end */

        /* v2.0 release */
    case B3SOI_NBC:
        value->rValue = here->B3SOInbc;
        return(OK);
    case B3SOI_NSEG:
        value->rValue = here->B3SOInseg;
        return(OK);
    case B3SOI_PDBCP:
        value->rValue = here->B3SOIpdbcp;
        return(OK);
    case B3SOI_PSBCP:
        value->rValue = here->B3SOIpsbcp;
        return(OK);
    case B3SOI_AGBCP:
        value->rValue = here->B3SOIagbcp;
        return(OK);
    case B3SOI_AEBCP:
        value->rValue = here->B3SOIaebcp;
        return(OK);
    case B3SOI_VBSUSR:
        value->rValue = here->B3SOIvbsusr;
        return(OK);
    case B3SOI_TNODEOUT:
        data->type = IF_INTEGER;
        value->iValue = here->B3SOItnodeout;
        return(OK);


    case B3SOI_IC_VBS:
        value->rValue = here->B3SOIicVBS;
        return(OK);
    case B3SOI_IC_VDS:
        value->rValue = here->B3SOIicVDS;
        return(OK);
    case B3SOI_IC_VGS:
        value->rValue = here->B3SOIicVGS;
        return(OK);
    case B3SOI_IC_VES:
        value->rValue = here->B3SOIicVES;
        return(OK);
    case B3SOI_IC_VPS:
        value->rValue = here->B3SOIicVPS;
        return(OK);
    case B3SOI_DNODE:
        data->type = IF_INTEGER;
        value->iValue = here->B3SOIdNode;
        return(OK);
    case B3SOI_GNODE:
        data->type = IF_INTEGER;
        value->iValue = here->B3SOIgNode;
        return(OK);
    case B3SOI_SNODE:
        data->type = IF_INTEGER;
        value->iValue = here->B3SOIsNode;
        return(OK);
    case B3SOI_BNODE:
        data->type = IF_INTEGER;
        value->iValue = here->B3SOIbNode;
        return(OK);
    case B3SOI_ENODE:
        data->type = IF_INTEGER;
        value->iValue = here->B3SOIeNode;
        return(OK);
    case B3SOI_DNODEPRIME:
        data->type = IF_INTEGER;
        value->iValue = here->B3SOIdNodePrime;
        return(OK);
    case B3SOI_SNODEPRIME:
        data->type = IF_INTEGER;
        value->iValue = here->B3SOIsNodePrime;
        return(OK);

        /* v3.1 added for RF by wanh */
    case B3SOI_GNODEEXT:
        data->type = IF_INTEGER;
        value->iValue = here->B3SOIgNodeExt;
        return(OK);
    case B3SOI_GNODEMID:
        data->type = IF_INTEGER;
        value->iValue = here->B3SOIgNodeMid;
        return(OK);
        /* added for RF by wanh end*/

    case B3SOI_SOURCECONDUCT:
        value->rValue = here->B3SOIsourceConductance;
        return(OK);
    case B3SOI_DRAINCONDUCT:
        value->rValue = here->B3SOIdrainConductance;
        return(OK);
    case B3SOI_VBD:
//            value->rValue = *(ckt->CKTstate0 + here->B3SOIvbd);
        if (ckt->CKTcurrentAnalysis & DOING_AC)
        {
            data->type = IF_COMPLEX;
            value->cValue.real = ckt->rhsOld(here->B3SOIbNode) -
                                 ckt->rhsOld(here->B3SOIdNodePrime);
            value->cValue.imag = ckt->irhsOld(here->B3SOIbNode) -
                                 ckt->irhsOld(here->B3SOIdNodePrime);
        }
        else
            value->rValue = ckt->interp(here->B3SOIvbd);
        return(OK);
    case B3SOI_VBS:
//            value->rValue = *(ckt->CKTstate0 + here->B3SOIvbs);
        if (ckt->CKTcurrentAnalysis & DOING_AC)
        {
            data->type = IF_COMPLEX;
            value->cValue.real = ckt->rhsOld(here->B3SOIbNode) -
                                 ckt->rhsOld(here->B3SOIsNodePrime);
            value->cValue.imag = ckt->irhsOld(here->B3SOIbNode) -
                                 ckt->irhsOld(here->B3SOIsNodePrime);
        }
        else
            value->rValue = ckt->interp(here->B3SOIvbs);
        return(OK);
    case B3SOI_VGS:
//            value->rValue = *(ckt->CKTstate0 + here->B3SOIvgs);
        if (ckt->CKTcurrentAnalysis & DOING_AC)
        {
            data->type = IF_COMPLEX;
            value->cValue.real = ckt->rhsOld(here->B3SOIgNode) -
                                 ckt->rhsOld(here->B3SOIsNodePrime);
            value->cValue.imag = ckt->irhsOld(here->B3SOIgNode) -
                                 ckt->irhsOld(here->B3SOIsNodePrime);
        }
        else
            value->rValue = ckt->interp(here->B3SOIvgs);
        return(OK);
    case B3SOI_VES:
//            value->rValue = *(ckt->CKTstate0 + here->B3SOIves);
        if (ckt->CKTcurrentAnalysis & DOING_AC)
        {
            data->type = IF_COMPLEX;
            value->cValue.real = ckt->rhsOld(here->B3SOIeNode) -
                                 ckt->rhsOld(here->B3SOIsNodePrime);
            value->cValue.imag = ckt->irhsOld(here->B3SOIeNode) -
                                 ckt->irhsOld(here->B3SOIsNodePrime);
        }
        else
            value->rValue = ckt->interp(here->B3SOIves);
        return(OK);
    case B3SOI_VDS:
//            value->rValue = *(ckt->CKTstate0 + here->B3SOIvds);
        if (ckt->CKTcurrentAnalysis & DOING_AC)
        {
            data->type = IF_COMPLEX;
            value->cValue.real = ckt->rhsOld(here->B3SOIdNodePrime) -
                                 ckt->rhsOld(here->B3SOIsNodePrime);
            value->cValue.imag = ckt->irhsOld(here->B3SOIdNodePrime) -
                                 ckt->irhsOld(here->B3SOIsNodePrime);
        }
        else
            value->rValue = ckt->interp(here->B3SOIvds);
        return(OK);
    case B3SOI_CD:
        value->rValue = here->B3SOIcd;
        value->rValue = ckt->interp(here->B3SOIa_cd, value->rValue);
        return(OK);

// SRW - added CD,CS,CG,CB,CE
    case B3SOI_ID:
        if (ckt->CKTcurrentAnalysis & DOING_AC)
        {
            data->type = IF_COMPLEX;
            if (here->B3SOIadjoint)
                here->B3SOIadjoint->matrix->compute_cplx(here->B3SOIdNode,
                        ckt->CKTrhsOld, ckt->CKTirhsOld,
                        &value->cValue.real, &value->cValue.imag);
        }
        else
            value->rValue = ckt->interp(here->B3SOIa_id);
        return(OK);
    case B3SOI_IS:
        if (ckt->CKTcurrentAnalysis & DOING_AC)
        {
            data->type = IF_COMPLEX;
            if (here->B3SOIadjoint)
                here->B3SOIadjoint->matrix->compute_cplx(here->B3SOIsNode,
                        ckt->CKTrhsOld, ckt->CKTirhsOld,
                        &value->cValue.real, &value->cValue.imag);
        }
        else
            value->rValue = ckt->interp(here->B3SOIa_is);
        return(OK);
    case B3SOI_IG:
        if (ckt->CKTcurrentAnalysis & DOING_AC)
        {
            data->type = IF_COMPLEX;
            if (here->B3SOIadjoint)
                here->B3SOIadjoint->matrix->compute_cplx(here->B3SOIgNode,
                        ckt->CKTrhsOld, ckt->CKTirhsOld,
                        &value->cValue.real, &value->cValue.imag);
        }
        else
            value->rValue = ckt->interp(here->B3SOIa_ig);
        return(OK);
    case B3SOI_IB:
        if (ckt->CKTcurrentAnalysis & DOING_AC)
        {
            data->type = IF_COMPLEX;
            if (here->B3SOIadjoint)
                here->B3SOIadjoint->matrix->compute_cplx(here->B3SOIbNode,
                        ckt->CKTrhsOld, ckt->CKTirhsOld,
                        &value->cValue.real, &value->cValue.imag);
        }
        else
            value->rValue = ckt->interp(here->B3SOIa_ib);
        return(OK);
    case B3SOI_IE:
        if (ckt->CKTcurrentAnalysis & DOING_AC)
        {
            data->type = IF_COMPLEX;
            if (here->B3SOIadjoint)
                here->B3SOIadjoint->matrix->compute_cplx(here->B3SOIeNode,
                        ckt->CKTrhsOld, ckt->CKTirhsOld,
                        &value->cValue.real, &value->cValue.imag);
        }
        else
            value->rValue = ckt->interp(here->B3SOIa_ie);
        return(OK);

    case B3SOI_CBS:
        value->rValue = here->B3SOIcjs;
        value->rValue = ckt->interp(here->B3SOIa_cbs, value->rValue);
        return(OK);
    case B3SOI_CBD:
        value->rValue = here->B3SOIcjd;
        value->rValue = ckt->interp(here->B3SOIa_cbd, value->rValue);
        return(OK);
    case B3SOI_GM:
        value->rValue = here->B3SOIgm;
        value->rValue = ckt->interp(here->B3SOIa_gm, value->rValue);
        return(OK);
    case B3SOI_GMID:
//            value->rValue = here->B3SOIgm/here->B3SOIcd;
    {
        double tmp1 = ckt->interp(here->B3SOIa_gm, here->B3SOIgm);
        double tmp2 = ckt->interp(here->B3SOIa_cd, here->B3SOIcd);
        if (tmp2 != 0.0)
            value->rValue = tmp1/tmp2;
        else
            value->rValue = 0.0;
    }
    return(OK);
    case B3SOI_GDS:
        value->rValue = here->B3SOIgds;
        value->rValue = ckt->interp(here->B3SOIa_gds, value->rValue);
        return(OK);
    case B3SOI_GMBS:
        value->rValue = here->B3SOIgmbs;
        value->rValue = ckt->interp(here->B3SOIa_gmbs, value->rValue);
        return(OK);
    case B3SOI_GBD:
        value->rValue = here->B3SOIgjdb;
        value->rValue = ckt->interp(here->B3SOIa_gbd, value->rValue);
        return(OK);
    case B3SOI_GBS:
        value->rValue = here->B3SOIgjsb;
        value->rValue = ckt->interp(here->B3SOIa_gbs, value->rValue);
        return(OK);
    case B3SOI_QB:
//            value->rValue = *(ckt->CKTstate0 + here->B3SOIqb);
        value->rValue = ckt->interp(here->B3SOIqb);
        return(OK);
    case B3SOI_CQB:
//            value->rValue = *(ckt->CKTstate0 + here->B3SOIcqb);
        value->rValue = ckt->interp(here->B3SOIcqb);
        return(OK);
    case B3SOI_QG:
//            value->rValue = *(ckt->CKTstate0 + here->B3SOIqg);
        value->rValue = ckt->interp(here->B3SOIqg);
        return(OK);
    case B3SOI_CQG:
//            value->rValue = *(ckt->CKTstate0 + here->B3SOIcqg);
        value->rValue = ckt->interp(here->B3SOIcqg);
        return(OK);
    case B3SOI_QD:
//            value->rValue = *(ckt->CKTstate0 + here->B3SOIqd);
        value->rValue = ckt->interp(here->B3SOIqd);
        return(OK);
    case B3SOI_CQD:
//            value->rValue = *(ckt->CKTstate0 + here->B3SOIcqd);
        value->rValue = ckt->interp(here->B3SOIcqd);
        return(OK);
    case B3SOI_CGG:
        value->rValue = here->B3SOIcggb;
        value->rValue = ckt->interp(here->B3SOIa_cggb, value->rValue);
        return(OK);
    case B3SOI_CGD:
        value->rValue = here->B3SOIcgdb;
        value->rValue = ckt->interp(here->B3SOIa_cgdb, value->rValue);
        return(OK);
    case B3SOI_CGS:
        value->rValue = here->B3SOIcgsb;
        value->rValue = ckt->interp(here->B3SOIa_cgsb, value->rValue);
        return(OK);
    case B3SOI_CDG:
        value->rValue = here->B3SOIcdgb;
        value->rValue = ckt->interp(here->B3SOIa_cdgb, value->rValue);
        return(OK);
    case B3SOI_CDD:
        value->rValue = here->B3SOIcddb;
        value->rValue = ckt->interp(here->B3SOIa_cddb, value->rValue);
        return(OK);
    case B3SOI_CDS:
        value->rValue = here->B3SOIcdsb;
        value->rValue = ckt->interp(here->B3SOIa_cdsb, value->rValue);
        return(OK);
    case B3SOI_CBG:
        value->rValue = here->B3SOIcbgb;
        value->rValue = ckt->interp(here->B3SOIa_cbgb, value->rValue);
        return(OK);
    case B3SOI_CBDB:
        value->rValue = here->B3SOIcbdb;
        value->rValue = ckt->interp(here->B3SOIa_cbdb, value->rValue);
        return(OK);
    case B3SOI_CBSB:
        value->rValue = here->B3SOIcbsb;
        value->rValue = ckt->interp(here->B3SOIa_cbsb, value->rValue);
        return(OK);
    case B3SOI_VON:
        value->rValue = here->B3SOIvon;
        value->rValue = ckt->interp(here->B3SOIa_von, value->rValue);
        return(OK);
    case B3SOI_VDSAT:
        value->rValue = here->B3SOIvdsat;
        value->rValue = ckt->interp(here->B3SOIa_vdsat, value->rValue);
        return(OK);
    case B3SOI_QBS:
//            value->rValue = *(ckt->CKTstate0 + here->B3SOIqbs);
        value->rValue = ckt->interp(here->B3SOIqbs);
        return(OK);
    case B3SOI_QBD:
//            value->rValue = *(ckt->CKTstate0 + here->B3SOIqbd);
        value->rValue = ckt->interp(here->B3SOIqbd);
        return(OK);
    default:
        return(E_BADPARM);
    }
    /* NOTREACHED */
}

