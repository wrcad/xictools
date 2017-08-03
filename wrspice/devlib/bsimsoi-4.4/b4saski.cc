
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

/***  B4SOI 12/16/2010 Released by Tanvir Morshed   ***/

/**********
 * Copyright 2010 Regents of the University of California.  All rights reserved.
 * Authors: 1998 Samuel Fung, Dennis Sinitsky and Stephen Tang
 * Authors: 1999-2004 Pin Su, Hui Wan, Wei Jin, b3soiask.c
 * Authors: 2005- Hui Wan, Xuemei Xi, Ali Niknejad, Chenming Hu.
 * Authors: 2009- Wenwei Yang, Chung-Hsun Lin, Ali Niknejad, Chenming Hu.
 * File: b4soiask.c
 * Modified by Hui Wan, Xuemei Xi 11/30/2005
 * Modified by Wenwei Yang, Chung-Hsun Lin, Darsen Lu 03/06/2009
 * Modified by Tanvir Morshed 09/22/2009
 * Modified by Tanvir Morshed 12/31/2009
 **********/

#include "b4sdefs.h"
#include "gencurrent.h"


// SRW - this function was rather extensively modified
//  1) call ckt->interp() for state table variables
//  2) use ac analysis functions for ac currents
//  3) added CD,CS,CG,CB and a few other entries


