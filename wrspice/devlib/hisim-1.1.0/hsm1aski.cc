
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

/***********************************************************************
 HiSIM v1.1.0
 File: hsm1ask.c of HiSIM v1.1.0

 Copyright (C) 2002 STARC

 June 30, 2002: developed by Hiroshima University and STARC
 June 30, 2002: posted by Keiichi MORIKAWA, STARC Physical Design Group
***********************************************************************/

#include "hsm1defs.h"
#include "gencurrent.h"


int
HSM1dev::askInst(const sCKT *ckt, const sGENinstance *geninst, int which,
    IFdata *data)
{
    const sHSM1instance *here = static_cast<const sHSM1instance*>(geninst);
    IFvalue *value = &data->v;

    // Need to override this for non-real returns.
    data->type = IF_REAL;

  switch (which) {
  case HSM1_L:
    value->rValue = here->HSM1_l;
    return(OK);
  case HSM1_W:
    value->rValue = here->HSM1_w;
    return(OK);
  case HSM1_AS:
    value->rValue = here->HSM1_as;
    return(OK);
  case HSM1_AD:
    value->rValue = here->HSM1_ad;
    return(OK);
  case HSM1_PS:
    value->rValue = here->HSM1_ps;
    return(OK);
  case HSM1_PD:
    value->rValue = here->HSM1_pd;
    return(OK);
  case HSM1_NRS:
    value->rValue = here->HSM1_nrs;
    return(OK);
  case HSM1_NRD:
    value->rValue = here->HSM1_nrd;
    return(OK);
  case HSM1_TEMP:
    value->rValue = here->HSM1_temp;
    return(OK);
  case HSM1_DTEMP:
    value->rValue = here->HSM1_dtemp;
    return(OK);
  case HSM1_OFF:
    value->iValue = here->HSM1_off;
    return(OK);
  case HSM1_IC_VBS:
    value->rValue = here->HSM1_icVBS;
    return(OK);
  case HSM1_IC_VDS:
    value->rValue = here->HSM1_icVDS;
    return(OK);
  case HSM1_IC_VGS:
    value->rValue = here->HSM1_icVGS;
    return(OK);
  case HSM1_DNODE:
    data->type = IF_INTEGER;
    value->iValue = here->HSM1dNode;
    return(OK);
  case HSM1_GNODE:
    data->type = IF_INTEGER;
    value->iValue = here->HSM1gNode;
    return(OK);
  case HSM1_SNODE:
    data->type = IF_INTEGER;
    value->iValue = here->HSM1sNode;
    return(OK);
  case HSM1_BNODE:
    data->type = IF_INTEGER;
    value->iValue = here->HSM1bNode;
    return(OK);
  case HSM1_DNODEPRIME:
    data->type = IF_INTEGER;
    value->iValue = here->HSM1dNodePrime;
    return(OK);
  case HSM1_SNODEPRIME:
    data->type = IF_INTEGER;
    value->iValue = here->HSM1sNodePrime;
    return(OK);
  case HSM1_SOURCECONDUCT:
    value->rValue = here->HSM1sourceConductance;
    return(OK);
  case HSM1_DRAINCONDUCT:
    value->rValue = here->HSM1drainConductance;
    return(OK);
  case HSM1_VBD:
//    value->rValue = *(ckt->CKTstate0 + here->HSM1vbd);
    value->rValue = ckt->interp(here->HSM1vbd);
    return(OK);
  case HSM1_VBS:
//    value->rValue = *(ckt->CKTstate0 + here->HSM1vbs);
    value->rValue = ckt->interp(here->HSM1vbs);
    return(OK);
  case HSM1_VGS:
//    value->rValue = *(ckt->CKTstate0 + here->HSM1vgs);
    value->rValue = ckt->interp(here->HSM1vgs);
    return(OK);
  case HSM1_VDS:
//    value->rValue = *(ckt->CKTstate0 + here->HSM1vds);
    value->rValue = ckt->interp(here->HSM1vds);
    return(OK);
  case HSM1_CD:
    value->rValue = here->HSM1_ids;
    return(OK);
  case HSM1_CBS:
    value->rValue = here->HSM1_ibs;
    return(OK);
  case HSM1_CBD:
    value->rValue = here->HSM1_ibs;
    return(OK);
  case HSM1_GM:
    value->rValue = here->HSM1_gm;
    return(OK);
  case HSM1_GDS:
    value->rValue = here->HSM1_gds;
    return(OK);
  case HSM1_GMBS:
    value->rValue = here->HSM1_gmbs;
    return(OK);
  case HSM1_GBD:
    value->rValue = here->HSM1_gbd;
    return(OK);
  case HSM1_GBS:
    value->rValue = here->HSM1_gbs;
    return(OK);
  case HSM1_QB:
//    value->rValue = *(ckt->CKTstate0 + here->HSM1qb); 
    value->rValue = ckt->interp(here->HSM1qb); 
    return(OK);
  case HSM1_CQB:
//    value->rValue = *(ckt->CKTstate0 + here->HSM1cqb); 
    value->rValue = ckt->interp(here->HSM1cqb); 
    return(OK);
  case HSM1_QG:
//    value->rValue = *(ckt->CKTstate0 + here->HSM1qg); 
    value->rValue = ckt->interp(here->HSM1qg); 
    return(OK);
  case HSM1_CQG:
//    value->rValue = *(ckt->CKTstate0 + here->HSM1cqg); 
    value->rValue = ckt->interp(here->HSM1cqg); 
    return(OK);
  case HSM1_QD:
//    value->rValue = *(ckt->CKTstate0 + here->HSM1qd); 
    value->rValue = ckt->interp(here->HSM1qd); 
    return(OK);
  case HSM1_CQD:
//    value->rValue = *(ckt->CKTstate0 + here->HSM1cqd); 
    value->rValue = ckt->interp(here->HSM1cqd); 
    return(OK);
  case HSM1_CGG:
    value->rValue = here->HSM1_cggb; 
    return(OK);
  case HSM1_CGD:
    value->rValue = here->HSM1_cgdb;
    return(OK);
  case HSM1_CGS:
    value->rValue = here->HSM1_cgsb;
    return(OK);
  case HSM1_CDG:
    value->rValue = here->HSM1_cdgb; 
    return(OK);
  case HSM1_CDD:
    value->rValue = here->HSM1_cddb; 
    return(OK);
  case HSM1_CDS:
    value->rValue = here->HSM1_cdsb; 
    return(OK);
  case HSM1_CBG:
    value->rValue = here->HSM1_cbgb;
    return(OK);
  case HSM1_CBDB:
    value->rValue = here->HSM1_cbdb;
    return(OK);
  case HSM1_CBSB:
    value->rValue = here->HSM1_cbsb;
    return(OK);
  case HSM1_CAPBD:
    value->rValue = here->HSM1_capbd; 
    return(OK);
  case HSM1_CAPBS:
    value->rValue = here->HSM1_capbs;
    return(OK);
  case HSM1_VON:
    value->rValue = here->HSM1_von; 
    return(OK);
  case HSM1_VDSAT:
    value->rValue = here->HSM1_vdsat; 
    return(OK);
  case HSM1_QBS:
//    value->rValue = *(ckt->CKTstate0 + here->HSM1qbs); 
    value->rValue = ckt->interp(here->HSM1qbs); 
    return(OK);
  case HSM1_QBD:
//    value->rValue = *(ckt->CKTstate0 + here->HSM1qbd); 
    value->rValue = ckt->interp(here->HSM1qbd); 
    return(OK);
  default:
    return(E_BADPARM);
  }
  /* NOTREACHED */
}

