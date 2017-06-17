
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
 $Id: b3acld.cc,v 1.3 2011/12/18 01:15:23 stevew Exp $
 *========================================================================*/

/**** BSIM3v3.3.0 beta, Released by Xuemei Xi 07/29/2005 ****/

/**********
 * Copyright 2004 Regents of the University of California. All rights reserved.
 * File: b3acld.c of BSIM3v3.3.0
 * Author: 1995 Min-Chie Jeng and Mansun Chan
 * Author: 1997-1999 Weidong Liu.
 * Author: 2001  Xuemei Xi
 **********/

#include "b3defs.h"
#include "gencurrent.h"

#define BSIM3nextModel      next()
#define BSIM3nextInstance   next()
#define BSIM3instances      inst()


int
BSIM3dev::acLoad(sGENmodel *genmod, sCKT *ckt)
{
    sBSIM3model *model = static_cast<sBSIM3model*>(genmod);
    sBSIM3instance *here;

    double xcggb, xcgdb, xcgsb, xcbgb, xcbdb, xcbsb, xcddb, xcssb, xcdgb;
    double gdpr, gspr, gds, gbd, gbs, capbd, capbs, xcsgb, xcdsb, xcsdb;
    double cggb, cgdb, cgsb, cbgb, cbdb, cbsb, cddb, cdgb, cdsb, omega;
    double GSoverlapCap, GDoverlapCap, GBoverlapCap, FwdSum, RevSum, Gm, Gmbs;
    double dxpart, sxpart, xgtg, xgtd, xgts, xgtb, xcqgb, xcqdb, xcqsb, xcqbb;
    double gbspsp, gbbdp, gbbsp, gbspg, gbspb;
    double gbspdp, gbdpdp, gbdpg, gbdpb, gbdpsp;
    double ddxpart_dVd, ddxpart_dVg, ddxpart_dVb, ddxpart_dVs;
    double dsxpart_dVd, dsxpart_dVg, dsxpart_dVb, dsxpart_dVs;
    double T1, CoxWL, qcheq, Cdg, Cdd, Cds, /*Cdb,*/ Csg, Csd, Css/*, Csb*/;
    double ScalingFactor = 1.0e-9;
    /* For ACNQSMOD */
    double T0, T2, T3, gmr, gmbsr, /*gdsr,*/ gmi, gmbsi, gdsi;
    double Cddr, Cdgr, Cdsr, Csdr, Csgr, Cssr, Cgdr, Cggr, Cgsr;
    double Cddi, Cdgi, Cdsi, Cdbi, Csdi, Csgi, Cssi, Csbi;
    double Cgdi, Cggi, Cgsi, Cgbi, Gmi, Gmbsi, FwdSumi, RevSumi;
    double xcdgbi, xcsgbi, xcddbi, xcdsbi, xcsdbi, xcssbi, xcdbbi;
    double xcsbbi, xcggbi, xcgdbi, xcgsbi, xcgbbi;

    omega = ckt->CKTomega;
    for (; model != NULL; model = model->BSIM3nextModel)
    {
        for (here = model->BSIM3instances; here!= NULL;
                here = here->BSIM3nextInstance)
        {
            Csd = -(here->BSIM3cddb + here->BSIM3cgdb + here->BSIM3cbdb);
            Csg = -(here->BSIM3cdgb + here->BSIM3cggb + here->BSIM3cbgb);
            Css = -(here->BSIM3cdsb + here->BSIM3cgsb + here->BSIM3cbsb);

            if (here->BSIM3acnqsMod)
            {
                T0 = omega * here->BSIM3taunet;
                T1 = T0 * T0;
                T2 = 1.0 / (1.0 + T1);
                T3 = T0 * T2;

                gmr = here->BSIM3gm * T2;
                gmbsr = here->BSIM3gmbs * T2;
                gds = here->BSIM3gds * T2;

                gmi = -here->BSIM3gm * T3;
                gmbsi = -here->BSIM3gmbs * T3;
                gdsi = -here->BSIM3gds * T3;

                Cddr = here->BSIM3cddb * T2;
                Cdgr = here->BSIM3cdgb * T2;
                Cdsr = here->BSIM3cdsb * T2;

                Cddi = here->BSIM3cddb * T3 * omega;
                Cdgi = here->BSIM3cdgb * T3 * omega;
                Cdsi = here->BSIM3cdsb * T3 * omega;
                Cdbi = -(Cddi + Cdgi + Cdsi);

                Csdr = Csd * T2;
                Csgr = Csg * T2;
                Cssr = Css * T2;

                Csdi = Csd * T3 * omega;
                Csgi = Csg * T3 * omega;
                Cssi = Css * T3 * omega;
                Csbi = -(Csdi + Csgi + Cssi);

                Cgdr = -(Cddr + Csdr + here->BSIM3cbdb);
                Cggr = -(Cdgr + Csgr + here->BSIM3cbgb);
                Cgsr = -(Cdsr + Cssr + here->BSIM3cbsb);

                Cgdi = -(Cddi + Csdi);
                Cggi = -(Cdgi + Csgi);
                Cgsi = -(Cdsi + Cssi);
                Cgbi = -(Cgdi + Cggi + Cgsi);
            }
            else /* QS */
            {
                gmr = here->BSIM3gm;
                gmbsr = here->BSIM3gmbs;
                gds = here->BSIM3gds;
                gmi = gmbsi = gdsi = 0.0;

                Cddr = here->BSIM3cddb;
                Cdgr = here->BSIM3cdgb;
                Cdsr = here->BSIM3cdsb;
                Cddi = Cdgi = Cdsi = Cdbi = 0.0;

                Csdr = Csd;
                Csgr = Csg;
                Cssr = Css;
                Csdi = Csgi = Cssi = Csbi = 0.0;

                Cgdr = here->BSIM3cgdb;
                Cggr = here->BSIM3cggb;
                Cgsr = here->BSIM3cgsb;
                Cgdi = Cggi = Cgsi = Cgbi = 0.0;
            }

            if (here->BSIM3mode >= 0)
            {
                Gm = gmr;
                Gmbs = gmbsr;
                FwdSum = Gm + Gmbs;
                RevSum = 0.0;
                Gmi = gmi;
                Gmbsi = gmbsi;
                FwdSumi = Gmi + Gmbsi;
                RevSumi = 0.0;

                gbbdp = -here->BSIM3gbds;
                gbbsp = here->BSIM3gbds + here->BSIM3gbgs + here->BSIM3gbbs;

                gbdpg = here->BSIM3gbgs;
                gbdpb = here->BSIM3gbbs;
                gbdpdp = here->BSIM3gbds;
                gbdpsp = -(gbdpg + gbdpb + gbdpdp);

                gbspdp = 0.0;
                gbspg = 0.0;
                gbspb = 0.0;
                gbspsp = 0.0;

                if (here->BSIM3nqsMod == 0 || here->BSIM3acnqsMod == 1)
                {
                    cggb = Cggr;
                    cgsb = Cgsr;
                    cgdb = Cgdr;

                    cbgb = here->BSIM3cbgb;
                    cbsb = here->BSIM3cbsb;
                    cbdb = here->BSIM3cbdb;

                    cdgb = Cdgr;
                    cdsb = Cdsr;
                    cddb = Cddr;

                    xgtg = xgtd = xgts = xgtb = 0.0;
                    sxpart = 0.6;
                    dxpart = 0.4;
                    ddxpart_dVd = ddxpart_dVg = ddxpart_dVb
                                                = ddxpart_dVs = 0.0;
                    dsxpart_dVd = dsxpart_dVg = dsxpart_dVb
                                                = dsxpart_dVs = 0.0;
                }
                else
                {
                    cggb = cgdb = cgsb = 0.0;
                    cbgb = cbdb = cbsb = 0.0;
                    cdgb = cddb = cdsb = 0.0;

                    xgtg = here->BSIM3gtg;
                    xgtd = here->BSIM3gtd;
                    xgts = here->BSIM3gts;
                    xgtb = here->BSIM3gtb;

                    xcqgb = here->BSIM3cqgb * omega;
                    xcqdb = here->BSIM3cqdb * omega;
                    xcqsb = here->BSIM3cqsb * omega;
                    xcqbb = here->BSIM3cqbb * omega;

                    CoxWL = model->BSIM3cox * here->pParam->BSIM3weffCV
                            * here->pParam->BSIM3leffCV;
                    qcheq = -(here->BSIM3qgate + here->BSIM3qbulk);
                    if (fabs(qcheq) <= 1.0e-5 * CoxWL)
                    {
                        if (model->BSIM3xpart < 0.5)
                        {
                            dxpart = 0.4;
                        }
                        else if (model->BSIM3xpart > 0.5)
                        {
                            dxpart = 0.0;
                        }
                        else
                        {
                            dxpart = 0.5;
                        }
                        ddxpart_dVd = ddxpart_dVg = ddxpart_dVb
                                                    = ddxpart_dVs = 0.0;
                    }
                    else
                    {
                        dxpart = here->BSIM3qdrn / qcheq;
                        Cdd = here->BSIM3cddb;
                        Csd = -(here->BSIM3cgdb + here->BSIM3cddb
                                + here->BSIM3cbdb);
                        ddxpart_dVd = (Cdd - dxpart * (Cdd + Csd)) / qcheq;
                        Cdg = here->BSIM3cdgb;
                        Csg = -(here->BSIM3cggb + here->BSIM3cdgb
                                + here->BSIM3cbgb);
                        ddxpart_dVg = (Cdg - dxpart * (Cdg + Csg)) / qcheq;

                        Cds = here->BSIM3cdsb;
                        Css = -(here->BSIM3cgsb + here->BSIM3cdsb
                                + here->BSIM3cbsb);
                        ddxpart_dVs = (Cds - dxpart * (Cds + Css)) / qcheq;

                        ddxpart_dVb = -(ddxpart_dVd + ddxpart_dVg
                                        + ddxpart_dVs);
                    }
                    sxpart = 1.0 - dxpart;
                    dsxpart_dVd = -ddxpart_dVd;
                    dsxpart_dVg = -ddxpart_dVg;
                    dsxpart_dVs = -ddxpart_dVs;
                    dsxpart_dVb = -(dsxpart_dVd + dsxpart_dVg + dsxpart_dVs);
                }
                xcdgbi = Cdgi;
                xcsgbi = Csgi;
                xcddbi = Cddi;
                xcdsbi = Cdsi;
                xcsdbi = Csdi;
                xcssbi = Cssi;
                xcdbbi = Cdbi;
                xcsbbi = Csbi;
                xcggbi = Cggi;
                xcgdbi = Cgdi;
                xcgsbi = Cgsi;
                xcgbbi = Cgbi;
            }
            else
            {
                Gm = -gmr;
                Gmbs = -gmbsr;
                FwdSum = 0.0;
                RevSum = -(Gm + Gmbs);
                Gmi = -gmi;
                Gmbsi = -gmbsi;
                FwdSumi = 0.0;
                RevSumi = -(Gmi + Gmbsi);

                gbbsp = -here->BSIM3gbds;
                gbbdp = here->BSIM3gbds + here->BSIM3gbgs + here->BSIM3gbbs;

                gbdpg = 0.0;
                gbdpsp = 0.0;
                gbdpb = 0.0;
                gbdpdp = 0.0;

                gbspg = here->BSIM3gbgs;
                gbspsp = here->BSIM3gbds;
                gbspb = here->BSIM3gbbs;
                gbspdp = -(gbspg + gbspsp + gbspb);

                if (here->BSIM3nqsMod == 0 || here->BSIM3acnqsMod == 1)
                {
                    cggb = Cggr;
                    cgsb = Cgdr;
                    cgdb = Cgsr;

                    cbgb = here->BSIM3cbgb;
                    cbsb = here->BSIM3cbdb;
                    cbdb = here->BSIM3cbsb;

                    cdgb = -(Cdgr + cggb + cbgb);
                    cdsb = -(Cddr + cgsb + cbsb);
                    cddb = -(Cdsr + cgdb + cbdb);

                    xgtg = xgtd = xgts = xgtb = 0.0;
                    sxpart = 0.4;
                    dxpart = 0.6;
                    ddxpart_dVd = ddxpart_dVg = ddxpart_dVb
                                                = ddxpart_dVs = 0.0;
                    dsxpart_dVd = dsxpart_dVg = dsxpart_dVb
                                                = dsxpart_dVs = 0.0;
                }
                else
                {
                    cggb = cgdb = cgsb = 0.0;
                    cbgb = cbdb = cbsb = 0.0;
                    cdgb = cddb = cdsb = 0.0;

                    xgtg = here->BSIM3gtg;
                    xgtd = here->BSIM3gts;
                    xgts = here->BSIM3gtd;
                    xgtb = here->BSIM3gtb;

                    xcqgb = here->BSIM3cqgb * omega;
                    xcqdb = here->BSIM3cqsb * omega;
                    xcqsb = here->BSIM3cqdb * omega;
                    xcqbb = here->BSIM3cqbb * omega;

                    CoxWL = model->BSIM3cox * here->pParam->BSIM3weffCV
                            * here->pParam->BSIM3leffCV;
                    qcheq = -(here->BSIM3qgate + here->BSIM3qbulk);
                    if (fabs(qcheq) <= 1.0e-5 * CoxWL)
                    {
                        if (model->BSIM3xpart < 0.5)
                        {
                            sxpart = 0.4;
                        }
                        else if (model->BSIM3xpart > 0.5)
                        {
                            sxpart = 0.0;
                        }
                        else
                        {
                            sxpart = 0.5;
                        }
                        dsxpart_dVd = dsxpart_dVg = dsxpart_dVb
                                                    = dsxpart_dVs = 0.0;
                    }
                    else
                    {
                        sxpart = here->BSIM3qdrn / qcheq;
                        Css = here->BSIM3cddb;
                        Cds = -(here->BSIM3cgdb + here->BSIM3cddb
                                + here->BSIM3cbdb);
                        dsxpart_dVs = (Css - sxpart * (Css + Cds)) / qcheq;
                        Csg = here->BSIM3cdgb;
                        Cdg = -(here->BSIM3cggb + here->BSIM3cdgb
                                + here->BSIM3cbgb);
                        dsxpart_dVg = (Csg - sxpart * (Csg + Cdg)) / qcheq;

                        Csd = here->BSIM3cdsb;
                        Cdd = -(here->BSIM3cgsb + here->BSIM3cdsb
                                + here->BSIM3cbsb);
                        dsxpart_dVd = (Csd - sxpart * (Csd + Cdd)) / qcheq;

                        dsxpart_dVb = -(dsxpart_dVd + dsxpart_dVg
                                        + dsxpart_dVs);
                    }
                    dxpart = 1.0 - sxpart;
                    ddxpart_dVd = -dsxpart_dVd;
                    ddxpart_dVg = -dsxpart_dVg;
                    ddxpart_dVs = -dsxpart_dVs;
                    ddxpart_dVb = -(ddxpart_dVd + ddxpart_dVg + ddxpart_dVs);
                }
                xcdgbi = Csgi;
                xcsgbi = Cdgi;
                xcddbi = Cssi;
                xcdsbi = Csdi;
                xcsdbi = Cdsi;
                xcssbi = Cddi;
                xcdbbi = Csbi;
                xcsbbi = Cdbi;
                xcggbi = Cggi;
                xcgdbi = Cgsi;
                xcgsbi = Cgdi;
                xcgbbi = Cgbi;
            }

            T1 = *(ckt->CKTstate0 + here->BSIM3qdef) * here->BSIM3gtau;
            gdpr = here->BSIM3drainConductance;
            gspr = here->BSIM3sourceConductance;
            gbd = here->BSIM3gbd;
            gbs = here->BSIM3gbs;
            capbd = here->BSIM3capbd;
            capbs = here->BSIM3capbs;

            GSoverlapCap = here->BSIM3cgso;
            GDoverlapCap = here->BSIM3cgdo;
            GBoverlapCap = here->pParam->BSIM3cgbo;

            xcdgb = (cdgb - GDoverlapCap) * omega;
            xcddb = (cddb + capbd + GDoverlapCap) * omega;
            xcdsb = cdsb * omega;
            xcsgb = -(cggb + cbgb + cdgb + GSoverlapCap) * omega;
            xcsdb = -(cgdb + cbdb + cddb) * omega;
            xcssb = (capbs + GSoverlapCap - (cgsb + cbsb + cdsb)) * omega;
            xcggb = (cggb + GDoverlapCap + GSoverlapCap + GBoverlapCap)
                    * omega;
            xcgdb = (cgdb - GDoverlapCap ) * omega;
            xcgsb = (cgsb - GSoverlapCap) * omega;
            xcbgb = (cbgb - GBoverlapCap) * omega;
            xcbdb = (cbdb - capbd ) * omega;
            xcbsb = (cbsb - capbs ) * omega;

            *(here->BSIM3GgPtr +1) += xcggb;
            *(here->BSIM3BbPtr +1) -= xcbgb + xcbdb + xcbsb;
            *(here->BSIM3DPdpPtr +1) += xcddb + gdsi + RevSumi;
            *(here->BSIM3SPspPtr +1) += xcssb + gdsi + FwdSumi;
            *(here->BSIM3GbPtr +1) -= xcggb + xcgdb + xcgsb;
            *(here->BSIM3GdpPtr +1) += xcgdb;
            *(here->BSIM3GspPtr +1) += xcgsb;
            *(here->BSIM3BgPtr +1) += xcbgb;
            *(here->BSIM3BdpPtr +1) += xcbdb;
            *(here->BSIM3BspPtr +1) += xcbsb;
            *(here->BSIM3DPgPtr +1) += xcdgb + Gmi;
            *(here->BSIM3DPbPtr +1) -= xcdgb + xcddb + xcdsb + Gmbsi;
            *(here->BSIM3DPspPtr +1) += xcdsb - gdsi - FwdSumi;
            *(here->BSIM3SPgPtr +1) += xcsgb - Gmi;
            *(here->BSIM3SPbPtr +1) -= xcsgb + xcsdb + xcssb - Gmbsi;
            *(here->BSIM3SPdpPtr +1) += xcsdb - gdsi - RevSumi;

            *(here->BSIM3DdPtr) += gdpr;
            *(here->BSIM3SsPtr) += gspr;
            *(here->BSIM3BbPtr) += gbd + gbs - here->BSIM3gbbs;
            *(here->BSIM3DPdpPtr) += gdpr + gds + gbd + RevSum + xcddbi
                                     + dxpart * xgtd + T1 * ddxpart_dVd + gbdpdp;
            *(here->BSIM3SPspPtr) += gspr + gds + gbs + FwdSum + xcssbi
                                     + sxpart * xgts + T1 * dsxpart_dVs + gbspsp;

            *(here->BSIM3DdpPtr) -= gdpr;
            *(here->BSIM3SspPtr) -= gspr;

            *(here->BSIM3BgPtr) -= here->BSIM3gbgs;
            *(here->BSIM3BdpPtr) -= gbd - gbbdp;
            *(here->BSIM3BspPtr) -= gbs - gbbsp;

            *(here->BSIM3DPdPtr) -= gdpr;
            *(here->BSIM3DPgPtr) += Gm + dxpart * xgtg + T1 * ddxpart_dVg
                                    + gbdpg + xcdgbi;
            *(here->BSIM3DPbPtr) -= gbd - Gmbs - dxpart * xgtb
                                    - T1 * ddxpart_dVb - gbdpb - xcdbbi;
            *(here->BSIM3DPspPtr) -= gds + FwdSum - dxpart * xgts
                                     - T1 * ddxpart_dVs - gbdpsp - xcdsbi;

            *(here->BSIM3SPgPtr) -= Gm - sxpart * xgtg - T1 * dsxpart_dVg
                                    - gbspg - xcsgbi;
            *(here->BSIM3SPsPtr) -= gspr;
            *(here->BSIM3SPbPtr) -= gbs + Gmbs - sxpart * xgtb
                                    - T1 * dsxpart_dVb - gbspb - xcsbbi;
            *(here->BSIM3SPdpPtr) -= gds + RevSum - sxpart * xgtd
                                     - T1 * dsxpart_dVd - gbspdp - xcsdbi;

            *(here->BSIM3GgPtr) -= xgtg - xcggbi;
            *(here->BSIM3GbPtr) -= xgtb - xcgbbi;
            *(here->BSIM3GdpPtr) -= xgtd - xcgdbi;
            *(here->BSIM3GspPtr) -= xgts - xcgsbi;

            if (here->BSIM3nqsMod)
            {
                if (here->BSIM3acnqsMod)
                {
                    (*(here->BSIM3QqPtr) += 1.0);
                    (*(here->BSIM3QgPtr) += 0.0);
                    (*(here->BSIM3QdpPtr) += 0.0);
                    (*(here->BSIM3QspPtr) += 0.0);
                    (*(here->BSIM3QbPtr) += 0.0);

                    (*(here->BSIM3DPqPtr) += 0.0);
                    (*(here->BSIM3SPqPtr) += 0.0);
                    (*(here->BSIM3GqPtr) += 0.0);

                }
                else
                {
                    *(here->BSIM3QqPtr +1) += omega * ScalingFactor;
                    *(here->BSIM3QgPtr +1) -= xcqgb;
                    *(here->BSIM3QdpPtr +1) -= xcqdb;
                    *(here->BSIM3QspPtr +1) -= xcqsb;
                    *(here->BSIM3QbPtr +1) -= xcqbb;

                    *(here->BSIM3QqPtr) += here->BSIM3gtau;

                    *(here->BSIM3DPqPtr) += dxpart * here->BSIM3gtau;
                    *(here->BSIM3SPqPtr) += sxpart * here->BSIM3gtau;
                    *(here->BSIM3GqPtr) -=  here->BSIM3gtau;

                    *(here->BSIM3QgPtr) +=  xgtg;
                    *(here->BSIM3QdpPtr) += xgtd;
                    *(here->BSIM3QspPtr) += xgts;
                    *(here->BSIM3QbPtr) += xgtb;
                }
            }

// SRW
            if (here->BSIM3adjoint)
            {
                BSIM3adj *adj = here->BSIM3adjoint;
                adj->matrix->clear();

                *(adj->BSIM3GgPtr +1) += xcggb;
                *(adj->BSIM3BbPtr +1) -= xcbgb + xcbdb + xcbsb;
                *(adj->BSIM3DPdpPtr +1) += xcddb + gdsi + RevSumi;
                *(adj->BSIM3SPspPtr +1) += xcssb + gdsi + FwdSumi;
                *(adj->BSIM3GbPtr +1) -= xcggb + xcgdb + xcgsb;
                *(adj->BSIM3GdpPtr +1) += xcgdb;
                *(adj->BSIM3GspPtr +1) += xcgsb;
                *(adj->BSIM3BgPtr +1) += xcbgb;
                *(adj->BSIM3BdpPtr +1) += xcbdb;
                *(adj->BSIM3BspPtr +1) += xcbsb;
                *(adj->BSIM3DPgPtr +1) += xcdgb + Gmi;
                *(adj->BSIM3DPbPtr +1) -= xcdgb + xcddb + xcdsb + Gmbsi;
                *(adj->BSIM3DPspPtr +1) += xcdsb - gdsi - FwdSumi;
                *(adj->BSIM3SPgPtr +1) += xcsgb - Gmi;
                *(adj->BSIM3SPbPtr +1) -= xcsgb + xcsdb + xcssb - Gmbsi;
                *(adj->BSIM3SPdpPtr +1) += xcsdb - gdsi - RevSumi;

                *(adj->BSIM3DdPtr) += gdpr;
                *(adj->BSIM3SsPtr) += gspr;
                *(adj->BSIM3BbPtr) += gbd + gbs - here->BSIM3gbbs;
                *(adj->BSIM3DPdpPtr) += gdpr + gds + gbd + RevSum + xcddbi
                                        + dxpart * xgtd + T1 * ddxpart_dVd + gbdpdp;
                *(adj->BSIM3SPspPtr) += gspr + gds + gbs + FwdSum + xcssbi
                                        + sxpart * xgts + T1 * dsxpart_dVs + gbspsp;

                *(adj->BSIM3DdpPtr) -= gdpr;
                *(adj->BSIM3SspPtr) -= gspr;

                *(adj->BSIM3BgPtr) -= here->BSIM3gbgs;
                *(adj->BSIM3BdpPtr) -= gbd - gbbdp;
                *(adj->BSIM3BspPtr) -= gbs - gbbsp;

                *(adj->BSIM3DPdPtr) -= gdpr;
                *(adj->BSIM3DPgPtr) += Gm + dxpart * xgtg + T1 * ddxpart_dVg
                                       + gbdpg + xcdgbi;
                *(adj->BSIM3DPbPtr) -= gbd - Gmbs - dxpart * xgtb
                                       - T1 * ddxpart_dVb - gbdpb - xcdbbi;
                *(adj->BSIM3DPspPtr) -= gds + FwdSum - dxpart * xgts
                                        - T1 * ddxpart_dVs - gbdpsp - xcdsbi;

                *(adj->BSIM3SPgPtr) -= Gm - sxpart * xgtg - T1 * dsxpart_dVg
                                       - gbspg - xcsgbi;
                *(adj->BSIM3SPsPtr) -= gspr;
                *(adj->BSIM3SPbPtr) -= gbs + Gmbs - sxpart * xgtb
                                       - T1 * dsxpart_dVb - gbspb - xcsbbi;
                *(adj->BSIM3SPdpPtr) -= gds + RevSum - sxpart * xgtd
                                        - T1 * dsxpart_dVd - gbspdp - xcsdbi;

                *(adj->BSIM3GgPtr) -= xgtg - xcggbi;
                *(adj->BSIM3GbPtr) -= xgtb - xcgbbi;
                *(adj->BSIM3GdpPtr) -= xgtd - xcgdbi;
                *(adj->BSIM3GspPtr) -= xgts - xcgsbi;

                if (here->BSIM3nqsMod)
                {
                    if (here->BSIM3acnqsMod)
                    {
                        (*(adj->BSIM3QqPtr) += 1.0);
                        (*(adj->BSIM3QgPtr) += 0.0);
                        (*(adj->BSIM3QdpPtr) += 0.0);
                        (*(adj->BSIM3QspPtr) += 0.0);
                        (*(adj->BSIM3QbPtr) += 0.0);

                        (*(adj->BSIM3DPqPtr) += 0.0);
                        (*(adj->BSIM3SPqPtr) += 0.0);
                        (*(adj->BSIM3GqPtr) += 0.0);

                    }
                    else
                    {
                        *(adj->BSIM3QqPtr +1) += omega * ScalingFactor;
                        *(adj->BSIM3QgPtr +1) -= xcqgb;
                        *(adj->BSIM3QdpPtr +1) -= xcqdb;
                        *(adj->BSIM3QspPtr +1) -= xcqsb;
                        *(adj->BSIM3QbPtr +1) -= xcqbb;

                        *(adj->BSIM3QqPtr) += here->BSIM3gtau;

                        *(adj->BSIM3DPqPtr) += dxpart * here->BSIM3gtau;
                        *(adj->BSIM3SPqPtr) += sxpart * here->BSIM3gtau;
                        *(adj->BSIM3GqPtr) -=  here->BSIM3gtau;

                        *(adj->BSIM3QgPtr) +=  xgtg;
                        *(adj->BSIM3QdpPtr) += xgtd;
                        *(adj->BSIM3QspPtr) += xgts;
                        *(adj->BSIM3QbPtr) += xgtb;
                    }
                }
            }
// SRW - end

        }
    }
    return(OK);
}

