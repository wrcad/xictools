
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
 $Id: b3load.cc,v 2.6 2015/06/11 01:12:28 stevew Exp $
 *========================================================================*/

/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1991 JianHui Huang and Min-Chie Jeng.
Modified by Mansun Chan  (1995)
Modified by Weidong Liu (1997-1998).
* Revision 3.2 1998/6/16  18:00:00  Weidong
* BSIM3v3.2 release
**********/

#include "b3defs.h"

#define MAX_EXP 5.834617425e14
#define MIN_EXP 1.713908431e-15
#define EXP_THRESHOLD 34.0
#define EPSOX 3.453133e-11
#define EPSSI 1.03594e-10
#define Charge_q 1.60219e-19
#define DELTA_1 0.02
#define DELTA_2 0.02
#define DELTA_3 0.02
#define DELTA_4 0.02


int
// SRW B3dev::load(sGENmodel *genmod, sCKT *ckt)
B3dev::load(sGENinstance *in_inst, sCKT *ckt)
{
    double SourceSatCurrent, DrainSatCurrent;
    double ag0, qgd, qgs, qgb, von, cbhat, VgstNVt, ExpVgst;
    double cdrain, cdhat, cdreq, ceqbd, ceqbs, ceqqb, ceqqd, ceqqg;
    double czbd, czbdsw, czbdswg, czbs, czbssw, czbsswg, evbd, evbs, arg, sarg;
    double delvbd, delvbs, delvds, delvgd, delvgs;
    double Vfbeff, dVfbeff_dVg, dVfbeff_dVd, dVfbeff_dVb, V3, V4;
    double gcbdb, gcbgb, gcbsb, gcddb, gcdgb, gcdsb, gcgdb, gcggb, gcgsb, gcsdb;
    double gcsgb, gcssb, MJ, MJSW, MJSWG;
    double vbd, vbs, vds, vgb, vgd, vgs, vgdo;
    double qgate, qbulk, qdrn, qsrc, qinoi, cqgate, cqbulk, cqdrn;
    double Vds, Vgs, Vbs, Gmbs, FwdSum, RevSum;
    double Vgs_eff, Vfb, dVfb_dVb, dVfb_dVd;
    double Phis, dPhis_dVb, sqrtPhis, dsqrtPhis_dVb, Vth, dVth_dVb, dVth_dVd;
    double Vgst, /*dVgst_dVg, dVgst_dVb,*/ dVgs_eff_dVg, Nvtm;
    double Vtm;
    double n, dn_dVb, dn_dVd, voffcv, noff, dnoff_dVd, dnoff_dVb;
    double ExpArg, V0, CoxWLcen, QovCox, LINK;
    double DeltaPhi, dDeltaPhi_dVg/*, dDeltaPhi_dVd, dDeltaPhi_dVb*/;
    double Cox, Tox, Tcen, dTcen_dVg, dTcen_dVd, dTcen_dVb;
    double Ccen, Coxeff, dCoxeff_dVg, dCoxeff_dVd, dCoxeff_dVb;
    double Denomi, dDenomi_dVg, dDenomi_dVd, dDenomi_dVb;
    double ueff, dueff_dVg, dueff_dVd, dueff_dVb; 
    double Esat, Vdsat;
    double EsatL, dEsatL_dVg, dEsatL_dVd, dEsatL_dVb;
    double dVdsat_dVg, dVdsat_dVb, dVdsat_dVd, Vasat, dAlphaz_dVg, dAlphaz_dVb; 
    double dVasat_dVg, dVasat_dVb, dVasat_dVd, Va;
    double dVa_dVd, dVa_dVg, dVa_dVb; 
    double Vbseff, dVbseff_dVb, VbseffCV, dVbseffCV_dVb; 
    double Arg1, One_Third_CoxWL, Two_Third_CoxWL, Alphaz, CoxWL; 
    double T0, dT0_dVg, dT0_dVd, dT0_dVb;
    double T1, dT1_dVg, dT1_dVd, dT1_dVb;
    double T2, dT2_dVg, dT2_dVd, dT2_dVb;
    double T3, dT3_dVg, dT3_dVd, dT3_dVb;
    double T4;
    double T5;
    double T6;
    double T7;
    double T8;
    double T9;
    double T10;
    double T11, T12;
    double tmp, Abulk, dAbulk_dVb, Abulk0, dAbulk0_dVb;
    double VACLM, dVACLM_dVg, dVACLM_dVd, dVACLM_dVb;
    double VADIBL, dVADIBL_dVg, dVADIBL_dVd, dVADIBL_dVb;
    double Xdep, dXdep_dVb, lt1, dlt1_dVb, ltw, dltw_dVb;
    double Delt_vth, dDelt_vth_dVb;
    double Theta0, dTheta0_dVb;
    double TempRatio, tmp1, tmp2, tmp3, tmp4;
    double DIBL_Sft, dDIBL_Sft_dVd, Lambda, dLambda_dVg;
    double Idtot, Ibtot;
    double tempv, a1, ScalingFactor;

    double Vgsteff, dVgsteff_dVg, dVgsteff_dVd, dVgsteff_dVb; 
    double Vdseff, dVdseff_dVg, dVdseff_dVd, dVdseff_dVb; 
    double VdseffCV, dVdseffCV_dVg, dVdseffCV_dVd, dVdseffCV_dVb; 
    double diffVds, dAbulk_dVg;
    double beta, dbeta_dVg, dbeta_dVd, dbeta_dVb;
    double gche, dgche_dVg, dgche_dVd, dgche_dVb;
    double fgche1, dfgche1_dVg, dfgche1_dVd, dfgche1_dVb;
    double fgche2, dfgche2_dVg, dfgche2_dVd, dfgche2_dVb;
    double Idl, dIdl_dVg, dIdl_dVd, dIdl_dVb;
    double Idsa, dIdsa_dVg, dIdsa_dVd, dIdsa_dVb;
    double Ids, Gm, Gds, Gmb;
    double Isub, Gbd, Gbg, Gbb;
    double VASCBE, dVASCBE_dVg, dVASCBE_dVd, dVASCBE_dVb;
    double CoxWovL;
    double Rds, dRds_dVg, dRds_dVb, WVCox, WVCoxRds;
    double Vgst2Vtm, VdsatCV, dVdsatCV_dVg, dVdsatCV_dVb;
    double Leff, Weff, dWeff_dVg, dWeff_dVb;
    double AbulkCV, dAbulkCV_dVb;
    double qgdo, qgso, cgdo, cgso;

    double qcheq, qdef, gqdef, cqdef, cqcheq, gtau_diff, gtau_drift;
    double gcqdb,gcqsb,gcqgb,gcqbb;
    double dxpart, sxpart, ggtg, ggtd, ggts, ggtb;
    double ddxpart_dVd, ddxpart_dVg, ddxpart_dVb, ddxpart_dVs;
    double dsxpart_dVd, dsxpart_dVg, dsxpart_dVb, dsxpart_dVs;

    double gbspsp, gbbdp, gbbsp, gbspg, gbspb, gbspdp; 
    double gbdpdp, gbdpg, gbdpb, gbdpsp; 
    double Cgg, Cgd, Cgb, Cdg, Cdd, Cds;
    double Csg, Csd, Css, Csb, Cbg, Cbd, Cbb;
    double Cgg1, Cgb1, Cgd1, Cbg1, Cbb1, Cbd1, Qac0, Qsub0;
    double dQac0_dVg, dQac0_dVd, dQac0_dVb, dQsub0_dVg, dQsub0_dVd, dQsub0_dVb;
       
    struct bsim3SizeDependParam *pParam;
    int ByPass, Check, ChargeComputationNeeded;

    ScalingFactor = 1.0e-9;
    /* SRW
    ChargeComputationNeeded =  
        ((ckt->CKTmode & (MODEAC | MODETRAN | MODEINITSMSIG)) ||
        ((ckt->CKTmode & MODETRANOP) && (ckt->CKTmode & MODEUIC)))
        ? 1 : 0;
    */
    ChargeComputationNeeded = ckt->CKTchargeCompNeeded;

    // SRW - avoid compiler warnings about unset variables
    cbhat = 0.0;
    cdhat = 0.0;
    qgate = 0.0;
    qbulk = 0.0;
    qdrn = 0.0;
    qcheq = 0.0;
    gqdef = 0.0;
    gcqdb = 0.0;
    gcqsb = 0.0;
    gcqgb = 0.0;
    gcqbb = 0.0;

/* SRW
sB3model *model = static_cast<sB3model*>(genmod);
for ( ; model; model = model->next()) {

    sB3instance *inst;
    for (inst = model->inst(); inst; inst = inst->next()) {
*/
    sB3instance *inst = (sB3instance*)in_inst;
    sB3model *model = (sB3model*)inst->GENmodPtr;

        Check = 1;
        ByPass = 0;
        pParam = inst->pParam;
        if ((ckt->CKTmode & MODEINITSMSIG)) {
            vbs = *(ckt->CKTstate0 + inst->B3vbs);
            vgs = *(ckt->CKTstate0 + inst->B3vgs);
            vds = *(ckt->CKTstate0 + inst->B3vds);
            qdef = *(ckt->CKTstate0 + inst->B3qdef);
        }
        else if ((ckt->CKTmode & MODEINITTRAN)) {
            vbs = *(ckt->CKTstate1 + inst->B3vbs);
            vgs = *(ckt->CKTstate1 + inst->B3vgs);
            vds = *(ckt->CKTstate1 + inst->B3vds);
            qdef = *(ckt->CKTstate1 + inst->B3qdef);
        }
        else if ((ckt->CKTmode & MODEINITJCT) && !inst->B3off) {
            vds = model->B3type * inst->B3icVDS;
            vgs = model->B3type * inst->B3icVGS;
            vbs = model->B3type * inst->B3icVBS;
            qdef = 0.0;

            if ((vds == 0.0) && (vgs == 0.0) && (vbs == 0.0) && 
                    ((ckt->CKTmode & (MODETRAN | MODEAC|MODEDCOP |
                    MODEDCTRANCURVE)) || (!(ckt->CKTmode & MODEUIC)))) {
                vbs = 0.0;
                vgs = model->B3type * pParam->B3vth0 + 0.1;
                vds = 0.1;
            }
        }
        else if ((ckt->CKTmode & (MODEINITJCT | MODEINITFIX)) && 
                (inst->B3off)) {
            qdef = vbs = vgs = vds = 0.0;
        }
        else {
#ifndef PREDICTOR
            if ((ckt->CKTmode & MODEINITPRED)) {
                vbs = DEV.pred(ckt, inst->B3vbs);
                vgs = DEV.pred(ckt, inst->B3vgs);
                vds = DEV.pred(ckt, inst->B3vds);
                qdef = DEV.pred(ckt, inst->B3qdef);

                *(ckt->CKTstate0 + inst->B3vbs) = 
                    *(ckt->CKTstate1 + inst->B3vbs);
                *(ckt->CKTstate0 + inst->B3vgs) = 
                    *(ckt->CKTstate1 + inst->B3vgs);
                *(ckt->CKTstate0 + inst->B3vds) = 
                    *(ckt->CKTstate1 + inst->B3vds);
                *(ckt->CKTstate0 + inst->B3vbd) = 
                    *(ckt->CKTstate0 + inst->B3vbs)
                    - *(ckt->CKTstate0 + inst->B3vds);
                /*
                double xfact = ckt->CKTdelta / ckt->CKTdeltaOld[1];
                *(ckt->CKTstate0 + inst->B3vbs) = 
                    *(ckt->CKTstate1 + inst->B3vbs);
                vbs = (1.0 + xfact)* (*(ckt->CKTstate1 + inst->B3vbs))
                    - (xfact * (*(ckt->CKTstate2 + inst->B3vbs)));
                *(ckt->CKTstate0 + inst->B3vgs) = 
                    *(ckt->CKTstate1 + inst->B3vgs);
                vgs = (1.0 + xfact)* (*(ckt->CKTstate1 + inst->B3vgs))
                    - (xfact * (*(ckt->CKTstate2 + inst->B3vgs)));
                *(ckt->CKTstate0 + inst->B3vds) = 
                    *(ckt->CKTstate1 + inst->B3vds);
                vds = (1.0 + xfact)* (*(ckt->CKTstate1 + inst->B3vds))
                    - (xfact * (*(ckt->CKTstate2 + inst->B3vds)));
                *(ckt->CKTstate0 + inst->B3vbd) = 
                    *(ckt->CKTstate0 + inst->B3vbs)
                    - *(ckt->CKTstate0 + inst->B3vds);
                *(ckt->CKTstate0 + inst->B3qdef) =
                    *(ckt->CKTstate1 + inst->B3qdef);
                qdef = (1.0 + xfact)* (*(ckt->CKTstate1 + inst->B3qdef))
                    -(xfact * (*(ckt->CKTstate2 + inst->B3qdef)));
                */
            }
            else {
#endif // PREDICTOR
                vbs = model->B3type
                    * (*(ckt->CKTrhsOld + inst->B3bNode)
                    - *(ckt->CKTrhsOld + inst->B3sNodePrime));
                vgs = model->B3type
                    * (*(ckt->CKTrhsOld + inst->B3gNode) 
                    - *(ckt->CKTrhsOld + inst->B3sNodePrime));
                vds = model->B3type
                    * (*(ckt->CKTrhsOld + inst->B3dNodePrime)
                    - *(ckt->CKTrhsOld + inst->B3sNodePrime));
                qdef = model->B3type
                    * (*(ckt->CKTrhsOld + inst->B3qNode));
#ifndef PREDICTOR
            }
#endif // PREDICTOR

            vbd = vbs - vds;
            vgd = vgs - vds;
            vgdo = *(ckt->CKTstate0 + inst->B3vgs)
                - *(ckt->CKTstate0 + inst->B3vds);
            delvbs = vbs - *(ckt->CKTstate0 + inst->B3vbs);
            delvbd = vbd - *(ckt->CKTstate0 + inst->B3vbd);
            delvgs = vgs - *(ckt->CKTstate0 + inst->B3vgs);
            delvds = vds - *(ckt->CKTstate0 + inst->B3vds);
            delvgd = vgd - vgdo;

            if (inst->B3mode >= 0) {
                Idtot = inst->B3cd + inst->B3csub - inst->B3cbd;
                cdhat = Idtot - inst->B3gbd * delvbd
                    + (inst->B3gmbs + inst->B3gbbs) * delvbs
                    + (inst->B3gm + inst->B3gbgs) * delvgs
                    + (inst->B3gds + inst->B3gbds) * delvds;
                Ibtot = inst->B3cbs + inst->B3cbd - inst->B3csub;
                cbhat = Ibtot + inst->B3gbd * delvbd
                    + (inst->B3gbs - inst->B3gbbs) * delvbs
                    - inst->B3gbgs * delvgs
                    - inst->B3gbds * delvds;
            }
            else {
                Idtot = inst->B3cd - inst->B3cbd;
                cdhat = Idtot - (inst->B3gbd - inst->B3gmbs) * delvbd
                    + inst->B3gm * delvgd
                    - inst->B3gds * delvds;
                Ibtot = inst->B3cbs + inst->B3cbd - inst->B3csub;
                cbhat = Ibtot + inst->B3gbs * delvbs
                    + (inst->B3gbd - inst->B3gbbs) * delvbd
                    - inst->B3gbgs * delvgd
                    + inst->B3gbds * delvds;
            }

#ifndef NOBYPASS
            // following should be one big if connected by && all over
            // the place, but some C compilers can't handle that, so
            // we split it up inst to let them digest it in stages
            //

            if ((!(ckt->CKTmode & MODEINITPRED)) &&
                    (ckt->CKTcurTask->TSKbypass))
            if ((FABS(delvbs) < (ckt->CKTcurTask->TSKreltol * SPMAX(FABS(vbs),
                FABS(*(ckt->CKTstate0+inst->B3vbs))) +
                ckt->CKTcurTask->TSKvoltTol)))
            if ((FABS(delvbd) < (ckt->CKTcurTask->TSKreltol * SPMAX(FABS(vbd),
                FABS(*(ckt->CKTstate0+inst->B3vbd))) +
                ckt->CKTcurTask->TSKvoltTol)))
            if ((FABS(delvgs) < (ckt->CKTcurTask->TSKreltol * SPMAX(FABS(vgs),
                FABS(*(ckt->CKTstate0+inst->B3vgs))) +
                ckt->CKTcurTask->TSKvoltTol)))
            if ((FABS(delvds) < (ckt->CKTcurTask->TSKreltol * SPMAX(FABS(vds),
                FABS(*(ckt->CKTstate0+inst->B3vds))) +
                ckt->CKTcurTask->TSKvoltTol)))
            if ((FABS(cdhat - Idtot) < ckt->CKTcurTask->TSKreltol
                    * SPMAX(FABS(cdhat),FABS(Idtot)) +
                    ckt->CKTcurTask->TSKabstol)) {
                tempv = SPMAX(FABS(cbhat),FABS(Ibtot)) +
                    ckt->CKTcurTask->TSKabstol;
                if ((FABS(cbhat - Ibtot)) <
                        ckt->CKTcurTask->TSKreltol * tempv) {
                    // bypass code
                    vbs = *(ckt->CKTstate0 + inst->B3vbs);
                    vbd = *(ckt->CKTstate0 + inst->B3vbd);
                    vgs = *(ckt->CKTstate0 + inst->B3vgs);
                    vds = *(ckt->CKTstate0 + inst->B3vds);
                    qdef = *(ckt->CKTstate0 + inst->B3qdef);

                    vgd = vgs - vds;
                    vgb = vgs - vbs;

                    cdrain = inst->B3cd;
                    if ((ckt->CKTmode & (MODETRAN | MODEAC)) || 
                            ((ckt->CKTmode & MODETRANOP) && 
                            (ckt->CKTmode & MODEUIC))) {
                        ByPass = 1;
                        qgate = inst->B3qgate;
                        qbulk = inst->B3qbulk;
                        qdrn = inst->B3qdrn;
                        goto line755;
                    }
                    else {
                        goto line850;
                    }
                }
            }

#endif // NOBYPASS
            von = inst->B3von;
            if (*(ckt->CKTstate0 + inst->B3vds) >= 0.0) {
                vgs = DEV.fetlim(vgs, *(ckt->CKTstate0+inst->B3vgs), von);
                vds = vgs - vgd;
                vds = DEV.limvds(vds, *(ckt->CKTstate0 + inst->B3vds));
                vgd = vgs - vds;
            }
            else {
                vgd = DEV.fetlim(vgd, vgdo, von);
                vds = vgs - vgd;
                vds = -DEV.limvds(-vds, -(*(ckt->CKTstate0+inst->B3vds)));
                vgs = vgd + vds;
            }

            if (vds >= 0.0) {
                vbs = DEV.pnjlim(vbs, *(ckt->CKTstate0 + inst->B3vbs),
                    CONSTvt0, model->B3vcrit, &Check);
                vbd = vbs - vds;
            }
            else {
                vbd = DEV.pnjlim(vbd, *(ckt->CKTstate0 + inst->B3vbd),
                    CONSTvt0, model->B3vcrit, &Check); 
                vbs = vbd + vds;
            }
        }

        // determine DC current and derivatives
        vbd = vbs - vds;
        vgd = vgs - vds;
        vgb = vgs - vbs;

        // Source/drain junction diode DC model begins
        Nvtm = model->B3vtm * model->B3jctEmissionCoeff;
        if ((inst->B3sourceArea <= 0.0) && (inst->B3sourcePerimeter <= 0.0)) {
            SourceSatCurrent = 1.0e-14;
        }
        else {
            SourceSatCurrent = inst->B3sourceArea
                * model->B3jctTempSatCurDensity
                + inst->B3sourcePerimeter
                * model->B3jctSidewallTempSatCurDensity;
        }
        if (SourceSatCurrent <= 0.0) {
            inst->B3gbs = ckt->CKTcurTask->TSKgmin;
            inst->B3cbs = inst->B3gbs * vbs;
        }
        else {
            if (model->B3ijth == 0.0) {
                evbs = exp(vbs / Nvtm);
                inst->B3gbs = SourceSatCurrent * evbs / Nvtm +
                    ckt->CKTcurTask->TSKgmin;
                inst->B3cbs = SourceSatCurrent * (evbs - 1.0)
                    + ckt->CKTcurTask->TSKgmin * vbs; 
            }
            else {
                if (vbs < inst->B3vjsm) {
                    evbs = exp(vbs / Nvtm);
                    inst->B3gbs = SourceSatCurrent * evbs / Nvtm +
                        ckt->CKTcurTask->TSKgmin;
                    inst->B3cbs = SourceSatCurrent * (evbs - 1.0)
                        + ckt->CKTcurTask->TSKgmin * vbs;
                }
                else {
                    T0 = (SourceSatCurrent + model->B3ijth) / Nvtm;
                    inst->B3gbs = T0 + ckt->CKTcurTask->TSKgmin;
                    inst->B3cbs = model->B3ijth + ckt->CKTcurTask->TSKgmin *
                        vbs + T0 * (vbs - inst->B3vjsm);
                }
            }
        }

        if ((inst->B3drainArea <= 0.0) && (inst->B3drainPerimeter <= 0.0)) {
            DrainSatCurrent = 1.0e-14;
        }
        else {
            DrainSatCurrent = inst->B3drainArea
                * model->B3jctTempSatCurDensity
                + inst->B3drainPerimeter
                * model->B3jctSidewallTempSatCurDensity;
        }
        if (DrainSatCurrent <= 0.0) {
            inst->B3gbd = ckt->CKTcurTask->TSKgmin;
            inst->B3cbd = inst->B3gbd * vbd;
        }
        else {
            if (model->B3ijth == 0.0) {
                evbd = exp(vbd / Nvtm);
                inst->B3gbd = DrainSatCurrent * evbd / Nvtm +
                    ckt->CKTcurTask->TSKgmin;
                inst->B3cbd = DrainSatCurrent * (evbd - 1.0)
                    + ckt->CKTcurTask->TSKgmin * vbd;
            }
            else {
                if (vbd < inst->B3vjdm) {
                    evbd = exp(vbd / Nvtm);
                    inst->B3gbd = DrainSatCurrent * evbd / Nvtm +
                        ckt->CKTcurTask->TSKgmin;
                    inst->B3cbd = DrainSatCurrent * (evbd - 1.0)
                        + ckt->CKTcurTask->TSKgmin * vbd;
                }
                else {
                    T0 = (DrainSatCurrent + model->B3ijth) / Nvtm;
                    inst->B3gbd = T0 + ckt->CKTcurTask->TSKgmin;
                    inst->B3cbd = model->B3ijth + ckt->CKTcurTask->TSKgmin *
                        vbd + T0 * (vbd - inst->B3vjdm);
                }
            }
        }
        // End of diode DC model

        if (vds >= 0.0) {
            // normal mode
            inst->B3mode = 1;
            Vds = vds;
            Vgs = vgs;
            Vbs = vbs;
        }
        else {
            // inverse mode
            inst->B3mode = -1;
            Vds = -vds;
            Vgs = vgd;
            Vbs = vbd;
        }

        T0 = Vbs - pParam->B3vbsc - 0.001;
        T1 = sqrt(T0 * T0 - 0.004 * pParam->B3vbsc);
        Vbseff = pParam->B3vbsc + 0.5 * (T0 + T1);
        dVbseff_dVb = 0.5 * (1.0 + T0 / T1);
        if (Vbseff < Vbs) {
            Vbseff = Vbs;
        }
        // Added to avoid the possible numerical problems due to computer
        // accuracy. See comments for diffVds

        if (Vbseff > 0.0) {
            T0 = pParam->B3phi / (pParam->B3phi + Vbseff);
            Phis = pParam->B3phi * T0;
            dPhis_dVb = -T0 * T0;
            sqrtPhis = pParam->B3phis3 / (pParam->B3phi + 0.5 * Vbseff);
            dsqrtPhis_dVb = -0.5 * sqrtPhis * sqrtPhis / pParam->B3phis3;
        }
        else {
            Phis = pParam->B3phi - Vbseff;
            dPhis_dVb = -1.0;
            sqrtPhis = sqrt(Phis);
            dsqrtPhis_dVb = -0.5 / sqrtPhis; 
        }
        Xdep = pParam->B3Xdep0 * sqrtPhis / pParam->B3sqrtPhi;
        dXdep_dVb = (pParam->B3Xdep0 / pParam->B3sqrtPhi) * dsqrtPhis_dVb;

        Leff = pParam->B3leff;
        Vtm = model->B3vtm;

        // Vth Calculation

        T3 = sqrt(Xdep);
        V0 = pParam->B3vbi - pParam->B3phi;

        T0 = pParam->B3dvt2 * Vbseff;
        if (T0 >= - 0.5) {
            T1 = 1.0 + T0;
            T2 = pParam->B3dvt2;
        }
        else {
            // Added to avoid any discontinuity problems caused by dvt2
            T4 = 1.0 / (3.0 + 8.0 * T0);
            T1 = (1.0 + 3.0 * T0) * T4; 
            T2 = pParam->B3dvt2 * T4 * T4;
        }
        lt1 = model->B3factor1 * T3 * T1;
        dlt1_dVb = model->B3factor1 * (0.5 / T3 * T1 * dXdep_dVb + T3 * T2);

        T0 = pParam->B3dvt2w * Vbseff;
        if (T0 >= - 0.5) {
            T1 = 1.0 + T0;
            T2 = pParam->B3dvt2w;
        }
        else {
            // Added to avoid any discontinuity problems caused by dvt2w
            T4 = 1.0 / (3.0 + 8.0 * T0);
            T1 = (1.0 + 3.0 * T0) * T4; 
            T2 = pParam->B3dvt2w * T4 * T4;
        }
        ltw = model->B3factor1 * T3 * T1;
        dltw_dVb = model->B3factor1 * (0.5 / T3 * T1 * dXdep_dVb + T3 * T2);

        T0 = -0.5 * pParam->B3dvt1 * Leff / lt1;
        if (T0 > -EXP_THRESHOLD) {
            T1 = exp(T0);
            Theta0 = T1 * (1.0 + 2.0 * T1);
            dT1_dVb = -T0 / lt1 * T1 * dlt1_dVb;
            dTheta0_dVb = (1.0 + 4.0 * T1) * dT1_dVb;
        }
        else {
            T1 = MIN_EXP;
            Theta0 = T1 * (1.0 + 2.0 * T1);
            dTheta0_dVb = 0.0;
        }

        inst->B3thetavth = pParam->B3dvt0 * Theta0;
        Delt_vth = inst->B3thetavth * V0;
        dDelt_vth_dVb = pParam->B3dvt0 * dTheta0_dVb * V0;

        T0 = -0.5 * pParam->B3dvt1w * pParam->B3weff * Leff / ltw;
        if (T0 > -EXP_THRESHOLD) {
            T1 = exp(T0);
            T2 = T1 * (1.0 + 2.0 * T1);
            dT1_dVb = -T0 / ltw * T1 * dltw_dVb;
            dT2_dVb = (1.0 + 4.0 * T1) * dT1_dVb;
        }
        else {
            T1 = MIN_EXP;
            T2 = T1 * (1.0 + 2.0 * T1);
            dT2_dVb = 0.0;
        }

        T0 = pParam->B3dvt0w * T2;
        T2 = T0 * V0;
        dT2_dVb = pParam->B3dvt0w * dT2_dVb * V0;

        TempRatio =  ckt->CKTcurTask->TSKtemp / model->B3tnom - 1.0;
        T0 = sqrt(1.0 + pParam->B3nlx / Leff);
        T1 = pParam->B3k1ox * (T0 - 1.0) * pParam->B3sqrtPhi
            + (pParam->B3kt1 + pParam->B3kt1l / Leff
            + pParam->B3kt2 * Vbseff) * TempRatio;
        tmp2 = model->B3tox * pParam->B3phi
            / (pParam->B3weff + pParam->B3w0);

        T3 = pParam->B3eta0 + pParam->B3etab * Vbseff;
        if (T3 < 1.0e-4) {
            // avoid  discontinuity problems caused by etab
            T9 = 1.0 / (3.0 - 2.0e4 * T3);
            T3 = (2.0e-4 - T3) * T9;
            T4 = T9 * T9;
        }
        else {
            T4 = 1.0;
        }
        dDIBL_Sft_dVd = T3 * pParam->B3theta0vb0;
        DIBL_Sft = dDIBL_Sft_dVd * Vds;

        Vth = model->B3type * pParam->B3vth0 - pParam->B3k1
            * pParam->B3sqrtPhi + pParam->B3k1ox * sqrtPhis
            - pParam->B3k2ox * Vbseff - Delt_vth - T2 + (pParam->B3k3
            + pParam->B3k3b * Vbseff) * tmp2 + T1 - DIBL_Sft;

        inst->B3von = Vth; 

        dVth_dVb = pParam->B3k1ox * dsqrtPhis_dVb - pParam->B3k2ox
            - dDelt_vth_dVb - dT2_dVb + pParam->B3k3b * tmp2
            - pParam->B3etab * Vds * pParam->B3theta0vb0 * T4
            + pParam->B3kt2 * TempRatio;
        dVth_dVd = -dDIBL_Sft_dVd;

        // Calculate n

        tmp2 = pParam->B3nfactor * EPSSI / Xdep;
        tmp3 = pParam->B3cdsc + pParam->B3cdscb * Vbseff
            + pParam->B3cdscd * Vds;
        tmp4 = (tmp2 + tmp3 * Theta0 + pParam->B3cit) / model->B3cox;
        if (tmp4 >= -0.5) {
            n = 1.0 + tmp4;
            dn_dVb = (-tmp2 / Xdep * dXdep_dVb + tmp3 * dTheta0_dVb
                + pParam->B3cdscb * Theta0) / model->B3cox;
            dn_dVd = pParam->B3cdscd * Theta0 / model->B3cox;
        }
        else {
            // avoid  discontinuity problems caused by tmp4
            T0 = 1.0 / (3.0 + 8.0 * tmp4);
            n = (1.0 + 3.0 * tmp4) * T0;
            T0 *= T0;
            dn_dVb = (-tmp2 / Xdep * dXdep_dVb + tmp3 * dTheta0_dVb
                + pParam->B3cdscb * Theta0) / model->B3cox * T0;
            dn_dVd = pParam->B3cdscd * Theta0 / model->B3cox * T0;
        }

        // Poly Gate Si Depletion Effect

        T0 = pParam->B3vfb + pParam->B3phi;
        if ((pParam->B3ngate > 1.e18) && (pParam->B3ngate < 1.e25) 
                && (Vgs > T0)) {
            // added to avoid the problem caused by ngate
            T1 = 1.0e6 * Charge_q * EPSSI * pParam->B3ngate
                / (model->B3cox * model->B3cox);
            T4 = sqrt(1.0 + 2.0 * (Vgs - T0) / T1);
            T2 = T1 * (T4 - 1.0);
            T3 = 0.5 * T2 * T2 / T1; /* T3 = Vpoly */
            T7 = 1.12 - T3 - 0.05;
            T6 = sqrt(T7 * T7 + 0.224);
            T5 = 1.12 - 0.5 * (T7 + T6);
            Vgs_eff = Vgs - T5;
            dVgs_eff_dVg = 1.0 - (0.5 - 0.5 / T4) * (1.0 + T7 / T6); 
        }
        else {
            Vgs_eff = Vgs;
            dVgs_eff_dVg = 1.0;
        }
        Vgst = Vgs_eff - Vth;

        // Effective Vgst (Vgsteff) Calculation

        T10 = 2.0 * n * Vtm;
        VgstNVt = Vgst / T10;
        ExpArg = (2.0 * pParam->B3voff - Vgst) / T10;

        // MCJ: Very small Vgst
        if (VgstNVt > EXP_THRESHOLD) {
            Vgsteff = Vgst;
            dVgsteff_dVg = dVgs_eff_dVg;
            dVgsteff_dVd = -dVth_dVd;
            dVgsteff_dVb = -dVth_dVb;
        }
        else if (ExpArg > EXP_THRESHOLD) {
            T0 = (Vgst - pParam->B3voff) / (n * Vtm);
            ExpVgst = exp(T0);
            Vgsteff = Vtm * pParam->B3cdep0 / model->B3cox * ExpVgst;
            dVgsteff_dVg = Vgsteff / (n * Vtm);
            dVgsteff_dVd = -dVgsteff_dVg * (dVth_dVd + T0 * Vtm * dn_dVd);
            dVgsteff_dVb = -dVgsteff_dVg * (dVth_dVb + T0 * Vtm * dn_dVb);
            dVgsteff_dVg *= dVgs_eff_dVg;
        }
        else {
            ExpVgst = exp(VgstNVt);
            T1 = T10 * log(1.0 + ExpVgst);
            dT1_dVg = ExpVgst / (1.0 + ExpVgst);
            dT1_dVb = -dT1_dVg * (dVth_dVb + Vgst / n * dn_dVb)
                + T1 / n * dn_dVb; 
            dT1_dVd = -dT1_dVg * (dVth_dVd + Vgst / n * dn_dVd)
                + T1 / n * dn_dVd;

            dT2_dVg = -model->B3cox / (Vtm * pParam->B3cdep0) * exp(ExpArg);
            T2 = 1.0 - T10 * dT2_dVg;
            dT2_dVd = -dT2_dVg * (dVth_dVd - 2.0 * Vtm * ExpArg * dn_dVd)
                + (T2 - 1.0) / n * dn_dVd;
            dT2_dVb = -dT2_dVg * (dVth_dVb - 2.0 * Vtm * ExpArg * dn_dVb)
                + (T2 - 1.0) / n * dn_dVb;

            Vgsteff = T1 / T2;
            T3 = T2 * T2;
            dVgsteff_dVg = (T2 * dT1_dVg - T1 * dT2_dVg) / T3 * dVgs_eff_dVg;
            dVgsteff_dVd = (T2 * dT1_dVd - T1 * dT2_dVd) / T3;
            dVgsteff_dVb = (T2 * dT1_dVb - T1 * dT2_dVb) / T3;
        }

        // Calculate Effective Channel Geometry

        T9 = sqrtPhis - pParam->B3sqrtPhi;
        Weff = pParam->B3weff - 2.0 * (pParam->B3dwg * Vgsteff 
            + pParam->B3dwb * T9); 
        dWeff_dVg = -2.0 * pParam->B3dwg;
        dWeff_dVb = -2.0 * pParam->B3dwb * dsqrtPhis_dVb;

        if (Weff < 2.0e-8) {
            // to avoid the discontinuity problem due to Weff
            T0 = 1.0 / (6.0e-8 - 2.0 * Weff);
            Weff = 2.0e-8 * (4.0e-8 - Weff) * T0;
            T0 *= T0 * 4.0e-16;
            dWeff_dVg *= T0;
            dWeff_dVb *= T0;
        }

        T0 = pParam->B3prwg * Vgsteff + pParam->B3prwb * T9;
        if (T0 >= -0.9) {
            Rds = pParam->B3rds0 * (1.0 + T0);
            dRds_dVg = pParam->B3rds0 * pParam->B3prwg;
            dRds_dVb = pParam->B3rds0 * pParam->B3prwb * dsqrtPhis_dVb;
        }
        else {
            // to avoid the discontinuity problem due to prwg and prwb
            T1 = 1.0 / (17.0 + 20.0 * T0);
            Rds = pParam->B3rds0 * (0.8 + T0) * T1;
            T1 *= T1;
            dRds_dVg = pParam->B3rds0 * pParam->B3prwg * T1;
            dRds_dVb = pParam->B3rds0 * pParam->B3prwb * dsqrtPhis_dVb * T1;
        }
      
        // Calculate Abulk

        T1 = 0.5 * pParam->B3k1ox / sqrtPhis;
        dT1_dVb = -T1 / sqrtPhis * dsqrtPhis_dVb;

        T9 = sqrt(pParam->B3xj * Xdep);
        tmp1 = Leff + 2.0 * T9;
        T5 = Leff / tmp1; 
        tmp2 = pParam->B3a0 * T5;
        tmp3 = pParam->B3weff + pParam->B3b1; 
        tmp4 = pParam->B3b0 / tmp3;
        T2 = tmp2 + tmp4;
        dT2_dVb = -T9 / tmp1 / Xdep * dXdep_dVb;
        T6 = T5 * T5;
        T7 = T5 * T6;

        Abulk0 = 1.0 + T1 * T2; 
        dAbulk0_dVb = T1 * tmp2 * dT2_dVb + T2 * dT1_dVb;

        T8 = pParam->B3ags * pParam->B3a0 * T7;
        dAbulk_dVg = -T1 * T8;
        Abulk = Abulk0 + dAbulk_dVg * Vgsteff; 
        dAbulk_dVb = dAbulk0_dVb - T8 * Vgsteff * (dT1_dVb
            + 3.0 * T1 * dT2_dVb);

        if (Abulk0 < 0.1) {
            // added to avoid the problems caused by Abulk0
            T9 = 1.0 / (3.0 - 20.0 * Abulk0);
            Abulk0 = (0.2 - Abulk0) * T9;
            dAbulk0_dVb *= T9 * T9;
        }

        if (Abulk < 0.1) {
            // added to avoid the problems caused by Abulk
            T9 = 1.0 / (3.0 - 20.0 * Abulk);
            Abulk = (0.2 - Abulk) * T9;
            dAbulk_dVb *= T9 * T9;
        }

        T2 = pParam->B3keta * Vbseff;
        if (T2 >= -0.9) {
            T0 = 1.0 / (1.0 + T2);
            dT0_dVb = -pParam->B3keta * T0 * T0;
        }
        else {
            T1 = 1.0 / (0.8 + T2);
            // added to avoid the problems caused by Keta
            T0 = (17.0 + 20.0 * T2) * T1;
            dT0_dVb = -pParam->B3keta * T1 * T1;
        }
        dAbulk_dVg *= T0;
        dAbulk_dVb = dAbulk_dVb * T0 + Abulk * dT0_dVb;
        dAbulk0_dVb = dAbulk0_dVb * T0 + Abulk0 * dT0_dVb;
        Abulk *= T0;
        Abulk0 *= T0;

        // Mobility calculation

        if (model->B3mobMod == 1) {
            T0 = Vgsteff + Vth + Vth;
            T2 = pParam->B3ua + pParam->B3uc * Vbseff;
            T3 = T0 / model->B3tox;
            T5 = T3 * (T2 + pParam->B3ub * T3);
            dDenomi_dVg = (T2 + 2.0 * pParam->B3ub * T3) / model->B3tox;
            dDenomi_dVd = dDenomi_dVg * 2.0 * dVth_dVd;
            dDenomi_dVb = dDenomi_dVg * 2.0 * dVth_dVb + pParam->B3uc * T3;
        }
        else if (model->B3mobMod == 2) {
            T5 = Vgsteff / model->B3tox * (pParam->B3ua
                + pParam->B3uc * Vbseff + pParam->B3ub * Vgsteff
                / model->B3tox);
            dDenomi_dVg = (pParam->B3ua + pParam->B3uc * Vbseff
                + 2.0 * pParam->B3ub * Vgsteff / model->B3tox)
                / model->B3tox;
            dDenomi_dVd = 0.0;
            dDenomi_dVb = Vgsteff * pParam->B3uc / model->B3tox; 
        }
        else {
            T0 = Vgsteff + Vth + Vth;
            T2 = 1.0 + pParam->B3uc * Vbseff;
            T3 = T0 / model->B3tox;
            T4 = T3 * (pParam->B3ua + pParam->B3ub * T3);
            T5 = T4 * T2;
            dDenomi_dVg = (pParam->B3ua + 2.0 * pParam->B3ub * T3) * T2
                / model->B3tox;
            dDenomi_dVd = dDenomi_dVg * 2.0 * dVth_dVd;
            dDenomi_dVb = dDenomi_dVg * 2.0 * dVth_dVb + pParam->B3uc * T4;
        }

        if (T5 >= -0.8) {
            Denomi = 1.0 + T5;
        }
        else {
            // Added to avoid the discontinuity problem caused by ua and ub
            T9 = 1.0 / (7.0 + 10.0 * T5);
            Denomi = (0.6 + T5) * T9;
            T9 *= T9;
            dDenomi_dVg *= T9;
            dDenomi_dVd *= T9;
            dDenomi_dVb *= T9;
        }

        inst->B3ueff = ueff = pParam->B3u0temp / Denomi;
        T9 = -ueff / Denomi;
        dueff_dVg = T9 * dDenomi_dVg;
        dueff_dVd = T9 * dDenomi_dVd;
        dueff_dVb = T9 * dDenomi_dVb;

        // Saturation Drain Voltage  Vdsat

        WVCox = Weff * pParam->B3vsattemp * model->B3cox;
        WVCoxRds = WVCox * Rds; 

        Esat = 2.0 * pParam->B3vsattemp / ueff;
        EsatL = Esat * Leff;
        T0 = -EsatL /ueff;
        dEsatL_dVg = T0 * dueff_dVg;
        dEsatL_dVd = T0 * dueff_dVd;
        dEsatL_dVb = T0 * dueff_dVb;
  
        // Sqrt()
        a1 = pParam->B3a1;
        if (a1 == 0.0) {
            Lambda = pParam->B3a2;
            dLambda_dVg = 0.0;
        }
        else if (a1 > 0.0) {
            // Added to avoid the discontinuity problem caused by
            // a1 and a2 (Lambda)
            T0 = 1.0 - pParam->B3a2;
            T1 = T0 - pParam->B3a1 * Vgsteff - 0.0001;
            T2 = sqrt(T1 * T1 + 0.0004 * T0);
            Lambda = pParam->B3a2 + T0 - 0.5 * (T1 + T2);
            dLambda_dVg = 0.5 * pParam->B3a1 * (1.0 + T1 / T2);
        }
        else {
            T1 = pParam->B3a2 + pParam->B3a1 * Vgsteff - 0.0001;
            T2 = sqrt(T1 * T1 + 0.0004 * pParam->B3a2);
            Lambda = 0.5 * (T1 + T2);
            dLambda_dVg = 0.5 * pParam->B3a1 * (1.0 + T1 / T2);
        }

        Vgst2Vtm = Vgsteff + 2.0 * Vtm;
        if (Rds > 0) {
            tmp2 = dRds_dVg / Rds + dWeff_dVg / Weff;
            tmp3 = dRds_dVb / Rds + dWeff_dVb / Weff;
        }
        else {
            tmp2 = dWeff_dVg / Weff;
            tmp3 = dWeff_dVb / Weff;
        }
        if ((Rds == 0.0) && (Lambda == 1.0)) {
            T0 = 1.0 / (Abulk * EsatL + Vgst2Vtm);
            tmp1 = 0.0;
            T1 = T0 * T0;
            T2 = Vgst2Vtm * T0;
            T3 = EsatL * Vgst2Vtm;
            Vdsat = T3 * T0;
                           
            dT0_dVg = -(Abulk * dEsatL_dVg + EsatL * dAbulk_dVg + 1.0) * T1;
            dT0_dVd = -(Abulk * dEsatL_dVd) * T1; 
            dT0_dVb = -(Abulk * dEsatL_dVb + dAbulk_dVb * EsatL) * T1;   

            dVdsat_dVg = T3 * dT0_dVg + T2 * dEsatL_dVg + EsatL * T0;
            dVdsat_dVd = T3 * dT0_dVd + T2 * dEsatL_dVd;
            dVdsat_dVb = T3 * dT0_dVb + T2 * dEsatL_dVb;   
        }
        else {
            tmp1 = dLambda_dVg / (Lambda * Lambda);
            T9 = Abulk * WVCoxRds;
            T8 = Abulk * T9;
            T7 = Vgst2Vtm * T9;
            T6 = Vgst2Vtm * WVCoxRds;
            T0 = 2.0 * Abulk * (T9 - 1.0 + 1.0 / Lambda); 
            dT0_dVg = 2.0 * (T8 * tmp2 - Abulk * tmp1
                + (2.0 * T9 + 1.0 / Lambda - 1.0) * dAbulk_dVg);
             
            dT0_dVb = 2.0 * (T8 * (2.0 / Abulk * dAbulk_dVb + tmp3)
                + (1.0 / Lambda - 1.0) * dAbulk_dVb);
            dT0_dVd = 0.0; 
            T1 = Vgst2Vtm * (2.0 / Lambda - 1.0) + Abulk * EsatL + 3.0 * T7;
             
            dT1_dVg = (2.0 / Lambda - 1.0) - 2.0 * Vgst2Vtm * tmp1
                + Abulk * dEsatL_dVg + EsatL * dAbulk_dVg + 3.0 * (T9
                + T7 * tmp2 + T6 * dAbulk_dVg);
            dT1_dVb = Abulk * dEsatL_dVb + EsatL * dAbulk_dVb
                + 3.0 * (T6 * dAbulk_dVb + T7 * tmp3);
            dT1_dVd = Abulk * dEsatL_dVd;

            T2 = Vgst2Vtm * (EsatL + 2.0 * T6);
            dT2_dVg = EsatL + Vgst2Vtm * dEsatL_dVg
                + T6 * (4.0 + 2.0 * Vgst2Vtm * tmp2);
            dT2_dVb = Vgst2Vtm * (dEsatL_dVb + 2.0 * T6 * tmp3);
            dT2_dVd = Vgst2Vtm * dEsatL_dVd;

            T3 = sqrt(T1 * T1 - 2.0 * T0 * T2);
            Vdsat = (T1 - T3) / T0;

            dT3_dVg = (T1 * dT1_dVg - 2.0 * (T0 * dT2_dVg + T2 * dT0_dVg))
                / T3;
            dT3_dVd = (T1 * dT1_dVd - 2.0 * (T0 * dT2_dVd + T2 * dT0_dVd))
                / T3;
            dT3_dVb = (T1 * dT1_dVb - 2.0 * (T0 * dT2_dVb + T2 * dT0_dVb))
                / T3;

            dVdsat_dVg = (dT1_dVg - (T1 * dT1_dVg - dT0_dVg * T2
                - T0 * dT2_dVg) / T3 - Vdsat * dT0_dVg) / T0;
            dVdsat_dVb = (dT1_dVb - (T1 * dT1_dVb - dT0_dVb * T2
                - T0 * dT2_dVb) / T3 - Vdsat * dT0_dVb) / T0;
            dVdsat_dVd = (dT1_dVd - (T1 * dT1_dVd - T0 * dT2_dVd) / T3) / T0;
        }
        inst->B3vdsat = Vdsat;

        // Effective Vds (Vdseff) Calculation

        T1 = Vdsat - Vds - pParam->B3delta;
        dT1_dVg = dVdsat_dVg;
        dT1_dVd = dVdsat_dVd - 1.0;
        dT1_dVb = dVdsat_dVb;

        T2 = sqrt(T1 * T1 + 4.0 * pParam->B3delta * Vdsat);
        T0 = T1 / T2;
        T3 = 2.0 * pParam->B3delta / T2;
        dT2_dVg = T0 * dT1_dVg + T3 * dVdsat_dVg;
        dT2_dVd = T0 * dT1_dVd + T3 * dVdsat_dVd;
        dT2_dVb = T0 * dT1_dVb + T3 * dVdsat_dVb;

        Vdseff = Vdsat - 0.5 * (T1 + T2);
        dVdseff_dVg = dVdsat_dVg - 0.5 * (dT1_dVg + dT2_dVg); 
        dVdseff_dVd = dVdsat_dVd - 0.5 * (dT1_dVd + dT2_dVd); 
        dVdseff_dVb = dVdsat_dVb - 0.5 * (dT1_dVb + dT2_dVb); 

        // Calculate VAsat

        tmp4 = 1.0 - 0.5 * Abulk * Vdsat / Vgst2Vtm;
        T9 = WVCoxRds * Vgsteff;
        T8 = T9 / Vgst2Vtm;
        T0 = EsatL + Vdsat + 2.0 * T9 * tmp4;
         
        T7 = 2.0 * WVCoxRds * tmp4;
        dT0_dVg = dEsatL_dVg + dVdsat_dVg + T7 * (1.0 + tmp2 * Vgsteff)
            - T8 * (Abulk * dVdsat_dVg - Abulk * Vdsat / Vgst2Vtm
            + Vdsat * dAbulk_dVg);   
          
        dT0_dVb = dEsatL_dVb + dVdsat_dVb + T7 * tmp3 * Vgsteff
            - T8 * (dAbulk_dVb * Vdsat + Abulk * dVdsat_dVb);
        dT0_dVd = dEsatL_dVd + dVdsat_dVd - T8 * Abulk * dVdsat_dVd;

        T9 = WVCoxRds * Abulk; 
        T1 = 2.0 / Lambda - 1.0 + T9; 
        dT1_dVg = -2.0 * tmp1 +  WVCoxRds * (Abulk * tmp2 + dAbulk_dVg);
        dT1_dVb = dAbulk_dVb * WVCoxRds + T9 * tmp3;

        Vasat = T0 / T1;
        dVasat_dVg = (dT0_dVg - Vasat * dT1_dVg) / T1;
        dVasat_dVb = (dT0_dVb - Vasat * dT1_dVb) / T1;
        dVasat_dVd = dT0_dVd / T1;

        if (Vdseff > Vds)
            Vdseff = Vds;
            // This code is added to fixed the problem
            //     caused by computer precision when
            //     Vds is very close to Vdseff.
        diffVds = Vds - Vdseff;

        // Calculate VACLM

        if ((pParam->B3pclm > 0.0) && (diffVds > 1.0e-10)) {
            T0 = 1.0 / (pParam->B3pclm * Abulk * pParam->B3litl);
            dT0_dVb = -T0 / Abulk * dAbulk_dVb;
            dT0_dVg = -T0 / Abulk * dAbulk_dVg; 
              
            T2 = Vgsteff / EsatL;
            T1 = Leff * (Abulk + T2); 
            dT1_dVg = Leff * ((1.0 - T2 * dEsatL_dVg) / EsatL + dAbulk_dVg);
            dT1_dVb = Leff * (dAbulk_dVb - T2 * dEsatL_dVb / EsatL);
            dT1_dVd = -T2 * dEsatL_dVd / Esat;

            T9 = T0 * T1;
            VACLM = T9 * diffVds;
            dVACLM_dVg = T0 * dT1_dVg * diffVds - T9 * dVdseff_dVg
                + T1 * diffVds * dT0_dVg;
            dVACLM_dVb = (dT0_dVb * T1 + T0 * dT1_dVb) * diffVds
                - T9 * dVdseff_dVb;
            dVACLM_dVd = T0 * dT1_dVd * diffVds + T9 * (1.0 - dVdseff_dVd);
        }
        else {
            VACLM = MAX_EXP;
            dVACLM_dVd = dVACLM_dVg = dVACLM_dVb = 0.0;
        }

        // Calculate VADIBL

        if (pParam->B3thetaRout > 0.0) {
            T8 = Abulk * Vdsat;
            T0 = Vgst2Vtm * T8;
            dT0_dVg = Vgst2Vtm * Abulk * dVdsat_dVg + T8
                + Vgst2Vtm * Vdsat * dAbulk_dVg;
            dT0_dVb = Vgst2Vtm * (dAbulk_dVb * Vdsat + Abulk * dVdsat_dVb);
            dT0_dVd = Vgst2Vtm * Abulk * dVdsat_dVd;

            T1 = Vgst2Vtm + T8;
            dT1_dVg = 1.0 + Abulk * dVdsat_dVg + Vdsat * dAbulk_dVg;
            dT1_dVb = Abulk * dVdsat_dVb + dAbulk_dVb * Vdsat;
            dT1_dVd = Abulk * dVdsat_dVd;

            T9 = T1 * T1;
            T2 = pParam->B3thetaRout;
            VADIBL = (Vgst2Vtm - T0 / T1) / T2;
            dVADIBL_dVg = (1.0 - dT0_dVg / T1 + T0 * dT1_dVg / T9) / T2;
            dVADIBL_dVb = (-dT0_dVb / T1 + T0 * dT1_dVb / T9) / T2;
            dVADIBL_dVd = (-dT0_dVd / T1 + T0 * dT1_dVd / T9) / T2;

            T7 = pParam->B3pdiblb * Vbseff;
            if (T7 >= -0.9) {
                T3 = 1.0 / (1.0 + T7);
                VADIBL *= T3;
                dVADIBL_dVg *= T3;
                dVADIBL_dVb = (dVADIBL_dVb - VADIBL * pParam->B3pdiblb) * T3;
                dVADIBL_dVd *= T3;
            }
            else {
                // Added to avoid the discontinuity problem caused by pdiblcb
                T4 = 1.0 / (0.8 + T7);
                T3 = (17.0 + 20.0 * T7) * T4;
                dVADIBL_dVg *= T3;
                dVADIBL_dVb = dVADIBL_dVb * T3
                    - VADIBL * pParam->B3pdiblb * T4 * T4;
                dVADIBL_dVd *= T3;
                VADIBL *= T3;
            }
        }
        else {
            VADIBL = MAX_EXP;
            dVADIBL_dVd = dVADIBL_dVg = dVADIBL_dVb = 0.0;
        }

        // Calculate VA
          
        T8 = pParam->B3pvag / EsatL;
        T9 = T8 * Vgsteff;
        if (T9 > -0.9) {
            T0 = 1.0 + T9;
            dT0_dVg = T8 * (1.0 - Vgsteff * dEsatL_dVg / EsatL);
            dT0_dVb = -T9 * dEsatL_dVb / EsatL;
            dT0_dVd = -T9 * dEsatL_dVd / EsatL;
        }
        else {
            // Added to avoid the discontinuity problems caused by pvag
            T1 = 1.0 / (17.0 + 20.0 * T9);
            T0 = (0.8 + T9) * T1;
            T1 *= T1;
            dT0_dVg = T8 * (1.0 - Vgsteff * dEsatL_dVg / EsatL) * T1;

            T9 *= T1 / EsatL;
            dT0_dVb = -T9 * dEsatL_dVb;
            dT0_dVd = -T9 * dEsatL_dVd;
        }
        
        tmp1 = VACLM * VACLM;
        tmp2 = VADIBL * VADIBL;
        tmp3 = VACLM + VADIBL;

        T1 = VACLM * VADIBL / tmp3;
        tmp3 *= tmp3;
        dT1_dVg = (tmp1 * dVADIBL_dVg + tmp2 * dVACLM_dVg) / tmp3;
        dT1_dVd = (tmp1 * dVADIBL_dVd + tmp2 * dVACLM_dVd) / tmp3;
        dT1_dVb = (tmp1 * dVADIBL_dVb + tmp2 * dVACLM_dVb) / tmp3;

        Va = Vasat + T0 * T1;
        dVa_dVg = dVasat_dVg + T1 * dT0_dVg + T0 * dT1_dVg;
        dVa_dVd = dVasat_dVd + T1 * dT0_dVd + T0 * dT1_dVd;
        dVa_dVb = dVasat_dVb + T1 * dT0_dVb + T0 * dT1_dVb;

        // Calculate VASCBE

        if (pParam->B3pscbe2 > 0.0) {
            if (diffVds > pParam->B3pscbe1 * pParam->B3litl / EXP_THRESHOLD) {
                T0 =  pParam->B3pscbe1 * pParam->B3litl / diffVds;
                VASCBE = Leff * exp(T0) / pParam->B3pscbe2;
                T1 = T0 * VASCBE / diffVds;
                dVASCBE_dVg = T1 * dVdseff_dVg;
                dVASCBE_dVd = -T1 * (1.0 - dVdseff_dVd);
                dVASCBE_dVb = T1 * dVdseff_dVb;
            }
            else {
                VASCBE = MAX_EXP * Leff/pParam->B3pscbe2;
                dVASCBE_dVg = dVASCBE_dVd = dVASCBE_dVb = 0.0;
            }
        }
        else {
            VASCBE = MAX_EXP;
            dVASCBE_dVg = dVASCBE_dVd = dVASCBE_dVb = 0.0;
        }

        // Calculate Ids

        CoxWovL = model->B3cox * Weff / Leff;
        beta = ueff * CoxWovL;
        dbeta_dVg = CoxWovL * dueff_dVg + beta * dWeff_dVg / Weff;
        dbeta_dVd = CoxWovL * dueff_dVd;
        dbeta_dVb = CoxWovL * dueff_dVb + beta * dWeff_dVb / Weff;

        T0 = 1.0 - 0.5 * Abulk * Vdseff / Vgst2Vtm;
        dT0_dVg = -0.5 * (Abulk * dVdseff_dVg 
            - Abulk * Vdseff / Vgst2Vtm + Vdseff * dAbulk_dVg) / Vgst2Vtm;
        dT0_dVd = -0.5 * Abulk * dVdseff_dVd / Vgst2Vtm;
        dT0_dVb = -0.5 * (Abulk * dVdseff_dVb + dAbulk_dVb * Vdseff)
            / Vgst2Vtm;

        fgche1 = Vgsteff * T0;
        dfgche1_dVg = Vgsteff * dT0_dVg + T0; 
        dfgche1_dVd = Vgsteff * dT0_dVd; 
        dfgche1_dVb = Vgsteff * dT0_dVb; 

        T9 = Vdseff / EsatL;
        fgche2 = 1.0 + T9;
        dfgche2_dVg = (dVdseff_dVg - T9 * dEsatL_dVg) / EsatL;
        dfgche2_dVd = (dVdseff_dVd - T9 * dEsatL_dVd) / EsatL;
        dfgche2_dVb = (dVdseff_dVb - T9 * dEsatL_dVb) / EsatL;
 
        gche = beta * fgche1 / fgche2;
        dgche_dVg = (beta * dfgche1_dVg + fgche1 * dbeta_dVg
            - gche * dfgche2_dVg) / fgche2;
        dgche_dVd = (beta * dfgche1_dVd + fgche1 * dbeta_dVd
            - gche * dfgche2_dVd) / fgche2;
        dgche_dVb = (beta * dfgche1_dVb + fgche1 * dbeta_dVb
            - gche * dfgche2_dVb) / fgche2;

        T0 = 1.0 + gche * Rds;
        T9 = Vdseff / T0;
        Idl = gche * T9;

        dIdl_dVg = (gche * dVdseff_dVg + T9 * dgche_dVg) / T0
            - Idl * gche / T0 * dRds_dVg ; 

        dIdl_dVd = (gche * dVdseff_dVd + T9 * dgche_dVd) / T0; 
        dIdl_dVb = (gche * dVdseff_dVb + T9 * dgche_dVb 
            - Idl * dRds_dVb * gche) / T0; 

        T9 =  diffVds / Va;
        T0 =  1.0 + T9;
        Idsa = Idl * T0;
        dIdsa_dVg = T0 * dIdl_dVg - Idl * (dVdseff_dVg + T9 * dVa_dVg) / Va;
        dIdsa_dVd = T0 * dIdl_dVd + Idl * (1.0 - dVdseff_dVd
            - T9 * dVa_dVd) / Va;
        dIdsa_dVb = T0 * dIdl_dVb - Idl * (dVdseff_dVb + T9 * dVa_dVb) / Va;

        T9 = diffVds / VASCBE;
        T0 = 1.0 + T9;
        Ids = Idsa * T0;

        Gm = T0 * dIdsa_dVg - Idsa * (dVdseff_dVg + T9 * dVASCBE_dVg) / VASCBE;
        Gds = T0 * dIdsa_dVd + Idsa * (1.0 - dVdseff_dVd
            - T9 * dVASCBE_dVd) / VASCBE;
        Gmb = T0 * dIdsa_dVb - Idsa * (dVdseff_dVb
            + T9 * dVASCBE_dVb) / VASCBE;

        Gds += Gm * dVgsteff_dVd;
        Gmb += Gm * dVgsteff_dVb;
        Gm *= dVgsteff_dVg;
        Gmb *= dVbseff_dVb;

        // Substrate current begins
        tmp = pParam->B3alpha0 + pParam->B3alpha1 * Leff;
        if ((tmp <= 0.0) || (pParam->B3beta0 <= 0.0)) {
            Isub = Gbd = Gbb = Gbg = 0.0;
        }
        else {
            T2 = tmp / Leff;
            if (diffVds > pParam->B3beta0 / EXP_THRESHOLD) {
                T0 = -pParam->B3beta0 / diffVds;
                T1 = T2 * diffVds * exp(T0);
                T3 = T1 / diffVds * (T0 - 1.0);
                dT1_dVg = T3 * dVdseff_dVg;
                dT1_dVd = T3 * (dVdseff_dVd - 1.0);
                dT1_dVb = T3 * dVdseff_dVb;
            }
            else {
                T3 = T2 * MIN_EXP;
                T1 = T3 * diffVds;
                dT1_dVg = -T3 * dVdseff_dVg;
                dT1_dVd = T3 * (1.0 - dVdseff_dVd);
                dT1_dVb = -T3 * dVdseff_dVb;
            }
            Isub = T1 * Idsa;
            Gbg = T1 * dIdsa_dVg + Idsa * dT1_dVg;
            Gbd = T1 * dIdsa_dVd + Idsa * dT1_dVd;
            Gbb = T1 * dIdsa_dVb + Idsa * dT1_dVb;

            Gbd += Gbg * dVgsteff_dVd;
            Gbb += Gbg * dVgsteff_dVb;
            Gbg *= dVgsteff_dVg;
            Gbb *= dVbseff_dVb; /* bug fixing */
        }
         
        cdrain = Ids;
        inst->B3gds = Gds;
        inst->B3gm = Gm;
        inst->B3gmbs = Gmb;
                   
        inst->B3gbbs = Gbb;
        inst->B3gbgs = Gbg;
        inst->B3gbds = Gbd;

        inst->B3csub = Isub;

        // B3 thermal noise Qinv calculated from all capMod 
        //   * 0, 1, 2 & 3 stored in inst->B3qinv 1/1998

        if ((model->B3xpart < 0) || (!ChargeComputationNeeded)) {
            qgate  = qdrn = qsrc = qbulk = 0.0;
            inst->B3cggb = inst->B3cgsb = inst->B3cgdb = 0.0;
            inst->B3cdgb = inst->B3cdsb = inst->B3cddb = 0.0;
            inst->B3cbgb = inst->B3cbsb = inst->B3cbdb = 0.0;
            inst->B3cqdb = inst->B3cqsb = inst->B3cqgb = inst->B3cqbb = 0.0;
            inst->B3gtau = 0.0;
            goto finished;
        }
        else if (model->B3capMod == 0) {
            if (Vbseff < 0.0) {
                Vbseff = Vbs;
                dVbseff_dVb = 1.0;
            }
            else {
                Vbseff = pParam->B3phi - Phis;
                dVbseff_dVb = -dPhis_dVb;
            }

            Vfb = pParam->B3vfbcv;
            Vth = Vfb + pParam->B3phi + pParam->B3k1ox * sqrtPhis; 
            Vgst = Vgs_eff - Vth;
            dVth_dVb = pParam->B3k1ox * dsqrtPhis_dVb; 
//            dVgst_dVb = -dVth_dVb;
//            dVgst_dVg = dVgs_eff_dVg; 

            CoxWL = model->B3cox * pParam->B3weffCV * pParam->B3leffCV;
            Arg1 = Vgs_eff - Vbseff - Vfb;

            if (Arg1 <= 0.0) {
                qgate = CoxWL * Arg1;
                qbulk = -qgate;
                qdrn = 0.0;

                inst->B3cggb = CoxWL * dVgs_eff_dVg;
                inst->B3cgdb = 0.0;
                inst->B3cgsb = CoxWL * (dVbseff_dVb - dVgs_eff_dVg);

                inst->B3cdgb = 0.0;
                inst->B3cddb = 0.0;
                inst->B3cdsb = 0.0;

                inst->B3cbgb = -CoxWL * dVgs_eff_dVg;
                inst->B3cbdb = 0.0;
                inst->B3cbsb = -inst->B3cgsb;
                inst->B3qinv = 0.0;
            }
            else if (Vgst <= 0.0) {
                T1 = 0.5 * pParam->B3k1ox;
                T2 = sqrt(T1 * T1 + Arg1);
                qgate = CoxWL * pParam->B3k1ox * (T2 - T1);
                qbulk = -qgate;
                qdrn = 0.0;

                T0 = CoxWL * T1 / T2;
                inst->B3cggb = T0 * dVgs_eff_dVg;
                inst->B3cgdb = 0.0;
                inst->B3cgsb = T0 * (dVbseff_dVb - dVgs_eff_dVg);
   
                inst->B3cdgb = 0.0;
                inst->B3cddb = 0.0;
                inst->B3cdsb = 0.0;

                inst->B3cbgb = -inst->B3cggb;
                inst->B3cbdb = 0.0;
                inst->B3cbsb = -inst->B3cgsb;
                inst->B3qinv = 0.0;
            }
            else {
                One_Third_CoxWL = CoxWL / 3.0;
                Two_Third_CoxWL = 2.0 * One_Third_CoxWL;

                AbulkCV = Abulk0 * pParam->B3abulkCVfactor;
                dAbulkCV_dVb = pParam->B3abulkCVfactor * dAbulk0_dVb;
                Vdsat = Vgst / AbulkCV;
                dVdsat_dVg = dVgs_eff_dVg / AbulkCV;
                dVdsat_dVb = - (Vdsat * dAbulkCV_dVb + dVth_dVb)/ AbulkCV; 

                if (model->B3xpart > 0.5) {
                    // 0/100 Charge partition model
                    if (Vdsat <= Vds) {
                        // saturation region
                        T1 = Vdsat / 3.0;
                        qgate = CoxWL * (Vgs_eff - Vfb
                            - pParam->B3phi - T1);
                        T2 = -Two_Third_CoxWL * Vgst;
                        qbulk = -(qgate + T2);
                        qdrn = 0.0;

                        inst->B3cggb = One_Third_CoxWL * (3.0
                            - dVdsat_dVg) * dVgs_eff_dVg;
                        T2 = -One_Third_CoxWL * dVdsat_dVb;
                        inst->B3cgsb = -(inst->B3cggb + T2);
                        inst->B3cgdb = 0.0;
       
                        inst->B3cdgb = 0.0;
                        inst->B3cddb = 0.0;
                        inst->B3cdsb = 0.0;

                        inst->B3cbgb = -(inst->B3cggb
                            - Two_Third_CoxWL * dVgs_eff_dVg);
                        T3 = -(T2 + Two_Third_CoxWL * dVth_dVb);
                        inst->B3cbsb = -(inst->B3cbgb + T3);
                        inst->B3cbdb = 0.0;
                        inst->B3qinv = -(qgate + qbulk);
                    }
                    else {
                        // linear region
                        Alphaz = Vgst / Vdsat;
                        T1 = 2.0 * Vdsat - Vds;
                        T2 = Vds / (3.0 * T1);
                        T3 = T2 * Vds;
                        T9 = 0.25 * CoxWL;
                        T4 = T9 * Alphaz;
                        T7 = 2.0 * Vds - T1 - 3.0 * T3;
                        T8 = T3 - T1 - 2.0 * Vds;
                        qgate = CoxWL * (Vgs_eff - Vfb 
                            - pParam->B3phi - 0.5 * (Vds - T3));
                        T10 = T4 * T8;
                        qdrn = T4 * T7;
                        qbulk = -(qgate + qdrn + T10);
  
                        T5 = T3 / T1;
                        inst->B3cggb = CoxWL * (1.0 - T5 * dVdsat_dVg)
                            * dVgs_eff_dVg;
                        T11 = -CoxWL * T5 * dVdsat_dVb;
                        inst->B3cgdb = CoxWL * (T2 - 0.5 + 0.5 * T5);
                        inst->B3cgsb = -(inst->B3cggb + T11
                            + inst->B3cgdb);
                        T6 = 1.0 / Vdsat;
                        dAlphaz_dVg = T6 * (1.0 - Alphaz * dVdsat_dVg);
                        dAlphaz_dVb = -T6 * (dVth_dVb + Alphaz * dVdsat_dVb);
                        T7 = T9 * T7;
                        T8 = T9 * T8;
                        T9 = 2.0 * T4 * (1.0 - 3.0 * T5);
                        inst->B3cdgb = (T7 * dAlphaz_dVg - T9
                            * dVdsat_dVg) * dVgs_eff_dVg;
                        T12 = T7 * dAlphaz_dVb - T9 * dVdsat_dVb;
                        inst->B3cddb = T4 * (3.0 - 6.0 * T2 - 3.0 * T5);
                        inst->B3cdsb = -(inst->B3cdgb + T12
                            + inst->B3cddb);

                        T9 = 2.0 * T4 * (1.0 + T5);
                        T10 = (T8 * dAlphaz_dVg - T9 * dVdsat_dVg)
                            * dVgs_eff_dVg;
                        T11 = T8 * dAlphaz_dVb - T9 * dVdsat_dVb;
                        T12 = T4 * (2.0 * T2 + T5 - 1.0); 
                        T0 = -(T10 + T11 + T12);

                        inst->B3cbgb = -(inst->B3cggb
                            + inst->B3cdgb + T10);
                        inst->B3cbdb = -(inst->B3cgdb 
                            + inst->B3cddb + T12);
                        inst->B3cbsb = -(inst->B3cgsb
                            + inst->B3cdsb + T0);
                        inst->B3qinv = -(qgate + qbulk);
                    }
                }
                else if (model->B3xpart < 0.5) {
                    // 40/60 Charge partition model
                    if (Vds >= Vdsat) {
                        // saturation region
                        T1 = Vdsat / 3.0;
                        qgate = CoxWL * (Vgs_eff - Vfb
                            - pParam->B3phi - T1);
                        T2 = -Two_Third_CoxWL * Vgst;
                        qbulk = -(qgate + T2);
                        qdrn = 0.4 * T2;

                        inst->B3cggb = One_Third_CoxWL * (3.0 
                            - dVdsat_dVg) * dVgs_eff_dVg;
                        T2 = -One_Third_CoxWL * dVdsat_dVb;
                        inst->B3cgsb = -(inst->B3cggb + T2);
                        inst->B3cgdb = 0.0;
       
                        T3 = 0.4 * Two_Third_CoxWL;
                        inst->B3cdgb = -T3 * dVgs_eff_dVg;
                        inst->B3cddb = 0.0;
                        T4 = T3 * dVth_dVb;
                        inst->B3cdsb = -(T4 + inst->B3cdgb);

                        inst->B3cbgb = -(inst->B3cggb 
                            - Two_Third_CoxWL * dVgs_eff_dVg);
                        T3 = -(T2 + Two_Third_CoxWL * dVth_dVb);
                        inst->B3cbsb = -(inst->B3cbgb + T3);
                        inst->B3cbdb = 0.0;
                        inst->B3qinv = -(qgate + qbulk);
                    }
                    else {
                        // linear region
                        Alphaz = Vgst / Vdsat;
                        T1 = 2.0 * Vdsat - Vds;
                        T2 = Vds / (3.0 * T1);
                        T3 = T2 * Vds;
                        T9 = 0.25 * CoxWL;
                        T4 = T9 * Alphaz;
                        qgate = CoxWL * (Vgs_eff - Vfb - pParam->B3phi
                            - 0.5 * (Vds - T3));

                        T5 = T3 / T1;
                        inst->B3cggb = CoxWL * (1.0 - T5 * dVdsat_dVg)
                            * dVgs_eff_dVg;
                        tmp = -CoxWL * T5 * dVdsat_dVb;
                        inst->B3cgdb = CoxWL * (T2 - 0.5 + 0.5 * T5);
                        inst->B3cgsb = -(inst->B3cggb 
                            + inst->B3cgdb + tmp);

                        T6 = 1.0 / Vdsat;
                        dAlphaz_dVg = T6 * (1.0 - Alphaz * dVdsat_dVg);
                        dAlphaz_dVb = -T6 * (dVth_dVb + Alphaz * dVdsat_dVb);

                        T6 = 8.0 * Vdsat * Vdsat - 6.0 * Vdsat * Vds
                            + 1.2 * Vds * Vds;
                        T8 = T2 / T1;
                        T7 = Vds - T1 - T8 * T6;
                        qdrn = T4 * T7;
                        T7 *= T9;
                        tmp = T8 / T1;
                        tmp1 = T4 * (2.0 - 4.0 * tmp * T6
                            + T8 * (16.0 * Vdsat - 6.0 * Vds));

                        inst->B3cdgb = (T7 * dAlphaz_dVg - tmp1
                            * dVdsat_dVg) * dVgs_eff_dVg;
                        T10 = T7 * dAlphaz_dVb - tmp1 * dVdsat_dVb;
                        inst->B3cddb = T4 * (2.0 - (1.0 / (3.0 * T1
                            * T1) + 2.0 * tmp) * T6 + T8
                            * (6.0 * Vdsat - 2.4 * Vds));
                        inst->B3cdsb = -(inst->B3cdgb 
                            + T10 + inst->B3cddb);

                        T7 = 2.0 * (T1 + T3);
                        qbulk = -(qgate - T4 * T7);
                        T7 *= T9;
                        T0 = 4.0 * T4 * (1.0 - T5);
                        T12 = (-T7 * dAlphaz_dVg - inst->B3cdgb
                            - T0 * dVdsat_dVg) * dVgs_eff_dVg;
                        T11 = -T7 * dAlphaz_dVb - T10 - T0 * dVdsat_dVb;
                        T10 = -4.0 * T4 * (T2 - 0.5 + 0.5 * T5) 
                            - inst->B3cddb;
                        tmp = -(T10 + T11 + T12);

                        inst->B3cbgb = -(inst->B3cggb 
                            + inst->B3cdgb + T12);
                        inst->B3cbdb = -(inst->B3cgdb
                            + inst->B3cddb + T11);
                        inst->B3cbsb = -(inst->B3cgsb
                            + inst->B3cdsb + tmp);
                        inst->B3qinv = -(qgate + qbulk);
                    }
                }
                else {
                    // 50/50 partitioning
                    if (Vds >= Vdsat) {
                        // saturation region
                        T1 = Vdsat / 3.0;
                        qgate = CoxWL * (Vgs_eff - Vfb
                            - pParam->B3phi - T1);
                        T2 = -Two_Third_CoxWL * Vgst;
                        qbulk = -(qgate + T2);
                        qdrn = 0.5 * T2;

                        inst->B3cggb = One_Third_CoxWL * (3.0
                            - dVdsat_dVg) * dVgs_eff_dVg;
                        T2 = -One_Third_CoxWL * dVdsat_dVb;
                        inst->B3cgsb = -(inst->B3cggb + T2);
                        inst->B3cgdb = 0.0;
       
                        inst->B3cdgb = -One_Third_CoxWL * dVgs_eff_dVg;
                        inst->B3cddb = 0.0;
                        T4 = One_Third_CoxWL * dVth_dVb;
                        inst->B3cdsb = -(T4 + inst->B3cdgb);

                        inst->B3cbgb = -(inst->B3cggb 
                            - Two_Third_CoxWL * dVgs_eff_dVg);
                        T3 = -(T2 + Two_Third_CoxWL * dVth_dVb);
                        inst->B3cbsb = -(inst->B3cbgb + T3);
                        inst->B3cbdb = 0.0;
                        inst->B3qinv = -(qgate + qbulk);
                    }
                    else {
                        // linear region
                        Alphaz = Vgst / Vdsat;
                        T1 = 2.0 * Vdsat - Vds;
                        T2 = Vds / (3.0 * T1);
                        T3 = T2 * Vds;
                        T9 = 0.25 * CoxWL;
                        T4 = T9 * Alphaz;
                        qgate = CoxWL * (Vgs_eff - Vfb - pParam->B3phi
                            - 0.5 * (Vds - T3));

                        T5 = T3 / T1;
                        inst->B3cggb = CoxWL * (1.0 - T5 * dVdsat_dVg)
                            * dVgs_eff_dVg;
                        tmp = -CoxWL * T5 * dVdsat_dVb;
                        inst->B3cgdb = CoxWL * (T2 - 0.5 + 0.5 * T5);
                        inst->B3cgsb = -(inst->B3cggb 
                            + inst->B3cgdb + tmp);

                        T6 = 1.0 / Vdsat;
                        dAlphaz_dVg = T6 * (1.0 - Alphaz * dVdsat_dVg);
                        dAlphaz_dVb = -T6 * (dVth_dVb + Alphaz * dVdsat_dVb);

                        T7 = T1 + T3;
                        qdrn = -T4 * T7;
                        qbulk = - (qgate + qdrn + qdrn);
                        T7 *= T9;
                        T0 = T4 * (2.0 * T5 - 2.0);

                        inst->B3cdgb = (T0 * dVdsat_dVg - T7
                            * dAlphaz_dVg) * dVgs_eff_dVg;
                        T12 = T0 * dVdsat_dVb - T7 * dAlphaz_dVb;
                        inst->B3cddb = T4 * (1.0 - 2.0 * T2 - T5);
                        inst->B3cdsb = -(inst->B3cdgb + T12
                            + inst->B3cddb);

                        inst->B3cbgb = -(inst->B3cggb
                            + 2.0 * inst->B3cdgb);
                        inst->B3cbdb = -(inst->B3cgdb
                            + 2.0 * inst->B3cddb);
                        inst->B3cbsb = -(inst->B3cgsb
                            + 2.0 * inst->B3cdsb);
                        inst->B3qinv = -(qgate + qbulk);
                    }
                }
            }
        } 
        else {
            if (Vbseff < 0.0) {
                VbseffCV = Vbseff;
                dVbseffCV_dVb = 1.0;
            }
            else {
                VbseffCV = pParam->B3phi - Phis;
                dVbseffCV_dVb = -dPhis_dVb;
            }

            CoxWL = model->B3cox * pParam->B3weffCV
                * pParam->B3leffCV;

            // Seperate VgsteffCV with noff and voffcv
            noff = n * pParam->B3noff;
            dnoff_dVd = pParam->B3noff * dn_dVd;
            dnoff_dVb = pParam->B3noff * dn_dVb;
            T0 = Vtm * noff;
            voffcv = pParam->B3voffcv;
            VgstNVt = (Vgst - voffcv) / T0;

            if (VgstNVt > EXP_THRESHOLD) {
                Vgsteff = Vgst - voffcv;
                dVgsteff_dVg = dVgs_eff_dVg;
                dVgsteff_dVd = -dVth_dVd;
                dVgsteff_dVb = -dVth_dVb;
            }
            else if (VgstNVt < -EXP_THRESHOLD) {
                Vgsteff = T0 * log(1.0 + MIN_EXP);
                dVgsteff_dVg = 0.0;
                dVgsteff_dVd = Vgsteff / noff;
                dVgsteff_dVb = dVgsteff_dVd * dnoff_dVb;
                dVgsteff_dVd *= dnoff_dVd;
            }
            else {
                ExpVgst = exp(VgstNVt);
                Vgsteff = T0 * log(1.0 + ExpVgst);
                dVgsteff_dVg = ExpVgst / (1.0 + ExpVgst);
                dVgsteff_dVd = -dVgsteff_dVg * (dVth_dVd + (Vgst - voffcv)
                    / noff * dnoff_dVd) + Vgsteff / noff * dnoff_dVd;
                dVgsteff_dVb = -dVgsteff_dVg * (dVth_dVb + (Vgst - voffcv)
                    / noff * dnoff_dVb) + Vgsteff / noff * dnoff_dVb;
                dVgsteff_dVg *= dVgs_eff_dVg;
            } // End of VgsteffCV - Weidong 5/1998

            if (model->B3capMod == 1) {
                if (model->B3version < 3.2) {
                    Vfb = Vth - pParam->B3phi - pParam->B3k1ox * sqrtPhis;
                    dVfb_dVb = dVth_dVb - pParam->B3k1ox * dsqrtPhis_dVb;
                    dVfb_dVd = dVth_dVd;
                } 
                else {
                    Vfb = pParam->B3vfbzb;
                    dVfb_dVb = dVfb_dVd = 0.0;
                }

                Arg1 = Vgs_eff - VbseffCV - Vfb - Vgsteff;

                if (Arg1 <= 0.0) {
                    qgate = CoxWL * Arg1;
                    Cgg = CoxWL * (dVgs_eff_dVg - dVgsteff_dVg);
                    Cgd = -CoxWL * (dVfb_dVd + dVgsteff_dVd);
                    Cgb = -CoxWL * (dVfb_dVb + dVbseffCV_dVb + dVgsteff_dVb);
                }
                else {
                    T0 = 0.5 * pParam->B3k1ox;
                    T1 = sqrt(T0 * T0 + Arg1);
                    T2 = CoxWL * T0 / T1;
              
                    qgate = CoxWL * pParam->B3k1ox * (T1 - T0);

                    Cgg = T2 * (dVgs_eff_dVg - dVgsteff_dVg);
                    Cgd = -T2 * (dVfb_dVd + dVgsteff_dVd);
                    Cgb = -T2 * (dVfb_dVb + dVbseffCV_dVb + dVgsteff_dVb);
                }
                qbulk = -qgate;
                Cbg = -Cgg;
                Cbd = -Cgd;
                Cbb = -Cgb;

                One_Third_CoxWL = CoxWL / 3.0;
                Two_Third_CoxWL = 2.0 * One_Third_CoxWL;
                AbulkCV = Abulk0 * pParam->B3abulkCVfactor;
                dAbulkCV_dVb = pParam->B3abulkCVfactor * dAbulk0_dVb;
                VdsatCV = Vgsteff / AbulkCV;
                if (VdsatCV < Vds) {
                    dVdsatCV_dVg = 1.0 / AbulkCV;
                    dVdsatCV_dVb = -VdsatCV * dAbulkCV_dVb / AbulkCV;
                    T0 = Vgsteff - VdsatCV / 3.0;
                    dT0_dVg = 1.0 - dVdsatCV_dVg / 3.0;
                    dT0_dVb = -dVdsatCV_dVb / 3.0;
                    qgate += CoxWL * T0;
                    Cgg1 = CoxWL * dT0_dVg; 
                    Cgb1 = CoxWL * dT0_dVb + Cgg1 * dVgsteff_dVb;
                    Cgd1 = Cgg1 * dVgsteff_dVd;
                    Cgg1 *= dVgsteff_dVg;
                    Cgg += Cgg1;
                    Cgb += Cgb1;
                    Cgd += Cgd1;

                    T0 = VdsatCV - Vgsteff;
                    dT0_dVg = dVdsatCV_dVg - 1.0;
                    dT0_dVb = dVdsatCV_dVb;
                    qbulk += One_Third_CoxWL * T0;
                    Cbg1 = One_Third_CoxWL * dT0_dVg;
                    Cbb1 = One_Third_CoxWL * dT0_dVb + Cbg1 * dVgsteff_dVb;
                    Cbd1 = Cbg1 * dVgsteff_dVd;
                    Cbg1 *= dVgsteff_dVg;
                    Cbg += Cbg1;
                    Cbb += Cbb1;
                    Cbd += Cbd1;

                    if (model->B3xpart > 0.5)
                        T0 = -Two_Third_CoxWL;
                    else if (model->B3xpart < 0.5)
                        T0 = -0.4 * CoxWL;
                    else
                        T0 = -One_Third_CoxWL;

                    qsrc = T0 * Vgsteff;
                    Csg = T0 * dVgsteff_dVg;
                    Csb = T0 * dVgsteff_dVb;
                    Csd = T0 * dVgsteff_dVd;
                    Cgb *= dVbseff_dVb;
                    Cbb *= dVbseff_dVb;
                    Csb *= dVbseff_dVb;
                }
                else {
                    T0 = AbulkCV * Vds;
                    T1 = 12.0 * (Vgsteff - 0.5 * T0 + 1.e-20);
                    T2 = Vds / T1;
                    T3 = T0 * T2;
                    dT3_dVg = -12.0 * T2 * T2 * AbulkCV;
                    dT3_dVd = 6.0 * T0 * (4.0 * Vgsteff - T0) / T1 / T1 - 0.5;
                    dT3_dVb = 12.0 * T2 * T2 * dAbulkCV_dVb * Vgsteff;

                    qgate += CoxWL * (Vgsteff - 0.5 * Vds + T3);
                    Cgg1 = CoxWL * (1.0 + dT3_dVg);
                    Cgb1 = CoxWL * dT3_dVb + Cgg1 * dVgsteff_dVb;
                    Cgd1 = CoxWL * dT3_dVd + Cgg1 * dVgsteff_dVd;
                    Cgg1 *= dVgsteff_dVg;
                    Cgg += Cgg1;
                    Cgb += Cgb1;
                    Cgd += Cgd1;

                    qbulk += CoxWL * (1.0 - AbulkCV) * (0.5 * Vds - T3);
                    Cbg1 = -CoxWL * ((1.0 - AbulkCV) * dT3_dVg);
                    Cbb1 = -CoxWL * ((1.0 - AbulkCV) * dT3_dVb
                        + (0.5 * Vds - T3) * dAbulkCV_dVb)
                        + Cbg1 * dVgsteff_dVb;
                    Cbd1 = -CoxWL * (1.0 - AbulkCV) * dT3_dVd
                        + Cbg1 * dVgsteff_dVd;
                    Cbg1 *= dVgsteff_dVg;
                    Cbg += Cbg1;
                    Cbb += Cbb1;
                    Cbd += Cbd1;

                    if (model->B3xpart > 0.5) {
                        // 0/100 Charge petition model
                    T1 = T1 + T1;
                    qsrc = -CoxWL * (0.5 * Vgsteff + 0.25 * T0
                        - T0 * T0 / T1);
                    Csg = -CoxWL * (0.5 + 24.0 * T0 * Vds / T1 / T1
                        * AbulkCV);
                    Csb = -CoxWL * (0.25 * Vds * dAbulkCV_dVb
                        - 12.0 * T0 * Vds / T1 / T1 * (4.0 * Vgsteff - T0)
                        * dAbulkCV_dVb) + Csg * dVgsteff_dVb;
                    Csd = -CoxWL * (0.25 * AbulkCV - 12.0 * AbulkCV * T0
                        / T1 / T1 * (4.0 * Vgsteff - T0))
                        + Csg * dVgsteff_dVd;
                    Csg *= dVgsteff_dVg;
                }
                else if (model->B3xpart < 0.5) {
                    // 40/60 Charge petition model
                    T1 = T1 / 12.0;
                    T2 = 0.5 * CoxWL / (T1 * T1);
                    T3 = Vgsteff * (2.0 * T0 * T0 / 3.0 + Vgsteff
                        * (Vgsteff - 4.0 * T0 / 3.0))
                        - 2.0 * T0 * T0 * T0 / 15.0;
                    qsrc = -T2 * T3;
                    T4 = 4.0 / 3.0 * Vgsteff * (Vgsteff - T0)
                        + 0.4 * T0 * T0;
                    Csg = -2.0 * qsrc / T1 - T2 * (Vgsteff * (3.0
                        * Vgsteff - 8.0 * T0 / 3.0)
                        + 2.0 * T0 * T0 / 3.0);
                    Csb = (qsrc / T1 * Vds + T2 * T4 * Vds) * dAbulkCV_dVb
                        + Csg * dVgsteff_dVb;
                    Csd = (qsrc / T1 + T2 * T4) * AbulkCV
                        + Csg * dVgsteff_dVd;
                    Csg *= dVgsteff_dVg;
                }
                else {
                    // 50/50 Charge petition model
                    qsrc = -0.5 * (qgate + qbulk);
                    Csg = -0.5 * (Cgg1 + Cbg1);
                    Csb = -0.5 * (Cgb1 + Cbb1); 
                    Csd = -0.5 * (Cgd1 + Cbd1); 
                }
                Cgb *= dVbseff_dVb;
                Cbb *= dVbseff_dVb;
                Csb *= dVbseff_dVb;
            }
            qdrn = -(qgate + qbulk + qsrc);
            inst->B3cggb = Cgg;
            inst->B3cgsb = -(Cgg + Cgd + Cgb);
            inst->B3cgdb = Cgd;
            inst->B3cdgb = -(Cgg + Cbg + Csg);
            inst->B3cdsb = (Cgg + Cgd + Cgb + Cbg + Cbd + Cbb
                + Csg + Csd + Csb);
            inst->B3cddb = -(Cgd + Cbd + Csd);
            inst->B3cbgb = Cbg;
            inst->B3cbsb = -(Cbg + Cbd + Cbb);
            inst->B3cbdb = Cbd;
            inst->B3qinv = -(qgate + qbulk);
        }

        else if (model->B3capMod == 2) {
            if (model->B3version < 3.2) {
                Vfb = Vth - pParam->B3phi - pParam->B3k1ox * sqrtPhis;
                dVfb_dVb = dVth_dVb - pParam->B3k1ox * dsqrtPhis_dVb;
                dVfb_dVd = dVth_dVd;
            }
            else {
                Vfb = pParam->B3vfbzb;
                dVfb_dVb = dVfb_dVd = 0.0;
            }

            V3 = Vfb - Vgs_eff + VbseffCV - DELTA_3;
            if (Vfb <= 0.0) {
                T0 = sqrt(V3 * V3 - 4.0 * DELTA_3 * Vfb);
                T2 = -DELTA_3 / T0;
            }
            else {
                T0 = sqrt(V3 * V3 + 4.0 * DELTA_3 * Vfb);
                T2 = DELTA_3 / T0;
            }

            T1 = 0.5 * (1.0 + V3 / T0);
            Vfbeff = Vfb - 0.5 * (V3 + T0);
            dVfbeff_dVd = (1.0 - T1 - T2) * dVfb_dVd;
            dVfbeff_dVg = T1 * dVgs_eff_dVg;
            dVfbeff_dVb = (1.0 - T1 - T2) * dVfb_dVb
                - T1 * dVbseffCV_dVb;
            Qac0 = CoxWL * (Vfbeff - Vfb);
            dQac0_dVg = CoxWL * dVfbeff_dVg;
            dQac0_dVd = CoxWL * (dVfbeff_dVd - dVfb_dVd);
            dQac0_dVb = CoxWL * (dVfbeff_dVb - dVfb_dVb);

            T0 = 0.5 * pParam->B3k1ox;
            T3 = Vgs_eff - Vfbeff - VbseffCV - Vgsteff;
            if (pParam->B3k1ox == 0.0) {
                T1 = 0.0;
                T2 = 0.0;
            }
            else if (T3 < 0.0) {
                T1 = T0 + T3 / pParam->B3k1ox;
                T2 = CoxWL;
            }
            else {
                T1 = sqrt(T0 * T0 + T3);
                T2 = CoxWL * T0 / T1;
            }

            Qsub0 = CoxWL * pParam->B3k1ox * (T1 - T0);

            dQsub0_dVg = T2 * (dVgs_eff_dVg - dVfbeff_dVg - dVgsteff_dVg);
            dQsub0_dVd = -T2 * (dVfbeff_dVd + dVgsteff_dVd);
            dQsub0_dVb = -T2 * (dVfbeff_dVb + dVbseffCV_dVb 
                + dVgsteff_dVb);

            AbulkCV = Abulk0 * pParam->B3abulkCVfactor;
            dAbulkCV_dVb = pParam->B3abulkCVfactor * dAbulk0_dVb;
            VdsatCV = Vgsteff / AbulkCV;

            V4 = VdsatCV - Vds - DELTA_4;
            T0 = sqrt(V4 * V4 + 4.0 * DELTA_4 * VdsatCV);
            VdseffCV = VdsatCV - 0.5 * (V4 + T0);
            T1 = 0.5 * (1.0 + V4 / T0);
            T2 = DELTA_4 / T0;
            T3 = (1.0 - T1 - T2) / AbulkCV;
            dVdseffCV_dVg = T3;
            dVdseffCV_dVd = T1;
            dVdseffCV_dVb = -T3 * VdsatCV * dAbulkCV_dVb;

            T0 = AbulkCV * VdseffCV;
            T1 = 12.0 * (Vgsteff - 0.5 * T0 + 1e-20);
            T2 = VdseffCV / T1;
            T3 = T0 * T2;

            T4 = (1.0 - 12.0 * T2 * T2 * AbulkCV);
            T5 = (6.0 * T0 * (4.0 * Vgsteff - T0) / (T1 * T1) - 0.5);
            T6 = 12.0 * T2 * T2 * Vgsteff;

            qinoi = -CoxWL * (Vgsteff - 0.5 * T0 + AbulkCV * T3);
            qgate = CoxWL * (Vgsteff - 0.5 * VdseffCV + T3);
            Cgg1 = CoxWL * (T4 + T5 * dVdseffCV_dVg);
            Cgd1 = CoxWL * T5 * dVdseffCV_dVd + Cgg1 * dVgsteff_dVd;
            Cgb1 = CoxWL * (T5 * dVdseffCV_dVb + T6 * dAbulkCV_dVb)
                + Cgg1 * dVgsteff_dVb;
            Cgg1 *= dVgsteff_dVg;

            T7 = 1.0 - AbulkCV;
            qbulk = CoxWL * T7 * (0.5 * VdseffCV - T3);
            T4 = -T7 * (T4 - 1.0);
            T5 = -T7 * T5;
            T6 = -(T7 * T6 + (0.5 * VdseffCV - T3));
            Cbg1 = CoxWL * (T4 + T5 * dVdseffCV_dVg);
            Cbd1 = CoxWL * T5 * dVdseffCV_dVd + Cbg1 * dVgsteff_dVd;
            Cbb1 = CoxWL * (T5 * dVdseffCV_dVb + T6 * dAbulkCV_dVb)
                + Cbg1 * dVgsteff_dVb;
            Cbg1 *= dVgsteff_dVg;

            if (model->B3xpart > 0.5) {
                // 0/100 Charge petition model
                T1 = T1 + T1;
                qsrc = -CoxWL * (0.5 * Vgsteff + 0.25 * T0
                    - T0 * T0 / T1);
                T7 = (4.0 * Vgsteff - T0) / (T1 * T1);
                T4 = -(0.5 + 24.0 * T0 * T0 / (T1 * T1));
                T5 = -(0.25 * AbulkCV - 12.0 * AbulkCV * T0 * T7);
                T6 = -(0.25 * VdseffCV - 12.0 * T0 * VdseffCV * T7);
                Csg = CoxWL * (T4 + T5 * dVdseffCV_dVg);
                Csd = CoxWL * T5 * dVdseffCV_dVd + Csg * dVgsteff_dVd;
                Csb = CoxWL * (T5 * dVdseffCV_dVb + T6 * dAbulkCV_dVb)
                    + Csg * dVgsteff_dVb;
                Csg *= dVgsteff_dVg;
            }
            else if (model->B3xpart < 0.5) {
                // 40/60 Charge petition model
                T1 = T1 / 12.0;
                T2 = 0.5 * CoxWL / (T1 * T1);
                T3 = Vgsteff * (2.0 * T0 * T0 / 3.0 + Vgsteff
                    * (Vgsteff - 4.0 * T0 / 3.0))
                    - 2.0 * T0 * T0 * T0 / 15.0;
                qsrc = -T2 * T3;
                T7 = 4.0 / 3.0 * Vgsteff * (Vgsteff - T0)
                    + 0.4 * T0 * T0;
                T4 = -2.0 * qsrc / T1 - T2 * (Vgsteff * (3.0
                    * Vgsteff - 8.0 * T0 / 3.0)
                    + 2.0 * T0 * T0 / 3.0);
                T5 = (qsrc / T1 + T2 * T7) * AbulkCV;
                T6 = (qsrc / T1 * VdseffCV + T2 * T7 * VdseffCV);
                Csg = (T4 + T5 * dVdseffCV_dVg);
                Csd = T5 * dVdseffCV_dVd + Csg * dVgsteff_dVd;
                Csb = (T5 * dVdseffCV_dVb + T6 * dAbulkCV_dVb)
                    + Csg * dVgsteff_dVb;
                Csg *= dVgsteff_dVg;
            }
            else {
                // 50/50 Charge petition model
                qsrc = -0.5 * (qgate + qbulk);
                Csg = -0.5 * (Cgg1 + Cbg1);
                Csb = -0.5 * (Cgb1 + Cbb1); 
                Csd = -0.5 * (Cgd1 + Cbd1); 
            }

            qgate += Qac0 + Qsub0;
            qbulk -= (Qac0 + Qsub0);
            qdrn = -(qgate + qbulk + qsrc);

            Cgg = dQac0_dVg + dQsub0_dVg + Cgg1;
            Cgd = dQac0_dVd + dQsub0_dVd + Cgd1;
            Cgb = dQac0_dVb + dQsub0_dVb + Cgb1;

            Cbg = Cbg1 - dQac0_dVg - dQsub0_dVg;
            Cbd = Cbd1 - dQac0_dVd - dQsub0_dVd;
            Cbb = Cbb1 - dQac0_dVb - dQsub0_dVb;

            Cgb *= dVbseff_dVb;
            Cbb *= dVbseff_dVb;
            Csb *= dVbseff_dVb;

            inst->B3cggb = Cgg;
            inst->B3cgsb = -(Cgg + Cgd + Cgb);
            inst->B3cgdb = Cgd;
            inst->B3cdgb = -(Cgg + Cbg + Csg);
            inst->B3cdsb = (Cgg + Cgd + Cgb + Cbg + Cbd + Cbb
                + Csg + Csd + Csb);
            inst->B3cddb = -(Cgd + Cbd + Csd);
            inst->B3cbgb = Cbg;
            inst->B3cbsb = -(Cbg + Cbd + Cbb);
            inst->B3cbdb = Cbd;
            inst->B3qinv = qinoi;
        } 

        // New Charge-Thickness capMod (CTM) begins - Weidong 7/1997

        else if (model->B3capMod == 3) {
            V3 = pParam->B3vfbzb - Vgs_eff + VbseffCV - DELTA_3;
            if (pParam->B3vfbzb <= 0.0) {
                T0 = sqrt(V3 * V3 - 4.0 * DELTA_3 * pParam->B3vfbzb);
                T2 = -DELTA_3 / T0;
            }
            else {
                T0 = sqrt(V3 * V3 + 4.0 * DELTA_3 * pParam->B3vfbzb);
                T2 = DELTA_3 / T0;
            }

            T1 = 0.5 * (1.0 + V3 / T0);
            Vfbeff = pParam->B3vfbzb - 0.5 * (V3 + T0);
            dVfbeff_dVg = T1 * dVgs_eff_dVg;
            dVfbeff_dVb = -T1 * dVbseffCV_dVb;

            Cox = model->B3cox;
            Tox = 1.0e8 * model->B3tox;
            T0 = (Vgs_eff - VbseffCV - pParam->B3vfbzb) / Tox;
            dT0_dVg = dVgs_eff_dVg / Tox;
            dT0_dVb = -dVbseffCV_dVb / Tox;

            tmp = T0 * pParam->B3acde;
            if ((-EXP_THRESHOLD < tmp) && (tmp < EXP_THRESHOLD)) {
                Tcen = pParam->B3ldeb * exp(tmp);
                dTcen_dVg = pParam->B3acde * Tcen;
                dTcen_dVb = dTcen_dVg * dT0_dVb;
                dTcen_dVg *= dT0_dVg;
            }
            else if (tmp <= -EXP_THRESHOLD) {
                Tcen = pParam->B3ldeb * MIN_EXP;
                dTcen_dVg = dTcen_dVb = 0.0;
            }
            else {
                Tcen = pParam->B3ldeb * MAX_EXP;
                dTcen_dVg = dTcen_dVb = 0.0;
            }

            LINK = 1.0e-3 * model->B3tox;
            V3 = pParam->B3ldeb - Tcen - LINK;
            V4 = sqrt(V3 * V3 + 4.0 * LINK * pParam->B3ldeb);
            Tcen = pParam->B3ldeb - 0.5 * (V3 + V4);
            T1 = 0.5 * (1.0 + V3 / V4);
            dTcen_dVg *= T1;
            dTcen_dVb *= T1;

            Ccen = EPSSI / Tcen;
            T2 = Cox / (Cox + Ccen);
            Coxeff = T2 * Ccen;
            T3 = -Ccen / Tcen;
            dCoxeff_dVg = T2 * T2 * T3;
            dCoxeff_dVb = dCoxeff_dVg * dTcen_dVb;
            dCoxeff_dVg *= dTcen_dVg;
            CoxWLcen = CoxWL * Coxeff / Cox;

            Qac0 = CoxWLcen * (Vfbeff - pParam->B3vfbzb);
            QovCox = Qac0 / Coxeff;
            dQac0_dVg = CoxWLcen * dVfbeff_dVg
                + QovCox * dCoxeff_dVg;
            dQac0_dVb = CoxWLcen * dVfbeff_dVb 
                + QovCox * dCoxeff_dVb;

            T0 = 0.5 * pParam->B3k1ox;
            T3 = Vgs_eff - Vfbeff - VbseffCV - Vgsteff;
            if (pParam->B3k1ox == 0.0) {
                T1 = 0.0;
                T2 = 0.0;
            }
            else if (T3 < 0.0) {
                T1 = T0 + T3 / pParam->B3k1ox;
                T2 = CoxWLcen;
            }
            else {
                T1 = sqrt(T0 * T0 + T3);
                T2 = CoxWLcen * T0 / T1;
            }

            Qsub0 = CoxWLcen * pParam->B3k1ox * (T1 - T0);
            QovCox = Qsub0 / Coxeff;
            dQsub0_dVg = T2 * (dVgs_eff_dVg - dVfbeff_dVg - dVgsteff_dVg)
                + QovCox * dCoxeff_dVg;
            dQsub0_dVd = -T2 * dVgsteff_dVd;
            dQsub0_dVb = -T2 * (dVfbeff_dVb + dVbseffCV_dVb + dVgsteff_dVb)
                + QovCox * dCoxeff_dVb;

            // Gate-bias dependent delta Phis begins
            if (pParam->B3k1ox <= 0.0) {
                Denomi = 0.25 * pParam->B3moin * Vtm;
                T0 = 0.5 * pParam->B3sqrtPhi;
            }
            else {
                Denomi = pParam->B3moin * Vtm
                    * pParam->B3k1ox * pParam->B3k1ox;
                T0 = pParam->B3k1ox * pParam->B3sqrtPhi;
            }
            T1 = 2.0 * T0 + Vgsteff;

            DeltaPhi = Vtm * log(1.0 + T1 * Vgsteff / Denomi);
            dDeltaPhi_dVg = 2.0 * Vtm * (T1 -T0) / (Denomi + T1 * Vgsteff);
//            dDeltaPhi_dVd = dDeltaPhi_dVg * dVgsteff_dVd;
//            dDeltaPhi_dVb = dDeltaPhi_dVg * dVgsteff_dVb;
            // End of delta Phis

            T3 = 4.0 * (Vth - pParam->B3vfbzb - pParam->B3phi);
            Tox += Tox;
            if (T3 >= 0.0)
                T0 = (Vgsteff + T3) / Tox;
            else
                T0 = (Vgsteff + 1.0e-20) / Tox;
            tmp = exp(0.7 * log(T0));
            T1 = 1.0 + tmp;
            T2 = 0.7 * tmp / (T0 * Tox);
            Tcen = 1.9e-9 / T1;
            dTcen_dVg = -1.9e-9 * T2 / T1 /T1;
            dTcen_dVd = dTcen_dVg * (4.0 * dVth_dVd + dVgsteff_dVd);
            dTcen_dVb = dTcen_dVg * (4.0 * dVth_dVb + dVgsteff_dVb);
            dTcen_dVg *= dVgsteff_dVg;

            Ccen = EPSSI / Tcen;
            T0 = Cox / (Cox + Ccen);
            Coxeff = T0 * Ccen;
            T1 = -Ccen / Tcen;
            dCoxeff_dVg = T0 * T0 * T1;
            dCoxeff_dVd = dCoxeff_dVg * dTcen_dVd;
            dCoxeff_dVb = dCoxeff_dVg * dTcen_dVb;
            dCoxeff_dVg *= dTcen_dVg;
            CoxWLcen = CoxWL * Coxeff / Cox;

            AbulkCV = Abulk0 * pParam->B3abulkCVfactor;
            dAbulkCV_dVb = pParam->B3abulkCVfactor * dAbulk0_dVb;
            VdsatCV = (Vgsteff - DeltaPhi) / AbulkCV;
            V4 = VdsatCV - Vds - DELTA_4;
            T0 = sqrt(V4 * V4 + 4.0 * DELTA_4 * VdsatCV);
            VdseffCV = VdsatCV - 0.5 * (V4 + T0);
            T1 = 0.5 * (1.0 + V4 / T0);
            T2 = DELTA_4 / T0;
            T3 = (1.0 - T1 - T2) / AbulkCV;
            T4 = T3 * ( 1.0 - dDeltaPhi_dVg);
            dVdseffCV_dVg = T4;
            dVdseffCV_dVd = T1;
            dVdseffCV_dVb = -T3 * VdsatCV * dAbulkCV_dVb;

            T0 = AbulkCV * VdseffCV;
            T1 = Vgsteff - DeltaPhi;
            T2 = 12.0 * (T1 - 0.5 * T0 + 1.0e-20);
            T3 = T0 / T2;
            T4 = 1.0 - 12.0 * T3 * T3;
            T5 = AbulkCV * (6.0 * T0 * (4.0 * T1 - T0) / (T2 * T2) - 0.5);
            T6 = T5 * VdseffCV / AbulkCV;

            qgate = qinoi = CoxWLcen * (T1 - T0 * (0.5 - T3));
            QovCox = qgate / Coxeff;
            Cgg1 = CoxWLcen * (T4 * (1.0 - dDeltaPhi_dVg) 
                + T5 * dVdseffCV_dVg);
            Cgd1 = CoxWLcen * T5 * dVdseffCV_dVd + Cgg1 
                * dVgsteff_dVd + QovCox * dCoxeff_dVd;
            Cgb1 = CoxWLcen * (T5 * dVdseffCV_dVb + T6 * dAbulkCV_dVb) 
                + Cgg1 * dVgsteff_dVb + QovCox * dCoxeff_dVb;
            Cgg1 = Cgg1 * dVgsteff_dVg + QovCox * dCoxeff_dVg;


            T7 = 1.0 - AbulkCV;
            T8 = T2 * T2;
            T9 = 12.0 * T7 * T0 * T0 / (T8 * AbulkCV);
            T10 = T9 * (1.0 - dDeltaPhi_dVg);
            T11 = -T7 * T5 / AbulkCV;
            T12 = -(T9 * T1 / AbulkCV + VdseffCV * (0.5 - T0 / T2));

            qbulk = CoxWLcen * T7 * (0.5 * VdseffCV - T0 * VdseffCV / T2);
            QovCox = qbulk / Coxeff;
            Cbg1 = CoxWLcen * (T10 + T11 * dVdseffCV_dVg);
            Cbd1 = CoxWLcen * T11 * dVdseffCV_dVd + Cbg1
                * dVgsteff_dVd + QovCox * dCoxeff_dVd; 
            Cbb1 = CoxWLcen * (T11 * dVdseffCV_dVb + T12 * dAbulkCV_dVb)
                + Cbg1 * dVgsteff_dVb + QovCox * dCoxeff_dVb;
            Cbg1 = Cbg1 * dVgsteff_dVg + QovCox * dCoxeff_dVg;

            if (model->B3xpart > 0.5) {
                // 0/100 partition
                qsrc = -CoxWLcen * (T1 / 2.0 + T0 / 4.0 
                    - 0.5 * T0 * T0 / T2);
                QovCox = qsrc / Coxeff;
                T2 += T2;
                T3 = T2 * T2;
                T7 = -(0.25 - 12.0 * T0 * (4.0 * T1 - T0) / T3);
                T4 = -(0.5 + 24.0 * T0 * T0 / T3) * (1.0 - dDeltaPhi_dVg);
                T5 = T7 * AbulkCV;
                T6 = T7 * VdseffCV;

                Csg = CoxWLcen * (T4 + T5 * dVdseffCV_dVg);
                Csd = CoxWLcen * T5 * dVdseffCV_dVd + Csg * dVgsteff_dVd
                    + QovCox * dCoxeff_dVd;
                Csb = CoxWLcen * (T5 * dVdseffCV_dVb + T6 * dAbulkCV_dVb)
                    + Csg * dVgsteff_dVb + QovCox * dCoxeff_dVb;
                Csg = Csg * dVgsteff_dVg + QovCox * dCoxeff_dVg;
            }
            else if (model->B3xpart < 0.5) {
                // 40/60 partition
                T2 = T2 / 12.0;
                T3 = 0.5 * CoxWLcen / (T2 * T2);
                T4 = T1 * (2.0 * T0 * T0 / 3.0 + T1 * (T1 - 4.0 
                    * T0 / 3.0)) - 2.0 * T0 * T0 * T0 / 15.0;
                qsrc = -T3 * T4;
                QovCox = qsrc / Coxeff;
                T8 = 4.0 / 3.0 * T1 * (T1 - T0) + 0.4 * T0 * T0;
                T5 = -2.0 * qsrc / T2 - T3 * (T1 * (3.0 * T1 - 8.0 
                    * T0 / 3.0) + 2.0 * T0 * T0 / 3.0);
                T6 = AbulkCV * (qsrc / T2 + T3 * T8);
                T7 = T6 * VdseffCV / AbulkCV; 

                Csg = T5 * (1.0 - dDeltaPhi_dVg) + T6 * dVdseffCV_dVg; 
                Csd = Csg * dVgsteff_dVd + T6 * dVdseffCV_dVd 
                    + QovCox * dCoxeff_dVd;
                Csb = Csg * dVgsteff_dVb + T6 * dVdseffCV_dVb 
                    + T7 * dAbulkCV_dVb + QovCox * dCoxeff_dVb; 
                Csg = Csg * dVgsteff_dVg + QovCox * dCoxeff_dVg;
            }
            else {
                // 50/50 partition
                qsrc = -0.5 * qgate;
                Csg = -0.5 * Cgg1;
                Csd = -0.5 * Cgd1; 
                Csb = -0.5 * Cgb1; 
            }

            qgate += Qac0 + Qsub0 - qbulk;
            qbulk -= (Qac0 + Qsub0);
            qdrn = -(qgate + qbulk + qsrc);

            Cbg = Cbg1 - dQac0_dVg - dQsub0_dVg;
            Cbd = Cbd1 - dQsub0_dVd;
            Cbb = Cbb1 - dQac0_dVb - dQsub0_dVb;

            Cgg = Cgg1 - Cbg;
            Cgd = Cgd1 - Cbd;
            Cgb = Cgb1 - Cbb;

            Cgb *= dVbseff_dVb;
            Cbb *= dVbseff_dVb;
            Csb *= dVbseff_dVb;

            inst->B3cggb = Cgg;
            inst->B3cgsb = -(Cgg + Cgd + Cgb);
            inst->B3cgdb = Cgd;
            inst->B3cdgb = -(Cgg + Cbg + Csg);
            inst->B3cdsb = (Cgg + Cgd + Cgb + Cbg + Cbd + Cbb
                + Csg + Csd + Csb);
            inst->B3cddb = -(Cgd + Cbd + Csd);
            inst->B3cbgb = Cbg;
            inst->B3cbsb = -(Cbg + Cbd + Cbb);
            inst->B3cbdb = Cbd;
            inst->B3qinv = -qinoi;
        }  // End of CTM
    }

finished: 
    // Returning Values to Calling Routine
        //
        //  COMPUTE EQUIVALENT DRAIN CURRENT SOURCE
        //

        inst->B3qgate = qgate;
        inst->B3qbulk = qbulk;
        inst->B3qdrn = qdrn;
        inst->B3cd = cdrain;

        if (ChargeComputationNeeded) {
            // charge storage elements
            // bulk-drain and bulk-source depletion capacitances
            //  czbd : zero bias drain junction capacitance
            //  czbs : zero bias source junction capacitance
            //  czbdsw: zero bias drain junction sidewall capacitance
            //   along field oxide
            //  czbssw: zero bias source junction sidewall capacitance
            //   along field oxide
            //  czbdswg: zero bias drain junction sidewall capacitance
            //   along gate side
            //  czbsswg: zero bias source junction sidewall capacitance
            //   along gate side

            czbd = model->B3unitAreaJctCap * inst->B3drainArea;
            czbs = model->B3unitAreaJctCap * inst->B3sourceArea;
            if (inst->B3drainPerimeter < pParam->B3weff) { 
                czbdswg = model->B3unitLengthGateSidewallJctCap 
                    * inst->B3drainPerimeter;
                czbdsw = 0.0;
            }
            else {
                czbdsw = model->B3unitLengthSidewallJctCap 
                    * (inst->B3drainPerimeter - pParam->B3weff);
                czbdswg = model->B3unitLengthGateSidewallJctCap
                    *  pParam->B3weff;
            }
            if (inst->B3sourcePerimeter < pParam->B3weff) {
                czbssw = 0.0; 
                czbsswg = model->B3unitLengthGateSidewallJctCap
                    * inst->B3sourcePerimeter;
            }
            else {
                czbssw = model->B3unitLengthSidewallJctCap 
                    * (inst->B3sourcePerimeter - pParam->B3weff);
                czbsswg = model->B3unitLengthGateSidewallJctCap
                    *  pParam->B3weff;
            }

            MJ = model->B3bulkJctBotGradingCoeff;
            MJSW = model->B3bulkJctSideGradingCoeff;
            MJSWG = model->B3bulkJctGateSideGradingCoeff;

            // Source Bulk Junction
            if (vbs == 0.0) {
                *(ckt->CKTstate0 + inst->B3qbs) = 0.0;
                inst->B3capbs = czbs + czbssw + czbsswg;
            }
            else if (vbs < 0.0) {
                if (czbs > 0.0) {
                    arg = 1.0 - vbs / model->B3PhiB;
                    if (MJ == 0.5)
                        sarg = 1.0 / sqrt(arg);
                    else
                        sarg = exp(-MJ * log(arg));
                    *(ckt->CKTstate0 + inst->B3qbs) = model->B3PhiB * czbs 
                        * (1.0 - arg * sarg) / (1.0 - MJ);
                    inst->B3capbs = czbs * sarg;
                }
                else {
                    *(ckt->CKTstate0 + inst->B3qbs) = 0.0;
                    inst->B3capbs = 0.0;
                }
                if (czbssw > 0.0) {
                    arg = 1.0 - vbs / model->B3PhiBSW;
                    if (MJSW == 0.5)
                        sarg = 1.0 / sqrt(arg);
                    else
                        sarg = exp(-MJSW * log(arg));
                    *(ckt->CKTstate0 + inst->B3qbs) += model->B3PhiBSW * czbssw
                        * (1.0 - arg * sarg) / (1.0 - MJSW);
                    inst->B3capbs += czbssw * sarg;
                }
                if (czbsswg > 0.0) {
                    arg = 1.0 - vbs / model->B3PhiBSWG;
                    if (MJSWG == 0.5)
                        sarg = 1.0 / sqrt(arg);
                    else
                        sarg = exp(-MJSWG * log(arg));
                    *(ckt->CKTstate0 + inst->B3qbs) += model->B3PhiBSWG *
                        czbsswg * (1.0 - arg * sarg) / (1.0 - MJSWG);
                    inst->B3capbs += czbsswg * sarg;
                }

            }
            else {
                T0 = czbs + czbssw + czbsswg;
                T1 = vbs * (czbs * MJ / model->B3PhiB + czbssw * MJSW 
                    / model->B3PhiBSW + czbsswg * MJSWG / model->B3PhiBSWG);    
                *(ckt->CKTstate0 + inst->B3qbs) = vbs * (T0 + 0.5 * T1);
                inst->B3capbs = T0 + T1;
            }

            // Drain Bulk Junction
            if (vbd == 0.0) {
                *(ckt->CKTstate0 + inst->B3qbd) = 0.0;
                inst->B3capbd = czbd + czbdsw + czbdswg;
            }
            else if (vbd < 0.0) {
                if (czbd > 0.0) {
                    arg = 1.0 - vbd / model->B3PhiB;
                    if (MJ == 0.5)
                        sarg = 1.0 / sqrt(arg);
                    else
                        sarg = exp(-MJ * log(arg));
                    *(ckt->CKTstate0 + inst->B3qbd) = model->B3PhiB * czbd 
                        * (1.0 - arg * sarg) / (1.0 - MJ);
                    inst->B3capbd = czbd * sarg;
                }
                else {
                    *(ckt->CKTstate0 + inst->B3qbd) = 0.0;
                    inst->B3capbd = 0.0;
                }
                if (czbdsw > 0.0) {
                    arg = 1.0 - vbd / model->B3PhiBSW;
                    if (MJSW == 0.5)
                        sarg = 1.0 / sqrt(arg);
                    else
                        sarg = exp(-MJSW * log(arg));
                    *(ckt->CKTstate0 + inst->B3qbd) += model->B3PhiBSW * czbdsw 
                        * (1.0 - arg * sarg) / (1.0 - MJSW);
                    inst->B3capbd += czbdsw * sarg;
                }
                if (czbdswg > 0.0) {
                    arg = 1.0 - vbd / model->B3PhiBSWG;
                    if (MJSWG == 0.5)
                        sarg = 1.0 / sqrt(arg);
                    else
                        sarg = exp(-MJSWG * log(arg));
                    *(ckt->CKTstate0 + inst->B3qbd) += model->B3PhiBSWG *
                        czbdswg * (1.0 - arg * sarg) / (1.0 - MJSWG);
                    inst->B3capbd += czbdswg * sarg;
                }
            }
            else {
                T0 = czbd + czbdsw + czbdswg;
                T1 = vbd * (czbd * MJ / model->B3PhiB + czbdsw * MJSW
                    / model->B3PhiBSW + czbdswg * MJSWG / model->B3PhiBSWG);
                *(ckt->CKTstate0 + inst->B3qbd) = vbd * (T0 + 0.5 * T1);
                inst->B3capbd = T0 + T1; 
            }
        }

        //
        //  check convergence
        //
        if ((inst->B3off == 0) || (!(ckt->CKTmode & MODEINITFIX))) {
            if (Check == 1) {
                ckt->incNoncon();  // SRW
#ifndef NEWCONV
            } 
            else {
                if (inst->B3mode >= 0) {
                    Idtot = inst->B3cd + inst->B3csub - inst->B3cbd;
                }
                else {
                    Idtot = inst->B3cd - inst->B3cbd;
                }
                double tol = ckt->CKTcurTask->TSKreltol * SPMAX(FABS(cdhat),
                    FABS(Idtot)) + ckt->CKTcurTask->TSKabstol;
                if (FABS(cdhat - Idtot) >= tol) {
                    ckt->incNoncon();  // SRW
                }
                else {
                    Ibtot = inst->B3cbs + inst->B3cbd - inst->B3csub;
                    double tol = ckt->CKTcurTask->TSKreltol * SPMAX(FABS(cbhat),
                        FABS(Ibtot)) + ckt->CKTcurTask->TSKabstol;
                    if (FABS(cbhat - Ibtot) > tol) {
                        ckt->incNoncon();  // SRW
                    }
                }
#endif // NEWCONV
            }
        }
        *(ckt->CKTstate0 + inst->B3vbs) = vbs;
        *(ckt->CKTstate0 + inst->B3vbd) = vbd;
        *(ckt->CKTstate0 + inst->B3vgs) = vgs;
        *(ckt->CKTstate0 + inst->B3vds) = vds;
        *(ckt->CKTstate0 + inst->B3qdef) = qdef;

        // bulk and channel charge plus overlaps

        if (!ChargeComputationNeeded)
            goto line850; 
         
line755:
        // NQS (Mansun 11/1993) modified by Weidong & Min-Chie 1997-1998
        if (inst->B3nqsMod) {
            qcheq = -(qbulk + qgate);

            inst->B3cqgb = -(inst->B3cggb + inst->B3cbgb);
            inst->B3cqdb = -(inst->B3cgdb + inst->B3cbdb);
            inst->B3cqsb = -(inst->B3cgsb + inst->B3cbsb);
            inst->B3cqbb = -(inst->B3cqgb + inst->B3cqdb
                + inst->B3cqsb);

            gtau_drift = fabs(pParam->B3tconst * qcheq) * ScalingFactor;
            T0 = pParam->B3leffCV * pParam->B3leffCV;
            gtau_diff = 16.0 * pParam->B3u0temp * model->B3vtm / T0
                * ScalingFactor;
            inst->B3gtau =  gtau_drift + gtau_diff;
        }

        if (model->B3capMod == 0) {
            if (vgd < 0.0) {   
                cgdo = pParam->B3cgdo;
                qgdo = pParam->B3cgdo * vgd;
            }
            else {
                cgdo = pParam->B3cgdo;
                qgdo =  pParam->B3cgdo * vgd;
            }

            if (vgs < 0.0) {   
                cgso = pParam->B3cgso;
                qgso = pParam->B3cgso * vgs;
            }
            else {
                cgso = pParam->B3cgso;
                qgso =  pParam->B3cgso * vgs;
            }
        }
        else if (model->B3capMod == 1) {
            if (vgd < 0.0) {
                T1 = sqrt(1.0 - 4.0 * vgd / pParam->B3ckappa);
                cgdo = pParam->B3cgdo + pParam->B3weffCV
                    * pParam->B3cgdl / T1;
                qgdo = pParam->B3cgdo * vgd - pParam->B3weffCV * 0.5
                    * pParam->B3cgdl * pParam->B3ckappa * (T1 - 1.0);
            }
            else {
                cgdo = pParam->B3cgdo + pParam->B3weffCV
                    * pParam->B3cgdl;
                qgdo = (pParam->B3weffCV * pParam->B3cgdl
                    + pParam->B3cgdo) * vgd;
            }

            if (vgs < 0.0) {
                T1 = sqrt(1.0 - 4.0 * vgs / pParam->B3ckappa);
                cgso = pParam->B3cgso + pParam->B3weffCV
                    * pParam->B3cgsl / T1;
                qgso = pParam->B3cgso * vgs - pParam->B3weffCV * 0.5
                    * pParam->B3cgsl * pParam->B3ckappa * (T1 - 1.0);
            }
            else {
                cgso = pParam->B3cgso + pParam->B3weffCV
                    * pParam->B3cgsl;
                qgso = (pParam->B3weffCV * pParam->B3cgsl
                    + pParam->B3cgso) * vgs;
            }
        }
        else {
            T0 = vgd + DELTA_1;
            T1 = sqrt(T0 * T0 + 4.0 * DELTA_1);
            T2 = 0.5 * (T0 - T1);

            T3 = pParam->B3weffCV * pParam->B3cgdl;
            T4 = sqrt(1.0 - 4.0 * T2 / pParam->B3ckappa);
            cgdo = pParam->B3cgdo + T3 - T3 * (1.0 - 1.0 / T4)
                * (0.5 - 0.5 * T0 / T1);
            qgdo = (pParam->B3cgdo + T3) * vgd - T3 * (T2
                + 0.5 * pParam->B3ckappa * (T4 - 1.0));

            T0 = vgs + DELTA_1;
            T1 = sqrt(T0 * T0 + 4.0 * DELTA_1);
            T2 = 0.5 * (T0 - T1);
            T3 = pParam->B3weffCV * pParam->B3cgsl;
            T4 = sqrt(1.0 - 4.0 * T2 / pParam->B3ckappa);
            cgso = pParam->B3cgso + T3 - T3 * (1.0 - 1.0 / T4)
                * (0.5 - 0.5 * T0 / T1);
            qgso = (pParam->B3cgso + T3) * vgs - T3 * (T2
                + 0.5 * pParam->B3ckappa * (T4 - 1.0));
        }

        inst->B3cgdo = cgdo;
        inst->B3cgso = cgso;

        ag0 = ckt->CKTag[0];
        if (inst->B3mode > 0) {
            if (inst->B3nqsMod == 0) {
                gcggb = (inst->B3cggb + cgdo + cgso
                    + pParam->B3cgbo ) * ag0;
                gcgdb = (inst->B3cgdb - cgdo) * ag0;
                gcgsb = (inst->B3cgsb - cgso) * ag0;

                gcdgb = (inst->B3cdgb - cgdo) * ag0;
                gcddb = (inst->B3cddb + inst->B3capbd + cgdo) * ag0;
                gcdsb = inst->B3cdsb * ag0;

                gcsgb = -(inst->B3cggb + inst->B3cbgb
                    + inst->B3cdgb + cgso) * ag0;
                gcsdb = -(inst->B3cgdb + inst->B3cbdb
                    + inst->B3cddb) * ag0;
                gcssb = (inst->B3capbs + cgso - (inst->B3cgsb
                    + inst->B3cbsb + inst->B3cdsb)) * ag0;

                gcbgb = (inst->B3cbgb - pParam->B3cgbo) * ag0;
                gcbdb = (inst->B3cbdb - inst->B3capbd) * ag0;
                gcbsb = (inst->B3cbsb - inst->B3capbs) * ag0;

                qgd = qgdo;
                qgs = qgso;
                qgb = pParam->B3cgbo * vgb;
                qgate += qgd + qgs + qgb;
                qbulk -= qgb;
                qdrn -= qgd;
                qsrc = -(qgate + qbulk + qdrn);

                ggtg = ggtd = ggtb = ggts = 0.0;
                sxpart = 0.6;
                dxpart = 0.4;
                ddxpart_dVd = ddxpart_dVg = ddxpart_dVb = ddxpart_dVs = 0.0;
                dsxpart_dVd = dsxpart_dVg = dsxpart_dVb = dsxpart_dVs = 0.0;
            }
            else {
                if (qcheq > 0.0)
                    T0 = pParam->B3tconst * qdef * ScalingFactor;
                else
                    T0 = -pParam->B3tconst * qdef * ScalingFactor;
                ggtg = inst->B3gtg = T0 * inst->B3cqgb;
                ggtd = inst->B3gtd = T0 * inst->B3cqdb;
                ggts = inst->B3gts = T0 * inst->B3cqsb;
                ggtb = inst->B3gtb = T0 * inst->B3cqbb;
                gqdef = ScalingFactor * ag0;

                gcqgb = inst->B3cqgb * ag0;
                gcqdb = inst->B3cqdb * ag0;
                gcqsb = inst->B3cqsb * ag0;
                gcqbb = inst->B3cqbb * ag0;

                gcggb = (cgdo + cgso + pParam->B3cgbo ) * ag0;
                gcgdb = -cgdo * ag0;
                gcgsb = -cgso * ag0;

                gcdgb = -cgdo * ag0;
                gcddb = (inst->B3capbd + cgdo) * ag0;
                gcdsb = 0.0;

                gcsgb = -cgso * ag0;
                gcsdb = 0.0;
                gcssb = (inst->B3capbs + cgso) * ag0;

                gcbgb = -pParam->B3cgbo * ag0;
                gcbdb = -inst->B3capbd * ag0;
                gcbsb = -inst->B3capbs * ag0;

                CoxWL = model->B3cox * pParam->B3weffCV
                    * pParam->B3leffCV;
                if (fabs(qcheq) <= 1.0e-5 * CoxWL) {
                    if (model->B3xpart < 0.5) {
                        dxpart = 0.4;
                    }
                    else if (model->B3xpart > 0.5) {
                        dxpart = 0.0;
                    }
                    else {
                        dxpart = 0.5;
                    }
                    ddxpart_dVd = ddxpart_dVg = ddxpart_dVb
                          = ddxpart_dVs = 0.0;
                }
                else {
                    dxpart = qdrn / qcheq;
                    Cdd = inst->B3cddb;
                    Csd = -(inst->B3cgdb + inst->B3cddb
                        + inst->B3cbdb);
                    ddxpart_dVd = (Cdd - dxpart * (Cdd + Csd)) / qcheq;
                    Cdg = inst->B3cdgb;
                    Csg = -(inst->B3cggb + inst->B3cdgb
                        + inst->B3cbgb);
                    ddxpart_dVg = (Cdg - dxpart * (Cdg + Csg)) / qcheq;

                    Cds = inst->B3cdsb;
                    Css = -(inst->B3cgsb + inst->B3cdsb
                        + inst->B3cbsb);
                    ddxpart_dVs = (Cds - dxpart * (Cds + Css)) / qcheq;

                    ddxpart_dVb = -(ddxpart_dVd + ddxpart_dVg + ddxpart_dVs);
                }
                sxpart = 1.0 - dxpart;
                dsxpart_dVd = -ddxpart_dVd;
                dsxpart_dVg = -ddxpart_dVg;
                dsxpart_dVs = -ddxpart_dVs;
                dsxpart_dVb = -(dsxpart_dVd + dsxpart_dVg + dsxpart_dVs);

                qgd = qgdo;
                qgs = qgso;
                qgb = pParam->B3cgbo * vgb;
                qgate = qgd + qgs + qgb;
                qbulk = -qgb;
                qdrn = -qgd;
                qsrc = -(qgate + qbulk + qdrn);
            }
        }
        else {
            if (inst->B3nqsMod == 0) {
                gcggb = (inst->B3cggb + cgdo + cgso
                    + pParam->B3cgbo ) * ag0;
                gcgdb = (inst->B3cgsb - cgdo) * ag0;
                gcgsb = (inst->B3cgdb - cgso) * ag0;

                gcdgb = -(inst->B3cggb + inst->B3cbgb
                    + inst->B3cdgb + cgdo) * ag0;
                gcddb = (inst->B3capbd + cgdo - (inst->B3cgsb
                    + inst->B3cbsb + inst->B3cdsb)) * ag0;
                gcdsb = -(inst->B3cgdb + inst->B3cbdb
                    + inst->B3cddb) * ag0;

                gcsgb = (inst->B3cdgb - cgso) * ag0;
                gcsdb = inst->B3cdsb * ag0;
                gcssb = (inst->B3cddb + inst->B3capbs + cgso) * ag0;

                gcbgb = (inst->B3cbgb - pParam->B3cgbo) * ag0;
                gcbdb = (inst->B3cbsb - inst->B3capbd) * ag0;
                gcbsb = (inst->B3cbdb - inst->B3capbs) * ag0;

                qgd = qgdo;
                qgs = qgso;
                qgb = pParam->B3cgbo * vgb;
                qgate += qgd + qgs + qgb;
                qbulk -= qgb;
                qsrc = qdrn - qgs;
                qdrn = -(qgate + qbulk + qsrc);

                ggtg = ggtd = ggtb = ggts = 0.0;
                sxpart = 0.4;
                dxpart = 0.6;
                ddxpart_dVd = ddxpart_dVg = ddxpart_dVb = ddxpart_dVs = 0.0;
                dsxpart_dVd = dsxpart_dVg = dsxpart_dVb = dsxpart_dVs = 0.0;
            }
            else {
                if (qcheq > 0.0)
                    T0 = pParam->B3tconst * qdef * ScalingFactor;
                else
                    T0 = -pParam->B3tconst * qdef * ScalingFactor;
                ggtg = inst->B3gtg = T0 * inst->B3cqgb;
                ggts = inst->B3gtd = T0 * inst->B3cqdb;
                ggtd = inst->B3gts = T0 * inst->B3cqsb;
                ggtb = inst->B3gtb = T0 * inst->B3cqbb;
                gqdef = ScalingFactor * ag0;

                gcqgb = inst->B3cqgb * ag0;
                gcqdb = inst->B3cqsb * ag0;
                gcqsb = inst->B3cqdb * ag0;
                gcqbb = inst->B3cqbb * ag0;

                gcggb = (cgdo + cgso + pParam->B3cgbo) * ag0;
                gcgdb = -cgdo * ag0;
                gcgsb = -cgso * ag0;

                gcdgb = -cgdo * ag0;
                gcddb = (inst->B3capbd + cgdo) * ag0;
                gcdsb = 0.0;

                gcsgb = -cgso * ag0;
                gcsdb = 0.0;
                gcssb = (inst->B3capbs + cgso) * ag0;

                gcbgb = -pParam->B3cgbo * ag0;
                gcbdb = -inst->B3capbd * ag0;
                gcbsb = -inst->B3capbs * ag0;

                CoxWL = model->B3cox * pParam->B3weffCV
                    * pParam->B3leffCV;
                if (fabs(qcheq) <= 1.0e-5 * CoxWL) {
                    if (model->B3xpart < 0.5) {
                        sxpart = 0.4;
                    }
                    else if (model->B3xpart > 0.5) {
                        sxpart = 0.0;
                    }
                    else {
                        sxpart = 0.5;
                    }
                    dsxpart_dVd = dsxpart_dVg = dsxpart_dVb
                        = dsxpart_dVs = 0.0;
                }
                else {
                    sxpart = qdrn / qcheq;
                    Css = inst->B3cddb;
                    Cds = -(inst->B3cgdb + inst->B3cddb
                        + inst->B3cbdb);
                    dsxpart_dVs = (Css - sxpart * (Css + Cds)) / qcheq;
                    Csg = inst->B3cdgb;
                    Cdg = -(inst->B3cggb + inst->B3cdgb
                        + inst->B3cbgb);
                    dsxpart_dVg = (Csg - sxpart * (Csg + Cdg)) / qcheq;

                    Csd = inst->B3cdsb;
                    Cdd = -(inst->B3cgsb + inst->B3cdsb
                        + inst->B3cbsb);
                    dsxpart_dVd = (Csd - sxpart * (Csd + Cdd)) / qcheq;

                    dsxpart_dVb = -(dsxpart_dVd + dsxpart_dVg + dsxpart_dVs);
                }
                dxpart = 1.0 - sxpart;
                ddxpart_dVd = -dsxpart_dVd;
                ddxpart_dVg = -dsxpart_dVg;
                ddxpart_dVs = -dsxpart_dVs;
                ddxpart_dVb = -(ddxpart_dVd + ddxpart_dVg + ddxpart_dVs);

                qgd = qgdo;
                qgs = qgso;
                qgb = pParam->B3cgbo * vgb;
                qgate = qgd + qgs + qgb;
                qbulk = -qgb;
                qsrc = -qgs;
                qdrn = -(qgate + qbulk + qsrc);
            }
        }

        cqdef = cqcheq = 0.0;
        if (ByPass) goto line860;

        *(ckt->CKTstate0 + inst->B3qg) = qgate;
        *(ckt->CKTstate0 + inst->B3qd) = qdrn
            - *(ckt->CKTstate0 + inst->B3qbd);
        *(ckt->CKTstate0 + inst->B3qb) = qbulk
            + *(ckt->CKTstate0 + inst->B3qbd)
            + *(ckt->CKTstate0 + inst->B3qbs);

        if (inst->B3nqsMod) {
            *(ckt->CKTstate0 + inst->B3qcdump) = qdef * ScalingFactor;
            *(ckt->CKTstate0 + inst->B3qcheq) = qcheq;
        }

        // store small signal parameters
        if (ckt->CKTmode & MODEINITSMSIG) {
            goto line1000;
        }
        if (!ChargeComputationNeeded)
            goto line850;

        if (ckt->CKTmode & MODEINITTRAN) {
            *(ckt->CKTstate1 + inst->B3qb) =
                *(ckt->CKTstate0 + inst->B3qb);
            *(ckt->CKTstate1 + inst->B3qg) =
                *(ckt->CKTstate0 + inst->B3qg);
            *(ckt->CKTstate1 + inst->B3qd) =
                *(ckt->CKTstate0 + inst->B3qd);
            if (inst->B3nqsMod) {
                *(ckt->CKTstate1 + inst->B3qcheq) =
                    *(ckt->CKTstate0 + inst->B3qcheq);
                *(ckt->CKTstate1 + inst->B3qcdump) =
                    *(ckt->CKTstate0 + inst->B3qcdump);
            }
        }

        ckt->integrate(inst->B3qb);
        ckt->integrate(inst->B3qg);
        ckt->integrate(inst->B3qd);

        if (inst->B3nqsMod) {
            ckt->integrate(inst->B3qcdump);
            ckt->integrate(inst->B3qcheq);
        }

        goto line860;

line850:
        // initialize to zero charge conductance and current
        ceqqg = ceqqb = ceqqd = 0.0;
        cqcheq = cqdef = 0.0;

        gcdgb = gcddb = gcdsb = 0.0;
        gcsgb = gcsdb = gcssb = 0.0;
        gcggb = gcgdb = gcgsb = 0.0;
        gcbgb = gcbdb = gcbsb = 0.0;

        gqdef = gcqgb = gcqdb = gcqsb = gcqbb = 0.0;
        ggtg = ggtd = ggtb = ggts = 0.0;
        sxpart = (1.0 - (dxpart = (inst->B3mode > 0) ? 0.4 : 0.6));
        ddxpart_dVd = ddxpart_dVg = ddxpart_dVb = ddxpart_dVs = 0.0;
        dsxpart_dVd = dsxpart_dVg = dsxpart_dVb = dsxpart_dVs = 0.0;

        if (inst->B3nqsMod)
            inst->B3gtau = 16.0 * pParam->B3u0temp * model->B3vtm 
                / pParam->B3leffCV / pParam->B3leffCV * ScalingFactor;
        else
            inst->B3gtau = 0.0;

        goto line900;
            
line860:
        // evaluate equivalent charge current

        cqgate = *(ckt->CKTstate0 + inst->B3cqg);
        cqbulk = *(ckt->CKTstate0 + inst->B3cqb);
        cqdrn = *(ckt->CKTstate0 + inst->B3cqd);

        ceqqg = cqgate - gcggb * vgb + gcgdb * vbd + gcgsb * vbs;
        ceqqb = cqbulk - gcbgb * vgb + gcbdb * vbd + gcbsb * vbs;
        ceqqd = cqdrn - gcdgb * vgb + gcddb * vbd + gcdsb * vbs;

        if (inst->B3nqsMod) {
            T0 = ggtg * vgb - ggtd * vbd - ggts * vbs;
            ceqqg += T0;
            T1 = qdef * inst->B3gtau;
            ceqqd -= dxpart * T0 + T1 * (ddxpart_dVg * vgb - ddxpart_dVd
                * vbd - ddxpart_dVs * vbs);
            cqdef = *(ckt->CKTstate0 + inst->B3cqcdump) - gqdef * qdef;
            cqcheq = *(ckt->CKTstate0 + inst->B3cqcheq)
                - (gcqgb * vgb - gcqdb * vbd  - gcqsb * vbs) + T0;
        }

        if (ckt->CKTmode & MODEINITTRAN) {
            *(ckt->CKTstate1 + inst->B3cqb) =
                *(ckt->CKTstate0 + inst->B3cqb);
            *(ckt->CKTstate1 + inst->B3cqg) =
                *(ckt->CKTstate0 + inst->B3cqg);
            *(ckt->CKTstate1 + inst->B3cqd) =
                *(ckt->CKTstate0 + inst->B3cqd);

            if (inst->B3nqsMod) {
                *(ckt->CKTstate1 + inst->B3cqcheq) =
                    *(ckt->CKTstate0 + inst->B3cqcheq);
                *(ckt->CKTstate1 + inst->B3cqcdump) =
                    *(ckt->CKTstate0 + inst->B3cqcdump);
            }
        }

        //
        //  load current vector
        //
line900:

        if (inst->B3mode >= 0) {
            Gm = inst->B3gm;
            Gmbs = inst->B3gmbs;
            FwdSum = Gm + Gmbs;
            RevSum = 0.0;
            cdreq = model->B3type * (cdrain - inst->B3gds * vds
                - Gm * vgs - Gmbs * vbs);

            ceqbd = -model->B3type * (inst->B3csub 
                - inst->B3gbds * vds - inst->B3gbgs * vgs
                - inst->B3gbbs * vbs);
            ceqbs = 0.0;

            gbbdp = -inst->B3gbds;
            gbbsp = (inst->B3gbds + inst->B3gbgs + inst->B3gbbs);

            gbdpg = inst->B3gbgs;
            gbdpdp = inst->B3gbds;
            gbdpb = inst->B3gbbs;
            gbdpsp = -(gbdpg + gbdpdp + gbdpb);

            gbspg = 0.0;
            gbspdp = 0.0;
            gbspb = 0.0;
            gbspsp = 0.0;
        }
        else {
            Gm = -inst->B3gm;
            Gmbs = -inst->B3gmbs;
            FwdSum = 0.0;
            RevSum = -(Gm + Gmbs);
            cdreq = -model->B3type * (cdrain + inst->B3gds * vds
                + Gm * vgd + Gmbs * vbd);

            ceqbs = -model->B3type * (inst->B3csub 
                + inst->B3gbds * vds - inst->B3gbgs * vgd
                - inst->B3gbbs * vbd);
            ceqbd = 0.0;

            gbbsp = -inst->B3gbds;
            gbbdp = (inst->B3gbds + inst->B3gbgs + inst->B3gbbs);

            gbdpg = 0.0;
            gbdpsp = 0.0;
            gbdpb = 0.0;
            gbdpdp = 0.0;

            gbspg = inst->B3gbgs;
            gbspsp = inst->B3gbds;
            gbspb = inst->B3gbbs;
            gbspdp = -(gbspg + gbspsp + gbspb);
        }

#define MSC(xx) inst->B3m*(xx)

        if (model->B3type > 0) {
            ceqbs += (inst->B3cbs - inst->B3gbs * vbs);
            ceqbd += (inst->B3cbd - inst->B3gbd * vbd);

            // save for interpolation
            *(ckt->CKTstate0 + inst->B3a_cd) = MSC(inst->B3cd +
                *(ckt->CKTstate0 + inst->B3cqd) - inst->B3cbd);
            *(ckt->CKTstate0 + inst->B3a_cbs) = MSC(inst->B3cbs);
            *(ckt->CKTstate0 + inst->B3a_cbd) = MSC(inst->B3cbd);
            *(ckt->CKTstate0 + inst->B3a_cggb) = MSC(inst->B3cggb);
            *(ckt->CKTstate0 + inst->B3a_cgdb) = MSC(inst->B3cgdb);
            *(ckt->CKTstate0 + inst->B3a_cgsb) = MSC(inst->B3cgsb);
            *(ckt->CKTstate0 + inst->B3a_cdgb) = MSC(inst->B3cdgb);
            *(ckt->CKTstate0 + inst->B3a_cddb) = MSC(inst->B3cddb);
            *(ckt->CKTstate0 + inst->B3a_cdsb) = MSC(inst->B3cdsb);
            *(ckt->CKTstate0 + inst->B3a_cbgb) = MSC(inst->B3cbgb);
            *(ckt->CKTstate0 + inst->B3a_cbdb) = MSC(inst->B3cbdb);
            *(ckt->CKTstate0 + inst->B3a_cbsb) = MSC(inst->B3cbsb);
            *(ckt->CKTstate0 + inst->B3a_von) = inst->B3von;
            *(ckt->CKTstate0 + inst->B3a_vdsat) = inst->B3vdsat;
        }
        else {
            ceqbs -= (inst->B3cbs - inst->B3gbs * vbs);
            ceqbd -= (inst->B3cbd - inst->B3gbd * vbd);
            ceqqg = -ceqqg;
            ceqqb = -ceqqb;
            ceqqd = -ceqqd;
            cqdef = -cqdef;
            cqcheq = -cqcheq;

            // save for interpolation
            *(ckt->CKTstate0 + inst->B3a_cd) = MSC(-inst->B3cd -
                *(ckt->CKTstate0 + inst->B3cqd) + inst->B3cbd);
            *(ckt->CKTstate0 + inst->B3a_cbs) = -MSC(inst->B3cbs);
            *(ckt->CKTstate0 + inst->B3a_cbd) = -MSC(inst->B3cbd);
            *(ckt->CKTstate0 + inst->B3a_cggb) = -MSC(inst->B3cggb);
            *(ckt->CKTstate0 + inst->B3a_cgdb) = -MSC(inst->B3cgdb);
            *(ckt->CKTstate0 + inst->B3a_cgsb) = -MSC(inst->B3cgsb);
            *(ckt->CKTstate0 + inst->B3a_cdgb) = -MSC(inst->B3cdgb);
            *(ckt->CKTstate0 + inst->B3a_cddb) = -MSC(inst->B3cddb);
            *(ckt->CKTstate0 + inst->B3a_cdsb) = -MSC(inst->B3cdsb);
            *(ckt->CKTstate0 + inst->B3a_cbgb) = -MSC(inst->B3cbgb);
            *(ckt->CKTstate0 + inst->B3a_cbdb) = -MSC(inst->B3cbdb);
            *(ckt->CKTstate0 + inst->B3a_cbsb) = -MSC(inst->B3cbsb);
            *(ckt->CKTstate0 + inst->B3a_von) = -inst->B3von;
            *(ckt->CKTstate0 + inst->B3a_vdsat) = -inst->B3vdsat;
        }

        // save for interpolation
        *(ckt->CKTstate0 + inst->B3a_gm) = MSC(inst->B3gm);
        *(ckt->CKTstate0 + inst->B3a_gds) = MSC(inst->B3gds);
        *(ckt->CKTstate0 + inst->B3a_gmbs) = MSC(inst->B3gmbs);
        *(ckt->CKTstate0 + inst->B3a_gbd) = MSC(inst->B3gbd);
        *(ckt->CKTstate0 + inst->B3a_gbs) = MSC(inst->B3gbs);
        *(ckt->CKTstate0 + inst->B3a_capbd) = MSC(inst->B3capbd);
        *(ckt->CKTstate0 + inst->B3a_capbs) = MSC(inst->B3capbs);

        ckt->rhsadd(inst->B3gNode, -MSC(ceqqg));
        ckt->rhsadd(inst->B3bNode, -MSC(ceqbs + ceqbd + ceqqb));
        ckt->rhsadd(inst->B3dNodePrime, MSC(ceqbd - cdreq - ceqqd));
        ckt->rhsadd(inst->B3sNodePrime, MSC(cdreq + ceqbs + ceqqg +
            ceqqb + ceqqd));
        if (inst->B3nqsMod)
            ckt->rhsadd(inst->B3qNode, MSC(cqcheq - cqdef));

        //
        //  load y matrix
        //

        T1 = qdef * inst->B3gtau;
        ckt->ldadd(inst->B3DdPtr, MSC(inst->B3drainConductance));
        ckt->ldadd(inst->B3GgPtr, MSC(gcggb - ggtg));
        ckt->ldadd(inst->B3SsPtr, MSC(inst->B3sourceConductance));
        ckt->ldadd(inst->B3BbPtr, MSC(inst->B3gbd + inst->B3gbs
            - gcbgb - gcbdb - gcbsb - inst->B3gbbs));
        ckt->ldadd(inst->B3DPdpPtr, MSC(inst->B3drainConductance
            + inst->B3gds + inst->B3gbd
            + RevSum + gcddb + dxpart * ggtd 
            + T1 * ddxpart_dVd + gbdpdp));
        ckt->ldadd(inst->B3SPspPtr, MSC(inst->B3sourceConductance
            + inst->B3gds + inst->B3gbs
            + FwdSum + gcssb + sxpart * ggts
            + T1 * dsxpart_dVs + gbspsp));
        ckt->ldadd(inst->B3DdpPtr, -MSC(inst->B3drainConductance));
        ckt->ldadd(inst->B3GbPtr, -MSC(gcggb + gcgdb + gcgsb + ggtb));
        ckt->ldadd(inst->B3GdpPtr, MSC(gcgdb - ggtd));
        ckt->ldadd(inst->B3GspPtr, MSC(gcgsb - ggts));
        ckt->ldadd(inst->B3SspPtr, -MSC(inst->B3sourceConductance));
        ckt->ldadd(inst->B3BgPtr, MSC(gcbgb - inst->B3gbgs));
        ckt->ldadd(inst->B3BdpPtr, MSC(gcbdb - inst->B3gbd + gbbdp));
        ckt->ldadd(inst->B3BspPtr, MSC(gcbsb - inst->B3gbs + gbbsp));
        ckt->ldadd(inst->B3DPdPtr, -MSC(inst->B3drainConductance));
        ckt->ldadd(inst->B3DPgPtr, MSC(Gm + gcdgb + dxpart * ggtg 
            + T1 * ddxpart_dVg + gbdpg));
        ckt->ldadd(inst->B3DPbPtr, -MSC(inst->B3gbd - Gmbs + gcdgb + gcddb
            + gcdsb - dxpart * ggtb - T1 * ddxpart_dVb - gbdpb));
        ckt->ldadd(inst->B3DPspPtr, -MSC(inst->B3gds + FwdSum - gcdsb
            - dxpart * ggts - T1 * ddxpart_dVs - gbdpsp));
        ckt->ldadd(inst->B3SPgPtr, MSC(gcsgb - Gm + sxpart * ggtg 
            + T1 * dsxpart_dVg + gbspg));
        ckt->ldadd(inst->B3SPsPtr, -MSC(inst->B3sourceConductance));
        ckt->ldadd(inst->B3SPbPtr, -MSC(inst->B3gbs + Gmbs + gcsgb + gcsdb
            + gcssb - sxpart * ggtb - T1 * dsxpart_dVb - gbspb));
        ckt->ldadd(inst->B3SPdpPtr, -MSC(inst->B3gds + RevSum - gcsdb
            - sxpart * ggtd - T1 * dsxpart_dVd - gbspdp));

        if (inst->B3nqsMod) {
            ckt->ldadd(inst->B3QqPtr, MSC(gqdef + inst->B3gtau));

            ckt->ldadd(inst->B3DPqPtr, MSC(dxpart * inst->B3gtau));
            ckt->ldadd(inst->B3SPqPtr, MSC(sxpart * inst->B3gtau));
            ckt->ldadd(inst->B3GqPtr, -MSC(inst->B3gtau));

            ckt->ldadd(inst->B3QgPtr, MSC(ggtg - gcqgb));
            ckt->ldadd(inst->B3QdpPtr, MSC(ggtd - gcqdb));
            ckt->ldadd(inst->B3QspPtr, MSC(ggts - gcqsb));
            ckt->ldadd(inst->B3QbPtr, MSC(ggtb - gcqbb));
        }

line1000:  ;

// SRW    }  // End of Mosfet Instance
// SRW}   // End of Model Instance

return (OK);
}

