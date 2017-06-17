
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
 $Id: jfet2aski.cc,v 2.12 2015/07/26 01:09:12 stevew Exp $
 *========================================================================*/

/**********
Based on jfetask.c
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1987 Mathew Lew and Thomas L. Quarles

Modified to add PS model and new parameter definitions ( Anthony E. Parker )
   Copyright 1994  Macquarie University, Sydney Australia.
   10 Feb 1994:   JFET2vtrap and JFET2pave added
**********/

#include "jfet2defs.h"


int
JFET2dev::askInst(const sCKT *ckt, const sGENinstance *geninst, int which,
    IFdata *data)
{
    const sJFET2instance *here = static_cast<const sJFET2instance*>(geninst);
    IFvalue *value = &data->v;

    // Need to override this for non-real returns.
    data->type = IF_REAL;

    switch(which) {
        case JFET2_TEMP:
            value->rValue = here->JFET2temp-CONSTCtoK;
            return(OK);
        case JFET2_AREA:
            value->rValue = here->JFET2area;
            return(OK);
        case JFET2_IC_VDS:
            value->rValue = here->JFET2icVDS;
            return(OK);
        case JFET2_IC_VGS:
            value->rValue = here->JFET2icVGS;
            return(OK);
        case JFET2_OFF:
            data->type = IF_INTEGER;
            value->iValue = here->JFET2off;
            return(OK);
        case JFET2_DRAINNODE:
            data->type = IF_INTEGER;
            value->iValue = here->JFET2drainNode;
            return(OK);
        case JFET2_GATENODE:
            data->type = IF_INTEGER;
            value->iValue = here->JFET2gateNode;
            return(OK);
        case JFET2_SOURCENODE:
            data->type = IF_INTEGER;
            value->iValue = here->JFET2sourceNode;
            return(OK);
        case JFET2_DRAINPRIMENODE:
            data->type = IF_INTEGER;
            value->iValue = here->JFET2drainPrimeNode;
            return(OK);
        case JFET2_SOURCEPRIMENODE:
            data->type = IF_INTEGER;
            value->iValue = here->JFET2sourcePrimeNode;
            return(OK);
        case JFET2_VGS:
//            value->rValue = *(ckt->CKTstate0 + here->JFET2vgs);
            value->rValue = ckt->interp(here->JFET2vgs);
            return(OK);
        case JFET2_VGD:
//            value->rValue = *(ckt->CKTstate0 + here->JFET2vgd);
            value->rValue = ckt->interp(here->JFET2vgd);
            return(OK);
        case JFET2_CG:
            if (ckt->CKTcurrentAnalysis & DOING_AC) {
                data->type = IF_COMPLEX;
                here->ac_cg(ckt, &data->v.cValue.real, &data->v.cValue.imag);
            }
            else {
//                value->rValue = *(ckt->CKTstate0 + here->JFET2cg);
                value->rValue = ckt->interp(here->JFET2cg);
            }
            return(OK);
        case JFET2_CD:
            if (ckt->CKTcurrentAnalysis & DOING_AC) {
                data->type = IF_COMPLEX;
                here->ac_cd(ckt, &data->v.cValue.real, &data->v.cValue.imag);
            }
            else {
//                value->rValue = *(ckt->CKTstate0 + here->JFET2cd);
                value->rValue = ckt->interp(here->JFET2cd);
            }
            return(OK);
        case JFET2_CGD:
//            value->rValue = *(ckt->CKTstate0 + here->JFET2cgd);
            value->rValue = ckt->interp(here->JFET2cgd);
            return(OK);
        case JFET2_GM:
//            value->rValue = *(ckt->CKTstate0 + here->JFET2gm);
            value->rValue = ckt->interp(here->JFET2gm);
            return(OK);
        case JFET2_GDS:
//            value->rValue = *(ckt->CKTstate0 + here->JFET2gds);
            value->rValue = ckt->interp(here->JFET2gds);
            return(OK);
        case JFET2_GGS:
//            value->rValue = *(ckt->CKTstate0 + here->JFET2ggs);
            value->rValue = ckt->interp(here->JFET2ggs);
            return(OK);
        case JFET2_GGD:
//            value->rValue = *(ckt->CKTstate0 + here->JFET2ggd);
            value->rValue = ckt->interp(here->JFET2ggd);
            return(OK);
        case JFET2_QGS:
//            value->rValue = *(ckt->CKTstate0 + here->JFET2qgs);
            value->rValue = ckt->interp(here->JFET2qgs);
            return(OK);
        case JFET2_CQGS:
//            value->rValue = *(ckt->CKTstate0 + here->JFET2cqgs);
            value->rValue = ckt->interp(here->JFET2cqgs);
            return(OK);
        case JFET2_QGD:
//            value->rValue = *(ckt->CKTstate0 + here->JFET2qgd);
            value->rValue = ckt->interp(here->JFET2qgd);
            return(OK);
        case JFET2_CQGD:
//            value->rValue = *(ckt->CKTstate0 + here->JFET2cqgd);
            value->rValue = ckt->interp(here->JFET2cqgd);
            return(OK);
        case JFET2_VTRAP:
//            value->rValue = *(ckt->CKTstate0 + here->JFET2vtrap);
            value->rValue = ckt->interp(here->JFET2vtrap);
            return(OK);
        case JFET2_PAVE:
//            value->rValue = *(ckt->CKTstate0 + here->JFET2pave);
            value->rValue = ckt->interp(here->JFET2pave);
            return(OK);
        case JFET2_CS :
            if (ckt->CKTcurrentAnalysis & DOING_AC) {
                data->type = IF_COMPLEX;
                here->ac_cs(ckt, &data->v.cValue.real, &data->v.cValue.imag);
/*
                errMsg = MALLOC(strlen(msg)+1);
                errRtn = "JFET2ask";
                strcpy(errMsg,msg);
                return(E_ASKCURRENT);
*/
            } else {
                value->rValue = -ckt->interp(here->JFET2cd);
                value->rValue -= ckt->interp(here->JFET2cg);
/*
                value->rValue = -*(ckt->CKTstate0 + here->JFET2cd);
                value->rValue -= *(ckt->CKTstate0 + here->JFET2cg);
*/
            }
            return(OK);
        case JFET2_POWER :
            if (ckt->CKTcurrentAnalysis & DOING_AC) {
                value->rValue = 0.0;
/*
                errMsg = MALLOC(strlen(msg)+1);
                errRtn = "JFET2ask";
                strcpy(errMsg,msg);
                return(E_ASKPOWER);
*/
            } else {
                value->rValue = ckt->interp(here->JFET2cd) *
                        ckt->rhsOld(here->JFET2drainNode);
                value->rValue += ckt->interp(here->JFET2cg) * 
                        ckt->rhsOld(here->JFET2gateNode);
                value->rValue -= (ckt->interp(here->JFET2cd) +
                        ckt->interp(here->JFET2cg)) *
                        ckt->rhsOld(here->JFET2sourceNode);
            }
            return(OK);
        default:
            return(E_BADPARM);
    }
    /* NOTREACHED */
}


