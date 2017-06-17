
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
 $Id: mesload.cc,v 1.4 2015/06/11 01:12:29 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 S. Hwang
         1993 Stephen R. Whiteley
****************************************************************************/

#include "mesdefs.h"

static double qggnew(double, double, double, double, double, double, double,
    double*, double*);


int
MESdev::load(sGENinstance *in_inst, sCKT *ckt)
{
    sTASK *tsk = ckt->CKTcurTask;
    sMESinstance *inst = (sMESinstance*)in_inst;
    sMESmodel *model = (sMESmodel*)inst->GENmodPtr;

    //
    //  dc model parameters 
    //
    double beta = model->MESbeta * inst->MESarea;
    double gdpr = model->MESdrainConduct * inst->MESarea;
    double gspr = model->MESsourceConduct * inst->MESarea;
    double csat = model->MESgateSatCurrent * inst->MESarea;
    double vcrit = model->MESvcrit;
    double vto = model->MESthreshold;
    //
    //    initialization
    //
    int icheck = 1;
    double cdrain;
    double vds;
    double vgd;
    double vgs;
    double cg;
    double cd;
    double cgd;
    double gm;
    double gds;
    double ggs;
    double ggd;
    if( ckt->CKTmode & MODEINITSMSIG) {
        vgs = *(ckt->CKTstate0 + inst->MESvgs);
        vgd = *(ckt->CKTstate0 + inst->MESvgd);
    }
    else if (ckt->CKTmode & MODEINITTRAN) {
        vgs = *(ckt->CKTstate1 + inst->MESvgs);
        vgd = *(ckt->CKTstate1 + inst->MESvgd);
    }
    else if ( (ckt->CKTmode & MODEINITJCT) &&
            (ckt->CKTmode & MODETRANOP) &&
            (ckt->CKTmode & MODEUIC) ) {
        vds = model->MEStype*inst->MESicVDS;
        vgs = model->MEStype*inst->MESicVGS;
        vgd = vgs-vds;
    }
    else if ( (ckt->CKTmode & MODEINITJCT) &&
            (inst->MESoff == 0)  ) {
        vgs = -1;
        vgd = -1;
    }
    else if( (ckt->CKTmode & MODEINITJCT) ||
            ((ckt->CKTmode & MODEINITFIX) && (inst->MESoff))) {
        vgs = 0;
        vgd = 0;
    }
    else {
#ifndef PREDICTOR
        if (ckt->CKTmode & MODEINITPRED) {
            vgs = DEV.pred(ckt, inst->MESvgs);
            vgd = DEV.pred(ckt, inst->MESvgd);

            *(ckt->CKTstate0 + inst->MESvgs) = 
                    *(ckt->CKTstate1 + inst->MESvgs);
            *(ckt->CKTstate0 + inst->MESvgd) = 
                    *(ckt->CKTstate1 + inst->MESvgd);
            *(ckt->CKTstate0 + inst->MEScg) = 
                    *(ckt->CKTstate1 + inst->MEScg);
            *(ckt->CKTstate0 + inst->MEScd) = 
                    *(ckt->CKTstate1 + inst->MEScd);
            *(ckt->CKTstate0 + inst->MEScgd) =
                    *(ckt->CKTstate1 + inst->MEScgd);
            *(ckt->CKTstate0 + inst->MESgm) =
                    *(ckt->CKTstate1 + inst->MESgm);
            *(ckt->CKTstate0 + inst->MESgds) =
                    *(ckt->CKTstate1 + inst->MESgds);
            *(ckt->CKTstate0 + inst->MESggs) =
                    *(ckt->CKTstate1 + inst->MESggs);
            *(ckt->CKTstate0 + inst->MESggd) =
                    *(ckt->CKTstate1 + inst->MESggd);
        }
        else {
#endif
            //
            //  compute new nonlinear branch voltages 
            //
            vgs = model->MEStype*
                (*(ckt->CKTrhsOld+ inst->MESgateNode)-
                *(ckt->CKTrhsOld+ 
                inst->MESsourcePrimeNode));
            vgd = model->MEStype*
                (*(ckt->CKTrhsOld+inst->MESgateNode)-
                *(ckt->CKTrhsOld+
                inst->MESdrainPrimeNode));
#ifndef PREDICTOR
        }
#endif
        double delvgs = vgs - *(ckt->CKTstate0 + inst->MESvgs);
        double delvgd = vgd - *(ckt->CKTstate0 + inst->MESvgd);
        double delvds = delvgs - delvgd;
        double cghat = *(ckt->CKTstate0 + inst->MEScg) + 
                *(ckt->CKTstate0 + inst->MESggd)*delvgd +
                *(ckt->CKTstate0 + inst->MESggs)*delvgs;
        double cdhat = *(ckt->CKTstate0 + inst->MEScd) +
                *(ckt->CKTstate0 + inst->MESgm)*delvgs +
                *(ckt->CKTstate0 + inst->MESgds)*delvds -
                *(ckt->CKTstate0 + inst->MESggd)*delvgd;
        //
        //   bypass if solution has not changed 
        //
        if ((tsk->TSKbypass) &&
            (!(ckt->CKTmode & MODEINITPRED)) &&
            (FABS(delvgs) < tsk->TSKreltol*SPMAX(FABS(vgs),
                FABS(*(ckt->CKTstate0 + inst->MESvgs)))+
                tsk->TSKvoltTol) )
        if ( (FABS(delvgd) < tsk->TSKreltol*SPMAX(FABS(vgd),
                FABS(*(ckt->CKTstate0 + inst->MESvgd)))+
                tsk->TSKvoltTol))
        if ( (FABS(cghat-*(ckt->CKTstate0 + inst->MEScg)) 
                < tsk->TSKreltol*SPMAX(FABS(cghat),
                FABS(*(ckt->CKTstate0 + inst->MEScg)))+
                tsk->TSKabstol) ) if ( /* hack - expression too big */
            (FABS(cdhat-*(ckt->CKTstate0 + inst->MEScd))
                < tsk->TSKreltol*SPMAX(FABS(cdhat),
                FABS(*(ckt->CKTstate0 + inst->MEScd)))+
                tsk->TSKabstol) ) {

            // we can do a bypass
            vgs = *(ckt->CKTstate0 + inst->MESvgs);
            vgd = *(ckt->CKTstate0 + inst->MESvgd);
            vds = vgs-vgd;
            cg = *(ckt->CKTstate0 + inst->MEScg);
            cd = *(ckt->CKTstate0 + inst->MEScd);
            cgd = *(ckt->CKTstate0 + inst->MEScgd);
            gm = *(ckt->CKTstate0 + inst->MESgm);
            gds = *(ckt->CKTstate0 + inst->MESgds);
            ggs = *(ckt->CKTstate0 + inst->MESggs);
            ggd = *(ckt->CKTstate0 + inst->MESggd);
            goto load;
        }
        //
        //  limit nonlinear branch voltages 
        //
        int ichk1 = 1;
        vgs = DEV.pnjlim(vgs, *(ckt->CKTstate0 + inst->MESvgs),
            CONSTvt0, vcrit, &icheck);
        vgd = DEV.pnjlim(vgd, *(ckt->CKTstate0 + inst->MESvgd),
            CONSTvt0, vcrit, &ichk1);
        if (ichk1 == 1)
            icheck = 1;
        vgs = DEV.fetlim(vgs, *(ckt->CKTstate0 + inst->MESvgs),
                model->MESthreshold);
        vgd = DEV.fetlim(vgd, *(ckt->CKTstate0 + inst->MESvgd),
                model->MESthreshold);
    }
    //
    //   determine dc current and derivatives 
    //
    vds = vgs-vgd;
    if (vgs <= -5*CONSTvt0) {
        ggs = -csat/vgs+tsk->TSKgmin;
        cg = ggs*vgs;
    }
    else {
        double evgs = exp(vgs/CONSTvt0);
        ggs = csat*evgs/CONSTvt0+tsk->TSKgmin;
        cg = csat*(evgs-1)+tsk->TSKgmin*vgs;
    }
    if (vgd <= -5*CONSTvt0) {
        ggd = -csat/vgd+tsk->TSKgmin;
        cgd = ggd*vgd;
    }
    else {
        double evgd = exp(vgd/CONSTvt0);
        ggd = csat*evgd/CONSTvt0+tsk->TSKgmin;
        cgd = csat*(evgd-1)+tsk->TSKgmin*vgd;
    }
    cg = cg+cgd;
    //
    //   compute drain current and derivitives for normal mode 
    //
    if (vds >= 0) {
        double vgst = vgs-model->MESthreshold;
        //
        //   normal mode, cutoff region 
        //
        if (vgst <= 0) {
            cdrain = 0;
            gm = 0;
            gds = 0;
        }
        else {
            double prod = 1 + model->MESlModulation * vds;
            double betap = beta * prod;
            double denom = 1 + model->MESb * vgst;
            double invdenom = 1 / denom;
            if (vds >= ( 3 / model->MESalpha ) ) {
                //
                //   normal mode, saturation region 
                //
                cdrain = betap * vgst * vgst * invdenom;
                gm = betap * vgst * (1 + denom) * invdenom * invdenom;
                gds = model->MESlModulation * beta * vgst * vgst * 
                        invdenom;
            }
            else {
                //
                //   normal mode, linear region 
                //
                double afact = 1 - model->MESalpha * vds / 3;
                double lfact = 1 - afact * afact * afact;
                cdrain = betap * vgst * vgst * invdenom * lfact;
                gm = betap * vgst * (1 + denom) * invdenom * invdenom *
                        lfact;
                gds = beta * vgst * vgst * invdenom * (model->MESalpha *
                    afact * afact * prod + lfact * 
                    model->MESlModulation);
            }
        }
    }
    else {
        //
        //   compute drain current and derivitives for inverse mode 
        //
        double vgdt = vgd - model->MESthreshold;
        if (vgdt <= 0) {
            //
            //   inverse mode, cutoff region 
            //
            cdrain = 0;
            gm = 0;
            gds = 0;
        }
        else {
            //
            //   inverse mode, saturation region 
            //
            double prod = 1 - model->MESlModulation * vds;
            double betap = beta * prod;
            double denom = 1 + model->MESb * vgdt;
            double invdenom = 1 / denom;
            if ( -vds >= ( 3 / model->MESalpha ) ) {
                cdrain = -betap * vgdt * vgdt * invdenom;
                gm = -betap * vgdt * (1 + denom) * invdenom * invdenom;
                gds = model->MESlModulation * beta * vgdt * vgdt *
                         invdenom-gm;
            }
            else {
                //
                //  inverse mode, linear region 
                //
                double afact = 1 + model->MESalpha * vds / 3;
                double lfact = 1 - afact * afact * afact;
                cdrain = -betap * vgdt * vgdt * invdenom * lfact;
                gm = -betap * vgdt * (1 + denom) * invdenom * 
                        invdenom * lfact;
                gds = beta * vgdt * vgdt * invdenom * (model->MESalpha *
                    afact * afact * prod + lfact * 
                    model->MESlModulation)-gm;
            }
        }
    }
    //
    //   compute equivalent drain current source 
    //
    cd = cdrain - cgd;
    if ((ckt->CKTmode & (MODETRAN|MODEINITSMSIG)) ||
            ((ckt->CKTmode & MODETRANOP) && (ckt->CKTmode & MODEUIC))){
        // 
        //    charge storage elements 
        //
        double czgs = model->MEScapGS * inst->MESarea;
        double czgd = model->MEScapGD * inst->MESarea;
        double phib = model->MESgatePotential;
        double vgs1 = *(ckt->CKTstate1 + inst->MESvgs);
        double vgd1 = *(ckt->CKTstate1 + inst->MESvgd);
        double vcap = 1 / model->MESalpha;

        double cgdna, cgdnb, cgdnc, cgdnd;
        double cgsna, cgsnb, cgsnc, cgsnd;
        double qgga = qggnew(vgs, vgd, phib, vcap, vto, czgs, czgd,
            &cgsna, &cgdna);
        double qggb = qggnew(vgs1, vgd, phib, vcap, vto, czgs, czgd,
            &cgsnb, &cgdnb);
        double qggc = qggnew(vgs, vgd1, phib, vcap, vto, czgs, czgd,
            &cgsnc, &cgdnc);
        double qggd = qggnew(vgs1, vgd1, phib, vcap, vto, czgs, czgd,
            &cgsnd, &cgdnd);

        if(ckt->CKTmode & MODEINITTRAN) {
            *(ckt->CKTstate1 + inst->MESqgs) = qgga;
            *(ckt->CKTstate1 + inst->MESqgd) = qgga;
        }
        *(ckt->CKTstate0+inst->MESqgs) = *(ckt->CKTstate1+inst->MESqgs)
                + 0.5 * (qgga-qggb + qggc-qggd);
        *(ckt->CKTstate0+inst->MESqgd) = *(ckt->CKTstate1+inst->MESqgd)
                + 0.5 * (qgga-qggc + qggb-qggd);
        double capgs = cgsna;
        double capgd = cgdna;

        //
        //   store small-signal parameters 
        //
        if( (!(ckt->CKTmode & MODETRANOP)) || 
                (!(ckt->CKTmode & MODEUIC)) ) {
            if(ckt->CKTmode & MODEINITSMSIG) {
                *(ckt->CKTstate0 + inst->MESqgs) = capgs;
                *(ckt->CKTstate0 + inst->MESqgd) = capgd;
                return (OK);
            }
            //
            //   transient analysis 
            //
            if(ckt->CKTmode & MODEINITTRAN) {
                *(ckt->CKTstate1 + inst->MESqgs) =
                        *(ckt->CKTstate0 + inst->MESqgs);
                *(ckt->CKTstate1 + inst->MESqgd) =
                        *(ckt->CKTstate0 + inst->MESqgd);
            }
            double ceq;
            double geq;
            ckt->integrate(&geq, &ceq, capgs, inst->MESqgs);
            ggs = ggs + geq;
            cg = cg + *(ckt->CKTstate0 + inst->MEScqgs);
            ckt->integrate(&geq, &ceq, capgd, inst->MESqgd);
            ggd = ggd + geq;
            cg = cg + *(ckt->CKTstate0 + inst->MEScqgd);
            cd = cd - *(ckt->CKTstate0 + inst->MEScqgd);
            cgd = cgd + *(ckt->CKTstate0 + inst->MEScqgd);
            if (ckt->CKTmode & MODEINITTRAN) {
                *(ckt->CKTstate1 + inst->MEScqgs) =
                        *(ckt->CKTstate0 + inst->MEScqgs);
                *(ckt->CKTstate1 + inst->MEScqgd) =
                        *(ckt->CKTstate0 + inst->MEScqgd);
            }
        }
    }
    //
    //  check convergence 
    //
    if( (!(ckt->CKTmode & MODEINITFIX)) ||
            (!(ckt->CKTmode & MODEUIC))) {
        if (icheck == 1) {
            ckt->incNoncon();  // SRW
            ckt->CKTtroubleElt = inst;
        }
    }
    *(ckt->CKTstate0 + inst->MESvgs) = vgs;
    *(ckt->CKTstate0 + inst->MESvgd) = vgd;
    *(ckt->CKTstate0 + inst->MEScg) = cg;
    *(ckt->CKTstate0 + inst->MEScd) = cd;
    *(ckt->CKTstate0 + inst->MEScgd) = cgd;
    *(ckt->CKTstate0 + inst->MESgm) = gm;
    *(ckt->CKTstate0 + inst->MESgds) = gds;
    *(ckt->CKTstate0 + inst->MESggs) = ggs;
    *(ckt->CKTstate0 + inst->MESggd) = ggd;
    //
    //    load current vector
    //
load:
    double ceqgd = model->MEStype*(cgd-ggd*vgd);
    double ceqgs = model->MEStype*((cg-cgd)-ggs*vgs);
    double cdreq = model->MEStype*((cd+cgd)-gds*vds-gm*vgs);
    ckt->rhsadd(inst->MESgateNode, (-ceqgs-ceqgd));
    ckt->rhsadd(inst->MESdrainPrimeNode,
            (-cdreq+ceqgd));
    ckt->rhsadd(inst->MESsourcePrimeNode,
            (cdreq+ceqgs));
    //
    //    load y matrix 
    //
    ckt->ldadd(inst->MESdrainDrainPrimePtr, -gdpr);
    ckt->ldadd(inst->MESgateDrainPrimePtr, -ggd);
    ckt->ldadd(inst->MESgateSourcePrimePtr, -ggs);
    ckt->ldadd(inst->MESsourceSourcePrimePtr, -gspr);
    ckt->ldadd(inst->MESdrainPrimeDrainPtr, -gdpr);
    ckt->ldadd(inst->MESdrainPrimeGatePtr, gm-ggd);
    ckt->ldadd(inst->MESdrainPrimeSourcePrimePtr, -gds-gm);
    ckt->ldadd(inst->MESsourcePrimeGatePtr, -ggs-gm);
    ckt->ldadd(inst->MESsourcePrimeSourcePtr, -gspr);
    ckt->ldadd(inst->MESsourcePrimeDrainPrimePtr, -gds);
    ckt->ldadd(inst->MESdrainDrainPtr, gdpr);
    ckt->ldadd(inst->MESgateGatePtr, ggd+ggs);
    ckt->ldadd(inst->MESsourceSourcePtr, gspr);
    ckt->ldadd(inst->MESdrainPrimeDrainPrimePtr, gdpr+gds+ggd);
    ckt->ldadd(inst->MESsourcePrimeSourcePrimePtr, gspr+gds+gm+ggs);
    return(OK);
}