int
B4SOIdev::askInst(const sCKT *ckt, const sGENinstance *geninst, int which,
    IFdata *data)
{
    const sB4SOIinstance *here = static_cast<const sB4SOIinstance*>(geninst);
    IFvalue *value = &data->v;

    // Need to override this for non-real returns.
    data->type = IF_REAL;

    switch(which)
    {
    case B4SOI_L:
        value->rValue = here->B4SOIl;
        return(OK);
    case B4SOI_W:
        value->rValue = here->B4SOIw;
        return(OK);

    case B4SOI_AS:
        value->rValue = here->B4SOIsourceArea;
        return(OK);
    case B4SOI_AD:
        value->rValue = here->B4SOIdrainArea;
        return(OK);
    case B4SOI_PS:
        value->rValue = here->B4SOIsourcePerimeter;
        return(OK);
    case B4SOI_PD:
        value->rValue = here->B4SOIdrainPerimeter;
        return(OK);
    case B4SOI_NRS:
        value->rValue = here->B4SOIsourceSquares;
        return(OK);
    case B4SOI_NRD:
        value->rValue = here->B4SOIdrainSquares;
        return(OK);
    case B4SOI_OFF:
        data->type = IF_INTEGER;
        value->iValue = here->B4SOIoff;
        return(OK);
    case B4SOI_BJTOFF:
        data->type = IF_INTEGER;
        value->iValue = here->B4SOIbjtoff;
        return(OK);

        // SRW
    case B4SOI_DEBUG:
        data->type = IF_INTEGER;
        value->iValue = here->B4SOIdebugMod;
        return(OK);

    case B4SOI_RTH0:
        value->rValue = here->B4SOIrth0;
        return(OK);
    case B4SOI_CTH0:
        value->rValue = here->B4SOIcth0;
        return(OK);
    case B4SOI_NRB:
        value->rValue = here->B4SOIbodySquares;
        return(OK);
    case B4SOI_FRBODY:
        value->rValue = here->B4SOIfrbody;
        return(OK);

    case B4SOI_QB:
        value->rValue = here->B4SOIqbulk;
        value->rValue = ckt->interp(here->B4SOIa_qbulk, value->rValue);
        return(OK);
    case B4SOI_QD:
        value->rValue = here->B4SOIqdrn;
        value->rValue = ckt->interp(here->B4SOIa_qdrn, value->rValue);
        return(OK);
    case B4SOI_QS:
        value->rValue = here->B4SOIqsrc;
        return(OK);
    case B4SOI_CGG:
        value->rValue = here->B4SOIcggb;
        value->rValue = ckt->interp(here->B4SOIa_cggb, value->rValue);
        return(OK);
    case B4SOI_CGD:
        value->rValue = here->B4SOIcgdb;
        value->rValue = ckt->interp(here->B4SOIa_cgdb, value->rValue);
        return(OK);
    case B4SOI_CGS:
        value->rValue = here->B4SOIcgsb;
        value->rValue = ckt->interp(here->B4SOIa_cgsb, value->rValue);
        return(OK);
    case B4SOI_CDG:
        value->rValue = here->B4SOIcdgb;
        value->rValue = ckt->interp(here->B4SOIa_cdgb, value->rValue);
        return(OK);
    case B4SOI_CDD:
        value->rValue = here->B4SOIcddb;
        value->rValue = ckt->interp(here->B4SOIa_cddb, value->rValue);
        return(OK);
    case B4SOI_CDS:
        value->rValue = here->B4SOIcdsb;
        value->rValue = ckt->interp(here->B4SOIa_cdsb, value->rValue);
        return(OK);
    case B4SOI_CBG:
        value->rValue = here->B4SOIcbgb;
        value->rValue = ckt->interp(here->B4SOIa_cbgb, value->rValue);
        return(OK);
    case B4SOI_CBD:
        value->rValue = here->B4SOIcbdb;
        value->rValue = ckt->interp(here->B4SOIa_cbdb, value->rValue);
        return(OK);
    case B4SOI_CBS:
        value->rValue = here->B4SOIcbsb;
        value->rValue = ckt->interp(here->B4SOIa_cbsb, value->rValue);
        return(OK);
    case B4SOI_CAPBD:
        value->rValue = here->B4SOIcapbd;
        return(OK);
    case B4SOI_CAPBS:
        value->rValue = here->B4SOIcapbs;
        return(OK);

        /* v4.0 */
    case B4SOI_RBSB:
        value->rValue = here->B4SOIrbsb;
        return(OK);
    case B4SOI_RBDB:
        value->rValue = here->B4SOIrbdb;
        return(OK);
    case B4SOI_CJSB:
        value->rValue = here->B4SOIcjsb;
        return(OK);
    case B4SOI_CJDB:
        value->rValue = here->B4SOIcjdb;
        return(OK);
    case B4SOI_SA:
        value->rValue = here->B4SOIsa ;
        return(OK);
    case B4SOI_SB:
        value->rValue = here->B4SOIsb ;
        return(OK);
    case B4SOI_SD:
        value->rValue = here->B4SOIsd ;
        return(OK);
    case B4SOI_RBODYMOD:
        data->type = IF_INTEGER;
        value->iValue = here->B4SOIrbodyMod;
        return(OK);
    case B4SOI_NF:
        value->rValue = here->B4SOInf;
        return(OK);
    case B4SOI_DELVTO:
        value->rValue = here->B4SOIdelvto;
        return(OK);

        /* v4.0 end */

        /* v3.2 */
    case B4SOI_SOIMOD:
        data->type = IF_INTEGER;
        value->iValue = here->B4SOIsoiMod;
        return(OK);

        /* v3.1 added rgate */
    case B4SOI_RGATEMOD:
        data->type = IF_INTEGER;
        value->iValue = here->B4SOIrgateMod;
        return(OK);
        /* v3.1 added rgate end */

        /* v2.0 release */
    case B4SOI_NBC:
        value->rValue = here->B4SOInbc;
        return(OK);
    case B4SOI_NSEG:
        value->rValue = here->B4SOInseg;
        return(OK);
    case B4SOI_PDBCP:
        value->rValue = here->B4SOIpdbcp;
        return(OK);
    case B4SOI_PSBCP:
        value->rValue = here->B4SOIpsbcp;
        return(OK);
    case B4SOI_AGBCP:
        value->rValue = here->B4SOIagbcp;
        return(OK);
    case B4SOI_AGBCP2:
        value->rValue = here->B4SOIagbcp2;
        return(OK);         /* v4.1 for BC improvement */
    case B4SOI_AGBCPD:      /* v4.0 */
        value->rValue = here->B4SOIagbcpd;
        return(OK);
    case B4SOI_AEBCP:
        value->rValue = here->B4SOIaebcp;
        return(OK);
    case B4SOI_VBSUSR:
        value->rValue = here->B4SOIvbsusr;
        return(OK);
    case B4SOI_TNODEOUT:
        data->type = IF_INTEGER;
        value->iValue = here->B4SOItnodeout;
        return(OK);


    case B4SOI_IC_VBS:
        value->rValue = here->B4SOIicVBS;
        return(OK);
    case B4SOI_IC_VDS:
        value->rValue = here->B4SOIicVDS;
        return(OK);
    case B4SOI_IC_VGS:
        value->rValue = here->B4SOIicVGS;
        return(OK);
    case B4SOI_IC_VES:
        value->rValue = here->B4SOIicVES;
        return(OK);
    case B4SOI_IC_VPS:
        value->rValue = here->B4SOIicVPS;
        return(OK);
    case B4SOI_DNODE:
        data->type = IF_INTEGER;
        value->iValue = here->B4SOIdNode;
        return(OK);
    case B4SOI_GNODE:
        data->type = IF_INTEGER;
        value->iValue = here->B4SOIgNode;
        return(OK);
    case B4SOI_SNODE:
        data->type = IF_INTEGER;
        value->iValue = here->B4SOIsNode;
        return(OK);
    case B4SOI_BNODE:
        data->type = IF_INTEGER;
        value->iValue = here->B4SOIbNode;
        return(OK);
    case B4SOI_ENODE:
        data->type = IF_INTEGER;
        value->iValue = here->B4SOIeNode;
        return(OK);
    case B4SOI_DNODEPRIME:
        data->type = IF_INTEGER;
        value->iValue = here->B4SOIdNodePrime;
        return(OK);
    case B4SOI_SNODEPRIME:
        data->type = IF_INTEGER;
        value->iValue = here->B4SOIsNodePrime;
        return(OK);

        /* v3.1 added for RF */
    case B4SOI_GNODEEXT:
        data->type = IF_INTEGER;
        value->iValue = here->B4SOIgNodeExt;
        return(OK);
    case B4SOI_GNODEMID:
        data->type = IF_INTEGER;
        value->iValue = here->B4SOIgNodeMid;
        return(OK);
        /* added for RF end*/

// SRW
    case B4SOI_PNODE:
        data->type = IF_INTEGER;
        value->iValue = here->B4SOIpNode;
        return(OK);
    case B4SOI_TEMPNODE:
        data->type = IF_INTEGER;
        value->iValue = here->B4SOItempNode;
        return(OK);
    case B4SOI_DBNODE:
        data->type = IF_INTEGER;
        value->iValue = here->B4SOIdbNode;
        return(OK);
    case B4SOI_SBNODE:
        data->type = IF_INTEGER;
        value->iValue = here->B4SOIsbNode;
        return(OK);
//

    case B4SOI_SOURCECONDUCT:
        value->rValue = here->B4SOIsourceConductance;
        return(OK);
    case B4SOI_DRAINCONDUCT:
        value->rValue = here->B4SOIdrainConductance;
        return(OK);
    case B4SOI_VBD:
//            value->rValue = *(ckt->CKTstate0 + here->B4SOIvbd);
        if (ckt->CKTcurrentAnalysis & DOING_AC)
        {
            data->type = IF_COMPLEX;
            value->cValue.real = ckt->rhsOld(here->B4SOIbNode) -
                                 ckt->rhsOld(here->B4SOIdNodePrime);
            value->cValue.imag = ckt->irhsOld(here->B4SOIbNode) -
                                 ckt->irhsOld(here->B4SOIdNodePrime);
        }
        else
            value->rValue = ckt->interp(here->B4SOIvbd);
        return(OK);
    case B4SOI_VBS:
//            value->rValue = *(ckt->CKTstate0 + here->B4SOIvbs);
        if (ckt->CKTcurrentAnalysis & DOING_AC)
        {
            data->type = IF_COMPLEX;
            value->cValue.real = ckt->rhsOld(here->B4SOIbNode) -
                                 ckt->rhsOld(here->B4SOIsNodePrime);
            value->cValue.imag = ckt->irhsOld(here->B4SOIbNode) -
                                 ckt->irhsOld(here->B4SOIsNodePrime);
        }
        else
            value->rValue = ckt->interp(here->B4SOIvbs);
        return(OK);
    case B4SOI_VGS:
//            value->rValue = *(ckt->CKTstate0 + here->B4SOIvgs);
        if (ckt->CKTcurrentAnalysis & DOING_AC)
        {
            data->type = IF_COMPLEX;
            value->cValue.real = ckt->rhsOld(here->B4SOIgNode) -
                                 ckt->rhsOld(here->B4SOIsNodePrime);
            value->cValue.imag = ckt->irhsOld(here->B4SOIgNode) -
                                 ckt->irhsOld(here->B4SOIsNodePrime);
        }
        else
            value->rValue = ckt->interp(here->B4SOIvgs);
        return(OK);
    case B4SOI_VES:
//            value->rValue = *(ckt->CKTstate0 + here->B4SOIves);
        if (ckt->CKTcurrentAnalysis & DOING_AC)
        {
            data->type = IF_COMPLEX;
            value->cValue.real = ckt->rhsOld(here->B4SOIeNode) -
                                 ckt->rhsOld(here->B4SOIsNodePrime);
            value->cValue.imag = ckt->irhsOld(here->B4SOIeNode) -
                                 ckt->irhsOld(here->B4SOIsNodePrime);
        }
        else
            value->rValue = ckt->interp(here->B4SOIves);
        return(OK);
    case B4SOI_VDS:
//            value->rValue = *(ckt->CKTstate0 + here->B4SOIvds);
        if (ckt->CKTcurrentAnalysis & DOING_AC)
        {
            data->type = IF_COMPLEX;
            value->cValue.real = ckt->rhsOld(here->B4SOIdNodePrime) -
                                 ckt->rhsOld(here->B4SOIsNodePrime);
            value->cValue.imag = ckt->irhsOld(here->B4SOIdNodePrime) -
                                 ckt->irhsOld(here->B4SOIsNodePrime);
        }
        else
            value->rValue = ckt->interp(here->B4SOIvds);
        return(OK);
    case B4SOI_CD:
        value->rValue = here->B4SOIcdrain;
        value->rValue = ckt->interp(here->B4SOIa_cd, value->rValue);
        return(OK);

// SRW added -----
    case B4SOI_ID:
        if (ckt->CKTcurrentAnalysis & DOING_AC)
        {
            data->type = IF_COMPLEX;
            if (here->B4SOIadjoint)
                here->B4SOIadjoint->matrix->compute_cplx(here->B4SOIdNode,
                        ckt->CKTrhsOld, ckt->CKTirhsOld,
                        &value->cValue.real, &value->cValue.imag);
        }
        else
            value->rValue = ckt->interp(here->B4SOIa_id);
        return(OK);
    case B4SOI_IS:
        if (ckt->CKTcurrentAnalysis & DOING_AC)
        {
            data->type = IF_COMPLEX;
            if (here->B4SOIadjoint)
                here->B4SOIadjoint->matrix->compute_cplx(here->B4SOIsNode,
                        ckt->CKTrhsOld, ckt->CKTirhsOld,
                        &value->cValue.real, &value->cValue.imag);
        }
        else
            value->rValue = ckt->interp(here->B4SOIa_is);
        return(OK);
    case B4SOI_IG:
        if (ckt->CKTcurrentAnalysis & DOING_AC)
        {
            data->type = IF_COMPLEX;
            if (here->B4SOIadjoint)
                here->B4SOIadjoint->matrix->compute_cplx(here->B4SOIgNode,
                        ckt->CKTrhsOld, ckt->CKTirhsOld,
                        &value->cValue.real, &value->cValue.imag);
        }
        else
            value->rValue = ckt->interp(here->B4SOIa_ig);
        return(OK);
    case B4SOI_IB:
        if (ckt->CKTcurrentAnalysis & DOING_AC)
        {
            data->type = IF_COMPLEX;
            if (here->B4SOIadjoint)
                here->B4SOIadjoint->matrix->compute_cplx(here->B4SOIbNode,
                        ckt->CKTrhsOld, ckt->CKTirhsOld,
                        &value->cValue.real, &value->cValue.imag);
        }
        else
            value->rValue = ckt->interp(here->B4SOIa_ib);
        return(OK);
    case B4SOI_IE:
        if (ckt->CKTcurrentAnalysis & DOING_AC)
        {
            data->type = IF_COMPLEX;
            if (here->B4SOIadjoint)
                here->B4SOIadjoint->matrix->compute_cplx(here->B4SOIeNode,
                        ckt->CKTrhsOld, ckt->CKTirhsOld,
                        &value->cValue.real, &value->cValue.imag);
        }
        else
            value->rValue = ckt->interp(here->B4SOIa_ie);
        return(OK);

    case B4SOI_QG:
        value->rValue = ckt->interp(here->B4SOIqg);
        return(OK);
// SRW --------

    case B4SOI_IBS:
        value->rValue = here->B4SOIibs;
        return(OK);
    case B4SOI_IBD:
        value->rValue = here->B4SOIibd;
        return(OK);
    case B4SOI_ISUB:
        value->rValue = here->B4SOIiii;
        return(OK);
    case B4SOI_IGIDL:
        value->rValue = here->B4SOIigidl;
        return(OK);
    case B4SOI_IGISL:
        value->rValue = here->B4SOIigisl;
        return(OK);
    case B4SOI_IGS:
        value->rValue = here->B4SOIIgs;
        return(OK);
    case B4SOI_IGD:
        value->rValue = here->B4SOIIgd;
        return(OK);
    case B4SOI_IGB:
        value->rValue = here->B4SOIIgb;
        return(OK);
    case B4SOI_IGCS:
        value->rValue = here->B4SOIIgcs;
        return(OK);
    case B4SOI_IGCD:
        value->rValue = here->B4SOIIgcd;
        return(OK);
    case B4SOI_GM:
        value->rValue = here->B4SOIgm;
        value->rValue = ckt->interp(here->B4SOIa_gm, value->rValue);
        return(OK);
    case B4SOI_GMID:
//            value->rValue = here->B4SOIgm/here->B4SOIcd;
    {
        double tmp1 = ckt->interp(here->B4SOIa_gm, here->B4SOIgm);
        double tmp2 = ckt->interp(here->B4SOIa_cd, here->B4SOIcd);
        if (tmp2 != 0.0)
            value->rValue = tmp1/tmp2;
        else
            value->rValue = 0.0;
    }
    return(OK);
    case B4SOI_GDS:
        value->rValue = here->B4SOIgds;
        value->rValue = ckt->interp(here->B4SOIa_gds, value->rValue);
        return(OK);
    case B4SOI_GMBS:
        value->rValue = here->B4SOIgmbs;
        value->rValue = ckt->interp(here->B4SOIa_gmbs, value->rValue);
        return(OK);
    case B4SOI_GBD:
        value->rValue = here->B4SOIgjdb;
        value->rValue = ckt->interp(here->B4SOIa_gbd, value->rValue);
        return(OK);
    case B4SOI_GBS:
        value->rValue = here->B4SOIgjsb;
        value->rValue = ckt->interp(here->B4SOIa_gbs, value->rValue);
        return(OK);
    case B4SOI_CQB:
//            value->rValue = *(ckt->CKTstate0 + here->B4SOIcqb);
        value->rValue = ckt->interp(here->B4SOIcqb);
        return(OK);
    case B4SOI_CQG:
//            value->rValue = *(ckt->CKTstate0 + here->B4SOIcqg);
        value->rValue = ckt->interp(here->B4SOIcqg);
        return(OK);
    case B4SOI_CQD:
//            value->rValue = *(ckt->CKTstate0 + here->B4SOIcqd);
        value->rValue = ckt->interp(here->B4SOIcqd);
        return(OK);
    case B4SOI_CBDB:
        value->rValue = here->B4SOIcbdb;
        value->rValue = ckt->interp(here->B4SOIa_cbdb, value->rValue);
        return(OK);
    case B4SOI_CBSB:
        value->rValue = here->B4SOIcbsb;
        value->rValue = ckt->interp(here->B4SOIa_cbsb, value->rValue);
        return(OK);
    case B4SOI_VON:
        value->rValue = here->B4SOIvon;
        value->rValue = ckt->interp(here->B4SOIa_von, value->rValue);
        return(OK);
    case B4SOI_VDSAT:
        value->rValue = here->B4SOIvdsat;
        value->rValue = ckt->interp(here->B4SOIa_vdsat, value->rValue);
        return(OK);
    case B4SOI_QBS:
//            value->rValue = *(ckt->CKTstate0 + here->B4SOIqbs);
        value->rValue = ckt->interp(here->B4SOIqbs);
        return(OK);
    case B4SOI_QBD:
//            value->rValue = *(ckt->CKTstate0 + here->B4SOIqbd);
        value->rValue = ckt->interp(here->B4SOIqbd);
        return(OK);
#ifdef B4SOI_DEBUG_OUT
    case B4SOI_DEBUG1:
        value->rValue = here->B4SOIdebug1;
        return(OK);
    case B4SOI_DEBUG2:
        value->rValue = here->B4SOIdebug2;
        return(OK);
    case B4SOI_DEBUG3:
        value->rValue = here->B4SOIdebug3;
        return(OK);
#endif
    default:
        return(E_BADPARM);
    }
    /* NOTREACHED */
}