void
sJFET2instance::ac_cd(const sCKT *ckt, double *cdr, double *cdi) const
{
    if (JFET2drainNode != JFET2drainPrimeNode) {
        double gdpr =
            static_cast<const sJFET2model*>(GENmodPtr)->JFET2drainConduct *
            JFET2area;
        *cdr = gdpr* (ckt->rhsOld(JFET2drainNode) -
            ckt->rhsOld(JFET2drainPrimeNode));
        *cdi = gdpr* (ckt->irhsOld(JFET2drainNode) -
            ckt->irhsOld(JFET2drainPrimeNode));
        return;
    }

    if (!ckt->CKTstates[0]) {
        *cdr = 0.0;
        *cdi = 0.0;
        return;
    }
    double gm  = *(ckt->CKTstate0 + JFET2gm);
    double gds = *(ckt->CKTstate0 + JFET2gds);
    double ggd = *(ckt->CKTstate0 + JFET2ggd);
    double xgd = *(ckt->CKTstate0 + JFET2qgd) * ckt->CKTomega;

    cIFcomplex Add(gds + ggd, xgd);
    cIFcomplex Adg(-ggd + gm, -xgd);
    cIFcomplex Ads(-gds - gm, 0);

    *cdr = Add.real* ckt->rhsOld(JFET2drainPrimeNode) -
        Add.imag* ckt->irhsOld(JFET2drainPrimeNode) +
        Adg.real* ckt->rhsOld(JFET2gateNode) -
        Adg.imag* ckt->irhsOld(JFET2gateNode) +
        Ads.real* ckt->rhsOld(JFET2sourcePrimeNode);

    *cdi = Add.real* ckt->irhsOld(JFET2drainPrimeNode) +
        Add.imag* ckt->rhsOld(JFET2drainPrimeNode) +
        Adg.real* ckt->irhsOld(JFET2gateNode) +
        Adg.imag* ckt->rhsOld(JFET2gateNode) +
        Ads.real* ckt->irhsOld(JFET2sourcePrimeNode);
}


