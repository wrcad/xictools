
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
STAG version 2.6
Copyright 2000 owned by the United Kingdom Secretary of State for Defence
acting through the Defence Evaluation and Research Agency.
Developed by :     Jim Benson,
                   Department of Electronics and Computer Science,
                   University of Southampton,
                   United Kingdom.
With help from :   Nele D'Halleweyn, Bill Redman-White, and Craig Easson.

Based on STAG version 2.1
Developed by :     Mike Lee,
With help from :   Bernard Tenbroek, Bill Redman-White, Mike Uren, Chris Edwards
                   and John Bunyan.
Acknowledgements : Rupert Howes and Pete Mole.
**********/

#include "soi3defs.h"
//#include "gencurrent.h"


int
SOI3dev::askInst(const sCKT *ckt, const sGENinstance *geninst, int which,
    IFdata *data)
{
    const sSOI3instance *here = static_cast<const sSOI3instance*>(geninst);
    IFvalue *value = &data->v;

    // Need to override this for non-real returns.
    data->type = IF_REAL;

    switch(which) {
        case SOI3_L:
            value->rValue = here->SOI3l;
                return(OK);
        case SOI3_W:
            value->rValue = here->SOI3w;
                return(OK);
        case SOI3_NRS:
            value->rValue = here->SOI3sourceSquares;
                return(OK);
        case SOI3_NRD:
            value->rValue = here->SOI3drainSquares;
                return(OK);
        case SOI3_OFF:
            value->rValue = here->SOI3off;
                return(OK);
        case SOI3_IC_VDS:
            value->rValue = here->SOI3icVDS;
                return(OK);
        case SOI3_IC_VGFS:
            value->rValue = here->SOI3icVGFS;
                return(OK);
        case SOI3_IC_VGBS:
            value->rValue = here->SOI3icVGBS;
                return(OK);
        case SOI3_IC_VBS:
            value->rValue = here->SOI3icVBS;
                return(OK);
        case SOI3_TEMP:
            value->rValue = here->SOI3temp-CONSTCtoK;
            return(OK);
        case SOI3_RT:
            value->rValue = here->SOI3rt;
            return(OK);
        case SOI3_CT:
            value->rValue = here->SOI3ct;
            return(OK);
        case SOI3_DNODE:
            data->type = IF_INTEGER;
            value->iValue = here->SOI3dNode;
            return(OK);
        case SOI3_GFNODE:
            data->type = IF_INTEGER;
            value->iValue = here->SOI3gfNode;
            return(OK);
        case SOI3_SNODE:
            data->type = IF_INTEGER;
            value->iValue = here->SOI3sNode;
            return(OK);
        case SOI3_GBNODE:
            data->type = IF_INTEGER;
            value->iValue = here->SOI3gbNode;
            return(OK);
        case SOI3_BNODE:
            data->type = IF_INTEGER;
            value->iValue = here->SOI3bNode;
            return(OK);
        case SOI3_DNODEPRIME:
            data->type = IF_INTEGER;
            value->iValue = here->SOI3dNodePrime;
            return(OK);
        case SOI3_SNODEPRIME:
            data->type = IF_INTEGER;
            value->iValue = here->SOI3sNodePrime;
            return(OK);
        case SOI3_TNODE:
            data->type = IF_INTEGER;
            value->iValue = here->SOI3toutNode;
            return(OK);
        case SOI3_BRANCH:
            data->type = IF_INTEGER;
            value->iValue = here->SOI3branch;
            return(OK);
        case SOI3_SOURCECONDUCT:
            value->rValue = here->SOI3sourceConductance;
            return(OK);
        case SOI3_DRAINCONDUCT:
            value->rValue = here->SOI3drainConductance;
            return(OK);
        case SOI3_VON:
            value->rValue = here->SOI3tVto;
            return(OK);
        case SOI3_VFBF:
            value->rValue = here->SOI3tVfbF;
            return(OK);
        case SOI3_VDSAT:
            value->rValue = here->SOI3vdsat;
            return(OK);
        case SOI3_SOURCEVCRIT:
            value->rValue = here->SOI3sourceVcrit;
            return(OK);
        case SOI3_DRAINVCRIT:
            value->rValue = here->SOI3drainVcrit;
            return(OK);
        case SOI3_ID:
            value->rValue = here->SOI3id;
            value->rValue = ckt->interp(here->SOI3a_id, value->rValue);
            return(OK);
        case SOI3_IBS:
            value->rValue = here->SOI3ibs;
            value->rValue = ckt->interp(here->SOI3a_ibs, value->rValue);
            return(OK);
        case SOI3_IBD:
            value->rValue = here->SOI3ibd;
            value->rValue = ckt->interp(here->SOI3a_ibd, value->rValue);
            return(OK);
        case SOI3_GMBS:
            value->rValue = here->SOI3gmbs;
            return(OK);
        case SOI3_GMF:
            value->rValue = here->SOI3gmf;
            return(OK);
        case SOI3_GMB:
            value->rValue = here->SOI3gmb;
            return(OK);
        case SOI3_GDS:
            value->rValue = here->SOI3gds;
            return(OK);
        case SOI3_GBD:
            value->rValue = here->SOI3gbd;
            return(OK);
        case SOI3_GBS:
            value->rValue = here->SOI3gbs;
            return(OK);
        case SOI3_CAPBD:
            value->rValue = here->SOI3capbd;
            return(OK);
        case SOI3_CAPBS:
            value->rValue = here->SOI3capbs;
            return(OK);
        case SOI3_CAPZEROBIASBD:
            value->rValue = here->SOI3Cbd;
            return(OK);
        case SOI3_CAPZEROBIASBS:
            value->rValue = here->SOI3Cbs;
            return(OK);

        case SOI3_VBD:
//            value->rValue = *(ckt->CKTstate0 + here->SOI3vbd);
            value->rValue = ckt->interp(here->SOI3vbd);
            return(OK);
        case SOI3_VBS:
//            value->rValue = *(ckt->CKTstate0 + here->SOI3vbs);
            value->rValue = ckt->interp(here->SOI3vbs);
            return(OK);
        case SOI3_VGFS:
//            value->rValue = *(ckt->CKTstate0 + here->SOI3vgfs);
            value->rValue = ckt->interp(here->SOI3vgfs);
            return(OK);
        case SOI3_VGBS:
//            value->rValue = *(ckt->CKTstate0 + here->SOI3vgbs);
            value->rValue = ckt->interp(here->SOI3vgbs);
            return(OK);
        case SOI3_VDS:
//            value->rValue = *(ckt->CKTstate0 + here->SOI3vds);
            value->rValue = ckt->interp(here->SOI3vds);
            return(OK);

        case SOI3_QGF:
//            value->rValue = *(ckt->CKTstate0 + here->SOI3qgf);
            value->rValue = ckt->interp(here->SOI3qgf);
            return(OK);
        case SOI3_IQGF:
//            value->rValue = *(ckt->CKTstate0 + here->SOI3iqgf);
            value->rValue = ckt->interp(here->SOI3iqgf);
            return(OK);
        case SOI3_QD:
//            value->rValue = *(ckt->CKTstate0 + here->SOI3qd);
            value->rValue = ckt->interp(here->SOI3qd);
            return(OK);
        case SOI3_IQD:
//            value->rValue = *(ckt->CKTstate0 + here->SOI3iqd);
            value->rValue = ckt->interp(here->SOI3iqd);
            return(OK);
        case SOI3_QS:
//            value->rValue = *(ckt->CKTstate0 + here->SOI3qs);
            value->rValue = ckt->interp(here->SOI3qs);
            return(OK);
        case SOI3_IQS:
//            value->rValue = *(ckt->CKTstate0 + here->SOI3iqs);
            value->rValue = ckt->interp(here->SOI3iqs);
            return(OK);
        case SOI3_CGFGF:
//            value->rValue = *(ckt->CKTstate0 + here->SOI3cgfgf);
            value->rValue = ckt->interp(here->SOI3cgfgf);
            return (OK);
        case SOI3_CGFD:
//            value->rValue = *(ckt->CKTstate0 + here->SOI3cgfd);
            value->rValue = ckt->interp(here->SOI3cgfd);
            return (OK);
        case SOI3_CGFS:
//            value->rValue = *(ckt->CKTstate0 + here->SOI3cgfs);
            value->rValue = ckt->interp(here->SOI3cgfs);
            return (OK);
        case SOI3_CGFDELTAT:
//            value->rValue = *(ckt->CKTstate0 + here->SOI3cgfdeltaT);
            value->rValue = ckt->interp(here->SOI3cgfdeltaT);
            return (OK);
        case SOI3_CDGF:
//            value->rValue = *(ckt->CKTstate0 + here->SOI3cdgf);
            value->rValue = ckt->interp(here->SOI3cdgf);
            return (OK);
        case SOI3_CDD:
//            value->rValue = *(ckt->CKTstate0 + here->SOI3cdd);
            value->rValue = ckt->interp(here->SOI3cdd);
            return (OK);
        case SOI3_CDS:
//            value->rValue = *(ckt->CKTstate0 + here->SOI3cds);
            value->rValue = ckt->interp(here->SOI3cds);
            return (OK);
        case SOI3_CDDELTAT:
//            value->rValue = *(ckt->CKTstate0 + here->SOI3cddeltaT);
            value->rValue = ckt->interp(here->SOI3cddeltaT);
            return (OK);
        case SOI3_CSGF:
//            value->rValue = *(ckt->CKTstate0 + here->SOI3csgf);
            value->rValue = ckt->interp(here->SOI3csgf);
            return (OK);
        case SOI3_CSD:
//            value->rValue = *(ckt->CKTstate0 + here->SOI3csd);
            value->rValue = ckt->interp(here->SOI3csd);
            return (OK);
        case SOI3_CSS:
//            value->rValue = *(ckt->CKTstate0 + here->SOI3css);
            value->rValue = ckt->interp(here->SOI3css);
            return (OK);
        case SOI3_CSDELTAT:
//            value->rValue = *(ckt->CKTstate0 + here->SOI3csdeltaT);
            value->rValue = ckt->interp(here->SOI3csdeltaT);
            return (OK);
        case SOI3_QBD:
//            value->rValue = *(ckt->CKTstate0 + here->SOI3qbd);
            value->rValue = ckt->interp(here->SOI3qbd);
            return(OK);
        case SOI3_IQBD:
//            value->rValue = *(ckt->CKTstate0 + here->SOI3iqbd);
            value->rValue = ckt->interp(here->SOI3iqbd);
            return(OK);
        case SOI3_QBS:
//            value->rValue = *(ckt->CKTstate0 + here->SOI3qbs);
            value->rValue = ckt->interp(here->SOI3qbs);
            return(OK);
        case SOI3_IQBS:
//            value->rValue = *(ckt->CKTstate0 + here->SOI3iqbs);
            value->rValue = ckt->interp(here->SOI3iqbs);
            return(OK);
/* extra stuff for newer model - msll Jan96 */
        case SOI3_VFBB:
            value->rValue = here->SOI3tVfbB;
            return(OK);
        case SOI3_RT1:
            value->rValue = here->SOI3rt1;
            return(OK);
        case SOI3_CT1:
            value->rValue = here->SOI3ct1;
            return(OK);
        case SOI3_RT2:
            value->rValue = here->SOI3rt2;
            return(OK);
        case SOI3_CT2:
            value->rValue = here->SOI3ct2;
            return(OK);
        case SOI3_RT3:
            value->rValue = here->SOI3rt3;
            return(OK);
        case SOI3_CT3:
            value->rValue = here->SOI3ct3;
            return(OK);
        case SOI3_RT4:
            value->rValue = here->SOI3rt4;
            return(OK);
        case SOI3_CT4:
            value->rValue = here->SOI3ct4;
            return(OK);

#ifdef this_is_not_used
        case SOI3_IS :
            if (ckt->CKTcurrentAnalysis & DOING_AC) {
                data->type = IF_COMPLEX;
                data->v.cValue.real = 0.0;
                data->v.cValue.imag = 0.0;
/*
                errMsg = MALLOC(strlen(msg)+1);
                errRtn = "SOI3ask.c";
                strcpy(errMsg,msg);
                return(E_ASKCURRENT);
*/
            } else {
                value->rValue = -ckt->interp(here->SOI3a_id);
                value->rValue -=
                    ckt->interp(here->SOI3a_ibd) +
                    ckt->interp(here->SOI3a_ibs) -
                    ckt->interp(here->SOI3iqgfb);
                if ((ckt->CKTcurrentAnalysis & DOING_TRAN) &&
                        !(ckt->CKTmode & MODETRANOP)) {
                    value->rValue -=
                        ckt->interp(here->SOI3iqgfb) +
                        ckt->interp(here->SOI3iqgfd) +
                        ckt->interp(here->SOI3iqgfs) +
                }
/*
                value->rValue = -here->SOI3id;
                value->rValue -= here->SOI3ibd + here->SOI3ibs -
                        *(ckt->CKTstate0 + here->SOI3iqgfb);
                if ((ckt->CKTcurrentAnalysis & DOING_TRAN) &&
                        !(ckt->CKTmode & MODETRANOP)) {
                    value->rValue -= *(ckt->CKTstate0 + here->SOI3iqgfb) +
                            *(ckt->CKTstate0 + here->SOI3iqgfd) +
                            *(ckt->CKTstate0 + here->SOI3iqgfs);
                }
*/
            }
            return(OK);
        case SOI3_IB :
            if (ckt->CKTcurrentAnalysis & DOING_AC) {
                data->type = IF_COMPLEX;
                data->v.cValue.real = 0.0;
                data->v.cValue.imag = 0.0;
/*
                errMsg = MALLOC(strlen(msg)+1);
                errRtn = "SOI3ask.c";
                strcpy(errMsg,msg);
                return(E_ASKCURRENT);
*/
            } else {
                value->rValue =
                    ckt->interp(here->SOI3a_ibd) +
                    ckt->interp(here->SOI3a_ibs) -
                    ckt->interp(here->SOI3iqgfb);
/*
                value->rValue = here->SOI3ibd + here->SOI3ibs -
                        *(ckt->CKTstate0 + here->SOI3iqgfb);
*/
            }
            return(OK);
        case SOI3_IGF :
            if (ckt->CKTcurrentAnalysis & DOING_AC) {
                data->type = IF_COMPLEX;
                data->v.cValue.real = 0.0;
                data->v.cValue.imag = 0.0;
/*
                errMsg = MALLOC(strlen(msg)+1);
                errRtn = "SOI3ask.c";
                strcpy(errMsg,msg);
                return(E_ASKCURRENT);
*/
            } else if (ckt->CKTcurrentAnalysis & (DOING_DCOP | DOING_TRCV)) {
                value->rValue = 0;
            } else if ((ckt->CKTcurrentAnalysis & DOING_TRAN) &&
                    (ckt->CKTmode & MODETRANOP)) {
                value->rValue = 0;
            } else {
                value->rValue =
                    ckt->interp(here->SOI3iqgfb) +
                    ckt->interp(here->SOI3iqgfd) +
                    ckt->interp(here->SOI3iqgfs);
/*
                value->rValue =   *(ckt->CKTstate0 + here->SOI3iqgfb) +
                        *(ckt->CKTstate0 + here->SOI3iqgfd) + *(ckt->CKTstate0 +
                        here->SOI3iqgfs);
*/
            }
            return(OK);
        case SOI3_IGB :
            if (ckt->CKTcurrentAnalysis & DOING_AC) {
                data->type = IF_COMPLEX;
                data->v.cValue.real = 0.0;
                data->v.cValue.imag = 0.0;
/*
                errMsg = MALLOC(strlen(msg)+1);
                errRtn = "SOI3ask.c";
                strcpy(errMsg,msg);
                return(E_ASKCURRENT);
*/
            } else if (ckt->CKTcurrentAnalysis & (DOING_DCOP | DOING_TRCV)) {
                value->rValue = 0;
            } else if ((ckt->CKTcurrentAnalysis & DOING_TRAN) &&
                    (ckt->CKTmode & MODETRANOP)) {
                value->rValue = 0;
            } else {
                value->rValue =
                    ckt->interp(here->SOI3iqgfb) +
                    ckt->interp(here->SOI3iqgfd) +
                    ckt->interp(here->SOI3iqgfs);
/*
                value->rValue =  *(ckt->CKTstate0 + here->SOI3iqgfb) +
                        *(ckt->CKTstate0 + here->SOI3iqgfd) + *(ckt->CKTstate0 +
                        here->SOI3iqgfs);
*/
            }
            return(OK);
        case SOI3_POWER :
            if (ckt->CKTcurrentAnalysis & DOING_AC) {
                data->type = IF_COMPLEX;
                data->v.cValue.real = 0.0;
                data->v.cValue.imag = 0.0;
/*
                errMsg = MALLOC(strlen(msg)+1);
                errRtn = "SOI3ask.c";
                strcpy(errMsg,msg);
                return(E_ASKPOWER);
*/
            } else {
                double temp;
                value->rValue = here->SOI3id *
                        *(ckt->CKTrhsOld + here->SOI3dNode);
                value->rValue += ((here->SOI3ibd + here->SOI3ibs) -
                        *(ckt->CKTstate0 + here->SOI3iqgfb)) *
                        *(ckt->CKTrhsOld + here->SOI3bNode);
                if ((ckt->CKTcurrentAnalysis & DOING_TRAN) &&
                        !(ckt->CKTmode & MODETRANOP)) {
                    value->rValue += (*(ckt->CKTstate0 + here->SOI3iqgfb) +
                            *(ckt->CKTstate0 + here->SOI3iqgfd) +
                            *(ckt->CKTstate0 + here->SOI3iqgfs)) *
                            *(ckt->CKTrhsOld + here->SOI3gfNode);
                }
                temp = -here->SOI3id;
                temp -= here->SOI3ibd + here->SOI3ibs ;
                if ((ckt->CKTcurrentAnalysis & DOING_TRAN) &&
                        !(ckt->CKTmode & MODETRANOP)) {
                    temp -= *(ckt->CKTstate0 + here->SOI3iqgfb) +
                            *(ckt->CKTstate0 + here->SOI3iqgfd) +
                            *(ckt->CKTstate0 + here->SOI3iqgfs);
                }
                value->rValue += temp * *(ckt->CKTrhsOld + here->SOI3sNode);
            }
            return(OK);
#endif

        default:
            return(E_BADPARM);
    }
    /* NOTREACHED */
}

