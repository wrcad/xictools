
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

/***********************************************************************
 HiSIM v1.1.0
 File: hsm1mask.c of HiSIM v1.1.0

 Copyright (C) 2002 STARC

 June 30, 2002: developed by Hiroshima University and STARC
 June 30, 2002: posted by Keiichi MORIKAWA, STARC Physical Design Group
***********************************************************************/

#include "hsm1defs.h"


int
HSM1dev::askModl(const sGENmodel *genmod, int which, IFdata *data)
{
    const sHSM1model *model = static_cast<const sHSM1model*>(genmod);
    IFvalue *value = &data->v;
    // Need to override this for non-real returns.
    data->type = IF_REAL;

  switch (which) {
  case HSM1_MOD_NMOS:
    value->iValue = model->HSM1_type;
    data->type = IF_INTEGER;
    return(OK);
  case  HSM1_MOD_PMOS:
    value->iValue = model->HSM1_type;
    data->type = IF_INTEGER;
    return(OK);
  case  HSM1_MOD_LEVEL:
    value->iValue = model->HSM1_level;
    data->type = IF_INTEGER;
    return(OK);
  case  HSM1_MOD_INFO:
    value->iValue = model->HSM1_info;
    data->type = IF_INTEGER;
    return(OK);
  case HSM1_MOD_NOISE:
    value->iValue = model->HSM1_noise;
    data->type = IF_INTEGER;
    return(OK);
  case HSM1_MOD_VERSION:
    value->iValue = model->HSM1_version;
    data->type = IF_INTEGER;
    return(OK);
  case HSM1_MOD_SHOW:
    value->iValue = model->HSM1_show;
    data->type = IF_INTEGER;
    return(OK);
  case  HSM1_MOD_CORSRD:
    value->iValue = model->HSM1_corsrd;
    data->type = IF_INTEGER;
    return(OK);
  case  HSM1_MOD_COIPRV:
    value->iValue = model->HSM1_coiprv;
    data->type = IF_INTEGER;
    return(OK);
  case  HSM1_MOD_COPPRV:
    value->iValue = model->HSM1_copprv;
    data->type = IF_INTEGER;
    return(OK);
  case  HSM1_MOD_COCGSO:
    value->iValue = model->HSM1_cocgso;
    data->type = IF_INTEGER;
    return(OK);
  case  HSM1_MOD_COCGDO:
    value->iValue = model->HSM1_cocgdo;
    data->type = IF_INTEGER;
    return(OK);
  case  HSM1_MOD_COCGBO:
    value->rValue = model->HSM1_cocgbo;
    return(OK);
  case  HSM1_MOD_COADOV:
    value->iValue = model->HSM1_coadov;
    data->type = IF_INTEGER;
    return(OK);
  case  HSM1_MOD_COXX08:
    value->iValue = model->HSM1_coxx08;
    data->type = IF_INTEGER;
    return(OK);
  case  HSM1_MOD_COXX09:
    value->iValue = model->HSM1_coxx09;
    data->type = IF_INTEGER;
    return(OK);
  case  HSM1_MOD_COISUB:
    value->iValue = model->HSM1_coisub;
    data->type = IF_INTEGER;
    return(OK);
  case  HSM1_MOD_COIIGS:
    value->iValue = model->HSM1_coiigs;
    data->type = IF_INTEGER;
    return(OK);
  case  HSM1_MOD_COGIDL:
    value->iValue = model->HSM1_cogidl;
    data->type = IF_INTEGER;
    return(OK);
  case  HSM1_MOD_COOVLP:
    value->iValue = model->HSM1_coovlp;
    data->type = IF_INTEGER;
    return(OK);
  case  HSM1_MOD_CONOIS:
    value->iValue = model->HSM1_conois;
    data->type = IF_INTEGER;
    return(OK);
  case  HSM1_MOD_COISTI: /* HiSIM1.1 */
    value->iValue = model->HSM1_coisti;
    data->type = IF_INTEGER;
    return(OK);
  case  HSM1_MOD_VMAX:
    value->rValue = model->HSM1_vmax;
    return(OK);
  case  HSM1_MOD_BGTMP1:
    value->rValue = model->HSM1_bgtmp1;
    return(OK);
  case  HSM1_MOD_BGTMP2:
    value->rValue = model->HSM1_bgtmp2;
    return(OK);
  case  HSM1_MOD_TOX:
    value->rValue = model->HSM1_tox;
    return(OK);
  case  HSM1_MOD_DL:
    value->rValue = model->HSM1_dl;
    return(OK);
  case  HSM1_MOD_DW:
    value->rValue = model->HSM1_dw;
    return(OK);
  case  HSM1_MOD_XJ: /* HiSIM1.0 */
    value->rValue = model->HSM1_xj;
    return(OK);
  case  HSM1_MOD_XQY: /* HiSIM1.1 */
    value->rValue = model->HSM1_xqy;
    return(OK);
  case  HSM1_MOD_RS:
    value->rValue = model->HSM1_rs;
    return(OK);
  case  HSM1_MOD_RD:
    value->rValue = model->HSM1_rd;
    return(OK);
  case  HSM1_MOD_VFBC:
    value->rValue = model->HSM1_vfbc;
    return(OK);
  case  HSM1_MOD_NSUBC:
    value->rValue = model->HSM1_nsubc;
      return(OK);
  case  HSM1_MOD_PARL1:
    value->rValue = model->HSM1_parl1;
    return(OK);
  case  HSM1_MOD_PARL2:
    value->rValue = model->HSM1_parl2;
    return(OK);
  case  HSM1_MOD_LP:
    value->rValue = model->HSM1_lp;
    return(OK);
  case  HSM1_MOD_NSUBP:
    value->rValue = model->HSM1_nsubp;
    return(OK);
  case  HSM1_MOD_SCP1:
    value->rValue = model->HSM1_scp1;
    return(OK);
  case  HSM1_MOD_SCP2:
    value->rValue = model->HSM1_scp2;
    return(OK);
  case  HSM1_MOD_SCP3:
    value->rValue = model->HSM1_scp3;
    return(OK);
  case  HSM1_MOD_SC1:
    value->rValue = model->HSM1_sc1;
    return(OK);
  case  HSM1_MOD_SC2:
    value->rValue = model->HSM1_sc2;
    return(OK);
  case  HSM1_MOD_SC3:
    value->rValue = model->HSM1_sc3;
    return(OK);
  case  HSM1_MOD_PGD1:
    value->rValue = model->HSM1_pgd1;
    return(OK);
  case  HSM1_MOD_PGD2:
    value->rValue = model->HSM1_pgd2;
    return(OK);
  case  HSM1_MOD_PGD3:
    value->rValue = model->HSM1_pgd3;
    return(OK);
  case  HSM1_MOD_NDEP:
    value->rValue = model->HSM1_ndep;
    return(OK);
  case  HSM1_MOD_NINV:
    value->rValue = model->HSM1_ninv;
    return(OK);
  case  HSM1_MOD_NINVD:
    value->rValue = model->HSM1_ninvd;
    return(OK);
  case  HSM1_MOD_MUECB0:
    value->rValue = model->HSM1_muecb0;
    return(OK);
  case  HSM1_MOD_MUECB1:
    value->rValue = model->HSM1_muecb1;
    return(OK);
  case  HSM1_MOD_MUEPH1:
    value->rValue = model->HSM1_mueph1;
    return(OK);
  case  HSM1_MOD_MUEPH0:
    value->rValue = model->HSM1_mueph0;
    return(OK);
  case  HSM1_MOD_MUEPH2:
    value->rValue = model->HSM1_mueph2;
    return(OK);
  case  HSM1_MOD_W0:
    value->rValue = model->HSM1_w0;
    return(OK);
  case  HSM1_MOD_MUESR1:
    value->rValue = model->HSM1_muesr1;
    return(OK);
  case  HSM1_MOD_MUESR0:
    value->rValue = model->HSM1_muesr0;
    return(OK);
  case  HSM1_MOD_BB:
    value->rValue = model->HSM1_bb;
  return(OK);
  case  HSM1_MOD_VDS0:
    value->rValue = model->HSM1_vds0;
    return(OK);
  case  HSM1_MOD_BC0:
    value->rValue = model->HSM1_bc0;
    return(OK);
  case  HSM1_MOD_BC1:
    value->rValue = model->HSM1_bc1;
    return(OK);
  case  HSM1_MOD_SUB1:
    value->rValue = model->HSM1_sub1;
    return(OK);
  case  HSM1_MOD_SUB2:
    value->rValue = model->HSM1_sub2;
    return(OK);
  case  HSM1_MOD_SUB3:
    value->rValue = model->HSM1_sub3;
    return(OK);
  case  HSM1_MOD_WVTHSC: /* HiSIM1.1 */
    value->rValue = model->HSM1_wvthsc;
    return(OK);
  case  HSM1_MOD_NSTI: /* HiSIM1.1 */
    value->rValue = model->HSM1_nsti;
    return(OK);
  case  HSM1_MOD_WSTI: /* HiSIM1.1 */
    value->rValue = model->HSM1_wsti;
    return(OK);
  case  HSM1_MOD_CGSO:
    value->rValue = model->HSM1_cgso;
    return(OK);
  case  HSM1_MOD_CGDO:
    value->rValue = model->HSM1_cgdo;
    return(OK);
  case  HSM1_MOD_CGBO:
    value->rValue = model->HSM1_cgbo;
    return(OK);
  case  HSM1_MOD_TPOLY:
    value->rValue = model->HSM1_tpoly;
    return(OK);
  case  HSM1_MOD_JS0:
    value->rValue = model->HSM1_js0;
    return(OK);
  case  HSM1_MOD_JS0SW:
    value->rValue = model->HSM1_js0sw;
    return(OK);
  case  HSM1_MOD_NJ:
    value->rValue = model->HSM1_nj;
    return(OK);
  case  HSM1_MOD_NJSW:
    value->rValue = model->HSM1_njsw;
    return(OK);
  case  HSM1_MOD_XTI:
    value->rValue = model->HSM1_xti;
    return(OK);
  case  HSM1_MOD_CJ:
    value->rValue = model->HSM1_cj;
    return(OK);
  case  HSM1_MOD_CJSW:
    value->rValue = model->HSM1_cjsw;
    return(OK);
  case  HSM1_MOD_CJSWG:
    value->rValue = model->HSM1_cjswg;
    return(OK);
  case  HSM1_MOD_MJ:
    value->rValue = model->HSM1_mj;
    return(OK);
  case  HSM1_MOD_MJSW:
    value->rValue = model->HSM1_mjsw;
    return(OK);
  case  HSM1_MOD_MJSWG:
    value->rValue = model->HSM1_mjswg;
    return(OK);
  case  HSM1_MOD_PB:
    value->rValue = model->HSM1_pbsw;
    return(OK);
  case  HSM1_MOD_PBSW:
    value->rValue = model->HSM1_pbsw;
    return(OK);
  case  HSM1_MOD_PBSWG:
    value->rValue = model->HSM1_pbswg;
    return(OK);
  case  HSM1_MOD_XPOLYD:
    value->rValue = model->HSM1_xpolyd;
    return(OK);
  case  HSM1_MOD_CLM1:
    value->rValue = model->HSM1_clm1;
    return(OK);
  case  HSM1_MOD_CLM2:
    value->rValue = model->HSM1_clm2;
    return(OK);
  case  HSM1_MOD_CLM3:
    value->rValue = model->HSM1_clm3;
    return(OK);
  case  HSM1_MOD_MUETMP:
    value->rValue = model->HSM1_muetmp;
    return(OK);
  case  HSM1_MOD_RPOCK1:
    value->rValue = model->HSM1_rpock1;
    return(OK);
  case  HSM1_MOD_RPOCK2:
    value->rValue = model->HSM1_rpock2;
    return(OK);
  case  HSM1_MOD_RPOCP1: /* HiSIM1.1 */
    value->rValue = model->HSM1_rpocp1;
    return(OK);
  case  HSM1_MOD_RPOCP2: /* HiSIM1.1 */
    value->rValue = model->HSM1_rpocp2;
    return(OK);
  case  HSM1_MOD_VOVER:
    value->rValue = model->HSM1_vover;
    return(OK);
  case  HSM1_MOD_VOVERP:
    value->rValue = model->HSM1_voverp;
    return(OK);
  case  HSM1_MOD_WFC:
    value->rValue = model->HSM1_wfc;
    return(OK);
  case  HSM1_MOD_QME1:
    value->rValue = model->HSM1_qme1;
    return(OK);
  case  HSM1_MOD_QME2:
    value->rValue = model->HSM1_qme2;
    return(OK);
  case  HSM1_MOD_QME3:
    value->rValue = model->HSM1_qme3;
    return(OK);
  case  HSM1_MOD_GIDL1:
    value->rValue = model->HSM1_gidl1;
    return(OK);
  case  HSM1_MOD_GIDL2:
    value->rValue = model->HSM1_gidl2;
    return(OK);
  case  HSM1_MOD_GIDL3:
    value->rValue = model->HSM1_gidl3;
    return(OK);
  case  HSM1_MOD_GLEAK1:
    value->rValue = model->HSM1_gleak1;
    return(OK);
  case  HSM1_MOD_GLEAK2:
    value->rValue = model->HSM1_gleak2;
    return(OK);
  case  HSM1_MOD_GLEAK3:
    value->rValue = model->HSM1_gleak3;
    return(OK);
  case  HSM1_MOD_VZADD0:
    value->rValue = model->HSM1_vzadd0;
    return(OK);
  case  HSM1_MOD_PZADD0:
    value->rValue = model->HSM1_pzadd0;
    return(OK);
  case  HSM1_MOD_NFTRP:
    value->rValue = model->HSM1_nftrp;
    return(OK);
  case  HSM1_MOD_NFALP:
    value->rValue = model->HSM1_nfalp;
    return(OK);
  case  HSM1_MOD_CIT:
    value->rValue = model->HSM1_cit;
    return(OK);
  case HSM1_MOD_KF:
    value->rValue = model->HSM1_kf;
    return(OK);
  case HSM1_MOD_AF:
    value->rValue = model->HSM1_af;
    return(OK);
  case HSM1_MOD_EF:
    value->rValue = model->HSM1_ef;
    return(OK);
  default:
    return(E_BADPARM);
  }
  /* NOTREACHED */
}