void
sJFET2instance::ac_cs(const sCKT *ckt, double *csr, double *csi) const
{
    if (JFET2sourceNode != JFET2sourcePrimeNode) {
        double gspr =
            static_cast<const sJFET2model*>(GENmodPtr)->JFET2drainConduct *
            JFET2area;
        *csr = gspr* (ckt->rhsOld(JFET2sourceNode) -
            ckt->rhsOld(JFET2sourcePrimeNode));
        *csi = gspr* (ckt->irhsOld(JFET2sourceNode) -
            ckt->irhsOld(JFET2sourcePrimeNode));
        return;
    }

    if (!ckt->CKTstates[0]) {
        *csr = 0.0;
        *csi = 0.0;
        return;
    }
    double gm  = *(ckt->CKTstate0 + JFET2gm);
    double gds = *(ckt->CKTstate0 + JFET2gds);
    double ggs = *(ckt->CKTstate0 + JFET2ggs);
    double xgs = *(ckt->CKTstate0 + JFET2qgs) * ckt->CKTomega;

    cIFcomplex Ass(gds + gm + ggs, xgs);
    cIFcomplex Asg(-ggs - gm, -xgs);
    cIFcomplex Asd(-gds, 0);

    *csr = Ass.real* ckt->rhsOld(JFET2sourcePrimeNode) -
        Ass.imag* ckt->irhsOld(JFET2sourcePrimeNode) +
        Asg.real* ckt->rhsOld(JFET2gateNode) -
        Asg.imag* ckt->irhsOld(JFET2gateNode) +
        Asd.real* ckt->rhsOld(JFET2drainPrimeNode);

    *csi = Ass.real* ckt->irhsOld(JFET2sourcePrimeNode) +
        Ass.imag* ckt->rhsOld(JFET2sourcePrimeNode) +
        Asg.real* ckt->irhsOld(JFET2gateNode) +
        Asg.imag* ckt->rhsOld(JFET2gateNode) +
        Asd.real* ckt->irhsOld(JFET2drainPrimeNode);
}


void
sJFET2instance::ac_cg(const sCKT *ckt, double *cgr, double *cgi) const
{
    if (!ckt->CKTstates[0]) {
        *cgr = 0.0;
        *cgi = 0.0;
        return;
    }
    double ggd = *(ckt->CKTstate0 + JFET2ggd);
    double ggs = *(ckt->CKTstate0 + JFET2ggs);
    double xgd = *(ckt->CKTstate0 + JFET2qgd) * ckt->CKTomega;
    double xgs = *(ckt->CKTstate0 + JFET2qgs) * ckt->CKTomega;

    cIFcomplex Agg(ggd + ggs, xgd + xgs);
    cIFcomplex Ags(-ggs, -xgs);
    cIFcomplex Agd(-ggd, -xgd);

    *cgr = Agg.real* ckt->rhsOld(JFET2gateNode) -
        Agg.imag* ckt->irhsOld(JFET2gateNode) +
        Ags.real* ckt->rhsOld(JFET2sourcePrimeNode) -
        Ags.imag* ckt->irhsOld(JFET2sourcePrimeNode) +
        Agd.real* ckt->rhsOld(JFET2drainPrimeNode) -
        Agd.imag* ckt->irhsOld(JFET2drainPrimeNode);

    *cgi = Agg.real* ckt->irhsOld(JFET2gateNode) +
        Agg.imag* ckt->rhsOld(JFET2gateNode) +
        Ags.real* ckt->irhsOld(JFET2sourcePrimeNode) +
        Ags.imag* ckt->rhsOld(JFET2sourcePrimeNode) +
        Agd.real* ckt->irhsOld(JFET2drainPrimeNode) +
        Agd.imag* ckt->rhsOld(JFET2drainPrimeNode);
}

