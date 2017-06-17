
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
 $Id: b4aski.cc,v 1.4 2015/07/26 01:09:10 stevew Exp $
 *========================================================================*/

/**** BSIM4.3.0 Released by Xuemei (Jane) Xi  05/09/2003 ****/

/**********
 * Copyright 2003 Regents of the University of California. All rights reserved.
 * File: b4ask.c of BSIM4.3.0.
 * Author: 2000 Weidong Liu
 * Authors: 2001- Xuemei Xi, Jin He, Kanyu Cao, Mohan Dunga, Mansun Chan, Ali Niknejad, Chenming Hu.
 * Project Director: Prof. Chenming Hu.
 * Modified by Xuemei Xi, 04/06/2001.
 * Modified by Xuemei Xi, 10/05/2001.
 * Modified by Xuemei Xi, 05/09/2003.
 **********/

#include "b4defs.h"
#include "gencurrent.h"

// SRW - this function was rather extensively modified
//  1) call ckt->interp() for state table variables
//  2) use ac analysis functions for ac currents
//  3) added CD,CS,CG,CB entries


int
BSIM4dev::askInst(const sCKT *ckt, const sGENinstance *geninst, int which,
    IFdata *data)
{
    const sBSIM4instance *here = static_cast<const sBSIM4instance*>(geninst);
    IFvalue *value = &data->v;

    // Need to override this for non-real returns.
    data->type = IF_REAL;

    switch(which)
    {
    case BSIM4_L:
        value->rValue = here->BSIM4l;
        return(OK);
    case BSIM4_W:
        value->rValue = here->BSIM4w;
        return(OK);
    case BSIM4_NF:
        value->rValue = here->BSIM4nf;
        return(OK);
    case BSIM4_MIN:
        data->type = IF_INTEGER;
        value->iValue = here->BSIM4min;
        return(OK);
    case BSIM4_AS:
        value->rValue = here->BSIM4sourceArea;
        return(OK);
    case BSIM4_AD:
        value->rValue = here->BSIM4drainArea;
        return(OK);
    case BSIM4_PS:
        value->rValue = here->BSIM4sourcePerimeter;
        return(OK);
    case BSIM4_PD:
        value->rValue = here->BSIM4drainPerimeter;
        return(OK);
    case BSIM4_NRS:
        value->rValue = here->BSIM4sourceSquares;
        return(OK);
    case BSIM4_NRD:
        value->rValue = here->BSIM4drainSquares;
        return(OK);
    case BSIM4_OFF:
        value->rValue = here->BSIM4off;
        return(OK);
    case BSIM4_SA:
        value->rValue = here->BSIM4sa;
        return(OK);
    case BSIM4_SB:
        value->rValue = here->BSIM4sb;
        return(OK);
    case BSIM4_SD:
        value->rValue = here->BSIM4sd;
        return(OK);
    case BSIM4_RBSB:
        value->rValue = here->BSIM4rbsb;
        return(OK);
    case BSIM4_RBDB:
        value->rValue = here->BSIM4rbdb;
        return(OK);
    case BSIM4_RBPB:
        value->rValue = here->BSIM4rbpb;
        return(OK);
    case BSIM4_RBPS:
        value->rValue = here->BSIM4rbps;
        return(OK);
    case BSIM4_RBPD:
        value->rValue = here->BSIM4rbpd;
        return(OK);
    case BSIM4_TRNQSMOD:
        data->type = IF_INTEGER;
        value->iValue = here->BSIM4trnqsMod;
        return(OK);
    case BSIM4_ACNQSMOD:
        data->type = IF_INTEGER;
        value->iValue = here->BSIM4acnqsMod;
        return(OK);
    case BSIM4_RBODYMOD:
        data->type = IF_INTEGER;
        value->iValue = here->BSIM4rbodyMod;
        return(OK);
    case BSIM4_RGATEMOD:
        data->type = IF_INTEGER;
        value->iValue = here->BSIM4rgateMod;
        return(OK);
    case BSIM4_GEOMOD:
        data->type = IF_INTEGER;
        value->iValue = here->BSIM4geoMod;
        return(OK);
    case BSIM4_RGEOMOD:
        data->type = IF_INTEGER;
        value->iValue = here->BSIM4rgeoMod;
        return(OK);
    case BSIM4_IC_VDS:
        value->rValue = here->BSIM4icVDS;
        return(OK);
    case BSIM4_IC_VGS:
        value->rValue = here->BSIM4icVGS;
        return(OK);
    case BSIM4_IC_VBS:
        value->rValue = here->BSIM4icVBS;
        return(OK);
    case BSIM4_DNODE:
        data->type = IF_INTEGER;
        value->iValue = here->BSIM4dNode;
        return(OK);
    case BSIM4_GNODEEXT:
        data->type = IF_INTEGER;
        value->iValue = here->BSIM4gNodeExt;
        return(OK);
    case BSIM4_SNODE:
        data->type = IF_INTEGER;
        value->iValue = here->BSIM4sNode;
        return(OK);
    case BSIM4_BNODE:
        data->type = IF_INTEGER;
        value->iValue = here->BSIM4bNode;
        return(OK);
    case BSIM4_DNODEPRIME:
        data->type = IF_INTEGER;
        value->iValue = here->BSIM4dNodePrime;
        return(OK);
    case BSIM4_GNODEPRIME:
        data->type = IF_INTEGER;
        value->iValue = here->BSIM4gNodePrime;
        return(OK);
    case BSIM4_GNODEMID:
        data->type = IF_INTEGER;
        value->iValue = here->BSIM4gNodeMid;
        return(OK);
    case BSIM4_SNODEPRIME:
        data->type = IF_INTEGER;
        value->iValue = here->BSIM4sNodePrime;
        return(OK);
    case BSIM4_DBNODE:
        data->type = IF_INTEGER;
        value->iValue = here->BSIM4dbNode;
        return(OK);
    case BSIM4_BNODEPRIME:
        data->type = IF_INTEGER;
        value->iValue = here->BSIM4bNodePrime;
        return(OK);
    case BSIM4_SBNODE:
        data->type = IF_INTEGER;
        value->iValue = here->BSIM4sbNode;
        return(OK);
    case BSIM4_SOURCECONDUCT:
        value->rValue = here->BSIM4sourceConductance;
        return(OK);
    case BSIM4_DRAINCONDUCT:
        value->rValue = here->BSIM4drainConductance;
        return(OK);
    case BSIM4_VBD:
//            value->rValue = *(ckt->CKTstate0 + here->BSIM4vbd);
        if (ckt->CKTcurrentAnalysis & DOING_AC)
        {
            data->type = IF_COMPLEX;
            value->cValue.real = ckt->rhsOld(here->BSIM4bNodePrime) -
                                 ckt->rhsOld(here->BSIM4dNodePrime);
            value->cValue.imag = ckt->irhsOld(here->BSIM4bNodePrime) -
                                 ckt->irhsOld(here->BSIM4dNodePrime);
        }
        else
            value->rValue = ckt->interp(here->BSIM4vbd);
        return(OK);
    case BSIM4_VBS:
//            value->rValue = *(ckt->CKTstate0 + here->BSIM4vbs);
        if (ckt->CKTcurrentAnalysis & DOING_AC)
        {
            data->type = IF_COMPLEX;
            value->cValue.real = ckt->rhsOld(here->BSIM4bNodePrime) -
                                 ckt->rhsOld(here->BSIM4sNodePrime);
            value->cValue.imag = ckt->irhsOld(here->BSIM4bNodePrime) -
                                 ckt->irhsOld(here->BSIM4sNodePrime);
        }
        else
            value->rValue = ckt->interp(here->BSIM4vbs);
        return(OK);
    case BSIM4_VGS:
//            value->rValue = *(ckt->CKTstate0 + here->BSIM4vgs);
        if (ckt->CKTcurrentAnalysis & DOING_AC)
        {
            data->type = IF_COMPLEX;
            value->cValue.real = ckt->rhsOld(here->BSIM4gNodePrime) -
                                 ckt->rhsOld(here->BSIM4sNodePrime);
            value->cValue.imag = ckt->irhsOld(here->BSIM4gNodePrime) -
                                 ckt->irhsOld(here->BSIM4sNodePrime);
        }
        else
            value->rValue = ckt->interp(here->BSIM4vgs);
        return(OK);
    case BSIM4_VDS:
//            value->rValue = *(ckt->CKTstate0 + here->BSIM4vds);
        if (ckt->CKTcurrentAnalysis & DOING_AC)
        {
            data->type = IF_COMPLEX;
            value->cValue.real = ckt->rhsOld(here->BSIM4dNodePrime) -
                                 ckt->rhsOld(here->BSIM4sNodePrime);
            value->cValue.imag = ckt->irhsOld(here->BSIM4dNodePrime) -
                                 ckt->irhsOld(here->BSIM4sNodePrime);
        }
        else
            value->rValue = ckt->interp(here->BSIM4vds);
        return(OK);
    case BSIM4_CD:
        value->rValue = here->BSIM4cd;
        value->rValue = ckt->interp(here->BSIM4a_cd, value->rValue);
        return(OK);


// SRW - added ID,IS,IG,IB
    case BSIM4_ID:
        if (ckt->CKTcurrentAnalysis & DOING_AC)
        {
            data->type = IF_COMPLEX;
            if (here->BSIM4adjoint)
                here->BSIM4adjoint->matrix->compute_cplx(here->BSIM4dNode,
                        ckt->CKTrhsOld, ckt->CKTirhsOld,
                        &value->cValue.real, &value->cValue.imag);
        }
        else
            value->rValue = ckt->interp(here->BSIM4a_id);
        return(OK);
    case BSIM4_IS:
        if (ckt->CKTcurrentAnalysis & DOING_AC)
        {
            data->type = IF_COMPLEX;
            if (here->BSIM4adjoint)
                here->BSIM4adjoint->matrix->compute_cplx(here->BSIM4sNode,
                        ckt->CKTrhsOld, ckt->CKTirhsOld,
                        &value->cValue.real, &value->cValue.imag);
        }
        else
            value->rValue = ckt->interp(here->BSIM4a_is);
        return(OK);
    case BSIM4_IG:
        if (ckt->CKTcurrentAnalysis & DOING_AC)
        {
            data->type = IF_COMPLEX;
            if (here->BSIM4adjoint)
                here->BSIM4adjoint->matrix->compute_cplx(here->BSIM4gNodeExt,
                        ckt->CKTrhsOld, ckt->CKTirhsOld,
                        &value->cValue.real, &value->cValue.imag);
        }
        else
            value->rValue = ckt->interp(here->BSIM4a_ig);
        return(OK);
    case BSIM4_IB:
        if (ckt->CKTcurrentAnalysis & DOING_AC)
        {
            data->type = IF_COMPLEX;
            if (here->BSIM4adjoint)
                here->BSIM4adjoint->matrix->compute_cplx(here->BSIM4bNode,
                        ckt->CKTrhsOld, ckt->CKTirhsOld,
                        &value->cValue.real, &value->cValue.imag);
        }
        else
            value->rValue = ckt->interp(here->BSIM4a_ib);
        return(OK);

    case BSIM4_CBS:
        value->rValue = here->BSIM4cbs;
        value->rValue = ckt->interp(here->BSIM4a_cbs, value->rValue);
        return(OK);
    case BSIM4_CBD:
        value->rValue = here->BSIM4cbd;
        value->rValue = ckt->interp(here->BSIM4a_cbd, value->rValue);
        return(OK);
    case BSIM4_CSUB:
        value->rValue = here->BSIM4csub;
        value->rValue = ckt->interp(here->BSIM4a_csub, value->rValue);
        return(OK);
    case BSIM4_IGIDL:
        value->rValue = here->BSIM4Igidl;
        value->rValue = ckt->interp(here->BSIM4a_Igidl, value->rValue);
        return(OK);
    case BSIM4_IGISL:
        value->rValue = here->BSIM4Igisl;
        value->rValue = ckt->interp(here->BSIM4a_Igisl, value->rValue);
        return(OK);
    case BSIM4_IGS:
        value->rValue = here->BSIM4Igs;
        value->rValue = ckt->interp(here->BSIM4a_Igs, value->rValue);
        return(OK);
    case BSIM4_IGD:
        value->rValue = here->BSIM4Igd;
        value->rValue = ckt->interp(here->BSIM4a_Igd, value->rValue);
        return(OK);
    case BSIM4_IGB:
        value->rValue = here->BSIM4Igb;
        value->rValue = ckt->interp(here->BSIM4a_Igb, value->rValue);
        return(OK);
    case BSIM4_IGCS:
        value->rValue = here->BSIM4Igcs;
        value->rValue = ckt->interp(here->BSIM4a_Igcs, value->rValue);
        return(OK);
    case BSIM4_IGCD:
        value->rValue = here->BSIM4Igcd;
        value->rValue = ckt->interp(here->BSIM4a_Igcd, value->rValue);
        return(OK);
    case BSIM4_GM:
        value->rValue = here->BSIM4gm;
        value->rValue = ckt->interp(here->BSIM4a_gm, value->rValue);
        return(OK);
    case BSIM4_GDS:
        value->rValue = here->BSIM4gds;
        value->rValue = ckt->interp(here->BSIM4a_gds, value->rValue);
        return(OK);
    case BSIM4_GMBS:
        value->rValue = here->BSIM4gmbs;
        value->rValue = ckt->interp(here->BSIM4a_gmbs, value->rValue);
        return(OK);
    case BSIM4_GBD:
        value->rValue = here->BSIM4gbd;
        value->rValue = ckt->interp(here->BSIM4a_gbd, value->rValue);
        return(OK);
    case BSIM4_GBS:
        value->rValue = here->BSIM4gbs;
        value->rValue = ckt->interp(here->BSIM4a_gbs, value->rValue);
        return(OK);
    case BSIM4_QB:
//            value->rValue = *(ckt->CKTstate0 + here->BSIM4qb);
        value->rValue = ckt->interp(here->BSIM4a_qbulk);
        return(OK);
    case BSIM4_CQB:
//            value->rValue = *(ckt->CKTstate0 + here->BSIM4cqb);
        value->rValue = ckt->interp(here->BSIM4cqb);
        return(OK);
    case BSIM4_QG:
//            value->rValue = *(ckt->CKTstate0 + here->BSIM4qg);
        value->rValue = ckt->interp(here->BSIM4a_qgate);
        return(OK);
    case BSIM4_CQG:
//            value->rValue = *(ckt->CKTstate0 + here->BSIM4cqg);
        value->rValue = ckt->interp(here->BSIM4cqg);
        return(OK);
    case BSIM4_QD:
//            value->rValue = *(ckt->CKTstate0 + here->BSIM4qd);
        value->rValue = ckt->interp(here->BSIM4a_qdrn);
        return(OK);
    case BSIM4_CQD:
//            value->rValue = *(ckt->CKTstate0 + here->BSIM4cqd);
        value->rValue = ckt->interp(here->BSIM4cqd);
        return(OK);
    case BSIM4_QS:
//            value->rValue = *(ckt->CKTstate0 + here->BSIM4qs);
        value->rValue = ckt->interp(here->BSIM4a_qsrc);
        return(OK);
    case BSIM4_CGGB:
        value->rValue = here->BSIM4cggb;
        value->rValue = ckt->interp(here->BSIM4a_cggb, value->rValue);
        return(OK);
    case BSIM4_CGDB:
        value->rValue = here->BSIM4cgdb;
        value->rValue = ckt->interp(here->BSIM4a_cgdb, value->rValue);
        return(OK);
    case BSIM4_CGSB:
        value->rValue = here->BSIM4cgsb;
        value->rValue = ckt->interp(here->BSIM4a_cgsb, value->rValue);
        return(OK);
    case BSIM4_CDGB:
        value->rValue = here->BSIM4cdgb;
        value->rValue = ckt->interp(here->BSIM4a_cdgb, value->rValue);
        return(OK);
    case BSIM4_CDDB:
        value->rValue = here->BSIM4cddb;
        value->rValue = ckt->interp(here->BSIM4a_cddb, value->rValue);
        return(OK);
    case BSIM4_CDSB:
        value->rValue = here->BSIM4cdsb;
        value->rValue = ckt->interp(here->BSIM4a_cdsb, value->rValue);
        return(OK);
    case BSIM4_CBGB:
        value->rValue = here->BSIM4cbgb;
        value->rValue = ckt->interp(here->BSIM4a_cbgb, value->rValue);
        return(OK);
    case BSIM4_CBDB:
        value->rValue = here->BSIM4cbdb;
        value->rValue = ckt->interp(here->BSIM4a_cbdb, value->rValue);
        return(OK);
    case BSIM4_CBSB:
        value->rValue = here->BSIM4cbsb;
        value->rValue = ckt->interp(here->BSIM4a_cbsb, value->rValue);
        return(OK);
    case BSIM4_CSGB:
        value->rValue = here->BSIM4csgb;
        value->rValue = ckt->interp(here->BSIM4a_csgb, value->rValue);
        return(OK);
    case BSIM4_CSDB:
        value->rValue = here->BSIM4csdb;
        value->rValue = ckt->interp(here->BSIM4a_csdb, value->rValue);
        return(OK);
    case BSIM4_CSSB:
        value->rValue = here->BSIM4cssb;
        value->rValue = ckt->interp(here->BSIM4a_cssb, value->rValue);
        return(OK);
        return(OK);
    case BSIM4_CGBB:
        value->rValue = here->BSIM4cgbb;
        value->rValue = ckt->interp(here->BSIM4a_cgbb, value->rValue);
        return(OK);
    case BSIM4_CDBB:
        value->rValue = here->BSIM4cdbb;
        value->rValue = ckt->interp(here->BSIM4a_cdbb, value->rValue);
        return(OK);
    case BSIM4_CSBB:
        value->rValue = here->BSIM4csbb;
        value->rValue = ckt->interp(here->BSIM4a_csbb, value->rValue);
        return(OK);
    case BSIM4_CBBB:
        value->rValue = here->BSIM4cbbb;
        value->rValue = ckt->interp(here->BSIM4a_cbbb, value->rValue);
        return(OK);
    case BSIM4_CAPBD:
        value->rValue = here->BSIM4capbd;
        value->rValue = ckt->interp(here->BSIM4a_capbd, value->rValue);
        return(OK);
    case BSIM4_CAPBS:
        value->rValue = here->BSIM4capbs;
        value->rValue = ckt->interp(here->BSIM4a_capbs, value->rValue);
        return(OK);
    case BSIM4_VON:
        value->rValue = here->BSIM4von;
        value->rValue = ckt->interp(here->BSIM4a_von, value->rValue);
        return(OK);
    case BSIM4_VDSAT:
        value->rValue = here->BSIM4vdsat;
        value->rValue = ckt->interp(here->BSIM4a_vdsat, value->rValue);
        return(OK);
    case BSIM4_QBS:
//            value->rValue = *(ckt->CKTstate0 + here->BSIM4qbs);
        value->rValue = ckt->interp(here->BSIM4qbs);
        return(OK);
    case BSIM4_QBD:
//            value->rValue = *(ckt->CKTstate0 + here->BSIM4qbd);
        value->rValue = ckt->interp(here->BSIM4qbd);
        return(OK);

// SRW - added
    case BSIM4_CQBD:
        value->rValue = ckt->interp(here->BSIM4cqbd);
        return(OK);
    case BSIM4_CQBS:
        value->rValue = ckt->interp(here->BSIM4cqbs);
        return(OK);
    case BSIM4_QINV:
        value->rValue = here->BSIM4qinv;
        value->rValue = ckt->interp(here->BSIM4a_qinv, value->rValue);
        return(OK);
    default:
        return(E_BADPARM);
    }
    /* NOTREACHED */
}

