
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
 $Id: b3aski.cc,v 1.4 2015/07/26 01:09:10 stevew Exp $
 *========================================================================*/

/**** BSIM3v3.3.0, Released by Xuemei Xi 07/29/2005 ****/

/**********
 * Copyright 2004 Regents of the University of California. All rights reserved.
 * File: b3ask.c of BSIM3v3.3.0
 * Author: 1995 Min-Chie Jeng and Mansun Chan
 * Author: 1997-1999 Weidong Liu.
 * Author: 2001  Xuemei Xi
 **********/

#include "b3defs.h"
#include "gencurrent.h"

// SRW - this function was rather extensively modified
//  1) call ckt->interp() for state table variables
//  2) use ac analysis functions for ac currents
//  3) added CD,CS,CG,CB entries

int
BSIM3dev::askInst(const sCKT *ckt, const sGENinstance *geninst, int which,
    IFdata *data)
{
    const sBSIM3instance *here = static_cast<const sBSIM3instance*>(geninst);
    IFvalue *value = &data->v;

    // Need to override this for non-real returns.
    data->type = IF_REAL;

    switch(which)
    {
    case BSIM3_L:
        value->rValue = here->BSIM3l;
        return(OK);
    case BSIM3_W:
        value->rValue = here->BSIM3w;
        return(OK);
    case BSIM3_AS:
        value->rValue = here->BSIM3sourceArea;
        return(OK);
    case BSIM3_AD:
        value->rValue = here->BSIM3drainArea;
        return(OK);
    case BSIM3_PS:
        value->rValue = here->BSIM3sourcePerimeter;
        return(OK);
    case BSIM3_PD:
        value->rValue = here->BSIM3drainPerimeter;
        return(OK);
    case BSIM3_NRS:
        value->rValue = here->BSIM3sourceSquares;
        return(OK);
    case BSIM3_NRD:
        value->rValue = here->BSIM3drainSquares;
        return(OK);
    case BSIM3_OFF:
        value->rValue = here->BSIM3off;
        return(OK);
    case BSIM3_NQSMOD:
        data->type = IF_INTEGER;
        value->iValue = here->BSIM3nqsMod;
        return(OK);
    case BSIM3_IC_VBS:
        value->rValue = here->BSIM3icVBS;
        return(OK);
    case BSIM3_IC_VDS:
        value->rValue = here->BSIM3icVDS;
        return(OK);
    case BSIM3_IC_VGS:
        value->rValue = here->BSIM3icVGS;
        return(OK);
    case BSIM3_DNODE:
        data->type = IF_INTEGER;
        value->iValue = here->BSIM3dNode;
        return(OK);
    case BSIM3_GNODE:
        data->type = IF_INTEGER;
        value->iValue = here->BSIM3gNode;
        return(OK);
    case BSIM3_SNODE:
        data->type = IF_INTEGER;
        value->iValue = here->BSIM3sNode;
        return(OK);
    case BSIM3_BNODE:
        data->type = IF_INTEGER;
        value->iValue = here->BSIM3bNode;
        return(OK);
    case BSIM3_DNODEPRIME:
        data->type = IF_INTEGER;
        value->iValue = here->BSIM3dNodePrime;
        return(OK);
    case BSIM3_SNODEPRIME:
        data->type = IF_INTEGER;
        value->iValue = here->BSIM3sNodePrime;
        return(OK);
    case BSIM3_SOURCECONDUCT:
        value->rValue = here->BSIM3sourceConductance;
        return(OK);
    case BSIM3_DRAINCONDUCT:
        value->rValue = here->BSIM3drainConductance;
        return(OK);
    case BSIM3_VBD:
//            value->rValue = *(ckt->CKTstate0 + here->BSIM3vbd);
        if (ckt->CKTcurrentAnalysis & DOING_AC)
        {
            data->type = IF_COMPLEX;
            value->cValue.real = ckt->rhsOld(here->BSIM3bNode) -
                                 ckt->rhsOld(here->BSIM3dNodePrime);
            value->cValue.imag = ckt->irhsOld(here->BSIM3bNode) -
                                 ckt->irhsOld(here->BSIM3dNodePrime);
        }
        else
            value->rValue = ckt->interp(here->BSIM3vbd);
        return(OK);
    case BSIM3_VBS:
//            value->rValue = *(ckt->CKTstate0 + here->BSIM3vbs);
        if (ckt->CKTcurrentAnalysis & DOING_AC)
        {
            data->type = IF_COMPLEX;
            value->cValue.real = ckt->rhsOld(here->BSIM3bNode) -
                                 ckt->rhsOld(here->BSIM3sNodePrime);
            value->cValue.imag = ckt->irhsOld(here->BSIM3bNode) -
                                 ckt->irhsOld(here->BSIM3sNodePrime);
        }
        else
            value->rValue = ckt->interp(here->BSIM3vbs);
        return(OK);
    case BSIM3_VGS:
//            value->rValue = *(ckt->CKTstate0 + here->BSIM3vgs);
        if (ckt->CKTcurrentAnalysis & DOING_AC)
        {
            data->type = IF_COMPLEX;
            value->cValue.real = ckt->rhsOld(here->BSIM3gNode) -
                                 ckt->rhsOld(here->BSIM3sNodePrime);
            value->cValue.imag = ckt->irhsOld(here->BSIM3gNode) -
                                 ckt->irhsOld(here->BSIM3sNodePrime);
        }
        else
            value->rValue = ckt->interp(here->BSIM3vgs);
        return(OK);
    case BSIM3_VDS:
//            value->rValue = *(ckt->CKTstate0 + here->BSIM3vds);
        if (ckt->CKTcurrentAnalysis & DOING_AC)
        {
            data->type = IF_COMPLEX;
            value->cValue.real = ckt->rhsOld(here->BSIM3dNodePrime) -
                                 ckt->rhsOld(here->BSIM3sNodePrime);
            value->cValue.imag = ckt->irhsOld(here->BSIM3dNodePrime) -
                                 ckt->irhsOld(here->BSIM3sNodePrime);
        }
        else
            value->rValue = ckt->interp(here->BSIM3vds);
        return(OK);
    case BSIM3_CD:
        value->rValue = here->BSIM3cd;
        value->rValue = ckt->interp(here->BSIM3a_cd, value->rValue);
        return(OK);

// SRW - added CD,CS,CG,CB
    case BSIM3_ID:
        if (ckt->CKTcurrentAnalysis & DOING_AC)
        {
            data->type = IF_COMPLEX;
            if (here->BSIM3adjoint)
                here->BSIM3adjoint->matrix->compute_cplx(here->BSIM3dNode,
                        ckt->CKTrhsOld, ckt->CKTirhsOld,
                        &value->cValue.real, &value->cValue.imag);
        }
        else
            value->rValue = ckt->interp(here->BSIM3a_id);
        return(OK);
    case BSIM3_IS:
        if (ckt->CKTcurrentAnalysis & DOING_AC)
        {
            data->type = IF_COMPLEX;
            if (here->BSIM3adjoint)
                here->BSIM3adjoint->matrix->compute_cplx(here->BSIM3sNode,
                        ckt->CKTrhsOld, ckt->CKTirhsOld,
                        &value->cValue.real, &value->cValue.imag);
        }
        else
            value->rValue = ckt->interp(here->BSIM3a_is);
        return(OK);
    case BSIM3_IG:
        if (ckt->CKTcurrentAnalysis & DOING_AC)
        {
            data->type = IF_COMPLEX;
            if (here->BSIM3adjoint)
                here->BSIM3adjoint->matrix->compute_cplx(here->BSIM3gNode,
                        ckt->CKTrhsOld, ckt->CKTirhsOld,
                        &value->cValue.real, &value->cValue.imag);
        }
        else
            value->rValue = ckt->interp(here->BSIM3a_ig);
        return(OK);
    case BSIM3_IB:
        if (ckt->CKTcurrentAnalysis & DOING_AC)
        {
            data->type = IF_COMPLEX;
            if (here->BSIM3adjoint)
                here->BSIM3adjoint->matrix->compute_cplx(here->BSIM3bNode,
                        ckt->CKTrhsOld, ckt->CKTirhsOld,
                        &value->cValue.real, &value->cValue.imag);
        }
        else
            value->rValue = ckt->interp(here->BSIM3a_ib);
        return(OK);

    case BSIM3_CBS:
        value->rValue = here->BSIM3cbs;
        value->rValue = ckt->interp(here->BSIM3a_cbs, value->rValue);
        return(OK);
    case BSIM3_CBD:
        value->rValue = here->BSIM3cbd;
        value->rValue = ckt->interp(here->BSIM3a_cbd, value->rValue);
        return(OK);
    case BSIM3_GM:
        value->rValue = here->BSIM3gm;
        value->rValue = ckt->interp(here->BSIM3a_gm, value->rValue);
        return(OK);
    case BSIM3_GDS:
        value->rValue = here->BSIM3gds;
        value->rValue = ckt->interp(here->BSIM3a_gds, value->rValue);
        return(OK);
    case BSIM3_GMBS:
        value->rValue = here->BSIM3gmbs;
        value->rValue = ckt->interp(here->BSIM3a_gmbs, value->rValue);
        return(OK);
    case BSIM3_GBD:
        value->rValue = here->BSIM3gbd;
        value->rValue = ckt->interp(here->BSIM3a_gbd, value->rValue);
        return(OK);
    case BSIM3_GBS:
        value->rValue = here->BSIM3gbs;
        value->rValue = ckt->interp(here->BSIM3a_gbs, value->rValue);
        return(OK);
    case BSIM3_QB:
//            value->rValue = *(ckt->CKTstate0 + here->BSIM3qb);
        value->rValue = ckt->interp(here->BSIM3qb);
        return(OK);
    case BSIM3_CQB:
//            value->rValue = *(ckt->CKTstate0 + here->BSIM3cqb);
        value->rValue = ckt->interp(here->BSIM3cqb);
        return(OK);
    case BSIM3_QG:
//            value->rValue = *(ckt->CKTstate0 + here->BSIM3qg);
        value->rValue = ckt->interp(here->BSIM3qg);
        return(OK);
    case BSIM3_CQG:
//            value->rValue = *(ckt->CKTstate0 + here->BSIM3cqg);
        value->rValue = ckt->interp(here->BSIM3cqg);
        return(OK);
    case BSIM3_QD:
//            value->rValue = *(ckt->CKTstate0 + here->BSIM3qd);
        value->rValue = ckt->interp(here->BSIM3qd);
        return(OK);
    case BSIM3_CQD:
//            value->rValue = *(ckt->CKTstate0 + here->BSIM3cqd);
        value->rValue = ckt->interp(here->BSIM3cqd);
        return(OK);
    case BSIM3_CGG:
        value->rValue = here->BSIM3cggb;
        value->rValue = ckt->interp(here->BSIM3a_cggb, value->rValue);
        return(OK);
    case BSIM3_CGD:
        value->rValue = here->BSIM3cgdb;
        value->rValue = ckt->interp(here->BSIM3a_cgdb, value->rValue);
        return(OK);
    case BSIM3_CGS:
        value->rValue = here->BSIM3cgsb;
        value->rValue = ckt->interp(here->BSIM3a_cgsb, value->rValue);
        return(OK);
    case BSIM3_CDG:
        value->rValue = here->BSIM3cdgb;
        value->rValue = ckt->interp(here->BSIM3a_cdgb, value->rValue);
        return(OK);
    case BSIM3_CDD:
        value->rValue = here->BSIM3cddb;
        value->rValue = ckt->interp(here->BSIM3a_cddb, value->rValue);
        return(OK);
    case BSIM3_CDS:
        value->rValue = here->BSIM3cdsb;
        value->rValue = ckt->interp(here->BSIM3a_cdsb, value->rValue);
        return(OK);
    case BSIM3_CBG:
        value->rValue = here->BSIM3cbgb;
        value->rValue = ckt->interp(here->BSIM3a_cbgb, value->rValue);
        return(OK);
    case BSIM3_CBDB:
        value->rValue = here->BSIM3cbdb;
        value->rValue = ckt->interp(here->BSIM3a_cbdb, value->rValue);
        return(OK);
    case BSIM3_CBSB:
        value->rValue = here->BSIM3cbsb;
        value->rValue = ckt->interp(here->BSIM3a_cbsb, value->rValue);
        return(OK);
    case BSIM3_CAPBD:
        value->rValue = here->BSIM3capbd;
        value->rValue = ckt->interp(here->BSIM3a_capbd, value->rValue);
        return(OK);
    case BSIM3_CAPBS:
        value->rValue = here->BSIM3capbs;
        value->rValue = ckt->interp(here->BSIM3a_capbs, value->rValue);
        return(OK);
    case BSIM3_VON:
        value->rValue = here->BSIM3von;
        value->rValue = ckt->interp(here->BSIM3a_von, value->rValue);
        return(OK);
    case BSIM3_VDSAT:
        value->rValue = here->BSIM3vdsat;
        value->rValue = ckt->interp(here->BSIM3a_vdsat, value->rValue);
        return(OK);
    case BSIM3_QBS:
//            value->rValue = *(ckt->CKTstate0 + here->BSIM3qbs);
        value->rValue = ckt->interp(here->BSIM3qbs);
        return(OK);
    case BSIM3_QBD:
//            value->rValue = *(ckt->CKTstate0 + here->BSIM3qbd);
        value->rValue = ckt->interp(here->BSIM3qbd);
        return(OK);
    default:
        return(E_BADPARM);
    }
    /* NOTREACHED */
}