// function qggnew  - private, used by MESload
//
static double 
qggnew(double vgs, double vgd, double phib, double vcap, double vto,
    double cgs, double cgd, double *cgsnew, double *cgdnew)
{
    double veroot = sqrt( (vgs - vgd) * (vgs - vgd) + vcap*vcap );
    double veff1 = 0.5 * (vgs + vgd + veroot);
    double veff2 = veff1 - veroot;
    double del = 0.2;
    double vnroot = sqrt( (veff1 - vto)*(veff1 - vto) + del * del );
    double vnew1 = 0.5 * (veff1 + vto + vnroot);
    double vnew3 = vnew1;
    double vmax = 0.5;
    double ext;
    if (vnew1 < vmax)
        ext = 0.0;
    else {
        vnew1 = vmax;
        ext = (vnew3 - vmax)/sqrt(1 - vmax/phib);
    }

    double qroot = sqrt(1 - vnew1/phib);
    double qggval = cgs * (2*phib*(1-qroot) + ext) + cgd*veff2;
    double par1 = 0.5 * ( 1 + (veff1-vto)/vnroot);
    double cfact = (vgs- vgd)/veroot;
    double cplus = 0.5 * (1 + cfact);
    double cminus = cplus - cfact;
    *cgsnew = cgs/qroot*par1*cplus + cgd*cminus;
    *cgdnew = cgs/qroot*par1*cminus + cgd*cplus;
    return (qggval);
}
