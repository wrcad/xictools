
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
 $Id: b1load.cc,v 1.5 2015/06/11 01:12:28 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Hong J. Park, Thomas L. Quarles 
         1993 Stephen R. Whiteley
****************************************************************************/

#include "b1defs.h"


int
// SRWB1dev::load(sGENmodel *genmod, sCKT *ckt)
B1dev::load(sGENinstance *in_inst, sCKT *ckt)
{
    sTASK *tsk = ckt->CKTcurTask;
    double DrainSatCurrent = 0.0;
    double EffectiveLength = 0.0;
    double GateBulkOverlapCap = 0.0;
    double GateDrainOverlapCap = 0.0;
    double GateSourceOverlapCap = 0.0;
    double SourceSatCurrent = 0.0;
    double DrainArea = 0.0;
    double SourceArea = 0.0;
    double DrainPerimeter = 0.0;
    double SourcePerimeter = 0.0;
    double arg = 0.0;
    double capbd = 0.0;
    double capbs = 0.0;
    double cbd = 0.0;
    double cbhat = 0.0;
    double cbs = 0.0;
    double cd = 0.0;
    double cdrain = 0.0;
    double cdhat = 0.0;
    double cdreq = 0.0;
    double ceq = 0.0;
    double ceqbd = 0.0;
    double ceqbs = 0.0;
    double ceqqb = 0.0;
    double ceqqd = 0.0;
    double ceqqg = 0.0;
    double czbd = 0.0;
    double czbdsw = 0.0;
    double czbs = 0.0;
    double czbssw = 0.0;
    double delvbd = 0.0;
    double delvbs = 0.0;
    double delvds = 0.0;
    double delvgd = 0.0;
    double delvgs = 0.0;
    double evbd = 0.0;
    double evbs = 0.0;
    double gbd = 0.0;
    double gbs = 0.0;
    double gcbdb = 0.0;
    double gcbgb = 0.0;
    double gcbsb = 0.0;
    double gcddb = 0.0;
    double gcdgb = 0.0;
    double gcdsb = 0.0;
    double gcgdb = 0.0;
    double gcggb = 0.0;
    double gcgsb = 0.0;
    double gcsdb = 0.0;
    double gcsgb = 0.0;
    double gcssb = 0.0;
    double gds = 0.0;
    double geq = 0.0;
    double gm = 0.0;
    double gmbs = 0.0;
    double sarg = 0.0;
    double sargsw = 0.0;
    double vbd = 0.0;
    double vbs = 0.0;
    double vcrit = 0.0;
    double vds = 0.0;
    double vdsat = 0.0;
    double vgb = 0.0;
    double vgd = 0.0;
    double vgdo = 0.0;
    double vgs = 0.0;
    double von = 0.0;
    double xnrm = 0.0;
    double xrev = 0.0;
    int Check = 0;
    double cgdb = 0.0;
    double cgsb = 0.0;
    double cbdb = 0.0;
    double cdgb = 0.0;
    double cddb = 0.0;
    double cdsb = 0.0;
    double cggb = 0.0;
    double cbgb = 0.0;
    double cbsb = 0.0;
    double csgb = 0.0;
    double cssb = 0.0;
    double csdb = 0.0;
    double PhiB = 0.0;
    double PhiBSW = 0.0;
    double MJ = 0.0;
    double MJSW = 0.0;
    double argsw = 0.0;
    double qgate = 0.0;
    double qbulk = 0.0;
    double qdrn = 0.0;
    double qsrc = 0.0;
    double cqgate = 0.0;
    double cqbulk = 0.0;
    double cqdrn = 0.0;
    double vt0 = 0.0;
    double args[8];
    int    ByPass = 0;
    double tempv = 0.0;

/* SRW
    sB1model *model = static_cast<sB1model*>(genmod);
    for ( ; model; model = model->next()) {
        sB1instance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {
*/
    sB1instance *inst = (sB1instance*)in_inst;
    sB1model *model = (sB1model*)inst->GENmodPtr;

            EffectiveLength=inst->B1l - model->B1deltaL * 1.e-6;/* m */
            DrainArea = inst->B1drainArea;
            SourceArea = inst->B1sourceArea;
            DrainPerimeter = inst->B1drainPerimeter;
            SourcePerimeter = inst->B1sourcePerimeter;
            if( (DrainSatCurrent=DrainArea*model->B1jctSatCurDensity) 
                    < 1e-15){
                DrainSatCurrent = 1.0e-15;
            }
            if( (SourceSatCurrent=SourceArea*model->B1jctSatCurDensity)
                    <1.0e-15){
                SourceSatCurrent = 1.0e-15;
            }
            GateSourceOverlapCap = model->B1gateSourceOverlapCap *inst->B1w;
            GateDrainOverlapCap = model->B1gateDrainOverlapCap * inst->B1w;
            GateBulkOverlapCap = model->B1gateBulkOverlapCap *EffectiveLength;
            von = model->B1type * inst->B1von;
            vdsat = model->B1type * inst->B1vdsat;
            vt0 = model->B1type * inst->B1vt0;

            Check=1;
            ByPass = 0;
            if((ckt->CKTmode & MODEINITSMSIG)) {
                vbs= *(ckt->CKTstate0 + inst->B1vbs);
                vgs= *(ckt->CKTstate0 + inst->B1vgs);
                vds= *(ckt->CKTstate0 + inst->B1vds);
            } else if ((ckt->CKTmode & MODEINITTRAN)) {
                vbs= *(ckt->CKTstate1 + inst->B1vbs);
                vgs= *(ckt->CKTstate1 + inst->B1vgs);
                vds= *(ckt->CKTstate1 + inst->B1vds);
            } else if((ckt->CKTmode & MODEINITJCT) && !inst->B1off) {
                vds= model->B1type * inst->B1icVDS;
                vgs= model->B1type * inst->B1icVGS;
                vbs= model->B1type * inst->B1icVBS;
                if((vds==0) && (vgs==0) && (vbs==0) && 
                        ((ckt->CKTmode & 
                        (MODETRAN|MODEAC|MODEDCOP|MODEDCTRANCURVE)) ||
                        (!(ckt->CKTmode & MODEUIC)))) {
                    vbs = -1;
                    vgs = vt0;
                    vds = 0;
                }
            } else if((ckt->CKTmode & (MODEINITJCT | MODEINITFIX) ) && 
                    (inst->B1off)) {
                vbs=vgs=vds=0;
            } else {
#ifndef PREDICTOR
                if((ckt->CKTmode & MODEINITPRED)) {
                    vbs = DEV.pred(ckt, inst->B1vbs);
                    vgs = DEV.pred(ckt, inst->B1vgs);
                    vds = DEV.pred(ckt, inst->B1vds);

                    *(ckt->CKTstate0 + inst->B1vbs) = 
                            *(ckt->CKTstate1 + inst->B1vbs);
                    *(ckt->CKTstate0 + inst->B1vgs) = 
                            *(ckt->CKTstate1 + inst->B1vgs);
                    *(ckt->CKTstate0 + inst->B1vds) = 
                            *(ckt->CKTstate1 + inst->B1vds);
                    *(ckt->CKTstate0 + inst->B1vbd) = 
                            *(ckt->CKTstate0 + inst->B1vbs)-
                            *(ckt->CKTstate0 + inst->B1vds);
                    *(ckt->CKTstate0 + inst->B1cd) = 
                            *(ckt->CKTstate1 + inst->B1cd);
                    *(ckt->CKTstate0 + inst->B1cbs) = 
                            *(ckt->CKTstate1 + inst->B1cbs);
                    *(ckt->CKTstate0 + inst->B1cbd) = 
                            *(ckt->CKTstate1 + inst->B1cbd);
                    *(ckt->CKTstate0 + inst->B1gm) = 
                            *(ckt->CKTstate1 + inst->B1gm);
                    *(ckt->CKTstate0 + inst->B1gds) = 
                            *(ckt->CKTstate1 + inst->B1gds);
                    *(ckt->CKTstate0 + inst->B1gmbs) = 
                            *(ckt->CKTstate1 + inst->B1gmbs);
                    *(ckt->CKTstate0 + inst->B1gbd) = 
                            *(ckt->CKTstate1 + inst->B1gbd);
                    *(ckt->CKTstate0 + inst->B1gbs) = 
                            *(ckt->CKTstate1 + inst->B1gbs);
                    *(ckt->CKTstate0 + inst->B1cggb) = 
                            *(ckt->CKTstate1 + inst->B1cggb);
                    *(ckt->CKTstate0 + inst->B1cbgb) = 
                            *(ckt->CKTstate1 + inst->B1cbgb);
                    *(ckt->CKTstate0 + inst->B1cbsb) = 
                            *(ckt->CKTstate1 + inst->B1cbsb);
                    *(ckt->CKTstate0 + inst->B1cgdb) = 
                            *(ckt->CKTstate1 + inst->B1cgdb);
                    *(ckt->CKTstate0 + inst->B1cgsb) = 
                            *(ckt->CKTstate1 + inst->B1cgsb);
                    *(ckt->CKTstate0 + inst->B1cbdb) = 
                            *(ckt->CKTstate1 + inst->B1cbdb);
                    *(ckt->CKTstate0 + inst->B1cdgb) = 
                            *(ckt->CKTstate1 + inst->B1cdgb);
                    *(ckt->CKTstate0 + inst->B1cddb) = 
                            *(ckt->CKTstate1 + inst->B1cddb);
                    *(ckt->CKTstate0 + inst->B1cdsb) = 
                            *(ckt->CKTstate1 + inst->B1cdsb);
                } else {
#endif /* PREDICTOR */
                    vbs = model->B1type * ( 
                        *(ckt->CKTrhsOld+inst->B1bNode) -
                        *(ckt->CKTrhsOld+inst->B1sNodePrime));
                    vgs = model->B1type * ( 
                        *(ckt->CKTrhsOld+inst->B1gNode) -
                        *(ckt->CKTrhsOld+inst->B1sNodePrime));
                    vds = model->B1type * ( 
                        *(ckt->CKTrhsOld+inst->B1dNodePrime) -
                        *(ckt->CKTrhsOld+inst->B1sNodePrime));
#ifndef PREDICTOR
                }
#endif /* PREDICTOR */
                vbd=vbs-vds;
                vgd=vgs-vds;
                vgdo = *(ckt->CKTstate0 + inst->B1vgs) - 
                    *(ckt->CKTstate0 + inst->B1vds);
                delvbs = vbs - *(ckt->CKTstate0 + inst->B1vbs);
                delvbd = vbd - *(ckt->CKTstate0 + inst->B1vbd);
                delvgs = vgs - *(ckt->CKTstate0 + inst->B1vgs);
                delvds = vds - *(ckt->CKTstate0 + inst->B1vds);
                delvgd = vgd-vgdo;

                if (inst->B1mode >= 0) {
                    cdhat=
                        *(ckt->CKTstate0 + inst->B1cd) -
                        *(ckt->CKTstate0 + inst->B1gbd) * delvbd +
                        *(ckt->CKTstate0 + inst->B1gmbs) * delvbs +
                        *(ckt->CKTstate0 + inst->B1gm) * delvgs + 
                        *(ckt->CKTstate0 + inst->B1gds) * delvds ;
                } else {
                    cdhat=
                        *(ckt->CKTstate0 + inst->B1cd) -
                        ( *(ckt->CKTstate0 + inst->B1gbd) -
                          *(ckt->CKTstate0 + inst->B1gmbs)) * delvbd -
                        *(ckt->CKTstate0 + inst->B1gm) * delvgd +
                        *(ckt->CKTstate0 + inst->B1gds) * delvds;
                }
                cbhat=
                    *(ckt->CKTstate0 + inst->B1cbs) +
                    *(ckt->CKTstate0 + inst->B1cbd) +
                    *(ckt->CKTstate0 + inst->B1gbd) * delvbd +
                    *(ckt->CKTstate0 + inst->B1gbs) * delvbs ;

                /* now lets see if we can bypass (ugh) */

                /* following should be one big if connected by && all over
                 * the place, but some C compilers can't handle that, so
                 * we split it up inst to let them digest it in stages
                 */
                tempv = SPMAX(FABS(cbhat),FABS(*(ckt->CKTstate0 + inst->B1cbs)
                        + *(ckt->CKTstate0 + inst->B1cbd)))+tsk->TSKabstol;
                if((!(ckt->CKTmode & MODEINITPRED)) && (tsk->TSKbypass) )
                if( (FABS(delvbs) < (tsk->TSKreltol * SPMAX(FABS(vbs),
                        FABS(*(ckt->CKTstate0+inst->B1vbs)))+
                        tsk->TSKvoltTol)) )
                if ( (FABS(delvbd) < (tsk->TSKreltol * SPMAX(FABS(vbd),
                        FABS(*(ckt->CKTstate0+inst->B1vbd)))+
                        tsk->TSKvoltTol)) )
                if( (FABS(delvgs) < (tsk->TSKreltol * SPMAX(FABS(vgs),
                        FABS(*(ckt->CKTstate0+inst->B1vgs)))+
                        tsk->TSKvoltTol)))
                if ( (FABS(delvds) < (tsk->TSKreltol * SPMAX(FABS(vds),
                        FABS(*(ckt->CKTstate0+inst->B1vds)))+
                        tsk->TSKvoltTol)) )
                if( (FABS(cdhat- *(ckt->CKTstate0 + inst->B1cd)) <
                        tsk->TSKreltol * SPMAX(FABS(cdhat),FABS(*(ckt->CKTstate0 +
                        inst->B1cd))) + tsk->TSKabstol) )
                if ( (FABS(cbhat-(*(ckt->CKTstate0 + inst->B1cbs) +
                        *(ckt->CKTstate0 + inst->B1cbd))) < tsk->TSKreltol *
                        tempv)) {
                    /* bypass code */
                    vbs = *(ckt->CKTstate0 + inst->B1vbs);
                    vbd = *(ckt->CKTstate0 + inst->B1vbd);
                    vgs = *(ckt->CKTstate0 + inst->B1vgs);
                    vds = *(ckt->CKTstate0 + inst->B1vds);
                    vgd = vgs - vds;
                    vgb = vgs - vbs;
                    cd = *(ckt->CKTstate0 + inst->B1cd);
                    cbs = *(ckt->CKTstate0 + inst->B1cbs);
                    cbd = *(ckt->CKTstate0 + inst->B1cbd);
                    cdrain = inst->B1mode * (cd + cbd);
                    gm = *(ckt->CKTstate0 + inst->B1gm);
                    gds = *(ckt->CKTstate0 + inst->B1gds);
                    gmbs = *(ckt->CKTstate0 + inst->B1gmbs);
                    gbd = *(ckt->CKTstate0 + inst->B1gbd);
                    gbs = *(ckt->CKTstate0 + inst->B1gbs);
                    if((ckt->CKTmode & (MODETRAN | MODEAC)) || 
                            ((ckt->CKTmode & MODETRANOP) && 
                            (ckt->CKTmode & MODEUIC))) {
                        cggb = *(ckt->CKTstate0 + inst->B1cggb);
                        cgdb = *(ckt->CKTstate0 + inst->B1cgdb);
                        cgsb = *(ckt->CKTstate0 + inst->B1cgsb);
                        cbgb = *(ckt->CKTstate0 + inst->B1cbgb);
                        cbdb = *(ckt->CKTstate0 + inst->B1cbdb);
                        cbsb = *(ckt->CKTstate0 + inst->B1cbsb);
                        cdgb = *(ckt->CKTstate0 + inst->B1cdgb);
                        cddb = *(ckt->CKTstate0 + inst->B1cddb);
                        cdsb = *(ckt->CKTstate0 + inst->B1cdsb);
                        capbs = *(ckt->CKTstate0 + inst->B1capbs);
                        capbd = *(ckt->CKTstate0 + inst->B1capbd);
                        ByPass = 1;
                        goto line755;
                    } else {
                        goto line850;
                    }
                }

                von = model->B1type * inst->B1von;
                if(*(ckt->CKTstate0 + inst->B1vds) >=0) {
                    vgs = DEV.fetlim(vgs,*(ckt->CKTstate0 + inst->B1vgs), von);
                    vds = vgs - vgd;
                    vds = DEV.limvds(vds,*(ckt->CKTstate0 + inst->B1vds));
                    vgd = vgs - vds;
                } else {
                    vgd = DEV.fetlim(vgd,vgdo,von);
                    vds = vgs - vgd;
                    vds = -DEV.limvds(-vds,-(*(ckt->CKTstate0 + inst->B1vds)));
                    vgs = vgd + vds;
                }
                if(vds >= 0) {
                    vcrit =CONSTvt0*log(CONSTvt0/(CONSTroot2*SourceSatCurrent));
                    vbs = DEV.pnjlim(vbs,*(ckt->CKTstate0 + inst->B1vbs),
                            CONSTvt0,vcrit,&Check); /* B1 test */
                    vbd = vbs-vds;
                } else {
                    vcrit = CONSTvt0*log(CONSTvt0/(CONSTroot2*DrainSatCurrent));
                    vbd = DEV.pnjlim(vbd,*(ckt->CKTstate0 + inst->B1vbd),
                            CONSTvt0,vcrit,&Check); /* B1 test*/
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
                inst->B1mode = 1;
            } else {
                /* inverse mode */
                inst->B1mode = -1;
            }
            // call B1evaluate to calculate drain current and its 
            // derivatives and charge and capacitances related to gate
            // drain, and bulk
            //
           if( vds >= 0 )  {
                evaluate(vds,vbs,vgs,inst,model,&gm,&gds,&gmbs,&qgate,
                    &qbulk,&qdrn,&cggb,&cgdb,&cgsb,&cbgb,&cbdb,&cbsb,&cdgb,
                    &cddb,&cdsb,&cdrain,&von,&vdsat,ckt);
            }
            else {
                evaluate(-vds,vbd,vgd,inst,model,&gm,&gds,&gmbs,&qgate,
                    &qbulk,&qsrc,&cggb,&cgsb,&cgdb,&cbgb,&cbsb,&cbdb,&csgb,
                    &cssb,&csdb,&cdrain,&von,&vdsat,ckt);
            }
          
            inst->B1von = model->B1type * von;
            inst->B1vdsat = model->B1type * vdsat;  

        

            /*
             *  COMPUTE EQUIVALENT DRAIN CURRENT SOURCE
             */
            cd=inst->B1mode * cdrain - cbd;
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

                czbd  = model->B1unitAreaJctCap * DrainArea;
                czbs  = model->B1unitAreaJctCap * SourceArea;
                czbdsw= model->B1unitLengthSidewallJctCap * DrainPerimeter;
                czbssw= model->B1unitLengthSidewallJctCap * SourcePerimeter;
                PhiB = model->B1bulkJctPotential;
                PhiBSW = model->B1sidewallJctPotential;
                MJ = model->B1bulkJctBotGradingCoeff;
                MJSW = model->B1bulkJctSideGradingCoeff;

                /* Source Bulk Junction */
                if( vbs < 0 ) {  
                    arg = 1 - vbs / PhiB;
                    argsw = 1 - vbs / PhiBSW;
                    sarg = exp(-MJ*log(arg));
                    sargsw = exp(-MJSW*log(argsw));
                    *(ckt->CKTstate0 + inst->B1qbs) =
                        PhiB * czbs * (1-arg*sarg)/(1-MJ) + PhiBSW * 
                    czbssw * (1-argsw*sargsw)/(1-MJSW);
                    capbs = czbs * sarg + czbssw * sargsw ;
                } else {  
                    *(ckt->CKTstate0+inst->B1qbs) =
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
                    *(ckt->CKTstate0 + inst->B1qbd) =
                        PhiB * czbd * (1-arg*sarg)/(1-MJ) + PhiBSW * 
                    czbdsw * (1-argsw*sargsw)/(1-MJSW);
                    capbd = czbd * sarg + czbdsw * sargsw ;
                } else {  
                    *(ckt->CKTstate0+inst->B1qbd) =
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
            if ( (inst->B1off == 0)  || (!(ckt->CKTmode & MODEINITFIX)) ){
                if (Check == 1) {
                    ckt->incNoncon();  // SRW
                    ckt->CKTtroubleElt = inst;
                }
            }
            *(ckt->CKTstate0 + inst->B1vbs) = vbs;
            *(ckt->CKTstate0 + inst->B1vbd) = vbd;
            *(ckt->CKTstate0 + inst->B1vgs) = vgs;
            *(ckt->CKTstate0 + inst->B1vds) = vds;
            *(ckt->CKTstate0 + inst->B1cd) = cd;
            *(ckt->CKTstate0 + inst->B1cbs) = cbs;
            *(ckt->CKTstate0 + inst->B1cbd) = cbd;
            *(ckt->CKTstate0 + inst->B1gm) = gm;
            *(ckt->CKTstate0 + inst->B1gds) = gds;
            *(ckt->CKTstate0 + inst->B1gmbs) = gmbs;
            *(ckt->CKTstate0 + inst->B1gbd) = gbd;
            *(ckt->CKTstate0 + inst->B1gbs) = gbs;

            *(ckt->CKTstate0 + inst->B1cggb) = cggb;
            *(ckt->CKTstate0 + inst->B1cgdb) = cgdb;
            *(ckt->CKTstate0 + inst->B1cgsb) = cgsb;

            *(ckt->CKTstate0 + inst->B1cbgb) = cbgb;
            *(ckt->CKTstate0 + inst->B1cbdb) = cbdb;
            *(ckt->CKTstate0 + inst->B1cbsb) = cbsb;

            *(ckt->CKTstate0 + inst->B1cdgb) = cdgb;
            *(ckt->CKTstate0 + inst->B1cddb) = cddb;
            *(ckt->CKTstate0 + inst->B1cdsb) = cdsb;

            *(ckt->CKTstate0 + inst->B1capbs) = capbs;
            *(ckt->CKTstate0 + inst->B1capbd) = capbd;

           /* bulk and channel charge plus overlaps */

            if((!(ckt->CKTmode & (MODETRAN | MODEAC))) &&
                    ((!(ckt->CKTmode & MODETRANOP)) ||
                    (!(ckt->CKTmode & MODEUIC)))  && (!(ckt->CKTmode 
                    &  MODEINITSMSIG))) goto line850;
         
line755:
            /* new 3f2 */
            if( inst->B1mode > 0 ) {

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
                        cbgb,cbdb,cbsb,
                        cdgb,cddb,cdsb,
                        &gcggb,&gcgdb,&gcgsb,
                        &gcbgb,&gcbdb,&gcbsb,
                        &gcdgb,&gcddb,&gcdsb,&gcsgb,&gcsdb,&gcssb,
                        &qgate,&qbulk,
                        &qdrn,&qsrc);
            } else {

                args[0] = GateSourceOverlapCap;
                args[1] = GateDrainOverlapCap;
                args[2] = GateBulkOverlapCap;
                args[3] = capbs;
                args[4] = capbd;
                args[5] = cggb;
                args[6] = cgsb;
                args[7] = cgdb;

                mosCap(ckt,vgs,vgd,vgb,
                        args,
                        cbgb,cbsb,cbdb,
                        csgb,cssb,csdb,
                        &gcggb,&gcgsb,&gcgdb,
                        &gcbgb,&gcbsb,&gcbdb,
                        &gcsgb,&gcssb,&gcsdb,&gcdgb,&gcdsb,&gcddb,
                        &qgate,&qbulk,
                        &qsrc,&qdrn);
            }

            /* old 3e2
            if( inst->B1mode > 0 ) {
                mosCap(ckt,vgd,vgs,vgb,GateDrainOverlapCap,
                        GateSourceOverlapCap,GateBulkOverlapCap,capbd,capbs
                        ,cggb,cgdb,cgsb,cbgb,cbdb,cbsb,cdgb,cddb,cdsb
                        ,&gcggb,&gcgdb,&gcgsb,&gcbgb,&gcbdb,&gcbsb,&gcdgb
                        ,&gcddb,&gcdsb,&gcsgb,&gcsdb,&gcssb,&qgate,&qbulk
                        ,&qdrn,&qsrc);
            } else {
                mosCap(ckt,vgs,vgd,vgb,GateSourceOverlapCap,
                    GateDrainOverlapCap,GateBulkOverlapCap,capbs,capbd
                    ,cggb,cgsb,cgdb,cbgb,cbsb,cbdb,csgb,cssb,csdb
                    ,&gcggb,&gcgsb,&gcgdb,&gcbgb,&gcbsb,&gcbdb,&gcsgb
                    ,&gcssb,&gcsdb,&gcdgb,&gcdsb,&gcddb,&qgate,&qbulk
                    ,&qsrc,&qdrn);
            }
            */
             
            if(ByPass) goto line860;
            *(ckt->CKTstate0 + inst->B1qg) = qgate;
            *(ckt->CKTstate0 + inst->B1qd) = qdrn -  
                    *(ckt->CKTstate0 + inst->B1qbd);
            *(ckt->CKTstate0 + inst->B1qb) = qbulk +  
                    *(ckt->CKTstate0 + inst->B1qbd) +  
                    *(ckt->CKTstate0 + inst->B1qbs); 

            /* store small signal parameters */
            if((!(ckt->CKTmode & (MODEAC | MODETRAN))) &&
                    (ckt->CKTmode & MODETRANOP ) && (ckt->CKTmode &
                    MODEUIC ))   goto line850;
            if(ckt->CKTmode & MODEINITSMSIG ) {  
                *(ckt->CKTstate0+inst->B1cggb) = cggb;
                *(ckt->CKTstate0+inst->B1cgdb) = cgdb;
                *(ckt->CKTstate0+inst->B1cgsb) = cgsb;
                *(ckt->CKTstate0+inst->B1cbgb) = cbgb;
                *(ckt->CKTstate0+inst->B1cbdb) = cbdb;
                *(ckt->CKTstate0+inst->B1cbsb) = cbsb;
                *(ckt->CKTstate0+inst->B1cdgb) = cdgb;
                *(ckt->CKTstate0+inst->B1cddb) = cddb;
                *(ckt->CKTstate0+inst->B1cdsb) = cdsb;     
                *(ckt->CKTstate0+inst->B1capbd) = capbd;
                *(ckt->CKTstate0+inst->B1capbs) = capbs;

                goto line1000;
            }
       
            if(ckt->CKTmode & MODEINITTRAN ) { 
                *(ckt->CKTstate1+inst->B1qb) =
                    *(ckt->CKTstate0+inst->B1qb) ;
                *(ckt->CKTstate1+inst->B1qg) =
                    *(ckt->CKTstate0+inst->B1qg) ;
                *(ckt->CKTstate1+inst->B1qd) =
                    *(ckt->CKTstate0+inst->B1qd) ;
            }
       
       
            ckt->integrate(&geq,&ceq,0.0,inst->B1qb);
            ckt->integrate(&geq,&ceq,0.0,inst->B1qg);
            ckt->integrate(&geq,&ceq,0.0,inst->B1qd);
      
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
            cqgate = *(ckt->CKTstate0 + inst->B1iqg);
            cqbulk = *(ckt->CKTstate0 + inst->B1iqb);
            cqdrn = *(ckt->CKTstate0 + inst->B1iqd);
            ceqqg = cqgate - gcggb * vgb + gcgdb * vbd + gcgsb * vbs;
            ceqqb = cqbulk - gcbgb * vgb + gcbdb * vbd + gcbsb * vbs;
            ceqqd = cqdrn - gcdgb * vgb + gcddb * vbd + gcdsb * vbs;

            if(ckt->CKTmode & MODEINITTRAN ) {  
                *(ckt->CKTstate1 + inst->B1iqb) =  
                    *(ckt->CKTstate0 + inst->B1iqb);
                *(ckt->CKTstate1 + inst->B1iqg) =  
                    *(ckt->CKTstate0 + inst->B1iqg);
                *(ckt->CKTstate1 + inst->B1iqd) =  
                    *(ckt->CKTstate0 + inst->B1iqd);
            }

            /*
             *  load current vector
             */
line900:
   
            ceqbs = model->B1type * (cbs-(gbs-tsk->TSKgmin)*vbs);
            ceqbd = model->B1type * (cbd-(gbd-tsk->TSKgmin)*vbd);
     
            ceqqg = model->B1type * ceqqg;
            ceqqb = model->B1type * ceqqb;
            ceqqd =  model->B1type * ceqqd;
            if (inst->B1mode >= 0) {
                xnrm=1;
                xrev=0;
                cdreq=model->B1type*(cdrain-gds*vds-gm*vgs-gmbs*vbs);
            } else {
                xnrm=0;
                xrev=1;
                cdreq = -(model->B1type)*(cdrain+gds*vds-gm*vgd-gmbs*vbd);
            }

#define MSC(xx) inst->B1m*(xx)

            ckt->rhsadd(inst->B1gNode, -MSC(ceqqg));
            ckt->rhsadd(inst->B1bNode, -MSC(ceqbs+ceqbd+ceqqb));
            ckt->rhsadd(inst->B1dNodePrime, MSC(ceqbd-cdreq-ceqqd));
            ckt->rhsadd(inst->B1sNodePrime, MSC(cdreq+ceqbs+ceqqg+ceqqb+ceqqd));

            /*
             *  load y matrix
             */

            ckt->ldadd(inst->B1DdPtr, MSC(inst->B1drainConductance));
            ckt->ldadd(inst->B1GgPtr, MSC(gcggb));
            ckt->ldadd(inst->B1SsPtr, MSC(inst->B1sourceConductance));
            ckt->ldadd(inst->B1BbPtr, MSC(gbd+gbs-gcbgb-gcbdb-gcbsb));
            ckt->ldadd(inst->B1DPdpPtr,
                MSC(inst->B1drainConductance+gds+gbd+xrev*(gm+gmbs)+gcddb));
            ckt->ldadd(inst->B1SPspPtr,
                MSC(inst->B1sourceConductance+gds+gbs+xnrm*(gm+gmbs)+gcssb));
            ckt->ldadd(inst->B1DdpPtr, MSC(-inst->B1drainConductance));
            ckt->ldadd(inst->B1GbPtr, MSC(-gcggb-gcgdb-gcgsb));
            ckt->ldadd(inst->B1GdpPtr, MSC(gcgdb));
            ckt->ldadd(inst->B1GspPtr, MSC(gcgsb));
            ckt->ldadd(inst->B1SspPtr, MSC(-inst->B1sourceConductance));
            ckt->ldadd(inst->B1BgPtr, MSC(gcbgb));
            ckt->ldadd(inst->B1BdpPtr, MSC(-gbd+gcbdb));
            ckt->ldadd(inst->B1BspPtr, MSC(-gbs+gcbsb));
            ckt->ldadd(inst->B1DPdPtr, MSC(-inst->B1drainConductance));
            ckt->ldadd(inst->B1DPgPtr, MSC((xnrm-xrev)*gm+gcdgb));
            ckt->ldadd(inst->B1DPbPtr, MSC(-gbd+(xnrm-xrev)*gmbs-gcdgb-gcddb-gcdsb));
            ckt->ldadd(inst->B1DPspPtr, MSC(-gds-xnrm*(gm+gmbs)+gcdsb));
            ckt->ldadd(inst->B1SPgPtr, MSC(-(xnrm-xrev)*gm+gcsgb));
            ckt->ldadd(inst->B1SPsPtr, MSC(-inst->B1sourceConductance));
            ckt->ldadd(inst->B1SPbPtr, MSC(-gbs-(xnrm-xrev)*gmbs-gcsgb-gcsdb-gcssb));
            ckt->ldadd(inst->B1SPdpPtr, MSC(-gds-xrev*(gm+gmbs)+gcsdb));


line1000:  ;

// SRW        }   /* End of Mosfet Instance */

// SRW    }       /* End of Model Instance */
    return(OK);
}

