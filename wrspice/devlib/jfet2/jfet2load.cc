
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
Based on jfetload.c
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1985 Thomas L. Quarles

Modified to add PS model and new parameter definitions ( Anthony E. Parker )
   Copyright 1994  Macquarie University, Sydney Australia.
   10 Feb 1994:  New code added to call psmodel.c routines
**********/

#include "jfet2defs.h"

#define JFET2nextModel      next()
#define JFET2nextInstance   next()
#define JFET2instances      inst()
#define GENinstance sGENinstance
#define CKTreltol CKTcurTask->TSKreltol
#define CKTabstol CKTcurTask->TSKabstol
#define CKTvoltTol CKTcurTask->TSKvoltTol
#define CKTgmin CKTcurTask->TSKgmin
#define CKTbypass CKTcurTask->TSKbypass
#define CKTfixLimit CKTcurTask->TSKfixLimit
#define MAX SPMAX
#define MIN SPMIN
#define NIintegrate(ck, g, c, a, b) ((ck)->integrate(g, c, a, b),OK)


int
// SRW JFET2dev::load(sGENmodel *genmod, sCKT *ckt)
JFET2dev::load(sGENinstance *in_inst, sCKT *ckt)
{
// SRW    sJFET2model *model = static_cast<sJFET2model*>(genmod);
// SRW    sJFET2instance *here;
    sJFET2instance *here = (sJFET2instance*)in_inst;
    sJFET2model *model = (sJFET2model*)here->GENmodPtr;
    double capgd;
    double capgs;
    double cd;
    double cdhat = 0.0;
    double cdreq;
    double ceq;
    double ceqgd;
    double ceqgs;
    double cg;
    double cgd;
    double cghat = 0.0;
//    double czgd;
//    double czgdf2;
//    double czgs;
//    double czgsf2;
    double delvds;
    double delvgd;
    double delvgs;
//    double fcpb2;
    double gdpr;
    double gds;
    double geq;
    double ggd;
    double ggs;
    double gm;
    double gspr;
//    double sarg;
//    double twop;
    double vds;
    double vgd;
    double vgs;
    double xfact;
    int icheck;
    int ichk1;
    int error;

/* SRW
    for( ; model != NULL; model = model->JFET2nextModel ) {

        for (here = model->JFET2instances; here != NULL ;
                here=here->JFET2nextInstance) {
*/

            /*
             *  dc model parameters 
             */
            gdpr=model->JFET2drainConduct*here->JFET2area;
            gspr=model->JFET2sourceConduct*here->JFET2area;
            /*
             *    initialization
             */
            icheck=1;
            if( ckt->CKTmode & MODEINITSMSIG) {
                vgs= *(ckt->CKTstate0 + here->JFET2vgs);
                vgd= *(ckt->CKTstate0 + here->JFET2vgd);
            } else if (ckt->CKTmode & MODEINITTRAN) {
                vgs= *(ckt->CKTstate1 + here->JFET2vgs);
                vgd= *(ckt->CKTstate1 + here->JFET2vgd);
            } else if ( (ckt->CKTmode & MODEINITJCT) &&
                    (ckt->CKTmode & MODETRANOP) &&
                    (ckt->CKTmode & MODEUIC) ) {
                vds=model->JFET2type*here->JFET2icVDS;
                vgs=model->JFET2type*here->JFET2icVGS;
                vgd=vgs-vds;
            } else if ( (ckt->CKTmode & MODEINITJCT) &&
                    (here->JFET2off == 0)  ) {
                vgs = -1;
                vgd = -1;
            } else if( (ckt->CKTmode & MODEINITJCT) ||
                    ((ckt->CKTmode & MODEINITFIX) && (here->JFET2off))) {
                vgs = 0;
                vgd = 0;
            } else {
#ifndef PREDICTOR
                if(ckt->CKTmode & MODEINITPRED) {
                    xfact=ckt->CKTdelta/ckt->CKTdeltaOld[1];
                    *(ckt->CKTstate0 + here->JFET2vgs)= 
                            *(ckt->CKTstate1 + here->JFET2vgs);
                    vgs=(1+xfact)* *(ckt->CKTstate1 + here->JFET2vgs)-xfact*
                            *(ckt->CKTstate2 + here->JFET2vgs);
                    *(ckt->CKTstate0 + here->JFET2vgd)= 
                            *(ckt->CKTstate1 + here->JFET2vgd);
                    vgd=(1+xfact)* *(ckt->CKTstate1 + here->JFET2vgd)-xfact*
                            *(ckt->CKTstate2 + here->JFET2vgd);
                    *(ckt->CKTstate0 + here->JFET2cg)= 
                            *(ckt->CKTstate1 + here->JFET2cg);
                    *(ckt->CKTstate0 + here->JFET2cd)= 
                            *(ckt->CKTstate1 + here->JFET2cd);
                    *(ckt->CKTstate0 + here->JFET2cgd)=
                            *(ckt->CKTstate1 + here->JFET2cgd);
                    *(ckt->CKTstate0 + here->JFET2gm)=
                            *(ckt->CKTstate1 + here->JFET2gm);
                    *(ckt->CKTstate0 + here->JFET2gds)=
                            *(ckt->CKTstate1 + here->JFET2gds);
                    *(ckt->CKTstate0 + here->JFET2ggs)=
                            *(ckt->CKTstate1 + here->JFET2ggs);
                    *(ckt->CKTstate0 + here->JFET2ggd)=
                            *(ckt->CKTstate1 + here->JFET2ggd);
                } else {
#endif /*PREDICTOR*/
                    /*
                     *  compute new nonlinear branch voltages 
                     */
                    vgs=model->JFET2type*
                        (*(ckt->CKTrhsOld+ here->JFET2gateNode)-
                        *(ckt->CKTrhsOld+ 
                        here->JFET2sourcePrimeNode));
                    vgd=model->JFET2type*
                        (*(ckt->CKTrhsOld+here->JFET2gateNode)-
                        *(ckt->CKTrhsOld+
                        here->JFET2drainPrimeNode));
#ifndef PREDICTOR
                }
#endif /*PREDICTOR*/
                delvgs=vgs- *(ckt->CKTstate0 + here->JFET2vgs);
                delvgd=vgd- *(ckt->CKTstate0 + here->JFET2vgd);
                delvds=delvgs-delvgd;
                cghat= *(ckt->CKTstate0 + here->JFET2cg)+ 
                        *(ckt->CKTstate0 + here->JFET2ggd)*delvgd+
                        *(ckt->CKTstate0 + here->JFET2ggs)*delvgs;
                cdhat= *(ckt->CKTstate0 + here->JFET2cd)+
                        *(ckt->CKTstate0 + here->JFET2gm)*delvgs+
                        *(ckt->CKTstate0 + here->JFET2gds)*delvds-
                        *(ckt->CKTstate0 + here->JFET2ggd)*delvgd;
                /*
                 *   bypass if solution has not changed 
                 */
                if((ckt->CKTbypass) &&
                    (!(ckt->CKTmode & MODEINITPRED)) &&
                    (FABS(delvgs) < ckt->CKTreltol*MAX(FABS(vgs),
                        FABS(*(ckt->CKTstate0 + here->JFET2vgs)))+
                        ckt->CKTvoltTol) )
        if ( (FABS(delvgd) < ckt->CKTreltol*MAX(FABS(vgd),
                        FABS(*(ckt->CKTstate0 + here->JFET2vgd)))+
                        ckt->CKTvoltTol))
        if ( (FABS(cghat-*(ckt->CKTstate0 + here->JFET2cg)) 
                        < ckt->CKTreltol*MAX(FABS(cghat),
                        FABS(*(ckt->CKTstate0 + here->JFET2cg)))+
                        ckt->CKTabstol) ) if ( /* hack - expression too big */
                    (FABS(cdhat-*(ckt->CKTstate0 + here->JFET2cd))
                        < ckt->CKTreltol*MAX(FABS(cdhat),
                        FABS(*(ckt->CKTstate0 + here->JFET2cd)))+
                        ckt->CKTabstol) ) {

                    /* we can do a bypass */
                    vgs= *(ckt->CKTstate0 + here->JFET2vgs);
                    vgd= *(ckt->CKTstate0 + here->JFET2vgd);
                    vds= vgs-vgd;
                    cg= *(ckt->CKTstate0 + here->JFET2cg);
                    cd= *(ckt->CKTstate0 + here->JFET2cd);
                    cgd= *(ckt->CKTstate0 + here->JFET2cgd);
                    gm= *(ckt->CKTstate0 + here->JFET2gm);
                    gds= *(ckt->CKTstate0 + here->JFET2gds);
                    ggs= *(ckt->CKTstate0 + here->JFET2ggs);
                    ggd= *(ckt->CKTstate0 + here->JFET2ggd);
                    goto load;
                }
                /*
                 *  limit nonlinear branch voltages 
                 */
                ichk1=1;
                vgs = DEV.pnjlim(vgs,*(ckt->CKTstate0 + here->JFET2vgs),
                        (here->JFET2temp*CONSTKoverQ), here->JFET2vcrit, &icheck);
                vgd = DEV.pnjlim(vgd,*(ckt->CKTstate0 + here->JFET2vgd),
                        (here->JFET2temp*CONSTKoverQ), here->JFET2vcrit,&ichk1);
                if (ichk1 == 1) {
                    icheck=1;
                }
                vgs = DEV.fetlim(vgs,*(ckt->CKTstate0 + here->JFET2vgs),
                        model->JFET2vto);
                vgd = DEV.fetlim(vgd,*(ckt->CKTstate0 + here->JFET2vgd),
                        model->JFET2vto);
            }
            /*
             *   determine dc current and derivatives 
             */
            vds=vgs-vgd;
            if (vds < 0.0) {
               cd = -PSids(ckt, model, here, vgd, vgs,
                            &cgd, &cg, &ggd, &ggs, &gm, &gds);
               gds += gm;
               gm  = -gm;
            } else {
               cd =  PSids(ckt, model, here, vgs, vgd, 
                            &cg, &cgd, &ggs, &ggd, &gm, &gds);
            }
            cg = cg + cgd;
            cd = cd - cgd;

            if ( (ckt->CKTmode & (MODETRAN | MODEAC | MODEINITSMSIG) ) ||
                    ((ckt->CKTmode & MODETRANOP) && (ckt->CKTmode & MODEUIC)) ){
                /* 
                 *    charge storage elements 
                 */
                double capds = model->JFET2capds*here->JFET2area;

                PScharge(ckt, model, here, vgs, vgd, &capgs, &capgd);

                *(ckt->CKTstate0 + here->JFET2qds) = capds * vds;
            
                /*
                 *   store small-signal parameters 
                 */
                if( (!(ckt->CKTmode & MODETRANOP)) || 
                        (!(ckt->CKTmode & MODEUIC)) ) {
                    if(ckt->CKTmode & MODEINITSMSIG) {
                        *(ckt->CKTstate0 + here->JFET2qgs) = capgs;
                        *(ckt->CKTstate0 + here->JFET2qgd) = capgd;
                        *(ckt->CKTstate0 + here->JFET2qds) = capds;
// SRW                        continue; /*go to 1000*/
                        return (OK);
                    }
                    /*
                     *   transient analysis 
                     */
                    if(ckt->CKTmode & MODEINITTRAN) {
                        *(ckt->CKTstate1 + here->JFET2qgs) =
                                *(ckt->CKTstate0 + here->JFET2qgs);
                        *(ckt->CKTstate1 + here->JFET2qgd) =
                                *(ckt->CKTstate0 + here->JFET2qgd);
                        *(ckt->CKTstate1 + here->JFET2qds) =
                                *(ckt->CKTstate0 + here->JFET2qds);
                    }
                    error = NIintegrate(ckt,&geq,&ceq,capgs,here->JFET2qgs);
                    if(error) return(error);
                    ggs = ggs + geq;
                    cg = cg + *(ckt->CKTstate0 + here->JFET2cqgs);
                    error = NIintegrate(ckt,&geq,&ceq,capgd,here->JFET2qgd);
                    if(error) return(error);
                    ggd = ggd + geq;
                    cg = cg + *(ckt->CKTstate0 + here->JFET2cqgd);
                    cd = cd - *(ckt->CKTstate0 + here->JFET2cqgd);
                    cgd = cgd + *(ckt->CKTstate0 + here->JFET2cqgd);
                    error = NIintegrate(ckt,&geq,&ceq,capds,here->JFET2qds);
                    cd = cd + *(ckt->CKTstate0 + here->JFET2cqds);
                    if(error) return(error);
                    if (ckt->CKTmode & MODEINITTRAN) {
                        *(ckt->CKTstate1 + here->JFET2cqgs) =
                                *(ckt->CKTstate0 + here->JFET2cqgs);
                        *(ckt->CKTstate1 + here->JFET2cqgd) =
                                *(ckt->CKTstate0 + here->JFET2cqgd);
                        *(ckt->CKTstate1 + here->JFET2cqds) =
                                *(ckt->CKTstate0 + here->JFET2cqds);
                    }
                }
            }
            /*
             *  check convergence 
             */
            if( (!(ckt->CKTmode & MODEINITFIX)) | (!(ckt->CKTmode & MODEUIC))) {
                if( (icheck == 1)
#ifndef NEWCONV
/* XXX */
#endif /*NEWCONV*/
                        || (FABS(cghat-cg) >= ckt->CKTreltol*
                            MAX(FABS(cghat),FABS(cg))+ckt->CKTabstol) ||
                        (FABS(cdhat-cd) > ckt->CKTreltol*
                            MAX(FABS(cdhat),FABS(cd))+ckt->CKTabstol) 
                        ) {
                    ckt->incNoncon();  // SRW
		    ckt->CKTtroubleElt = (GENinstance *) here;
                }
            }
            *(ckt->CKTstate0 + here->JFET2vgs) = vgs;
            *(ckt->CKTstate0 + here->JFET2vgd) = vgd;
            *(ckt->CKTstate0 + here->JFET2cg) = cg;
            *(ckt->CKTstate0 + here->JFET2cd) = cd;
            *(ckt->CKTstate0 + here->JFET2cgd) = cgd;
            *(ckt->CKTstate0 + here->JFET2gm) = gm;
            *(ckt->CKTstate0 + here->JFET2gds) = gds;
            *(ckt->CKTstate0 + here->JFET2ggs) = ggs;
            *(ckt->CKTstate0 + here->JFET2ggd) = ggd;
            /*
             *    load current vector
             */
load:
            ceqgd=model->JFET2type*(cgd-ggd*vgd);
            ceqgs=model->JFET2type*((cg-cgd)-ggs*vgs);
            cdreq=model->JFET2type*((cd+cgd)-gds*vds-gm*vgs);
            ckt->rhsadd(here->JFET2gateNode, (-ceqgs-ceqgd));
            ckt->rhsadd(here->JFET2drainPrimeNode,
                    (-cdreq+ceqgd));
            ckt->rhsadd(here->JFET2sourcePrimeNode,
                    (cdreq+ceqgs));
            /*
             *    load y matrix 
             */
            ckt->ldadd(here->JFET2drainDrainPrimePtr, -gdpr);
            ckt->ldadd(here->JFET2gateDrainPrimePtr, -ggd);
            ckt->ldadd(here->JFET2gateSourcePrimePtr, -ggs);
            ckt->ldadd(here->JFET2sourceSourcePrimePtr, -gspr);
            ckt->ldadd(here->JFET2drainPrimeDrainPtr, -gdpr);
            ckt->ldadd(here->JFET2drainPrimeGatePtr, gm-ggd);
            ckt->ldadd(here->JFET2drainPrimeSourcePrimePtr, -gds-gm);
            ckt->ldadd(here->JFET2sourcePrimeGatePtr, -ggs-gm);
            ckt->ldadd(here->JFET2sourcePrimeSourcePtr, -gspr);
            ckt->ldadd(here->JFET2sourcePrimeDrainPrimePtr, -gds);
            ckt->ldadd(here->JFET2drainDrainPtr, gdpr);
            ckt->ldadd(here->JFET2gateGatePtr, ggd+ggs);
            ckt->ldadd(here->JFET2sourceSourcePtr, gspr);
            ckt->ldadd(here->JFET2drainPrimeDrainPrimePtr, gdpr+gds+ggd);
            ckt->ldadd(here->JFET2sourcePrimeSourcePrimePtr, gspr+gds+gm+ggs);
// SRW        }
// SRW    }
    return(OK);
}
