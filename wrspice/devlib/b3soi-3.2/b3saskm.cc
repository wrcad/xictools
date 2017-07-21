
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
 $Id: b3saskm.cc,v 1.4 2015/07/26 01:09:09 stevew Exp $
 *========================================================================*/

/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1998 Samuel Fung, Dennis Sinitsky and Stephen Tang
File: b3soimask.c          98/5/01
Modified by Pin Su and Jan Feng	99/2/15
Modified by Pin Su 99/4/30
Modified by Wei Jin 99/9/27
Modified by Pin Su 00/3/1
Modified by Pin Su 01/2/15
Modified by Pin Su 02/3/5
Modified by Pin Su 02/5/20
Modified by Pin Su and Hui Wan 02/11/12
Modified by Pin Su and Hui Wan 03/07/30
**********/

#include "b3sdefs.h"


int
B3SOIdev::askModl(const sGENmodel *genmod, int which, IFdata *data)
{
    const sB3SOImodel *model = static_cast<const sB3SOImodel*>(genmod);
    IFvalue *value = &data->v;
    // Need to override this for non-real returns.
    data->type = IF_REAL;

    switch(which)
    {
    case B3SOI_MOD_MOBMOD:
        value->iValue = model->B3SOImobMod;
        data->type = IF_INTEGER;
        return(OK);
    case B3SOI_MOD_PARAMCHK:
        value->iValue = model->B3SOIparamChk;
        data->type = IF_INTEGER;
        return(OK);
    case B3SOI_MOD_BINUNIT:
        value->iValue = model->B3SOIbinUnit;
        data->type = IF_INTEGER;
        return(OK);
    case B3SOI_MOD_CAPMOD:
        value->iValue = model->B3SOIcapMod;
        data->type = IF_INTEGER;
        return(OK);
    case B3SOI_MOD_SHMOD:
        value->iValue = model->B3SOIshMod;
        data->type = IF_INTEGER;
        return(OK);

        /*        case B3SOI_MOD_NOIMOD:
                    value->iValue = model->B3SOInoiMod;
        data->type = IF_INTEGER;
                    return(OK); v3.2 */

    case  B3SOI_MOD_VERSION :
        value->rValue = model->B3SOIversion;
        return(OK);
    case  B3SOI_MOD_TOX :
        value->rValue = model->B3SOItox;
        return(OK);

    case  B3SOI_MOD_TOXM :
        value->rValue = model->B3SOItoxm;
        return(OK); /* v3.2 */

        /* v2.2.3 */
    case  B3SOI_MOD_DTOXCV :
        value->rValue = model->B3SOIdtoxcv;
        return(OK);

    case  B3SOI_MOD_CDSC :
        value->rValue = model->B3SOIcdsc;
        return(OK);
    case  B3SOI_MOD_CDSCB :
        value->rValue = model->B3SOIcdscb;
        return(OK);

    case  B3SOI_MOD_CDSCD :
        value->rValue = model->B3SOIcdscd;
        return(OK);

    case  B3SOI_MOD_CIT :
        value->rValue = model->B3SOIcit;
        return(OK);
    case  B3SOI_MOD_NFACTOR :
        value->rValue = model->B3SOInfactor;
        return(OK);
    case B3SOI_MOD_VSAT:
        value->rValue = model->B3SOIvsat;
        return(OK);
    case B3SOI_MOD_AT:
        value->rValue = model->B3SOIat;
        return(OK);
    case B3SOI_MOD_A0:
        value->rValue = model->B3SOIa0;
        return(OK);

    case B3SOI_MOD_AGS:
        value->rValue = model->B3SOIags;
        return(OK);

    case B3SOI_MOD_A1:
        value->rValue = model->B3SOIa1;
        return(OK);
    case B3SOI_MOD_A2:
        value->rValue = model->B3SOIa2;
        return(OK);
    case B3SOI_MOD_KETA:
        value->rValue = model->B3SOIketa;
        return(OK);
    case B3SOI_MOD_NSUB:
        value->rValue = model->B3SOInsub;
        return(OK);
    case B3SOI_MOD_NPEAK:
        value->rValue = model->B3SOInpeak;
        return(OK);
    case B3SOI_MOD_NGATE:
        value->rValue = model->B3SOIngate;
        return(OK);
    case B3SOI_MOD_GAMMA1:
        value->rValue = model->B3SOIgamma1;
        return(OK);
    case B3SOI_MOD_GAMMA2:
        value->rValue = model->B3SOIgamma2;
        return(OK);
    case B3SOI_MOD_VBX:
        value->rValue = model->B3SOIvbx;
        return(OK);
    case B3SOI_MOD_VBM:
        value->rValue = model->B3SOIvbm;
        return(OK);
    case B3SOI_MOD_XT:
        value->rValue = model->B3SOIxt;
        return(OK);
    case  B3SOI_MOD_K1:
        value->rValue = model->B3SOIk1;
        return(OK);
    case  B3SOI_MOD_KT1:
        value->rValue = model->B3SOIkt1;
        return(OK);
    case  B3SOI_MOD_KT1L:
        value->rValue = model->B3SOIkt1l;
        return(OK);
    case  B3SOI_MOD_KT2 :
        value->rValue = model->B3SOIkt2;
        return(OK);
    case  B3SOI_MOD_K2 :
        value->rValue = model->B3SOIk2;
        return(OK);
    case  B3SOI_MOD_K3:
        value->rValue = model->B3SOIk3;
        return(OK);
    case  B3SOI_MOD_K3B:
        value->rValue = model->B3SOIk3b;
        return(OK);
    case  B3SOI_MOD_W0:
        value->rValue = model->B3SOIw0;
        return(OK);
    case  B3SOI_MOD_NLX:
        value->rValue = model->B3SOInlx;
        return(OK);
    case  B3SOI_MOD_DVT0 :
        value->rValue = model->B3SOIdvt0;
        return(OK);
    case  B3SOI_MOD_DVT1 :
        value->rValue = model->B3SOIdvt1;
        return(OK);
    case  B3SOI_MOD_DVT2 :
        value->rValue = model->B3SOIdvt2;
        return(OK);
    case  B3SOI_MOD_DVT0W :
        value->rValue = model->B3SOIdvt0w;
        return(OK);
    case  B3SOI_MOD_DVT1W :
        value->rValue = model->B3SOIdvt1w;
        return(OK);
    case  B3SOI_MOD_DVT2W :
        value->rValue = model->B3SOIdvt2w;
        return(OK);
    case  B3SOI_MOD_DROUT :
        value->rValue = model->B3SOIdrout;
        return(OK);
    case  B3SOI_MOD_DSUB :
        value->rValue = model->B3SOIdsub;
        return(OK);
    case B3SOI_MOD_VTH0:
        value->rValue = model->B3SOIvth0;
        return(OK);
    case B3SOI_MOD_UA:
        value->rValue = model->B3SOIua;
        return(OK);
    case B3SOI_MOD_UA1:
        value->rValue = model->B3SOIua1;
        return(OK);
    case B3SOI_MOD_UB:
        value->rValue = model->B3SOIub;
        return(OK);
    case B3SOI_MOD_UB1:
        value->rValue = model->B3SOIub1;
        return(OK);
    case B3SOI_MOD_UC:
        value->rValue = model->B3SOIuc;
        return(OK);
    case B3SOI_MOD_UC1:
        value->rValue = model->B3SOIuc1;
        return(OK);
    case B3SOI_MOD_U0:
        value->rValue = model->B3SOIu0;
        return(OK);
    case B3SOI_MOD_UTE:
        value->rValue = model->B3SOIute;
        return(OK);
    case B3SOI_MOD_VOFF:
        value->rValue = model->B3SOIvoff;
        return(OK);
    case B3SOI_MOD_DELTA:
        value->rValue = model->B3SOIdelta;
        return(OK);
    case B3SOI_MOD_RDSW:
        value->rValue = model->B3SOIrdsw;
        return(OK);
    case B3SOI_MOD_PRWG:
        value->rValue = model->B3SOIprwg;
        return(OK);
    case B3SOI_MOD_PRWB:
        value->rValue = model->B3SOIprwb;
        return(OK);
    case B3SOI_MOD_PRT:
        value->rValue = model->B3SOIprt;
        return(OK);
    case B3SOI_MOD_ETA0:
        value->rValue = model->B3SOIeta0;
        return(OK);
    case B3SOI_MOD_ETAB:
        value->rValue = model->B3SOIetab;
        return(OK);
    case B3SOI_MOD_PCLM:
        value->rValue = model->B3SOIpclm;
        return(OK);
    case B3SOI_MOD_PDIBL1:
        value->rValue = model->B3SOIpdibl1;
        return(OK);
    case B3SOI_MOD_PDIBL2:
        value->rValue = model->B3SOIpdibl2;
        return(OK);
    case B3SOI_MOD_PDIBLB:
        value->rValue = model->B3SOIpdiblb;
        return(OK);
    case B3SOI_MOD_PVAG:
        value->rValue = model->B3SOIpvag;
        return(OK);
    case B3SOI_MOD_WR:
        value->rValue = model->B3SOIwr;
        return(OK);
    case B3SOI_MOD_DWG:
        value->rValue = model->B3SOIdwg;
        return(OK);
    case B3SOI_MOD_DWB:
        value->rValue = model->B3SOIdwb;
        return(OK);
    case B3SOI_MOD_B0:
        value->rValue = model->B3SOIb0;
        return(OK);
    case B3SOI_MOD_B1:
        value->rValue = model->B3SOIb1;
        return(OK);
    case B3SOI_MOD_ALPHA0:
        value->rValue = model->B3SOIalpha0;
        return(OK);

    case B3SOI_MOD_CGSL:
        value->rValue = model->B3SOIcgsl;
        return(OK);
    case B3SOI_MOD_CGDL:
        value->rValue = model->B3SOIcgdl;
        return(OK);
    case B3SOI_MOD_CKAPPA:
        value->rValue = model->B3SOIckappa;
        return(OK);
    case B3SOI_MOD_CF:
        value->rValue = model->B3SOIcf;
        return(OK);
    case B3SOI_MOD_CLC:
        value->rValue = model->B3SOIclc;
        return(OK);
    case B3SOI_MOD_CLE:
        value->rValue = model->B3SOIcle;
        return(OK);
    case B3SOI_MOD_DWC:
        value->rValue = model->B3SOIdwc;
        return(OK);
    case B3SOI_MOD_DLC:
        value->rValue = model->B3SOIdlc;
        return(OK);


    case B3SOI_MOD_TBOX:
        value->rValue = model->B3SOItbox;
        return(OK);
    case B3SOI_MOD_TSI:
        value->rValue = model->B3SOItsi;
        return(OK);
    case B3SOI_MOD_RTH0:
        value->rValue = model->B3SOIrth0;
        return(OK);
    case B3SOI_MOD_CTH0:
        value->rValue = model->B3SOIcth0;
        return(OK);
    case B3SOI_MOD_NDIODE:
        value->rValue = model->B3SOIndiode;
        return(OK);
    case B3SOI_MOD_XBJT:
        value->rValue = model->B3SOIxbjt;
        return(OK);

    case B3SOI_MOD_XDIF:
        value->rValue = model->B3SOIxdif;
        return(OK);

    case B3SOI_MOD_XREC:
        value->rValue = model->B3SOIxrec;
        return(OK);
    case B3SOI_MOD_XTUN:
        value->rValue = model->B3SOIxtun;
        return(OK);
    case B3SOI_MOD_TT:
        value->rValue = model->B3SOItt;
        return(OK);
    case B3SOI_MOD_VSDTH:
        value->rValue = model->B3SOIvsdth;
        return(OK);
    case B3SOI_MOD_VSDFB:
        value->rValue = model->B3SOIvsdfb;
        return(OK);
    case B3SOI_MOD_CSDMIN:
        value->rValue = model->B3SOIcsdmin;
        return(OK);
    case B3SOI_MOD_ASD:
        value->rValue = model->B3SOIasd;
        return(OK);

    case  B3SOI_MOD_TNOM :
        value->rValue = model->B3SOItnom;
        return(OK);
    case B3SOI_MOD_CGSO:
        value->rValue = model->B3SOIcgso;
        return(OK);
    case B3SOI_MOD_CGDO:
        value->rValue = model->B3SOIcgdo;
        return(OK);
    case B3SOI_MOD_CGEO:
        value->rValue = model->B3SOIcgeo;
        return(OK);


    case B3SOI_MOD_XPART:
        value->rValue = model->B3SOIxpart;
        return(OK);
    case B3SOI_MOD_RSH:
        value->rValue = model->B3SOIsheetResistance;
        return(OK);
    case B3SOI_MOD_PBSWG:
        value->rValue = model->B3SOIGatesidewallJctPotential;
        return(OK);
    case B3SOI_MOD_MJSWG:
        value->rValue = model->B3SOIbodyJctGateSideGradingCoeff;
        return(OK);
    case B3SOI_MOD_CJSWG:
        value->rValue = model->B3SOIunitLengthGateSidewallJctCap;
        return(OK);
    case B3SOI_MOD_CSDESW:
        value->rValue = model->B3SOIcsdesw;
        return(OK);
    case B3SOI_MOD_LINT:
        value->rValue = model->B3SOILint;
        return(OK);
    case B3SOI_MOD_LL:
        value->rValue = model->B3SOILl;
        return(OK);
        /* v2.2.3 */
    case B3SOI_MOD_LLC:
        value->rValue = model->B3SOILlc;
        return(OK);

    case B3SOI_MOD_LLN:
        value->rValue = model->B3SOILln;
        return(OK);
    case B3SOI_MOD_LW:
        value->rValue = model->B3SOILw;
        return(OK);
        /* v2.2.3 */
    case B3SOI_MOD_LWC:
        value->rValue = model->B3SOILwc;
        return(OK);

    case B3SOI_MOD_LWN:
        value->rValue = model->B3SOILwn;
        return(OK);
    case B3SOI_MOD_LWL:
        value->rValue = model->B3SOILwl;
        return(OK);
        /* v2.2.3 */
    case B3SOI_MOD_LWLC:
        value->rValue = model->B3SOILwlc;
        return(OK);

    case B3SOI_MOD_WINT:
        value->rValue = model->B3SOIWint;
        return(OK);
    case B3SOI_MOD_WL:
        value->rValue = model->B3SOIWl;
        return(OK);
        /* v2.2.3 */
    case B3SOI_MOD_WLC:
        value->rValue = model->B3SOIWlc;
        return(OK);

    case B3SOI_MOD_WLN:
        value->rValue = model->B3SOIWln;
        return(OK);
    case B3SOI_MOD_WW:
        value->rValue = model->B3SOIWw;
        return(OK);
        /* v2.2.3 */
    case B3SOI_MOD_WWC:
        value->rValue = model->B3SOIWwc;
        return(OK);

    case B3SOI_MOD_WWN:
        value->rValue = model->B3SOIWwn;
        return(OK);
    case B3SOI_MOD_WWL:
        value->rValue = model->B3SOIWwl;
        return(OK);
        /* v2.2.3 */
    case B3SOI_MOD_WWLC:
        value->rValue = model->B3SOIWwlc;
        return(OK);

    case B3SOI_MOD_NOIA:
        value->rValue = model->B3SOIoxideTrapDensityA;
        return(OK);
    case B3SOI_MOD_NOIB:
        value->rValue = model->B3SOIoxideTrapDensityB;
        return(OK);
    case B3SOI_MOD_NOIC:
        value->rValue = model->B3SOIoxideTrapDensityC;
        return(OK);
    case B3SOI_MOD_NOIF:
        value->rValue = model->B3SOInoif;
        return(OK);
    case B3SOI_MOD_EM:
        value->rValue = model->B3SOIem;
        return(OK);
    case B3SOI_MOD_EF:
        value->rValue = model->B3SOIef;
        return(OK);
    case B3SOI_MOD_AF:
        value->rValue = model->B3SOIaf;
        return(OK);
    case B3SOI_MOD_KF:
        value->rValue = model->B3SOIkf;
        return(OK);


        /* v2.0 release */
    case B3SOI_MOD_K1W1:
        value->rValue = model->B3SOIk1w1;
        return(OK);
    case B3SOI_MOD_K1W2:
        value->rValue = model->B3SOIk1w2;
        return(OK);
    case B3SOI_MOD_KETAS:
        value->rValue = model->B3SOIketas;
        return(OK);
    case B3SOI_MOD_DWBC:
        value->rValue = model->B3SOIdwbc;
        return(OK);
    case B3SOI_MOD_BETA0:
        value->rValue = model->B3SOIbeta0;
        return(OK);
    case B3SOI_MOD_BETA1:
        value->rValue = model->B3SOIbeta1;
        return(OK);
    case B3SOI_MOD_BETA2:
        value->rValue = model->B3SOIbeta2;
        return(OK);
    case B3SOI_MOD_VDSATII0:
        value->rValue = model->B3SOIvdsatii0;
        return(OK);
    case B3SOI_MOD_TII:
        value->rValue = model->B3SOItii;
        return(OK);
    case B3SOI_MOD_LII:
        value->rValue = model->B3SOIlii;
        return(OK);
    case B3SOI_MOD_SII0:
        value->rValue = model->B3SOIsii0;
        return(OK);
    case B3SOI_MOD_SII1:
        value->rValue = model->B3SOIsii1;
        return(OK);
    case B3SOI_MOD_SII2:
        value->rValue = model->B3SOIsii2;
        return(OK);
    case B3SOI_MOD_SIID:
        value->rValue = model->B3SOIsiid;
        return(OK);
    case B3SOI_MOD_FBJTII:
        value->rValue = model->B3SOIfbjtii;
        return(OK);
    case B3SOI_MOD_ESATII:
        value->rValue = model->B3SOIesatii;
        return(OK);
    case B3SOI_MOD_NTUN:
        value->rValue = model->B3SOIntun;
        return(OK);
    case B3SOI_MOD_NRECF0:
        value->rValue = model->B3SOInrecf0;
        return(OK);
    case B3SOI_MOD_NRECR0:
        value->rValue = model->B3SOInrecr0;
        return(OK);
    case B3SOI_MOD_ISBJT:
        value->rValue = model->B3SOIisbjt;
        return(OK);
    case B3SOI_MOD_ISDIF:
        value->rValue = model->B3SOIisdif;
        return(OK);
    case B3SOI_MOD_ISREC:
        value->rValue = model->B3SOIisrec;
        return(OK);
    case B3SOI_MOD_ISTUN:
        value->rValue = model->B3SOIistun;
        return(OK);
    case B3SOI_MOD_LN:
        value->rValue = model->B3SOIln;
        return(OK);
    case B3SOI_MOD_VREC0:
        value->rValue = model->B3SOIvrec0;
        return(OK);
    case B3SOI_MOD_VTUN0:
        value->rValue = model->B3SOIvtun0;
        return(OK);
    case B3SOI_MOD_NBJT:
        value->rValue = model->B3SOInbjt;
        return(OK);
    case B3SOI_MOD_LBJT0:
        value->rValue = model->B3SOIlbjt0;
        return(OK);
    case B3SOI_MOD_LDIF0:
        value->rValue = model->B3SOIldif0;
        return(OK);
    case B3SOI_MOD_VABJT:
        value->rValue = model->B3SOIvabjt;
        return(OK);
    case B3SOI_MOD_AELY:
        value->rValue = model->B3SOIaely;
        return(OK);
    case B3SOI_MOD_AHLI:
        value->rValue = model->B3SOIahli;
        return(OK);

        // SRW
    case B3SOI_MOD_XJ :
        value->rValue = model->B3SOIxj;
        return (OK);

    case B3SOI_MOD_RBODY:
        value->rValue = model->B3SOIrbody;
        return(OK);
    case B3SOI_MOD_RBSH:
        value->rValue = model->B3SOIrbsh;
        return(OK);
    case B3SOI_MOD_NTRECF:
        value->rValue = model->B3SOIntrecf;
        return(OK);
    case B3SOI_MOD_NTRECR:
        value->rValue = model->B3SOIntrecr;
        return(OK);
    case B3SOI_MOD_NDIF:
        value->rValue = model->B3SOIndif;
        return(OK);
    case B3SOI_MOD_DLCB:
        value->rValue = model->B3SOIdlcb;
        return(OK);
    case B3SOI_MOD_FBODY:
        value->rValue = model->B3SOIfbody;
        return(OK);
    case B3SOI_MOD_TCJSWG:
        value->rValue = model->B3SOItcjswg;
        return(OK);
    case B3SOI_MOD_TPBSWG:
        value->rValue = model->B3SOItpbswg;
        return(OK);
    case B3SOI_MOD_ACDE:
        value->rValue = model->B3SOIacde;
        return(OK);
    case B3SOI_MOD_MOIN:
        value->rValue = model->B3SOImoin;
        return(OK);
    case B3SOI_MOD_NOFF:
        value->rValue = model->B3SOInoff;
        return(OK); /* v3.2 */
    case B3SOI_MOD_DELVT:
        value->rValue = model->B3SOIdelvt;
        return(OK);
    case  B3SOI_MOD_KB1:
        value->rValue = model->B3SOIkb1;
        return(OK);
    case B3SOI_MOD_DLBG:
        value->rValue = model->B3SOIdlbg;
        return(OK);

    case B3SOI_MOD_NGIDL:
        value->rValue = model->B3SOIngidl;
        return(OK);
    case B3SOI_MOD_AGIDL:
        value->rValue = model->B3SOIagidl;
        return(OK);
    case B3SOI_MOD_BGIDL:
        value->rValue = model->B3SOIbgidl;
        return(OK);

        /* v3.0 */
    case B3SOI_MOD_SOIMOD:
        value->iValue = model->B3SOIsoiMod;
        data->type = IF_INTEGER;
        return(OK); /* v3.2 bug fix */
    case B3SOI_MOD_VBS0PD:
        value->rValue = model->B3SOIvbs0pd;
        return(OK); /* v3.2 */
    case B3SOI_MOD_VBS0FD:
        value->rValue = model->B3SOIvbs0fd;
        return(OK); /* v3.2 */
    case B3SOI_MOD_VBSA:
        value->rValue = model->B3SOIvbsa;
        return(OK);
    case B3SOI_MOD_NOFFFD:
        value->rValue = model->B3SOInofffd;
        return(OK);
    case B3SOI_MOD_VOFFFD:
        value->rValue = model->B3SOIvofffd;
        return(OK);
    case B3SOI_MOD_K1B:
        value->rValue = model->B3SOIk1b;
        return(OK);
    case B3SOI_MOD_K2B:
        value->rValue = model->B3SOIk2b;
        return(OK);
    case B3SOI_MOD_DK2B:
        value->rValue = model->B3SOIdk2b;
        return(OK);
    case B3SOI_MOD_DVBD0:
        value->rValue = model->B3SOIdvbd0;
        return(OK);
    case B3SOI_MOD_DVBD1:
        value->rValue = model->B3SOIdvbd1;
        return(OK);
    case B3SOI_MOD_MOINFD:
        value->rValue = model->B3SOImoinFD;
        return(OK);



        /* v2.2 release */
    case B3SOI_MOD_WTH0:
        value->rValue = model->B3SOIwth0;
        return(OK);
    case B3SOI_MOD_RHALO:
        value->rValue = model->B3SOIrhalo;
        return(OK);
    case B3SOI_MOD_NTOX:
        value->rValue = model->B3SOIntox;
        return(OK);
    case B3SOI_MOD_TOXREF:
        value->rValue = model->B3SOItoxref;
        return(OK);
    case B3SOI_MOD_EBG:
        value->rValue = model->B3SOIebg;
        return(OK);
    case B3SOI_MOD_VEVB:
        value->rValue = model->B3SOIvevb;
        return(OK);
    case B3SOI_MOD_ALPHAGB1:
        value->rValue = model->B3SOIalphaGB1;
        return(OK);
    case B3SOI_MOD_BETAGB1:
        value->rValue = model->B3SOIbetaGB1;
        return(OK);
    case B3SOI_MOD_VGB1:
        value->rValue = model->B3SOIvgb1;
        return(OK);
    case B3SOI_MOD_VECB:
        value->rValue = model->B3SOIvecb;
        return(OK);
    case B3SOI_MOD_ALPHAGB2:
        value->rValue = model->B3SOIalphaGB2;
        return(OK);
    case B3SOI_MOD_BETAGB2:
        value->rValue = model->B3SOIbetaGB2;
        return(OK);
    case B3SOI_MOD_VGB2:
        value->rValue = model->B3SOIvgb2;
        return(OK);
    case B3SOI_MOD_TOXQM:
        value->rValue = model->B3SOItoxqm;
        return(OK);
    case B3SOI_MOD_VOXH:
        value->rValue = model->B3SOIvoxh;
        return(OK);
    case B3SOI_MOD_DELTAVOX:
        value->rValue = model->B3SOIdeltavox;
        return(OK);

        /* v3.2 */
    case B3SOI_MOD_FNOIMOD :
        value->iValue = model->B3SOIfnoiMod;
        data->type = IF_INTEGER;
        return(OK);
    case B3SOI_MOD_TNOIMOD :
        value->iValue = model->B3SOItnoiMod;
        data->type = IF_INTEGER;
        return(OK);
    case B3SOI_MOD_TNOIA:
        value->rValue = model->B3SOItnoia;
        return(OK);
    case B3SOI_MOD_TNOIB:
        value->rValue = model->B3SOItnoib;
        return(OK);
    case B3SOI_MOD_RNOIA:
        value->rValue = model->B3SOIrnoia;
        return(OK);
    case B3SOI_MOD_RNOIB:
        value->rValue = model->B3SOIrnoib;
        return(OK);
    case B3SOI_MOD_NTNOI:
        value->rValue = model->B3SOIntnoi;
        return(OK);
        /* v3.2 */

        /* v3.1 wanh added for RF */
    case B3SOI_MOD_RGATEMOD :
        value->iValue = model->B3SOIrgateMod;
        data->type = IF_INTEGER;
        return(OK);
    case B3SOI_MOD_XRCRG1:
// SRW -- Was iValue, bug?
        value->rValue = model->B3SOIxrcrg1;
        return(OK);
    case B3SOI_MOD_XRCRG2:
// SRW -- Was iValue, bug?
        value->rValue = model->B3SOIxrcrg2;
        return(OK);
    case B3SOI_MOD_RSHG:
        value->rValue = model->B3SOIrshg;
        return(OK);
    case B3SOI_MOD_NGCON:
        value->rValue = model->B3SOIngcon;
        return(OK);
    case B3SOI_MOD_XGW:
        value->rValue = model->B3SOIxgw;
        return(OK);
    case B3SOI_MOD_XGL:
        value->rValue = model->B3SOIxgl;
        return(OK);
        /* v3.1 wanh added for RF end */

        /* v3.0 */
    case B3SOI_MOD_IGBMOD:
        value->iValue = model->B3SOIigbMod;
        data->type = IF_INTEGER;
        return(OK);
    case B3SOI_MOD_IGCMOD:
        value->iValue = model->B3SOIigcMod;
        data->type = IF_INTEGER;
        return(OK);
    case B3SOI_MOD_AIGC:
        value->rValue = model->B3SOIaigc;
        return(OK);
    case B3SOI_MOD_BIGC:
        value->rValue = model->B3SOIbigc;
        return(OK);
    case B3SOI_MOD_CIGC:
        value->rValue = model->B3SOIcigc;
        return(OK);
    case B3SOI_MOD_AIGSD:
        value->rValue = model->B3SOIaigsd;
        return(OK);
    case B3SOI_MOD_BIGSD:
        value->rValue = model->B3SOIbigsd;
        return(OK);
    case B3SOI_MOD_CIGSD:
        value->rValue = model->B3SOIcigsd;
        return(OK);
    case B3SOI_MOD_NIGC:
        value->rValue = model->B3SOInigc;
        return(OK);
    case B3SOI_MOD_PIGCD:
        value->rValue = model->B3SOIpigcd;
        return(OK);
    case B3SOI_MOD_POXEDGE:
        value->rValue = model->B3SOIpoxedge;
        return(OK);
    case B3SOI_MOD_DLCIG:
        value->rValue = model->B3SOIdlcig;
        return(OK);



        /* Added for binning - START */
        /* Length Dependence */
        /* v3.1 */
    case B3SOI_MOD_LXJ:
        value->rValue = model->B3SOIlxj;
        return(OK);
    case B3SOI_MOD_LALPHAGB1:
        value->rValue = model->B3SOIlalphaGB1;
        return(OK);
    case B3SOI_MOD_LALPHAGB2:
        value->rValue = model->B3SOIlalphaGB2;
        return(OK);
    case B3SOI_MOD_LBETAGB1:
        value->rValue = model->B3SOIlbetaGB1;
        return(OK);
    case B3SOI_MOD_LBETAGB2:
        value->rValue = model->B3SOIlbetaGB2;
        return(OK);
    case B3SOI_MOD_LNDIF:
        value->rValue = model->B3SOIlndif;
        return(OK);
    case B3SOI_MOD_LNTRECF:
        value->rValue = model->B3SOIlntrecf;
        return(OK);
    case B3SOI_MOD_LNTRECR:
        value->rValue = model->B3SOIlntrecr;
        return(OK);
    case B3SOI_MOD_LXBJT:
        value->rValue = model->B3SOIlxbjt;
        return(OK);
    case B3SOI_MOD_LXDIF:
        value->rValue = model->B3SOIlxdif;
        return(OK);
    case B3SOI_MOD_LXREC:
        value->rValue = model->B3SOIlxrec;
        return(OK);
    case B3SOI_MOD_LXTUN:
        value->rValue = model->B3SOIlxtun;
        return(OK);
    case B3SOI_MOD_LCGDL:
        value->rValue = model->B3SOIlcgdl;
        return(OK);
    case B3SOI_MOD_LCGSL:
        value->rValue = model->B3SOIlcgsl;
        return(OK);
    case B3SOI_MOD_LCKAPPA:
        value->rValue = model->B3SOIlckappa;
        return(OK);
    case B3SOI_MOD_LUTE:
        value->rValue = model->B3SOIlute;
        return(OK);
    case B3SOI_MOD_LKT1:
        value->rValue = model->B3SOIlkt1;
        return(OK);
    case B3SOI_MOD_LKT2:
        value->rValue = model->B3SOIlkt2;
        return(OK);
    case B3SOI_MOD_LKT1L:
        value->rValue = model->B3SOIlkt1l;
        return(OK);
    case B3SOI_MOD_LUA1:
        value->rValue = model->B3SOIlua1;
        return(OK);
    case B3SOI_MOD_LUB1:
        value->rValue = model->B3SOIlub1;
        return(OK);
    case B3SOI_MOD_LUC1:
        value->rValue = model->B3SOIluc1;
        return(OK);
    case B3SOI_MOD_LAT:
        value->rValue = model->B3SOIlat;
        return(OK);
    case B3SOI_MOD_LPRT:
        value->rValue = model->B3SOIlprt;
        return(OK);


        /* v3.0 */
    case B3SOI_MOD_LAIGC:
        value->rValue = model->B3SOIlaigc;
        return(OK);
    case B3SOI_MOD_LBIGC:
        value->rValue = model->B3SOIlbigc;
        return(OK);
    case B3SOI_MOD_LCIGC:
        value->rValue = model->B3SOIlcigc;
        return(OK);
    case B3SOI_MOD_LAIGSD:
        value->rValue = model->B3SOIlaigsd;
        return(OK);
    case B3SOI_MOD_LBIGSD:
        value->rValue = model->B3SOIlbigsd;
        return(OK);
    case B3SOI_MOD_LCIGSD:
        value->rValue = model->B3SOIlcigsd;
        return(OK);
    case B3SOI_MOD_LNIGC:
        value->rValue = model->B3SOIlnigc;
        return(OK);
    case B3SOI_MOD_LPIGCD:
        value->rValue = model->B3SOIlpigcd;
        return(OK);
    case B3SOI_MOD_LPOXEDGE:
        value->rValue = model->B3SOIlpoxedge;
        return(OK);

    case B3SOI_MOD_LNPEAK:
        value->rValue = model->B3SOIlnpeak;
        return(OK);
    case B3SOI_MOD_LNSUB:
        value->rValue = model->B3SOIlnsub;
        return(OK);
    case B3SOI_MOD_LNGATE:
        value->rValue = model->B3SOIlngate;
        return(OK);
    case B3SOI_MOD_LVTH0:
        value->rValue = model->B3SOIlvth0;
        return(OK);
    case  B3SOI_MOD_LK1:
        value->rValue = model->B3SOIlk1;
        return(OK);
    case  B3SOI_MOD_LK1W1:
        value->rValue = model->B3SOIlk1w1;
        return(OK);
    case  B3SOI_MOD_LK1W2:
        value->rValue = model->B3SOIlk1w2;
        return(OK);
    case  B3SOI_MOD_LK2:
        value->rValue = model->B3SOIlk2;
        return(OK);
    case  B3SOI_MOD_LK3:
        value->rValue = model->B3SOIlk3;
        return(OK);
    case  B3SOI_MOD_LK3B:
        value->rValue = model->B3SOIlk3b;
        return(OK);
    case  B3SOI_MOD_LKB1:
        value->rValue = model->B3SOIlkb1;
        return(OK);
    case  B3SOI_MOD_LW0:
        value->rValue = model->B3SOIlw0;
        return(OK);
    case  B3SOI_MOD_LNLX:
        value->rValue = model->B3SOIlnlx;
        return(OK);
    case  B3SOI_MOD_LDVT0 :
        value->rValue = model->B3SOIldvt0;
        return(OK);
    case  B3SOI_MOD_LDVT1 :
        value->rValue = model->B3SOIldvt1;
        return(OK);
    case  B3SOI_MOD_LDVT2 :
        value->rValue = model->B3SOIldvt2;
        return(OK);
    case  B3SOI_MOD_LDVT0W :
        value->rValue = model->B3SOIldvt0w;
        return(OK);
    case  B3SOI_MOD_LDVT1W :
        value->rValue = model->B3SOIldvt1w;
        return(OK);
    case  B3SOI_MOD_LDVT2W :
        value->rValue = model->B3SOIldvt2w;
        return(OK);
    case B3SOI_MOD_LU0:
        value->rValue = model->B3SOIlu0;
        return(OK);
    case B3SOI_MOD_LUA:
        value->rValue = model->B3SOIlua;
        return(OK);
    case B3SOI_MOD_LUB:
        value->rValue = model->B3SOIlub;
        return(OK);
    case B3SOI_MOD_LUC:
        value->rValue = model->B3SOIluc;
        return(OK);
    case B3SOI_MOD_LVSAT:
        value->rValue = model->B3SOIlvsat;
        return(OK);
    case B3SOI_MOD_LA0:
        value->rValue = model->B3SOIla0;
        return(OK);
    case B3SOI_MOD_LAGS:
        value->rValue = model->B3SOIlags;
        return(OK);
    case B3SOI_MOD_LB0:
        value->rValue = model->B3SOIlb0;
        return(OK);
    case B3SOI_MOD_LB1:
        value->rValue = model->B3SOIlb1;
        return(OK);
    case B3SOI_MOD_LKETA:
        value->rValue = model->B3SOIlketa;
        return(OK);
    case B3SOI_MOD_LKETAS:
        value->rValue = model->B3SOIlketas;
        return(OK);
    case B3SOI_MOD_LA1:
        value->rValue = model->B3SOIla1;
        return(OK);
    case B3SOI_MOD_LA2:
        value->rValue = model->B3SOIla2;
        return(OK);
    case B3SOI_MOD_LRDSW:
        value->rValue = model->B3SOIlrdsw;
        return(OK);
    case B3SOI_MOD_LPRWB:
        value->rValue = model->B3SOIlprwb;
        return(OK);
    case B3SOI_MOD_LPRWG:
        value->rValue = model->B3SOIlprwg;
        return(OK);
    case B3SOI_MOD_LWR:
        value->rValue = model->B3SOIlwr;
        return(OK);
    case  B3SOI_MOD_LNFACTOR :
        value->rValue = model->B3SOIlnfactor;
        return(OK);
    case B3SOI_MOD_LDWG:
        value->rValue = model->B3SOIldwg;
        return(OK);
    case B3SOI_MOD_LDWB:
        value->rValue = model->B3SOIldwb;
        return(OK);
    case B3SOI_MOD_LVOFF:
        value->rValue = model->B3SOIlvoff;
        return(OK);
    case B3SOI_MOD_LETA0:
        value->rValue = model->B3SOIleta0;
        return(OK);
    case B3SOI_MOD_LETAB:
        value->rValue = model->B3SOIletab;
        return(OK);
    case  B3SOI_MOD_LDSUB :
        value->rValue = model->B3SOIldsub;
        return(OK);
    case  B3SOI_MOD_LCIT :
        value->rValue = model->B3SOIlcit;
        return(OK);
    case  B3SOI_MOD_LCDSC :
        value->rValue = model->B3SOIlcdsc;
        return(OK);
    case  B3SOI_MOD_LCDSCB :
        value->rValue = model->B3SOIlcdscb;
        return(OK);
    case  B3SOI_MOD_LCDSCD :
        value->rValue = model->B3SOIlcdscd;
        return(OK);
    case B3SOI_MOD_LPCLM:
        value->rValue = model->B3SOIlpclm;
        return(OK);
    case B3SOI_MOD_LPDIBL1:
        value->rValue = model->B3SOIlpdibl1;
        return(OK);
    case B3SOI_MOD_LPDIBL2:
        value->rValue = model->B3SOIlpdibl2;
        return(OK);
    case B3SOI_MOD_LPDIBLB:
        value->rValue = model->B3SOIlpdiblb;
        return(OK);
    case  B3SOI_MOD_LDROUT :
        value->rValue = model->B3SOIldrout;
        return(OK);
    case B3SOI_MOD_LPVAG:
        value->rValue = model->B3SOIlpvag;
        return(OK);
    case B3SOI_MOD_LDELTA:
        value->rValue = model->B3SOIldelta;
        return(OK);
    case B3SOI_MOD_LALPHA0:
        value->rValue = model->B3SOIlalpha0;
        return(OK);
    case B3SOI_MOD_LFBJTII:
        value->rValue = model->B3SOIlfbjtii;
        return(OK);
    case B3SOI_MOD_LBETA0:
        value->rValue = model->B3SOIlbeta0;
        return(OK);
    case B3SOI_MOD_LBETA1:
        value->rValue = model->B3SOIlbeta1;
        return(OK);
    case B3SOI_MOD_LBETA2:
        value->rValue = model->B3SOIlbeta2;
        return(OK);
    case B3SOI_MOD_LVDSATII0:
        value->rValue = model->B3SOIlvdsatii0;
        return(OK);
    case B3SOI_MOD_LLII:
        value->rValue = model->B3SOIllii;
        return(OK);
    case B3SOI_MOD_LESATII:
        value->rValue = model->B3SOIlesatii;
        return(OK);
    case B3SOI_MOD_LSII0:
        value->rValue = model->B3SOIlsii0;
        return(OK);
    case B3SOI_MOD_LSII1:
        value->rValue = model->B3SOIlsii1;
        return(OK);
    case B3SOI_MOD_LSII2:
        value->rValue = model->B3SOIlsii2;
        return(OK);
    case B3SOI_MOD_LSIID:
        value->rValue = model->B3SOIlsiid;
        return(OK);
    case B3SOI_MOD_LAGIDL:
        value->rValue = model->B3SOIlagidl;
        return(OK);
    case B3SOI_MOD_LBGIDL:
        value->rValue = model->B3SOIlbgidl;
        return(OK);
    case B3SOI_MOD_LNGIDL:
        value->rValue = model->B3SOIlngidl;
        return(OK);
    case B3SOI_MOD_LNTUN:
        value->rValue = model->B3SOIlntun;
        return(OK);
    case B3SOI_MOD_LNDIODE:
        value->rValue = model->B3SOIlndiode;
        return(OK);
    case B3SOI_MOD_LNRECF0:
        value->rValue = model->B3SOIlnrecf0;
        return(OK);
    case B3SOI_MOD_LNRECR0:
        value->rValue = model->B3SOIlnrecr0;
        return(OK);
    case B3SOI_MOD_LISBJT:
        value->rValue = model->B3SOIlisbjt;
        return(OK);
    case B3SOI_MOD_LISDIF:
        value->rValue = model->B3SOIlisdif;
        return(OK);
    case B3SOI_MOD_LISREC:
        value->rValue = model->B3SOIlisrec;
        return(OK);
    case B3SOI_MOD_LISTUN:
        value->rValue = model->B3SOIlistun;
        return(OK);
    case B3SOI_MOD_LVREC0:
        value->rValue = model->B3SOIlvrec0;
        return(OK);
    case B3SOI_MOD_LVTUN0:
        value->rValue = model->B3SOIlvtun0;
        return(OK);
    case B3SOI_MOD_LNBJT:
        value->rValue = model->B3SOIlnbjt;
        return(OK);
    case B3SOI_MOD_LLBJT0:
        value->rValue = model->B3SOIllbjt0;
        return(OK);
    case B3SOI_MOD_LVABJT:
        value->rValue = model->B3SOIlvabjt;
        return(OK);
    case B3SOI_MOD_LAELY:
        value->rValue = model->B3SOIlaely;
        return(OK);
    case B3SOI_MOD_LAHLI:
        value->rValue = model->B3SOIlahli;
        return(OK);
        /* CV Model */
    case B3SOI_MOD_LVSDFB:
        value->rValue = model->B3SOIlvsdfb;
        return(OK);
    case B3SOI_MOD_LVSDTH:
        value->rValue = model->B3SOIlvsdth;
        return(OK);
    case B3SOI_MOD_LDELVT:
        value->rValue = model->B3SOIldelvt;
        return(OK);
    case B3SOI_MOD_LACDE:
        value->rValue = model->B3SOIlacde;
        return(OK);
    case B3SOI_MOD_LMOIN:
        value->rValue = model->B3SOIlmoin;
        return(OK);
    case B3SOI_MOD_LNOFF:
        value->rValue = model->B3SOIlnoff;
        return(OK); /* v3.2 */

        /* Width Dependence */
        /* v3.1 */
    case B3SOI_MOD_WXJ:
        value->rValue = model->B3SOIwxj;
        return(OK);
    case B3SOI_MOD_WALPHAGB1:
        value->rValue = model->B3SOIwalphaGB1;
        return(OK);
    case B3SOI_MOD_WALPHAGB2:
        value->rValue = model->B3SOIwalphaGB2;
        return(OK);
    case B3SOI_MOD_WBETAGB1:
        value->rValue = model->B3SOIwbetaGB1;
        return(OK);
    case B3SOI_MOD_WBETAGB2:
        value->rValue = model->B3SOIwbetaGB2;
        return(OK);
    case B3SOI_MOD_WNDIF:
        value->rValue = model->B3SOIwndif;
        return(OK);
    case B3SOI_MOD_WNTRECF:
        value->rValue = model->B3SOIwntrecf;
        return(OK);
    case B3SOI_MOD_WNTRECR:
        value->rValue = model->B3SOIwntrecr;
        return(OK);
    case B3SOI_MOD_WXBJT:
        value->rValue = model->B3SOIwxbjt;
        return(OK);
    case B3SOI_MOD_WXDIF:
        value->rValue = model->B3SOIwxdif;
        return(OK);
    case B3SOI_MOD_WXREC:
        value->rValue = model->B3SOIwxrec;
        return(OK);
    case B3SOI_MOD_WXTUN:
        value->rValue = model->B3SOIwxtun;
        return(OK);
    case B3SOI_MOD_WCGDL:
        value->rValue = model->B3SOIwcgdl;
        return(OK);
    case B3SOI_MOD_WCGSL:
        value->rValue = model->B3SOIwcgsl;
        return(OK);
    case B3SOI_MOD_WCKAPPA:
        value->rValue = model->B3SOIwckappa;
        return(OK);
    case B3SOI_MOD_WUTE:
        value->rValue = model->B3SOIwute;
        return(OK);
    case B3SOI_MOD_WKT1:
        value->rValue = model->B3SOIwkt1;
        return(OK);
    case B3SOI_MOD_WKT2:
        value->rValue = model->B3SOIwkt2;
        return(OK);
    case B3SOI_MOD_WKT1L:
        value->rValue = model->B3SOIwkt1l;
        return(OK);
    case B3SOI_MOD_WUA1:
        value->rValue = model->B3SOIwua1;
        return(OK);
    case B3SOI_MOD_WUB1:
        value->rValue = model->B3SOIwub1;
        return(OK);
    case B3SOI_MOD_WUC1:
        value->rValue = model->B3SOIwuc1;
        return(OK);
    case B3SOI_MOD_WAT:
        value->rValue = model->B3SOIwat;
        return(OK);
    case B3SOI_MOD_WPRT:
        value->rValue = model->B3SOIwprt;
        return(OK);

        /* v3.0 */
    case B3SOI_MOD_WAIGC:
        value->rValue = model->B3SOIwaigc;
        return(OK);
    case B3SOI_MOD_WBIGC:
        value->rValue = model->B3SOIwbigc;
        return(OK);
    case B3SOI_MOD_WCIGC:
        value->rValue = model->B3SOIwcigc;
        return(OK);
    case B3SOI_MOD_WAIGSD:
        value->rValue = model->B3SOIwaigsd;
        return(OK);
    case B3SOI_MOD_WBIGSD:
        value->rValue = model->B3SOIwbigsd;
        return(OK);
    case B3SOI_MOD_WCIGSD:
        value->rValue = model->B3SOIwcigsd;
        return(OK);
    case B3SOI_MOD_WNIGC:
        value->rValue = model->B3SOIwnigc;
        return(OK);
    case B3SOI_MOD_WPIGCD:
        value->rValue = model->B3SOIwpigcd;
        return(OK);
    case B3SOI_MOD_WPOXEDGE:
        value->rValue = model->B3SOIwpoxedge;
        return(OK);

    case B3SOI_MOD_WNPEAK:
        value->rValue = model->B3SOIwnpeak;
        return(OK);
    case B3SOI_MOD_WNSUB:
        value->rValue = model->B3SOIwnsub;
        return(OK);
    case B3SOI_MOD_WNGATE:
        value->rValue = model->B3SOIwngate;
        return(OK);
    case B3SOI_MOD_WVTH0:
        value->rValue = model->B3SOIwvth0;
        return(OK);
    case  B3SOI_MOD_WK1:
        value->rValue = model->B3SOIwk1;
        return(OK);
    case  B3SOI_MOD_WK1W1:
        value->rValue = model->B3SOIwk1w1;
        return(OK);
    case  B3SOI_MOD_WK1W2:
        value->rValue = model->B3SOIwk1w2;
        return(OK);
    case  B3SOI_MOD_WK2:
        value->rValue = model->B3SOIwk2;
        return(OK);
    case  B3SOI_MOD_WK3:
        value->rValue = model->B3SOIwk3;
        return(OK);
    case  B3SOI_MOD_WK3B:
        value->rValue = model->B3SOIwk3b;
        return(OK);
    case  B3SOI_MOD_WKB1:
        value->rValue = model->B3SOIwkb1;
        return(OK);
    case  B3SOI_MOD_WW0:
        value->rValue = model->B3SOIww0;
        return(OK);
    case  B3SOI_MOD_WNLX:
        value->rValue = model->B3SOIwnlx;
        return(OK);
    case  B3SOI_MOD_WDVT0 :
        value->rValue = model->B3SOIwdvt0;
        return(OK);
    case  B3SOI_MOD_WDVT1 :
        value->rValue = model->B3SOIwdvt1;
        return(OK);
    case  B3SOI_MOD_WDVT2 :
        value->rValue = model->B3SOIwdvt2;
        return(OK);
    case  B3SOI_MOD_WDVT0W :
        value->rValue = model->B3SOIwdvt0w;
        return(OK);
    case  B3SOI_MOD_WDVT1W :
        value->rValue = model->B3SOIwdvt1w;
        return(OK);
    case  B3SOI_MOD_WDVT2W :
        value->rValue = model->B3SOIwdvt2w;
        return(OK);
    case B3SOI_MOD_WU0:
        value->rValue = model->B3SOIwu0;
        return(OK);
    case B3SOI_MOD_WUA:
        value->rValue = model->B3SOIwua;
        return(OK);
    case B3SOI_MOD_WUB:
        value->rValue = model->B3SOIwub;
        return(OK);
    case B3SOI_MOD_WUC:
        value->rValue = model->B3SOIwuc;
        return(OK);
    case B3SOI_MOD_WVSAT:
        value->rValue = model->B3SOIwvsat;
        return(OK);
    case B3SOI_MOD_WA0:
        value->rValue = model->B3SOIwa0;
        return(OK);
    case B3SOI_MOD_WAGS:
        value->rValue = model->B3SOIwags;
        return(OK);
    case B3SOI_MOD_WB0:
        value->rValue = model->B3SOIwb0;
        return(OK);
    case B3SOI_MOD_WB1:
        value->rValue = model->B3SOIwb1;
        return(OK);
    case B3SOI_MOD_WKETA:
        value->rValue = model->B3SOIwketa;
        return(OK);
    case B3SOI_MOD_WKETAS:
        value->rValue = model->B3SOIwketas;
        return(OK);
    case B3SOI_MOD_WA1:
        value->rValue = model->B3SOIwa1;
        return(OK);
    case B3SOI_MOD_WA2:
        value->rValue = model->B3SOIwa2;
        return(OK);
    case B3SOI_MOD_WRDSW:
        value->rValue = model->B3SOIwrdsw;
        return(OK);
    case B3SOI_MOD_WPRWB:
        value->rValue = model->B3SOIwprwb;
        return(OK);
    case B3SOI_MOD_WPRWG:
        value->rValue = model->B3SOIwprwg;
        return(OK);
    case B3SOI_MOD_WWR:
        value->rValue = model->B3SOIwwr;
        return(OK);
    case  B3SOI_MOD_WNFACTOR :
        value->rValue = model->B3SOIwnfactor;
        return(OK);
    case B3SOI_MOD_WDWG:
        value->rValue = model->B3SOIwdwg;
        return(OK);
    case B3SOI_MOD_WDWB:
        value->rValue = model->B3SOIwdwb;
        return(OK);
    case B3SOI_MOD_WVOFF:
        value->rValue = model->B3SOIwvoff;
        return(OK);
    case B3SOI_MOD_WETA0:
        value->rValue = model->B3SOIweta0;
        return(OK);
    case B3SOI_MOD_WETAB:
        value->rValue = model->B3SOIwetab;
        return(OK);
    case  B3SOI_MOD_WDSUB :
        value->rValue = model->B3SOIwdsub;
        return(OK);
    case  B3SOI_MOD_WCIT :
        value->rValue = model->B3SOIwcit;
        return(OK);
    case  B3SOI_MOD_WCDSC :
        value->rValue = model->B3SOIwcdsc;
        return(OK);
    case  B3SOI_MOD_WCDSCB :
        value->rValue = model->B3SOIwcdscb;
        return(OK);
    case  B3SOI_MOD_WCDSCD :
        value->rValue = model->B3SOIwcdscd;
        return(OK);
    case B3SOI_MOD_WPCLM:
        value->rValue = model->B3SOIwpclm;
        return(OK);
    case B3SOI_MOD_WPDIBL1:
        value->rValue = model->B3SOIwpdibl1;
        return(OK);
    case B3SOI_MOD_WPDIBL2:
        value->rValue = model->B3SOIwpdibl2;
        return(OK);
    case B3SOI_MOD_WPDIBLB:
        value->rValue = model->B3SOIwpdiblb;
        return(OK);
    case  B3SOI_MOD_WDROUT :
        value->rValue = model->B3SOIwdrout;
        return(OK);
    case B3SOI_MOD_WPVAG:
        value->rValue = model->B3SOIwpvag;
        return(OK);
    case B3SOI_MOD_WDELTA:
        value->rValue = model->B3SOIwdelta;
        return(OK);
    case B3SOI_MOD_WALPHA0:
        value->rValue = model->B3SOIwalpha0;
        return(OK);
    case B3SOI_MOD_WFBJTII:
        value->rValue = model->B3SOIwfbjtii;
        return(OK);
    case B3SOI_MOD_WBETA0:
        value->rValue = model->B3SOIwbeta0;
        return(OK);
    case B3SOI_MOD_WBETA1:
        value->rValue = model->B3SOIwbeta1;
        return(OK);
    case B3SOI_MOD_WBETA2:
        value->rValue = model->B3SOIwbeta2;
        return(OK);
    case B3SOI_MOD_WVDSATII0:
        value->rValue = model->B3SOIwvdsatii0;
        return(OK);
    case B3SOI_MOD_WLII:
        value->rValue = model->B3SOIwlii;
        return(OK);
    case B3SOI_MOD_WESATII:
        value->rValue = model->B3SOIwesatii;
        return(OK);
    case B3SOI_MOD_WSII0:
        value->rValue = model->B3SOIwsii0;
        return(OK);
    case B3SOI_MOD_WSII1:
        value->rValue = model->B3SOIwsii1;
        return(OK);
    case B3SOI_MOD_WSII2:
        value->rValue = model->B3SOIwsii2;
        return(OK);
    case B3SOI_MOD_WSIID:
        value->rValue = model->B3SOIwsiid;
        return(OK);
    case B3SOI_MOD_WAGIDL:
        value->rValue = model->B3SOIwagidl;
        return(OK);
    case B3SOI_MOD_WBGIDL:
        value->rValue = model->B3SOIwbgidl;
        return(OK);
    case B3SOI_MOD_WNGIDL:
        value->rValue = model->B3SOIwngidl;
        return(OK);
    case B3SOI_MOD_WNTUN:
        value->rValue = model->B3SOIwntun;
        return(OK);
    case B3SOI_MOD_WNDIODE:
        value->rValue = model->B3SOIwndiode;
        return(OK);
    case B3SOI_MOD_WNRECF0:
        value->rValue = model->B3SOIwnrecf0;
        return(OK);
    case B3SOI_MOD_WNRECR0:
        value->rValue = model->B3SOIwnrecr0;
        return(OK);
    case B3SOI_MOD_WISBJT:
        value->rValue = model->B3SOIwisbjt;
        return(OK);
    case B3SOI_MOD_WISDIF:
        value->rValue = model->B3SOIwisdif;
        return(OK);
    case B3SOI_MOD_WISREC:
        value->rValue = model->B3SOIwisrec;
        return(OK);
    case B3SOI_MOD_WISTUN:
        value->rValue = model->B3SOIwistun;
        return(OK);
    case B3SOI_MOD_WVREC0:
        value->rValue = model->B3SOIwvrec0;
        return(OK);
    case B3SOI_MOD_WVTUN0:
        value->rValue = model->B3SOIwvtun0;
        return(OK);
    case B3SOI_MOD_WNBJT:
        value->rValue = model->B3SOIwnbjt;
        return(OK);
    case B3SOI_MOD_WLBJT0:
        value->rValue = model->B3SOIwlbjt0;
        return(OK);
    case B3SOI_MOD_WVABJT:
        value->rValue = model->B3SOIwvabjt;
        return(OK);
    case B3SOI_MOD_WAELY:
        value->rValue = model->B3SOIwaely;
        return(OK);
    case B3SOI_MOD_WAHLI:
        value->rValue = model->B3SOIwahli;
        return(OK);
        /* CV Model */
    case B3SOI_MOD_WVSDFB:
        value->rValue = model->B3SOIwvsdfb;
        return(OK);
    case B3SOI_MOD_WVSDTH:
        value->rValue = model->B3SOIwvsdth;
        return(OK);
    case B3SOI_MOD_WDELVT:
        value->rValue = model->B3SOIwdelvt;
        return(OK);
    case B3SOI_MOD_WACDE:
        value->rValue = model->B3SOIwacde;
        return(OK);
    case B3SOI_MOD_WMOIN:
        value->rValue = model->B3SOIwmoin;
        return(OK);
    case B3SOI_MOD_WNOFF:
        value->rValue = model->B3SOIwnoff;
        return(OK); /* v3.2 */

        /* Cross-term Dependence */
        /* v3.1 */
    case B3SOI_MOD_PXJ:
        value->rValue = model->B3SOIpxj;
        return(OK);
    case B3SOI_MOD_PALPHAGB1:
        value->rValue = model->B3SOIpalphaGB1;
        return(OK);
    case B3SOI_MOD_PALPHAGB2:
        value->rValue = model->B3SOIpalphaGB2;
        return(OK);
    case B3SOI_MOD_PBETAGB1:
        value->rValue = model->B3SOIpbetaGB1;
        return(OK);
    case B3SOI_MOD_PBETAGB2:
        value->rValue = model->B3SOIpbetaGB2;
        return(OK);
    case B3SOI_MOD_PNDIF:
        value->rValue = model->B3SOIpndif;
        return(OK);
    case B3SOI_MOD_PNTRECF:
        value->rValue = model->B3SOIpntrecf;
        return(OK);
    case B3SOI_MOD_PNTRECR:
        value->rValue = model->B3SOIpntrecr;
        return(OK);
    case B3SOI_MOD_PXBJT:
        value->rValue = model->B3SOIpxbjt;
        return(OK);
    case B3SOI_MOD_PXDIF:
        value->rValue = model->B3SOIpxdif;
        return(OK);
    case B3SOI_MOD_PXREC:
        value->rValue = model->B3SOIpxrec;
        return(OK);
    case B3SOI_MOD_PXTUN:
        value->rValue = model->B3SOIpxtun;
        return(OK);
    case B3SOI_MOD_PCGDL:
        value->rValue = model->B3SOIpcgdl;
        return(OK);
    case B3SOI_MOD_PCGSL:
        value->rValue = model->B3SOIpcgsl;
        return(OK);
    case B3SOI_MOD_PCKAPPA:
        value->rValue = model->B3SOIpckappa;
        return(OK);
    case B3SOI_MOD_PUTE:
        value->rValue = model->B3SOIpute;
        return(OK);
    case B3SOI_MOD_PKT1:
        value->rValue = model->B3SOIpkt1;
        return(OK);
    case B3SOI_MOD_PKT2:
        value->rValue = model->B3SOIpkt2;
        return(OK);
    case B3SOI_MOD_PKT1L:
        value->rValue = model->B3SOIpkt1l;
        return(OK);
    case B3SOI_MOD_PUA1:
        value->rValue = model->B3SOIpua1;
        return(OK);
    case B3SOI_MOD_PUB1:
        value->rValue = model->B3SOIpub1;
        return(OK);
    case B3SOI_MOD_PUC1:
        value->rValue = model->B3SOIpuc1;
        return(OK);
    case B3SOI_MOD_PAT:
        value->rValue = model->B3SOIpat;
        return(OK);
    case B3SOI_MOD_PPRT:
        value->rValue = model->B3SOIpprt;
        return(OK);


        /* v3.0 */
    case B3SOI_MOD_PAIGC:
        value->rValue = model->B3SOIpaigc;
        return(OK);
    case B3SOI_MOD_PBIGC:
        value->rValue = model->B3SOIpbigc;
        return(OK);
    case B3SOI_MOD_PCIGC:
        value->rValue = model->B3SOIpcigc;
        return(OK);
    case B3SOI_MOD_PAIGSD:
        value->rValue = model->B3SOIpaigsd;
        return(OK);
    case B3SOI_MOD_PBIGSD:
        value->rValue = model->B3SOIpbigsd;
        return(OK);
    case B3SOI_MOD_PCIGSD:
        value->rValue = model->B3SOIpcigsd;
        return(OK);
    case B3SOI_MOD_PNIGC:
        value->rValue = model->B3SOIpnigc;
        return(OK);
    case B3SOI_MOD_PPIGCD:
        value->rValue = model->B3SOIppigcd;
        return(OK);
    case B3SOI_MOD_PPOXEDGE:
        value->rValue = model->B3SOIppoxedge;
        return(OK);

    case B3SOI_MOD_PNPEAK:
        value->rValue = model->B3SOIpnpeak;
        return(OK);
    case B3SOI_MOD_PNSUB:
        value->rValue = model->B3SOIpnsub;
        return(OK);
    case B3SOI_MOD_PNGATE:
        value->rValue = model->B3SOIpngate;
        return(OK);
    case B3SOI_MOD_PVTH0:
        value->rValue = model->B3SOIpvth0;
        return(OK);
    case  B3SOI_MOD_PK1:
        value->rValue = model->B3SOIpk1;
        return(OK);
    case  B3SOI_MOD_PK1W1:
        value->rValue = model->B3SOIpk1w1;
        return(OK);
    case  B3SOI_MOD_PK1W2:
        value->rValue = model->B3SOIpk1w2;
        return(OK);
    case  B3SOI_MOD_PK2:
        value->rValue = model->B3SOIpk2;
        return(OK);
    case  B3SOI_MOD_PK3:
        value->rValue = model->B3SOIpk3;
        return(OK);
    case  B3SOI_MOD_PK3B:
        value->rValue = model->B3SOIpk3b;
        return(OK);
    case  B3SOI_MOD_PKB1:
        value->rValue = model->B3SOIpkb1;
        return(OK);
    case  B3SOI_MOD_PW0:
        value->rValue = model->B3SOIpw0;
        return(OK);
    case  B3SOI_MOD_PNLX:
        value->rValue = model->B3SOIpnlx;
        return(OK);
    case  B3SOI_MOD_PDVT0 :
        value->rValue = model->B3SOIpdvt0;
        return(OK);
    case  B3SOI_MOD_PDVT1 :
        value->rValue = model->B3SOIpdvt1;
        return(OK);
    case  B3SOI_MOD_PDVT2 :
        value->rValue = model->B3SOIpdvt2;
        return(OK);
    case  B3SOI_MOD_PDVT0W :
        value->rValue = model->B3SOIpdvt0w;
        return(OK);
    case  B3SOI_MOD_PDVT1W :
        value->rValue = model->B3SOIpdvt1w;
        return(OK);
    case  B3SOI_MOD_PDVT2W :
        value->rValue = model->B3SOIpdvt2w;
        return(OK);
    case B3SOI_MOD_PU0:
        value->rValue = model->B3SOIpu0;
        return(OK);
    case B3SOI_MOD_PUA:
        value->rValue = model->B3SOIpua;
        return(OK);
    case B3SOI_MOD_PUB:
        value->rValue = model->B3SOIpub;
        return(OK);
    case B3SOI_MOD_PUC:
        value->rValue = model->B3SOIpuc;
        return(OK);
    case B3SOI_MOD_PVSAT:
        value->rValue = model->B3SOIpvsat;
        return(OK);
    case B3SOI_MOD_PA0:
        value->rValue = model->B3SOIpa0;
        return(OK);
    case B3SOI_MOD_PAGS:
        value->rValue = model->B3SOIpags;
        return(OK);
    case B3SOI_MOD_PB0:
        value->rValue = model->B3SOIpb0;
        return(OK);
    case B3SOI_MOD_PB1:
        value->rValue = model->B3SOIpb1;
        return(OK);
    case B3SOI_MOD_PKETA:
        value->rValue = model->B3SOIpketa;
        return(OK);
    case B3SOI_MOD_PKETAS:
        value->rValue = model->B3SOIpketas;
        return(OK);
    case B3SOI_MOD_PA1:
        value->rValue = model->B3SOIpa1;
        return(OK);
    case B3SOI_MOD_PA2:
        value->rValue = model->B3SOIpa2;
        return(OK);
    case B3SOI_MOD_PRDSW:
        value->rValue = model->B3SOIprdsw;
        return(OK);
    case B3SOI_MOD_PPRWB:
        value->rValue = model->B3SOIpprwb;
        return(OK);
    case B3SOI_MOD_PPRWG:
        value->rValue = model->B3SOIpprwg;
        return(OK);
    case B3SOI_MOD_PWR:
        value->rValue = model->B3SOIpwr;
        return(OK);
    case  B3SOI_MOD_PNFACTOR :
        value->rValue = model->B3SOIpnfactor;
        return(OK);
    case B3SOI_MOD_PDWG:
        value->rValue = model->B3SOIpdwg;
        return(OK);
    case B3SOI_MOD_PDWB:
        value->rValue = model->B3SOIpdwb;
        return(OK);
    case B3SOI_MOD_PVOFF:
        value->rValue = model->B3SOIpvoff;
        return(OK);
    case B3SOI_MOD_PETA0:
        value->rValue = model->B3SOIpeta0;
        return(OK);
    case B3SOI_MOD_PETAB:
        value->rValue = model->B3SOIpetab;
        return(OK);
    case  B3SOI_MOD_PDSUB :
        value->rValue = model->B3SOIpdsub;
        return(OK);
    case  B3SOI_MOD_PCIT :
        value->rValue = model->B3SOIpcit;
        return(OK);
    case  B3SOI_MOD_PCDSC :
        value->rValue = model->B3SOIpcdsc;
        return(OK);
    case  B3SOI_MOD_PCDSCB :
        value->rValue = model->B3SOIpcdscb;
        return(OK);
    case  B3SOI_MOD_PCDSCD :
        value->rValue = model->B3SOIpcdscd;
        return(OK);
    case B3SOI_MOD_PPCLM:
        value->rValue = model->B3SOIppclm;
        return(OK);
    case B3SOI_MOD_PPDIBL1:
        value->rValue = model->B3SOIppdibl1;
        return(OK);
    case B3SOI_MOD_PPDIBL2:
        value->rValue = model->B3SOIppdibl2;
        return(OK);
    case B3SOI_MOD_PPDIBLB:
        value->rValue = model->B3SOIppdiblb;
        return(OK);
    case  B3SOI_MOD_PDROUT :
        value->rValue = model->B3SOIpdrout;
        return(OK);
    case B3SOI_MOD_PPVAG:
        value->rValue = model->B3SOIppvag;
        return(OK);
    case B3SOI_MOD_PDELTA:
        value->rValue = model->B3SOIpdelta;
        return(OK);
    case B3SOI_MOD_PALPHA0:
        value->rValue = model->B3SOIpalpha0;
        return(OK);
    case B3SOI_MOD_PFBJTII:
        value->rValue = model->B3SOIpfbjtii;
        return(OK);
    case B3SOI_MOD_PBETA0:
        value->rValue = model->B3SOIpbeta0;
        return(OK);
    case B3SOI_MOD_PBETA1:
        value->rValue = model->B3SOIpbeta1;
        return(OK);
    case B3SOI_MOD_PBETA2:
        value->rValue = model->B3SOIpbeta2;
        return(OK);
    case B3SOI_MOD_PVDSATII0:
        value->rValue = model->B3SOIpvdsatii0;
        return(OK);
    case B3SOI_MOD_PLII:
        value->rValue = model->B3SOIplii;
        return(OK);
    case B3SOI_MOD_PESATII:
        value->rValue = model->B3SOIpesatii;
        return(OK);
    case B3SOI_MOD_PSII0:
        value->rValue = model->B3SOIpsii0;
        return(OK);
    case B3SOI_MOD_PSII1:
        value->rValue = model->B3SOIpsii1;
        return(OK);
    case B3SOI_MOD_PSII2:
        value->rValue = model->B3SOIpsii2;
        return(OK);
    case B3SOI_MOD_PSIID:
        value->rValue = model->B3SOIpsiid;
        return(OK);
    case B3SOI_MOD_PAGIDL:
        value->rValue = model->B3SOIpagidl;
        return(OK);
    case B3SOI_MOD_PBGIDL:
        value->rValue = model->B3SOIpbgidl;
        return(OK);
    case B3SOI_MOD_PNGIDL:
        value->rValue = model->B3SOIpngidl;
        return(OK);
    case B3SOI_MOD_PNTUN:
        value->rValue = model->B3SOIpntun;
        return(OK);
    case B3SOI_MOD_PNDIODE:
        value->rValue = model->B3SOIpndiode;
        return(OK);
    case B3SOI_MOD_PNRECF0:
        value->rValue = model->B3SOIpnrecf0;
        return(OK);
    case B3SOI_MOD_PNRECR0:
        value->rValue = model->B3SOIpnrecr0;
        return(OK);
    case B3SOI_MOD_PISBJT:
        value->rValue = model->B3SOIpisbjt;
        return(OK);
    case B3SOI_MOD_PISDIF:
        value->rValue = model->B3SOIpisdif;
        return(OK);
    case B3SOI_MOD_PISREC:
        value->rValue = model->B3SOIpisrec;
        return(OK);
    case B3SOI_MOD_PISTUN:
        value->rValue = model->B3SOIpistun;
        return(OK);
    case B3SOI_MOD_PVREC0:
        value->rValue = model->B3SOIpvrec0;
        return(OK);
    case B3SOI_MOD_PVTUN0:
        value->rValue = model->B3SOIpvtun0;
        return(OK);
    case B3SOI_MOD_PNBJT:
        value->rValue = model->B3SOIpnbjt;
        return(OK);
    case B3SOI_MOD_PLBJT0:
        value->rValue = model->B3SOIplbjt0;
        return(OK);
    case B3SOI_MOD_PVABJT:
        value->rValue = model->B3SOIpvabjt;
        return(OK);
    case B3SOI_MOD_PAELY:
        value->rValue = model->B3SOIpaely;
        return(OK);
    case B3SOI_MOD_PAHLI:
        value->rValue = model->B3SOIpahli;
        return(OK);
        /* CV Model */
    case B3SOI_MOD_PVSDFB:
        value->rValue = model->B3SOIpvsdfb;
        return(OK);
    case B3SOI_MOD_PVSDTH:
        value->rValue = model->B3SOIpvsdth;
        return(OK);
    case B3SOI_MOD_PDELVT:
        value->rValue = model->B3SOIpdelvt;
        return(OK);
    case B3SOI_MOD_PACDE:
        value->rValue = model->B3SOIpacde;
        return(OK);
    case B3SOI_MOD_PMOIN:
        value->rValue = model->B3SOIpmoin;
        return(OK);
    case B3SOI_MOD_PNOFF:
        value->rValue = model->B3SOIpnoff;
        return(OK); /* v3.2 */
        /* Added for binning - END */

    default:
        return(E_BADPARM);
    }
    /* NOTREACHED */
}

