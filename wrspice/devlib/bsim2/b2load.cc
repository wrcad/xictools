
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
 $Id: b2load.cc,v 2.9 2015/06/11 01:12:28 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Hong June Park, Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/

#include "b2defs.h"
#include <stdio.h>


int
// SRW B2dev::load(sGENmodel *genmod, sCKT *ckt)
B2dev::load(sGENinstance *in_inst, sCKT *ckt)
{
    sTASK *tsk = ckt->CKTcurTask;
    double DrainSatCurrent;
    double EffectiveLength;
    double GateBulkOverlapCap;
    double GateDrainOverlapCap;
    double GateSourceOverlapCap;
    double SourceSatCurrent;
    double DrainArea;
    double SourceArea;
    double DrainPerimeter;
    double SourcePerimeter;
    double arg;
    double capbd=0;
    double capbs=0;
    double cbd;
    double cbhat;
    double cbs;
    double cd;
    double cdrain;
    double cdhat;
    double cdreq;
    double ceq;
    double ceqbd;
    double ceqbs;
    double ceqqb;
    double ceqqd;
    double ceqqg;
    double czbd;
    double czbdsw;
    double czbs;
    double czbssw;
    double delvbd;
    double delvbs;
    double delvds;
    double delvgd;
    double delvgs;
    double evbd;
    double evbs;
    double gbd;
    double gbs;
    double gcbdb;
    double gcbgb;
    double gcbsb;
    double gcddb;
    double gcdgb;
    double gcdsb;
    double gcgdb;
    double gcggb;
    double gcgsb;
    double gcsdb;
    double gcsgb;
    double gcssb;
    double gds;
    double geq;
    double gm;
    double gmbs;
    double sarg;
    double sargsw;
    double vbd;
    double vbs;
    double vcrit;
    double vds;
    double vdsat;
    double vgb;
    double vgd;
    double vgdo;
    double vgs;
    double von;
    double xnrm;
    double xrev;
    int Check;
    double cgdb;
    double cgsb;
    double cbdb;
    double cdgb;
    double cddb;
    double cdsb;
    double cggb;
    double cbgb;
    double cbsb;
    double csgb;
    double cssb;
    double csdb;
    double PhiB;
    double PhiBSW;
    double MJ;
    double MJSW;
    double argsw;
    double qgate;
    double qbulk;
    double qdrn;
    double qsrc;
    double cqgate;
    double cqbulk;
    double cqdrn;
    double vt0;
    double args[8];
    int    ByPass;
    double tempv;

/* SRW
    sB2model *model = static_cast<sB2model*>(genmod);
    for ( ; model; model = model->next()) {
        sB2instance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {
*/
    sB2instance *inst = (sB2instance*)in_inst;
    sB2model *model = (sB2model*)inst->GENmodPtr;

            EffectiveLength=inst->B2l - model->B2deltaL * 1.e-6;/* m */
            DrainArea = inst->B2drainArea;
            SourceArea = inst->B2sourceArea;
            DrainPerimeter = inst->B2drainPerimeter;
            SourcePerimeter = inst->B2sourcePerimeter;
            if( (DrainSatCurrent=DrainArea*model->B2jctSatCurDensity) 
                    < 1e-15){
                DrainSatCurrent = 1.0e-15;
            }
            if( (SourceSatCurrent=SourceArea*model->B2jctSatCurDensity)
                    <1.0e-15){
                SourceSatCurrent = 1.0e-15;
            }
            GateSourceOverlapCap = model->B2gateSourceOverlapCap *inst->B2w;
            GateDrainOverlapCap = model->B2gateDrainOverlapCap * inst->B2w;
            GateBulkOverlapCap = model->B2gateBulkOverlapCap *EffectiveLength;
            von = model->B2type * inst->B2von;
            vdsat = model->B2type * inst->B2vdsat;
            vt0 = model->B2type * inst->pParam->B2vt0;

            Check=1;
            ByPass = 0;
            if((ckt->CKTmode & MODEINITSMSIG)) {
                vbs= *(ckt->CKTstate0 + inst->B2vbs);
                vgs= *(ckt->CKTstate0 + inst->B2vgs);
                vds= *(ckt->CKTstate0 + inst->B2vds);
            } else if ((ckt->CKTmode & MODEINITTRAN)) {
                vbs= *(ckt->CKTstate1 + inst->B2vbs);
                vgs= *(ckt->CKTstate1 + inst->B2vgs);
                vds= *(ckt->CKTstate1 + inst->B2vds);
            } else if((ckt->CKTmode & MODEINITJCT) && !inst->B2off) {
                vds= model->B2type * inst->B2icVDS;
                vgs= model->B2type * inst->B2icVGS;
                vbs= model->B2type * inst->B2icVBS;
                if((vds==0) && (vgs==0) && (vbs==0) && 
                        ((ckt->CKTmode & 
                        (MODETRAN|MODEAC|MODEDCOP|MODEDCTRANCURVE)) ||
                        (!(ckt->CKTmode & MODEUIC)))) {
                    vbs = -1;
                    vgs = vt0;
                    vds = 0;
                }
            } else if((ckt->CKTmode & (MODEINITJCT | MODEINITFIX) ) && 
                    (inst->B2off)) {
                vbs=vgs=vds=0;
            } else {
#ifndef PREDICTOR
                if((ckt->CKTmode & MODEINITPRED)) {
                    vbs = DEV.pred(ckt, inst->B2vbs);
                    vgs = DEV.pred(ckt, inst->B2vgs);
                    vds = DEV.pred(ckt, inst->B2vds);

                    *(ckt->CKTstate0 + inst->B2vbs) = 
                            *(ckt->CKTstate1 + inst->B2vbs);
                    *(ckt->CKTstate0 + inst->B2vgs) = 
                            *(ckt->CKTstate1 + inst->B2vgs);
                    *(ckt->CKTstate0 + inst->B2vds) = 
                            *(ckt->CKTstate1 + inst->B2vds);
                    *(ckt->CKTstate0 + inst->B2vbd) = 
                            *(ckt->CKTstate0 + inst->B2vbs)-
                            *(ckt->CKTstate0 + inst->B2vds);
                    *(ckt->CKTstate0 + inst->B2cd) = 
                            *(ckt->CKTstate1 + inst->B2cd);
                    *(ckt->CKTstate0 + inst->B2cbs) = 
                            *(ckt->CKTstate1 + inst->B2cbs);
                    *(ckt->CKTstate0 + inst->B2cbd) = 
                            *(ckt->CKTstate1 + inst->B2cbd);
                    *(ckt->CKTstate0 + inst->B2gm) = 
                            *(ckt->CKTstate1 + inst->B2gm);
                    *(ckt->CKTstate0 + inst->B2gds) = 
                            *(ckt->CKTstate1 + inst->B2gds);
                    *(ckt->CKTstate0 + inst->B2gmbs) = 
                            *(ckt->CKTstate1 + inst->B2gmbs);
                    *(ckt->CKTstate0 + inst->B2gbd) = 
                            *(ckt->CKTstate1 + inst->B2gbd);
                    *(ckt->CKTstate0 + inst->B2gbs) = 
                            *(ckt->CKTstate1 + inst->B2gbs);
                    *(ckt->CKTstate0 + inst->B2cggb) = 
                            *(ckt->CKTstate1 + inst->B2cggb);
                    *(ckt->CKTstate0 + inst->B2cbgb) = 
                            *(ckt->CKTstate1 + inst->B2cbgb);
                    *(ckt->CKTstate0 + inst->B2cbsb) = 
                            *(ckt->CKTstate1 + inst->B2cbsb);
                    *(ckt->CKTstate0 + inst->B2cgdb) = 
                            *(ckt->CKTstate1 + inst->B2cgdb);
                    *(ckt->CKTstate0 + inst->B2cgsb) = 
                            *(ckt->CKTstate1 + inst->B2cgsb);
                    *(ckt->CKTstate0 + inst->B2cbdb) = 
                            *(ckt->CKTstate1 + inst->B2cbdb);
                    *(ckt->CKTstate0 + inst->B2cdgb) = 
                            *(ckt->CKTstate1 + inst->B2cdgb);
                    *(ckt->CKTstate0 + inst->B2cddb) = 
                            *(ckt->CKTstate1 + inst->B2cddb);
                    *(ckt->CKTstate0 + inst->B2cdsb) = 
                            *(ckt->CKTstate1 + inst->B2cdsb);
                } else {
#endif /* PREDICTOR */
                    vbs = model->B2type * ( 
                        *(ckt->CKTrhsOld+inst->B2bNode) -
                        *(ckt->CKTrhsOld+inst->B2sNodePrime));
                    vgs = model->B2type * ( 
                        *(ckt->CKTrhsOld+inst->B2gNode) -
                        *(ckt->CKTrhsOld+inst->B2sNodePrime));
                    vds = model->B2type * ( 
                        *(ckt->CKTrhsOld+inst->B2dNodePrime) -
                        *(ckt->CKTrhsOld+inst->B2sNodePrime));
#ifndef PREDICTOR
                }
#endif /* PREDICTOR */
                vbd=vbs-vds;
                vgd=vgs-vds;
                vgdo = *(ckt->CKTstate0 + inst->B2vgs) - 
                    *(ckt->CKTstate0 + inst->B2vds);
                delvbs = vbs - *(ckt->CKTstate0 + inst->B2vbs);
                delvbd = vbd - *(ckt->CKTstate0 + inst->B2vbd);
                delvgs = vgs - *(ckt->CKTstate0 + inst->B2vgs);
                delvds = vds - *(ckt->CKTstate0 + inst->B2vds);
                delvgd = vgd-vgdo;

                if (inst->B2mode >= 0) {
                    cdhat=
                        *(ckt->CKTstate0 + inst->B2cd) -
                        *(ckt->CKTstate0 + inst->B2gbd) * delvbd +
                        *(ckt->CKTstate0 + inst->B2gmbs) * delvbs +
                        *(ckt->CKTstate0 + inst->B2gm) * delvgs + 
                        *(ckt->CKTstate0 + inst->B2gds) * delvds ;
                } else {
                    cdhat=
                        *(ckt->CKTstate0 + inst->B2cd) -
                        ( *(ckt->CKTstate0 + inst->B2gbd) -
                          *(ckt->CKTstate0 + inst->B2gmbs)) * delvbd -
                        *(ckt->CKTstate0 + inst->B2gm) * delvgd +
                        *(ckt->CKTstate0 + inst->B2gds) * delvds;
                }
                cbhat=
                    *(ckt->CKTstate0 + inst->B2cbs) +
                    *(ckt->CKTstate0 + inst->B2cbd) +
                    *(ckt->CKTstate0 + inst->B2gbd) * delvbd +
                    *(ckt->CKTstate0 + inst->B2gbs) * delvbs ;

                /* now lets see if we can bypass (ugh) */

                /* following should be one big if connected by && all over
                 * the place, but some C compilers can't handle that, so
                 * we split it up here to let them digest it in stages
                 */
                tempv = SPMAX(FABS(cbhat),FABS(*(ckt->CKTstate0 + inst->B2cbs)
                        + *(ckt->CKTstate0 + inst->B2cbd)))+tsk->TSKabstol;
                if((!(ckt->CKTmode & MODEINITPRED)) && (tsk->TSKbypass) )
                if( (FABS(delvbs) < (tsk->TSKreltol * SPMAX(FABS(vbs),
                        FABS(*(ckt->CKTstate0+inst->B2vbs)))+
                        tsk->TSKvoltTol)) )
                if ( (FABS(delvbd) < (tsk->TSKreltol * SPMAX(FABS(vbd),
                        FABS(*(ckt->CKTstate0+inst->B2vbd)))+
                        tsk->TSKvoltTol)) )
                if( (FABS(delvgs) < (tsk->TSKreltol * SPMAX(FABS(vgs),
                        FABS(*(ckt->CKTstate0+inst->B2vgs)))+
                        tsk->TSKvoltTol)))
                if ( (FABS(delvds) < (tsk->TSKreltol * SPMAX(FABS(vds),
                        FABS(*(ckt->CKTstate0+inst->B2vds)))+
                        tsk->TSKvoltTol)) )
                if( (FABS(cdhat- *(ckt->CKTstate0 + inst->B2cd)) <
                        tsk->TSKreltol * SPMAX(FABS(cdhat),FABS(*(ckt->CKTstate0 +
                        inst->B2cd))) + tsk->TSKabstol) )
                if ( (FABS(cbhat-(*(ckt->CKTstate0 + inst->B2cbs) +
                        *(ckt->CKTstate0 + inst->B2cbd))) < tsk->TSKreltol *
                        tempv)) {
                    /* bypass code */
                    vbs = *(ckt->CKTstate0 + inst->B2vbs);
                    vbd = *(ckt->CKTstate0 + inst->B2vbd);
                    vgs = *(ckt->CKTstate0 + inst->B2vgs);
                    vds = *(ckt->CKTstate0 + inst->B2vds);
                    vgd = vgs - vds;
                    vgb = vgs - vbs;
                    cd = *(ckt->CKTstate0 + inst->B2cd);
                    cbs = *(ckt->CKTstate0 + inst->B2cbs);
                    cbd = *(ckt->CKTstate0 + inst->B2cbd);
                    cdrain = inst->B2mode * (cd + cbd);
                    gm = *(ckt->CKTstate0 + inst->B2gm);
                    gds = *(ckt->CKTstate0 + inst->B2gds);
                    gmbs = *(ckt->CKTstate0 + inst->B2gmbs);
                    gbd = *(ckt->CKTstate0 + inst->B2gbd);
                    gbs = *(ckt->CKTstate0 + inst->B2gbs);
                    if((ckt->CKTmode & (MODETRAN | MODEAC)) || 
                            ((ckt->CKTmode & MODETRANOP) && 
                            (ckt->CKTmode & MODEUIC))) {
                        cggb = *(ckt->CKTstate0 + inst->B2cggb);
                        cgdb = *(ckt->CKTstate0 + inst->B2cgdb);
                        cgsb = *(ckt->CKTstate0 + inst->B2cgsb);
                        cbgb = *(ckt->CKTstate0 + inst->B2cbgb);
                        cbdb = *(ckt->CKTstate0 + inst->B2cbdb);
                        cbsb = *(ckt->CKTstate0 + inst->B2cbsb);
                        cdgb = *(ckt->CKTstate0 + inst->B2cdgb);
                        cddb = *(ckt->CKTstate0 + inst->B2cddb);
                        cdsb = *(ckt->CKTstate0 + inst->B2cdsb);
                        capbs = *(ckt->CKTstate0 + inst->B2capbs);
                        capbd = *(ckt->CKTstate0 + inst->B2capbd);
                        ByPass = 1;
                        goto line755;
                    } else {
                        goto line850;
                    }
                }

                von = model->B2type * inst->B2von;
                if(*(ckt->CKTstate0 + inst->B2vds) >=0) {
                    vgs = DEV.fetlim(vgs,*(ckt->CKTstate0 + inst->B2vgs), von);
                    vds = vgs - vgd;
                    vds = DEV.limvds(vds,*(ckt->CKTstate0 + inst->B2vds));
                    vgd = vgs - vds;
                } else {
                    vgd = DEV.fetlim(vgd,vgdo,von);
                    vds = vgs - vgd;
                    vds = -DEV.limvds(-vds,-(*(ckt->CKTstate0 + 
                            inst->B2vds)));
                    vgs = vgd + vds;
                }
                if(vds >= 0) {
                    vcrit = CONSTvt0 *log(CONSTvt0/(CONSTroot2*SourceSatCurrent));
                    vbs = DEV.pnjlim(vbs,*(ckt->CKTstate0 + inst->B2vbs),
                            CONSTvt0,vcrit,&Check); /* B2 test */
                    vbd = vbs-vds;
                } else {
                    vcrit = CONSTvt0 * log(CONSTvt0/(CONSTroot2*DrainSatCurrent));
                    vbd = DEV.pnjlim(vbd,*(ckt->CKTstate0 + inst->B2vbd),
                            CONSTvt0,vcrit,&Check); /* B2 test*/
                    vbs = vbd + vds;
                }
            } 

             /* determine DC current and derivatives */
            vbd = vbs - vds;
            vgd = vgs - vds;
            vgb = vgs - vbs;


            if(vbs <= 0.0 ) {
                gbs = SourceSatCurrent / CONSTvt0 + tsk->TSKgmin;
                cbs = gbs * vbs ;
            } else {
                evbs = exp(vbs/CONSTvt0);
                gbs = SourceSatCurrent*evbs/CONSTvt0 + tsk->TSKgmin;
                cbs = SourceSatCurrent * (evbs-1) + tsk->TSKgmin * vbs ;
            }
            if(vbd <= 0.0) {
                gbd = DrainSatCurrent / CONSTvt0 + tsk->TSKgmin;
                cbd = gbd * vbd ;
            } else {
                evbd = exp(vbd/CONSTvt0);
                gbd = DrainSatCurrent*evbd/CONSTvt0 +tsk->TSKgmin;
                cbd = DrainSatCurrent *(evbd-1)+tsk->TSKgmin*vbd;
            }
            /* line 400 */
            if(vds >= 0) {
                /* normal mode */
                inst->B2mode = 1;
            } else {
                /* inverse mode */
                inst->B2mode = -1;
            }
            /* call B2evaluate to calculate drain current and its 
             * derivatives and charge and capacitances related to gate
             * drain, and bulk
             */
           if( vds >= 0 )  {
                evaluate(vds,vbs,vgs,inst,model,&gm,&gds,&gmbs,&qgate,
                    &qbulk,&qdrn,&cggb,&cgdb,&cgsb,&cbgb,&cbdb,&cbsb,&cdgb,
                    &cddb,&cdsb,&cdrain,&von,&vdsat,ckt);
            } else {
                evaluate(-vds,vbd,vgd,inst,model,&gm,&gds,&gmbs,&qgate,
                    &qbulk,&qsrc,&cggb,&cgsb,&cgdb,&cbgb,&cbsb,&cbdb,&csgb,
                    &cssb,&csdb,&cdrain,&von,&vdsat,ckt);
            }

            inst->B2von = model->B2type * von;
            inst->B2vdsat = model->B2type * vdsat;  

        

            /*
             *  COMPUTE EQUIVALENT DRAIN CURRENT SOURCE
             */
            cd=inst->B2mode * cdrain - cbd;
            if ((ckt->CKTmode & (MODETRAN | MODEAC | MODEINITSMSIG)) ||
                    ((ckt->CKTmode & MODETRANOP ) && 
                    (ckt->CKTmode & MODEUIC))) {
                /*
                 *  charge storage elements
                 *
                 *   bulk-drain and bulk-source depletion capacitances
                 *  czbd : zero bias drain junction capacitance
                 *  czbs : zero bias source junction capacitance
                 * czbdsw:zero bias drain junction sidewall capacitance
                 * czbssw:zero bias source junction sidewall capacitance
                 */

                czbd  = model->B2unitAreaJctCap * DrainArea;
                czbs  = model->B2unitAreaJctCap * SourceArea;
                czbdsw= model->B2unitLengthSidewallJctCap * DrainPerimeter;
                czbssw= model->B2unitLengthSidewallJctCap * SourcePerimeter;
                PhiB = model->B2bulkJctPotential;
                PhiBSW = model->B2sidewallJctPotential;
                MJ = model->B2bulkJctBotGradingCoeff;
                MJSW = model->B2bulkJctSideGradingCoeff;

                /* Source Bulk Junction */
                if( vbs < 0 ) {  
                    arg = 1 - vbs / PhiB;
                    argsw = 1 - vbs / PhiBSW;
                    sarg = exp(-MJ*log(arg));
                    sargsw = exp(-MJSW*log(argsw));
                    *(ckt->CKTstate0 + inst->B2qbs) =
                        PhiB * czbs * (1-arg*sarg)/(1-MJ) + PhiBSW * 
                    czbssw * (1-argsw*sargsw)/(1-MJSW);
                    capbs = czbs * sarg + czbssw * sargsw ;
                } else {  
                    *(ckt->CKTstate0+inst->B2qbs) =
                        vbs*(czbs+czbssw)+ vbs*vbs*(czbs*MJ*0.5/PhiB 
                        + czbssw * MJSW * 0.5/PhiBSW);
                    capbs = czbs + czbssw + vbs *(czbs*MJ/PhiB+
                        czbssw * MJSW / PhiBSW );
                }

                /* Drain Bulk Junction */
                if( vbd < 0 ) {  
                    arg = 1 - vbd / PhiB;
                    argsw = 1 - vbd / PhiBSW;
                    sarg = exp(-MJ*log(arg));
                    sargsw = exp(-MJSW*log(argsw));
                    *(ckt->CKTstate0 + inst->B2qbd) =
                        PhiB * czbd * (1-arg*sarg)/(1-MJ) + PhiBSW * 
                    czbdsw * (1-argsw*sargsw)/(1-MJSW);
                    capbd = czbd * sarg + czbdsw * sargsw ;
                } else {  
                    *(ckt->CKTstate0+inst->B2qbd) =
                        vbd*(czbd+czbdsw)+ vbd*vbd*(czbd*MJ*0.5/PhiB 
                        + czbdsw * MJSW * 0.5/PhiBSW);
                    capbd = czbd + czbdsw + vbd *(czbd*MJ/PhiB+
                        czbdsw * MJSW / PhiBSW );
                }

            }




            /*
             *  check convergence
             */
            /* troubleElts new in 3f2 */
            if ( (inst->B2off == 0)  || (!(ckt->CKTmode & MODEINITFIX)) ){
                if (Check == 1) {
                    ckt->incNoncon();  // SRW
                    ckt->CKTtroubleElt = inst;
                }
            }
            *(ckt->CKTstate0 + inst->B2vbs) = vbs;
            *(ckt->CKTstate0 + inst->B2vbd) = vbd;
            *(ckt->CKTstate0 + inst->B2vgs) = vgs;
            *(ckt->CKTstate0 + inst->B2vds) = vds;
            *(ckt->CKTstate0 + inst->B2cd) = cd;
            *(ckt->CKTstate0 + inst->B2cbs) = cbs;
            *(ckt->CKTstate0 + inst->B2cbd) = cbd;
            *(ckt->CKTstate0 + inst->B2gm) = gm;
            *(ckt->CKTstate0 + inst->B2gds) = gds;
            *(ckt->CKTstate0 + inst->B2gmbs) = gmbs;
            *(ckt->CKTstate0 + inst->B2gbd) = gbd;
            *(ckt->CKTstate0 + inst->B2gbs) = gbs;

            *(ckt->CKTstate0 + inst->B2cggb) = cggb;
            *(ckt->CKTstate0 + inst->B2cgdb) = cgdb;
            *(ckt->CKTstate0 + inst->B2cgsb) = cgsb;

            *(ckt->CKTstate0 + inst->B2cbgb) = cbgb;
            *(ckt->CKTstate0 + inst->B2cbdb) = cbdb;
            *(ckt->CKTstate0 + inst->B2cbsb) = cbsb;

            *(ckt->CKTstate0 + inst->B2cdgb) = cdgb;
            *(ckt->CKTstate0 + inst->B2cddb) = cddb;
            *(ckt->CKTstate0 + inst->B2cdsb) = cdsb;

            *(ckt->CKTstate0 + inst->B2capbs) = capbs;
            *(ckt->CKTstate0 + inst->B2capbd) = capbd;

           /* bulk and channel charge plus overlaps */

            if((!(ckt->CKTmode & (MODETRAN | MODEAC))) &&
                    ((!(ckt->CKTmode & MODETRANOP)) ||
                    (!(ckt->CKTmode & MODEUIC)))  && (!(ckt->CKTmode 
                    &  MODEINITSMSIG))) goto line850; 
         
line755:
            if( inst->B2mode > 0 ) {

        args[0] = GateDrainOverlapCap;
        args[1] = GateSourceOverlapCap;
        args[2] = GateBulkOverlapCap;
        args[3] = capbd;
        args[4] = capbs;
        args[5] = cggb;
        args[6] = cgdb;
        args[7] = cgsb;

                mosCap(ckt,vgd,vgs,vgb,
            args,
            /*
            GateDrainOverlapCap,
            GateSourceOverlapCap,GateBulkOverlapCap,
            capbd,capbs,cggb,cgdb,cgsb,
            */
            cbgb,cbdb,cbsb,cdgb,cddb,cdsb
            ,&gcggb,&gcgdb,&gcgsb,&gcbgb,&gcbdb,&gcbsb,&gcdgb
            ,&gcddb,&gcdsb,&gcsgb,&gcsdb,&gcssb,&qgate,&qbulk
            ,&qdrn,&qsrc);
            } else {

        args[0] = GateSourceOverlapCap;
        args[1] = GateDrainOverlapCap;
        args[2] = GateBulkOverlapCap;
        args[3] = capbs;
        args[4] = capbd;
        args[5] = cggb;
        args[6] = cgsb;
        args[7] = cgdb;

                mosCap(ckt,vgs,vgd,vgb,args,
            /*
            GateSourceOverlapCap,
                    GateDrainOverlapCap,GateBulkOverlapCap,
            capbs,capbd,cggb,cgsb,cgdb,
            */
            cbgb,cbsb,cbdb,csgb,cssb,csdb
                    ,&gcggb,&gcgsb,&gcgdb,&gcbgb,&gcbsb,&gcbdb,&gcsgb
                    ,&gcssb,&gcsdb,&gcdgb,&gcdsb,&gcddb,&qgate,&qbulk
                    ,&qsrc,&qdrn);
            }
             
            if(ByPass) goto line860;
            *(ckt->CKTstate0 + inst->B2qg) = qgate;
            *(ckt->CKTstate0 + inst->B2qd) = qdrn -  
                    *(ckt->CKTstate0 + inst->B2qbd);
            *(ckt->CKTstate0 + inst->B2qb) = qbulk +  
                    *(ckt->CKTstate0 + inst->B2qbd) +  
                    *(ckt->CKTstate0 + inst->B2qbs); 

            /* store small signal parameters */
            if((!(ckt->CKTmode & (MODEAC | MODETRAN))) &&
                    (ckt->CKTmode & MODETRANOP ) && (ckt->CKTmode &
                    MODEUIC ))   goto line850;
            if(ckt->CKTmode & MODEINITSMSIG ) {  
                *(ckt->CKTstate0+inst->B2cggb) = cggb;
                *(ckt->CKTstate0+inst->B2cgdb) = cgdb;
                *(ckt->CKTstate0+inst->B2cgsb) = cgsb;
                *(ckt->CKTstate0+inst->B2cbgb) = cbgb;
                *(ckt->CKTstate0+inst->B2cbdb) = cbdb;
                *(ckt->CKTstate0+inst->B2cbsb) = cbsb;
                *(ckt->CKTstate0+inst->B2cdgb) = cdgb;
                *(ckt->CKTstate0+inst->B2cddb) = cddb;
                *(ckt->CKTstate0+inst->B2cdsb) = cdsb;     
                *(ckt->CKTstate0+inst->B2capbd) = capbd;
                *(ckt->CKTstate0+inst->B2capbs) = capbs;

                goto line1000;
            }
       
            if(ckt->CKTmode & MODEINITTRAN ) { 
                *(ckt->CKTstate1+inst->B2qb) =
                    *(ckt->CKTstate0+inst->B2qb) ;
                *(ckt->CKTstate1+inst->B2qg) =
                    *(ckt->CKTstate0+inst->B2qg) ;
                *(ckt->CKTstate1+inst->B2qd) =
                    *(ckt->CKTstate0+inst->B2qd) ;
            }
       
       
            ckt->integrate(&geq,&ceq,0.0,inst->B2qb);
            ckt->integrate(&geq,&ceq,0.0,inst->B2qg);
            ckt->integrate(&geq,&ceq,0.0,inst->B2qd);
      
            goto line860;

line850:
            /* initialize to zero charge conductance and current */
            ceqqg = ceqqb = ceqqd = 0.0;
            gcdgb = gcddb = gcdsb = 0.0;
            gcsgb = gcsdb = gcssb = 0.0;
            gcggb = gcgdb = gcgsb = 0.0;
            gcbgb = gcbdb = gcbsb = 0.0;
            goto line900;
            
line860:
            /* evaluate equivalent charge current */
            cqgate = *(ckt->CKTstate0 + inst->B2iqg);
            cqbulk = *(ckt->CKTstate0 + inst->B2iqb);
            cqdrn = *(ckt->CKTstate0 + inst->B2iqd);
            ceqqg = cqgate - gcggb * vgb + gcgdb * vbd + gcgsb * vbs;
            ceqqb = cqbulk - gcbgb * vgb + gcbdb * vbd + gcbsb * vbs;
            ceqqd = cqdrn - gcdgb * vgb + gcddb * vbd + gcdsb * vbs;

            if(ckt->CKTmode & MODEINITTRAN ) {  
                *(ckt->CKTstate1 + inst->B2iqb) =  
                    *(ckt->CKTstate0 + inst->B2iqb);
                *(ckt->CKTstate1 + inst->B2iqg) =  
                    *(ckt->CKTstate0 + inst->B2iqg);
                *(ckt->CKTstate1 + inst->B2iqd) =  
                    *(ckt->CKTstate0 + inst->B2iqd);
            }

            /*
             *  load current vector
             */
line900:
   
            ceqbs = model->B2type * (cbs-(gbs-tsk->TSKgmin)*vbs);
            ceqbd = model->B2type * (cbd-(gbd-tsk->TSKgmin)*vbd);
     
            ceqqg = model->B2type * ceqqg;
            ceqqb = model->B2type * ceqqb;
            ceqqd =  model->B2type * ceqqd;
            if (inst->B2mode >= 0) {
                xnrm=1;
                xrev=0;
                cdreq=model->B2type*(cdrain-gds*vds-gm*vgs-gmbs*vbs);
            } else {
                xnrm=0;
                xrev=1;
                cdreq = -(model->B2type)*(cdrain+gds*vds-gm*vgd-gmbs*vbd);
            }

#define MSC(xx) inst->B2m*(xx)

            ckt->rhsadd(inst->B2gNode, -MSC(ceqqg));
            ckt->rhsadd(inst->B2bNode, -MSC(ceqbs+ceqbd+ceqqb));
            ckt->rhsadd(inst->B2dNodePrime, MSC(ceqbd-cdreq-ceqqd));
            ckt->rhsadd(inst->B2sNodePrime, MSC(cdreq+ceqbs+ceqqg+ceqqb+ceqqd));

            /*
             *  load y matrix
             */

            ckt->ldadd(inst->B2DdPtr, MSC(inst->B2drainConductance));
            ckt->ldadd(inst->B2GgPtr, MSC(gcggb));
            ckt->ldadd(inst->B2SsPtr, MSC(inst->B2sourceConductance));
            ckt->ldadd(inst->B2BbPtr, MSC(gbd+gbs-gcbgb-gcbdb-gcbsb));
            ckt->ldadd(inst->B2DPdpPtr,
                MSC(inst->B2drainConductance+gds+gbd+xrev*(gm+gmbs)+gcddb));
            ckt->ldadd(inst->B2SPspPtr,
                MSC(inst->B2sourceConductance+gds+gbs+xnrm*(gm+gmbs)+gcssb));
            ckt->ldadd(inst->B2DdpPtr, MSC(-inst->B2drainConductance));
            ckt->ldadd(inst->B2GbPtr, MSC(-gcggb-gcgdb-gcgsb));
            ckt->ldadd(inst->B2GdpPtr, MSC(gcgdb));
            ckt->ldadd(inst->B2GspPtr, MSC(gcgsb));
            ckt->ldadd(inst->B2SspPtr, MSC(-inst->B2sourceConductance));
            ckt->ldadd(inst->B2BgPtr, MSC(gcbgb));
            ckt->ldadd(inst->B2BdpPtr, MSC(-gbd+gcbdb));
            ckt->ldadd(inst->B2BspPtr, MSC(-gbs+gcbsb));
            ckt->ldadd(inst->B2DPdPtr, MSC(-inst->B2drainConductance));
            ckt->ldadd(inst->B2DPgPtr, MSC((xnrm-xrev)*gm+gcdgb));
            ckt->ldadd(inst->B2DPbPtr, MSC(-gbd+(xnrm-xrev)*gmbs-gcdgb-gcddb-gcdsb));
            ckt->ldadd(inst->B2DPspPtr, MSC(-gds-xnrm*(gm+gmbs)+gcdsb));
            ckt->ldadd(inst->B2SPgPtr, MSC(-(xnrm-xrev)*gm+gcsgb));
            ckt->ldadd(inst->B2SPsPtr, MSC(-inst->B2sourceConductance));
            ckt->ldadd(inst->B2SPbPtr, MSC(-gbs-(xnrm-xrev)*gmbs-gcsgb-gcsdb-gcssb));
            ckt->ldadd(inst->B2SPdpPtr, MSC(-gds-xrev*(gm+gmbs)+gcsdb));


line1000:  ;

// SRW        }   /* End of Mosfet Instance */

// SRW    }       /* End of Model Instance */
    return(OK);
}

