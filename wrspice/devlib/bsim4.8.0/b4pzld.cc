
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

/**** BSIM4.8.0 Released by Navid Paydavosi 11/01/2013 ****/

/**********
 * Copyright 2006 Regents of the University of California. All rights reserved.
 * File: b4pzld.c of BSIM4.8.0.
 * Author: 2000 Weidong Liu
 * Authors: 2001- Xuemei Xi, Mohan Dunga, Ali Niknejad, Chenming Hu.
 * Authors: 2006- Mohan Dunga, Ali Niknejad, Chenming Hu
 * Authors: 2007- Mohan Dunga, Wenwei Yang, Ali Niknejad, Chenming Hu
 * Project Director: Prof. Chenming Hu.
 * Modified by Xuemei Xi, 10/05/2001.
 **********/

#include "b4defs.h"


#define BSIM4nextModel      next()
#define BSIM4nextInstance   next()
#define BSIM4instances      inst()


int
BSIM4dev::pzLoad(sGENmodel *genmod, sCKT *ckt, IFcomplex *s)
{
    sBSIM4model *model = static_cast<sBSIM4model*>(genmod);
    sBSIM4instance *here;

    double gjbd, gjbs, geltd, gcrg, gcrgg, gcrgd, gcrgs, gcrgb;
    double xcggb, xcgdb, xcgsb, xcgbb, xcbgb, xcbdb, xcbsb, xcbbb;
    double xcdgb, xcddb, xcdsb, xcdbb, xcsgb, xcsdb, xcssb, xcsbb;
    double gds, /*gbd, gbs, capbd, capbs,*/ FwdSum, RevSum, Gm, Gmbs;
    double gstot, gstotd, gstotg, gstots, gstotb, gspr;
    double gdtot, gdtotd, gdtotg, gdtots, gdtotb, gdpr;
    double gIstotg, gIstotd, gIstots, gIstotb;
    double gIdtotg, gIdtotd, gIdtots, gIdtotb;
    double gIbtotg, gIbtotd, gIbtots, gIbtotb;
    double gIgtotg, gIgtotd, gIgtots, gIgtotb;
    double cgso, cgdo/*, cgbo*/;
    double xcdbdb=0.0, xcsbsb=0.0, xcgmgmb=0.0, xcgmdb=0.0, xcgmsb=0.0, xcdgmb=0.0, xcsgmb=0.0;
    double xcgmbb=0.0, xcbgmb=0.0;
    double dxpart, sxpart, xgtg, xgtd, xgts, xgtb, xcqgb=0.0, xcqdb=0.0, xcqsb=0.0, xcqbb=0.0;
    double gbspsp, gbbdp, gbbsp, gbspg, gbspb;
    double gbspdp, gbdpdp, gbdpg, gbdpb, gbdpsp;
    double ddxpart_dVd, ddxpart_dVg, ddxpart_dVb, ddxpart_dVs;
    double dsxpart_dVd, dsxpart_dVg, dsxpart_dVb, dsxpart_dVs;
    double T0=0.0, T1, CoxWL, qcheq, Cdg, Cdd, Cds, /*Cdb,*/ Csg, Csd, Css/*, Csb*/;
    double ScalingFactor = 1.0e-9;
    struct bsim4SizeDependParam *pParam;
    double ggidld, ggidlg, ggidlb,/*ggisld,*/ ggislg, ggislb, ggisls;


    for (; model != NULL; model = model->BSIM4nextModel)
    {
        for (here = model->BSIM4instances; here!= NULL;
                here = here->BSIM4nextInstance)
        {
            pParam = here->pParam;
//            capbd = here->BSIM4capbd;
//            capbs = here->BSIM4capbs;
            cgso = here->BSIM4cgso;
            cgdo = here->BSIM4cgdo;
//            cgbo = pParam->BSIM4cgbo;

            if (here->BSIM4mode >= 0)
            {
                Gm = here->BSIM4gm;
                Gmbs = here->BSIM4gmbs;
                FwdSum = Gm + Gmbs;
                RevSum = 0.0;

                gbbdp = -(here->BSIM4gbds);
                gbbsp = here->BSIM4gbds + here->BSIM4gbgs + here->BSIM4gbbs;
                gbdpg = here->BSIM4gbgs;
                gbdpdp = here->BSIM4gbds;
                gbdpb = here->BSIM4gbbs;
                gbdpsp = -(gbdpg + gbdpdp + gbdpb);

                gbspdp = 0.0;
                gbspg = 0.0;
                gbspb = 0.0;
                gbspsp = 0.0;

                if (model->BSIM4igcMod)
                {
                    gIstotg = here->BSIM4gIgsg + here->BSIM4gIgcsg;
                    gIstotd = here->BSIM4gIgcsd;
                    gIstots = here->BSIM4gIgss + here->BSIM4gIgcss;
                    gIstotb = here->BSIM4gIgcsb;

                    gIdtotg = here->BSIM4gIgdg + here->BSIM4gIgcdg;
                    gIdtotd = here->BSIM4gIgdd + here->BSIM4gIgcdd;
                    gIdtots = here->BSIM4gIgcds;
                    gIdtotb = here->BSIM4gIgcdb;
                }
                else
                {
                    gIstotg = gIstotd = gIstots = gIstotb = 0.0;
                    gIdtotg = gIdtotd = gIdtots = gIdtotb = 0.0;
                }

                if (model->BSIM4igbMod)
                {
                    gIbtotg = here->BSIM4gIgbg;
                    gIbtotd = here->BSIM4gIgbd;
                    gIbtots = here->BSIM4gIgbs;
                    gIbtotb = here->BSIM4gIgbb;
                }
                else
                    gIbtotg = gIbtotd = gIbtots = gIbtotb = 0.0;

                if ((model->BSIM4igcMod != 0) || (model->BSIM4igbMod != 0))
                {
                    gIgtotg = gIstotg + gIdtotg + gIbtotg;
                    gIgtotd = gIstotd + gIdtotd + gIbtotd ;
                    gIgtots = gIstots + gIdtots + gIbtots;
                    gIgtotb = gIstotb + gIdtotb + gIbtotb;
                }
                else
                    gIgtotg = gIgtotd = gIgtots = gIgtotb = 0.0;

                if (here->BSIM4rgateMod == 2)
                    T0 = *(ckt->CKTstates[0] + here->BSIM4vges)
                         - *(ckt->CKTstates[0] + here->BSIM4vgs);
                else if (here->BSIM4rgateMod == 3)
                    T0 = *(ckt->CKTstates[0] + here->BSIM4vgms)
                         - *(ckt->CKTstates[0] + here->BSIM4vgs);
                if (here->BSIM4rgateMod > 1)
                {
                    gcrgd = here->BSIM4gcrgd * T0;
                    gcrgg = here->BSIM4gcrgg * T0;
                    gcrgs = here->BSIM4gcrgs * T0;
                    gcrgb = here->BSIM4gcrgb * T0;
                    gcrgg -= here->BSIM4gcrg;
                    gcrg = here->BSIM4gcrg;
                }
                else
                    gcrg = gcrgd = gcrgg = gcrgs = gcrgb = 0.0;

                if (here->BSIM4acnqsMod == 0)
                {
                    if (here->BSIM4rgateMod == 3)
                    {
                        xcgmgmb = cgdo + cgso + pParam->BSIM4cgbo;
                        xcgmdb = -cgdo;
                        xcgmsb = -cgso;
                        xcgmbb = -pParam->BSIM4cgbo;

                        xcdgmb = xcgmdb;
                        xcsgmb = xcgmsb;
                        xcbgmb = xcgmbb;

                        xcggb = here->BSIM4cggb;
                        xcgdb = here->BSIM4cgdb;
                        xcgsb = here->BSIM4cgsb;
                        xcgbb = -(xcggb + xcgdb + xcgsb);

                        xcdgb = here->BSIM4cdgb;
                        xcsgb = -(here->BSIM4cggb + here->BSIM4cbgb
                                  + here->BSIM4cdgb);
                        xcbgb = here->BSIM4cbgb;
                    }
                    else
                    {
                        xcggb = here->BSIM4cggb + cgdo + cgso
                                + pParam->BSIM4cgbo;
                        xcgdb = here->BSIM4cgdb - cgdo;
                        xcgsb = here->BSIM4cgsb - cgso;
                        xcgbb = -(xcggb + xcgdb + xcgsb);

                        xcdgb = here->BSIM4cdgb - cgdo;
                        xcsgb = -(here->BSIM4cggb + here->BSIM4cbgb
                                  + here->BSIM4cdgb + cgso);
                        xcbgb = here->BSIM4cbgb - pParam->BSIM4cgbo;

                        xcdgmb = xcsgmb = xcbgmb = 0.0;
                    }
                    xcddb = here->BSIM4cddb + here->BSIM4capbd + cgdo;
                    xcdsb = here->BSIM4cdsb;

                    xcsdb = -(here->BSIM4cgdb + here->BSIM4cbdb
                              + here->BSIM4cddb);
                    xcssb = here->BSIM4capbs + cgso - (here->BSIM4cgsb
                                                       + here->BSIM4cbsb + here->BSIM4cdsb);

                    if (!here->BSIM4rbodyMod)
                    {
                        xcdbb = -(xcdgb + xcddb + xcdsb + xcdgmb);
                        xcsbb = -(xcsgb + xcsdb + xcssb + xcsgmb);
                        xcbdb = here->BSIM4cbdb - here->BSIM4capbd;
                        xcbsb = here->BSIM4cbsb - here->BSIM4capbs;
                        xcdbdb = 0.0;
                    }
                    else
                    {
                        xcdbb  = -(here->BSIM4cddb + here->BSIM4cdgb
                                   + here->BSIM4cdsb);
                        xcsbb = -(xcsgb + xcsdb + xcssb + xcsgmb)
                                + here->BSIM4capbs;
                        xcbdb = here->BSIM4cbdb;
                        xcbsb = here->BSIM4cbsb;

                        xcdbdb = -here->BSIM4capbd;
                        xcsbsb = -here->BSIM4capbs;
                    }
                    xcbbb = -(xcbdb + xcbgb + xcbsb + xcbgmb);

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
                    xcggb = xcgdb = xcgsb = xcgbb = 0.0;
                    xcbgb = xcbdb = xcbsb = xcbbb = 0.0;
                    xcdgb = xcddb = xcdsb = xcdbb = 0.0;
                    xcsgb = xcsdb = xcssb = xcsbb = 0.0;

                    xgtg = here->BSIM4gtg;
                    xgtd = here->BSIM4gtd;
                    xgts = here->BSIM4gts;
                    xgtb = here->BSIM4gtb;

                    xcqgb = here->BSIM4cqgb;
                    xcqdb = here->BSIM4cqdb;
                    xcqsb = here->BSIM4cqsb;
                    xcqbb = here->BSIM4cqbb;

                    CoxWL = model->BSIM4coxe * here->pParam->BSIM4weffCV
                            * here->BSIM4nf * here->pParam->BSIM4leffCV;
                    qcheq = -(here->BSIM4qgate + here->BSIM4qbulk);
                    if (fabs(qcheq) <= 1.0e-5 * CoxWL)
                    {
                        if (model->BSIM4xpart < 0.5)
                        {
                            dxpart = 0.4;
                        }
                        else if (model->BSIM4xpart > 0.5)
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
                        dxpart = here->BSIM4qdrn / qcheq;
                        Cdd = here->BSIM4cddb;
                        Csd = -(here->BSIM4cgdb + here->BSIM4cddb
                                + here->BSIM4cbdb);
                        ddxpart_dVd = (Cdd - dxpart * (Cdd + Csd)) / qcheq;
                        Cdg = here->BSIM4cdgb;
                        Csg = -(here->BSIM4cggb + here->BSIM4cdgb
                                + here->BSIM4cbgb);
                        ddxpart_dVg = (Cdg - dxpart * (Cdg + Csg)) / qcheq;

                        Cds = here->BSIM4cdsb;
                        Css = -(here->BSIM4cgsb + here->BSIM4cdsb
                                + here->BSIM4cbsb);
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
            }
            else
            {
                Gm = -here->BSIM4gm;
                Gmbs = -here->BSIM4gmbs;
                FwdSum = 0.0;
                RevSum = -(Gm + Gmbs);

                gbbsp = -(here->BSIM4gbds);
                gbbdp = here->BSIM4gbds + here->BSIM4gbgs + here->BSIM4gbbs;

                gbdpg = 0.0;
                gbdpsp = 0.0;
                gbdpb = 0.0;
                gbdpdp = 0.0;

                gbspg = here->BSIM4gbgs;
                gbspsp = here->BSIM4gbds;
                gbspb = here->BSIM4gbbs;
                gbspdp = -(gbspg + gbspsp + gbspb);

                if (model->BSIM4igcMod)
                {
                    gIstotg = here->BSIM4gIgsg + here->BSIM4gIgcdg;
                    gIstotd = here->BSIM4gIgcds;
                    gIstots = here->BSIM4gIgss + here->BSIM4gIgcdd;
                    gIstotb = here->BSIM4gIgcdb;

                    gIdtotg = here->BSIM4gIgdg + here->BSIM4gIgcsg;
                    gIdtotd = here->BSIM4gIgdd + here->BSIM4gIgcss;
                    gIdtots = here->BSIM4gIgcsd;
                    gIdtotb = here->BSIM4gIgcsb;
                }
                else
                {
                    gIstotg = gIstotd = gIstots = gIstotb = 0.0;
                    gIdtotg = gIdtotd = gIdtots = gIdtotb  = 0.0;
                }

                if (model->BSIM4igbMod)
                {
                    gIbtotg = here->BSIM4gIgbg;
                    gIbtotd = here->BSIM4gIgbs;
                    gIbtots = here->BSIM4gIgbd;
                    gIbtotb = here->BSIM4gIgbb;
                }
                else
                    gIbtotg = gIbtotd = gIbtots = gIbtotb = 0.0;

                if ((model->BSIM4igcMod != 0) || (model->BSIM4igbMod != 0))
                {
                    gIgtotg = gIstotg + gIdtotg + gIbtotg;
                    gIgtotd = gIstotd + gIdtotd + gIbtotd ;
                    gIgtots = gIstots + gIdtots + gIbtots;
                    gIgtotb = gIstotb + gIdtotb + gIbtotb;
                }
                else
                    gIgtotg = gIgtotd = gIgtots = gIgtotb = 0.0;

                if (here->BSIM4rgateMod == 2)
                    T0 = *(ckt->CKTstates[0] + here->BSIM4vges)
                         - *(ckt->CKTstates[0] + here->BSIM4vgs);
                else if (here->BSIM4rgateMod == 3)
                    T0 = *(ckt->CKTstates[0] + here->BSIM4vgms)
                         - *(ckt->CKTstates[0] + here->BSIM4vgs);
                if (here->BSIM4rgateMod > 1)
                {
                    gcrgd = here->BSIM4gcrgs * T0;
                    gcrgg = here->BSIM4gcrgg * T0;
                    gcrgs = here->BSIM4gcrgd * T0;
                    gcrgb = here->BSIM4gcrgb * T0;
                    gcrgg -= here->BSIM4gcrg;
                    gcrg = here->BSIM4gcrg;
                }
                else
                    gcrg = gcrgd = gcrgg = gcrgs = gcrgb = 0.0;

                if (here->BSIM4acnqsMod == 0)
                {
                    if (here->BSIM4rgateMod == 3)
                    {
                        xcgmgmb = cgdo + cgso + pParam->BSIM4cgbo;
                        xcgmdb = -cgdo;
                        xcgmsb = -cgso;
                        xcgmbb = -pParam->BSIM4cgbo;

                        xcdgmb = xcgmdb;
                        xcsgmb = xcgmsb;
                        xcbgmb = xcgmbb;

                        xcggb = here->BSIM4cggb;
                        xcgdb = here->BSIM4cgsb;
                        xcgsb = here->BSIM4cgdb;
                        xcgbb = -(xcggb + xcgdb + xcgsb);

                        xcdgb = -(here->BSIM4cggb + here->BSIM4cbgb
                                  + here->BSIM4cdgb);
                        xcsgb = here->BSIM4cdgb;
                        xcbgb = here->BSIM4cbgb;
                    }
                    else
                    {
                        xcggb = here->BSIM4cggb + cgdo + cgso
                                + pParam->BSIM4cgbo;
                        xcgdb = here->BSIM4cgsb - cgdo;
                        xcgsb = here->BSIM4cgdb - cgso;
                        xcgbb = -(xcggb + xcgdb + xcgsb);

                        xcdgb = -(here->BSIM4cggb + here->BSIM4cbgb
                                  + here->BSIM4cdgb + cgdo);
                        xcsgb = here->BSIM4cdgb - cgso;
                        xcbgb = here->BSIM4cbgb - pParam->BSIM4cgbo;

                        xcdgmb = xcsgmb = xcbgmb = 0.0;
                    }
                    xcddb = here->BSIM4capbd + cgdo - (here->BSIM4cgsb
                                                       + here->BSIM4cbsb + here->BSIM4cdsb);
                    xcdsb = -(here->BSIM4cgdb + here->BSIM4cbdb
                              + here->BSIM4cddb);

                    xcsdb = here->BSIM4cdsb;
                    xcssb = here->BSIM4cddb + here->BSIM4capbs + cgso;

                    if (!here->BSIM4rbodyMod)
                    {
                        xcdbb = -(xcdgb + xcddb + xcdsb + xcdgmb);
                        xcsbb = -(xcsgb + xcsdb + xcssb + xcsgmb);
                        xcbdb = here->BSIM4cbsb - here->BSIM4capbd;
                        xcbsb = here->BSIM4cbdb - here->BSIM4capbs;
                        xcdbdb = 0.0;
                    }
                    else
                    {
                        xcdbb = -(xcdgb + xcddb + xcdsb + xcdgmb)
                                + here->BSIM4capbd;
                        xcsbb = -(here->BSIM4cddb + here->BSIM4cdgb
                                  + here->BSIM4cdsb);
                        xcbdb = here->BSIM4cbsb;
                        xcbsb = here->BSIM4cbdb;
                        xcdbdb = -here->BSIM4capbd;
                        xcsbsb = -here->BSIM4capbs;
                    }
                    xcbbb = -(xcbgb + xcbdb + xcbsb + xcbgmb);

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
                    xcggb = xcgdb = xcgsb = xcgbb = 0.0;
                    xcbgb = xcbdb = xcbsb = xcbbb = 0.0;
                    xcdgb = xcddb = xcdsb = xcdbb = 0.0;
                    xcsgb = xcsdb = xcssb = xcsbb = 0.0;

                    xgtg = here->BSIM4gtg;
                    xgtd = here->BSIM4gts;
                    xgts = here->BSIM4gtd;
                    xgtb = here->BSIM4gtb;

                    xcqgb = here->BSIM4cqgb;
                    xcqdb = here->BSIM4cqsb;
                    xcqsb = here->BSIM4cqdb;
                    xcqbb = here->BSIM4cqbb;

                    CoxWL = model->BSIM4coxe * here->pParam->BSIM4weffCV
                            * here->BSIM4nf * here->pParam->BSIM4leffCV;
                    qcheq = -(here->BSIM4qgate + here->BSIM4qbulk);
                    if (fabs(qcheq) <= 1.0e-5 * CoxWL)
                    {
                        if (model->BSIM4xpart < 0.5)
                        {
                            sxpart = 0.4;
                        }
                        else if (model->BSIM4xpart > 0.5)
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
                        sxpart = here->BSIM4qdrn / qcheq;
                        Css = here->BSIM4cddb;
                        Cds = -(here->BSIM4cgdb + here->BSIM4cddb
                                + here->BSIM4cbdb);
                        dsxpart_dVs = (Css - sxpart * (Css + Cds)) / qcheq;
                        Csg = here->BSIM4cdgb;
                        Cdg = -(here->BSIM4cggb + here->BSIM4cdgb
                                + here->BSIM4cbgb);
                        dsxpart_dVg = (Csg - sxpart * (Csg + Cdg)) / qcheq;

                        Csd = here->BSIM4cdsb;
                        Cdd = -(here->BSIM4cgsb + here->BSIM4cdsb
                                + here->BSIM4cbsb);
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
            }

            if (model->BSIM4rdsMod == 1)
            {
                gstot = here->BSIM4gstot;
                gstotd = here->BSIM4gstotd;
                gstotg = here->BSIM4gstotg;
                gstots = here->BSIM4gstots - gstot;
                gstotb = here->BSIM4gstotb;

                gdtot = here->BSIM4gdtot;
                gdtotd = here->BSIM4gdtotd - gdtot;
                gdtotg = here->BSIM4gdtotg;
                gdtots = here->BSIM4gdtots;
                gdtotb = here->BSIM4gdtotb;
            }
            else
            {
                gstot = gstotd = gstotg = gstots = gstotb = 0.0;
                gdtot = gdtotd = gdtotg = gdtots = gdtotb = 0.0;
            }


            T1 = *(ckt->CKTstate0 + here->BSIM4qdef) * here->BSIM4gtau;
            gds = here->BSIM4gds;

            /*
             * Loading PZ matrix
             */

            if (!model->BSIM4rdsMod)
            {
                gdpr = here->BSIM4drainConductance;
                gspr = here->BSIM4sourceConductance;
            }
            else
                gdpr = gspr = 0.0;

            if (!here->BSIM4rbodyMod)
            {
                gjbd = here->BSIM4gbd;
                gjbs = here->BSIM4gbs;
            }
            else
                gjbd = gjbs = 0.0;

            geltd = here->BSIM4grgeltd;

// SRW
#define M(x) (here->BSIM4m*(x))

            if (here->BSIM4rgateMod == 1)
            {
                *(here->BSIM4GEgePtr) += M(geltd);
                *(here->BSIM4GPgePtr) -= M(geltd);
                *(here->BSIM4GEgpPtr) -= M(geltd);

                *(here->BSIM4GPgpPtr ) += M(xcggb * s->real);
                *(here->BSIM4GPgpPtr +1) += M(xcggb * s->imag);
                *(here->BSIM4GPgpPtr) += M(geltd - xgtg + gIgtotg);
                *(here->BSIM4GPdpPtr ) += M(xcgdb * s->real);
                *(here->BSIM4GPdpPtr +1) += M(xcgdb * s->imag);
                *(here->BSIM4GPdpPtr) -= M(xgtd - gIgtotd);
                *(here->BSIM4GPspPtr ) += M(xcgsb * s->real);
                *(here->BSIM4GPspPtr +1) += M(xcgsb * s->imag);
                *(here->BSIM4GPspPtr) -= M(xgts - gIgtots);
                *(here->BSIM4GPbpPtr ) += M(xcgbb * s->real);
                *(here->BSIM4GPbpPtr +1) += M(xcgbb * s->imag);
                *(here->BSIM4GPbpPtr) -= M(xgtb - gIgtotb);
            }
            else if (here->BSIM4rgateMod == 2)
            {
                *(here->BSIM4GEgePtr) += M(gcrg);
                *(here->BSIM4GEgpPtr) += M(gcrgg);
                *(here->BSIM4GEdpPtr) += M(gcrgd);
                *(here->BSIM4GEspPtr) += M(gcrgs);
                *(here->BSIM4GEbpPtr) += M(gcrgb);

                *(here->BSIM4GPgePtr) -= M(gcrg);
                *(here->BSIM4GPgpPtr ) += M(xcggb * s->real);
                *(here->BSIM4GPgpPtr +1) += M(xcggb * s->imag);
                *(here->BSIM4GPgpPtr) -= M(gcrgg + xgtg - gIgtotg);
                *(here->BSIM4GPdpPtr ) += M(xcgdb * s->real);
                *(here->BSIM4GPdpPtr +1) += M(xcgdb * s->imag);
                *(here->BSIM4GPdpPtr) -= M(gcrgd + xgtd - gIgtotd);
                *(here->BSIM4GPspPtr ) += M(xcgsb * s->real);
                *(here->BSIM4GPspPtr +1) += M(xcgsb * s->imag);
                *(here->BSIM4GPspPtr) -= M(gcrgs + xgts - gIgtots);
                *(here->BSIM4GPbpPtr ) += M(xcgbb * s->real);
                *(here->BSIM4GPbpPtr +1) += M(xcgbb * s->imag);
                *(here->BSIM4GPbpPtr) -= M(gcrgb + xgtb - gIgtotb);
            }
            else if (here->BSIM4rgateMod == 3)
            {
                *(here->BSIM4GEgePtr) += M(geltd);
                *(here->BSIM4GEgmPtr) -= M(geltd);
                *(here->BSIM4GMgePtr) -= M(geltd);
                *(here->BSIM4GMgmPtr) += M(geltd + gcrg);
                *(here->BSIM4GMgmPtr ) += M(xcgmgmb * s->real);
                *(here->BSIM4GMgmPtr +1) += M(xcgmgmb * s->imag);

                *(here->BSIM4GMdpPtr) += M(gcrgd);
                *(here->BSIM4GMdpPtr ) += M(xcgmdb * s->real);
                *(here->BSIM4GMdpPtr +1) += M(xcgmdb * s->imag);
                *(here->BSIM4GMgpPtr) += M(gcrgg);
                *(here->BSIM4GMspPtr) += M(gcrgs);
                *(here->BSIM4GMspPtr ) += M(xcgmsb * s->real);
                *(here->BSIM4GMspPtr +1) += M(xcgmsb * s->imag);
                *(here->BSIM4GMbpPtr) += M(gcrgb);
                *(here->BSIM4GMbpPtr ) += M(xcgmbb * s->real);
                *(here->BSIM4GMbpPtr +1) += M(xcgmbb * s->imag);

                *(here->BSIM4DPgmPtr ) += M(xcdgmb * s->real);
                *(here->BSIM4DPgmPtr +1) += M(xcdgmb * s->imag);
                *(here->BSIM4GPgmPtr) -= M(gcrg);
                *(here->BSIM4SPgmPtr ) += M(xcsgmb * s->real);
                *(here->BSIM4SPgmPtr +1) += M(xcsgmb * s->imag);
                *(here->BSIM4BPgmPtr ) += M(xcbgmb * s->real);
                *(here->BSIM4BPgmPtr +1) += M(xcbgmb * s->imag);

                *(here->BSIM4GPgpPtr) -= M(gcrgg + xgtg - gIgtotg);
                *(here->BSIM4GPgpPtr ) += M(xcggb * s->real);
                *(here->BSIM4GPgpPtr +1) += M(xcggb * s->imag);
                *(here->BSIM4GPdpPtr) -= M(gcrgd + xgtd - gIgtotd);
                *(here->BSIM4GPdpPtr ) += M(xcgdb * s->real);
                *(here->BSIM4GPdpPtr +1) += M(xcgdb * s->imag);
                *(here->BSIM4GPspPtr) -= M(gcrgs + xgts - gIgtots);
                *(here->BSIM4GPspPtr ) += M(xcgsb * s->real);
                *(here->BSIM4GPspPtr +1) += M(xcgsb * s->imag);
                *(here->BSIM4GPbpPtr) -= M(gcrgb + xgtb - gIgtotb);
                *(here->BSIM4GPbpPtr ) += M(xcgbb * s->real);
                *(here->BSIM4GPbpPtr +1) += M(xcgbb * s->imag);
            }
            else
            {
                *(here->BSIM4GPdpPtr ) += M(xcgdb * s->real);
                *(here->BSIM4GPdpPtr +1) += M(xcgdb * s->imag);
                *(here->BSIM4GPdpPtr) -= M(xgtd - gIgtotd);
                *(here->BSIM4GPgpPtr ) += M(xcggb * s->real);
                *(here->BSIM4GPgpPtr +1) += M(xcggb * s->imag);
                *(here->BSIM4GPgpPtr) -= M(xgtg - gIgtotg);
                *(here->BSIM4GPspPtr ) += M(xcgsb * s->real);
                *(here->BSIM4GPspPtr +1) += M(xcgsb * s->imag);
                *(here->BSIM4GPspPtr) -= M(xgts - gIgtots);
                *(here->BSIM4GPbpPtr ) += M(xcgbb * s->real);
                *(here->BSIM4GPbpPtr +1) += M(xcgbb * s->imag);
                *(here->BSIM4GPbpPtr) -= M(xgtb - gIgtotb);
            }

            if (model->BSIM4rdsMod)
            {
                (*(here->BSIM4DgpPtr) += M(gdtotg));
                (*(here->BSIM4DspPtr) += M(gdtots));
                (*(here->BSIM4DbpPtr) += M(gdtotb));
                (*(here->BSIM4SdpPtr) += M(gstotd));
                (*(here->BSIM4SgpPtr) += M(gstotg));
                (*(here->BSIM4SbpPtr) += M(gstotb));
            }

            *(here->BSIM4DPdpPtr ) += M(xcddb * s->real);
            *(here->BSIM4DPdpPtr +1) += M(xcddb * s->imag);
            *(here->BSIM4DPdpPtr) += M(gdpr + gds + here->BSIM4gbd
                                       - gdtotd + RevSum + gbdpdp - gIdtotd
                                       + dxpart * xgtd + T1 * ddxpart_dVd);
            *(here->BSIM4DPdPtr) -= M(gdpr + gdtot);
            *(here->BSIM4DPgpPtr ) += M(xcdgb * s->real);
            *(here->BSIM4DPgpPtr +1) += M(xcdgb * s->imag);
            *(here->BSIM4DPgpPtr) += M(Gm - gdtotg + gbdpg - gIdtotg
                                       + T1 * ddxpart_dVg + dxpart * xgtg);
            *(here->BSIM4DPspPtr ) += M(xcdsb * s->real);
            *(here->BSIM4DPspPtr +1) += M(xcdsb * s->imag);
            *(here->BSIM4DPspPtr) -= M(gds + FwdSum + gdtots - gbdpsp + gIdtots
                                       - T1 * ddxpart_dVs - dxpart * xgts);
            *(here->BSIM4DPbpPtr ) += M(xcdbb * s->real);
            *(here->BSIM4DPbpPtr +1) += M(xcdbb * s->imag);
            *(here->BSIM4DPbpPtr) -= M(gjbd + gdtotb - Gmbs - gbdpb + gIdtotb
                                       - T1 * ddxpart_dVb - dxpart * xgtb);

            *(here->BSIM4DdpPtr) -= M(gdpr - gdtotd);
            *(here->BSIM4DdPtr) += M(gdpr + gdtot);

            *(here->BSIM4SPdpPtr ) += M(xcsdb * s->real);
            *(here->BSIM4SPdpPtr +1) += M(xcsdb * s->imag);
            *(here->BSIM4SPdpPtr) -= M(gds + gstotd + RevSum - gbspdp + gIstotd
                                       - T1 * dsxpart_dVd - sxpart * xgtd);
            *(here->BSIM4SPgpPtr ) += M(xcsgb * s->real);
            *(here->BSIM4SPgpPtr +1) += M(xcsgb * s->imag);
            *(here->BSIM4SPgpPtr) -= M(Gm + gstotg - gbspg + gIstotg
                                       - T1 * dsxpart_dVg - sxpart * xgtg);
            *(here->BSIM4SPspPtr ) += M(xcssb * s->real);
            *(here->BSIM4SPspPtr +1) += M(xcssb * s->imag);
            *(here->BSIM4SPspPtr) += M(gspr + gds + here->BSIM4gbs - gIstots
                                       - gstots + FwdSum + gbspsp
                                       + sxpart * xgts + T1 * dsxpart_dVs);
            *(here->BSIM4SPsPtr) -= M(gspr + gstot);
            *(here->BSIM4SPbpPtr ) += M(xcsbb * s->real);
            *(here->BSIM4SPbpPtr +1) += M(xcsbb * s->imag);
            *(here->BSIM4SPbpPtr) -= M(gjbs + gstotb + Gmbs - gbspb + gIstotb
                                       - T1 * dsxpart_dVb - sxpart * xgtb);

            *(here->BSIM4SspPtr) -= M(gspr - gstots);
            *(here->BSIM4SsPtr) += M(gspr + gstot);

            *(here->BSIM4BPdpPtr ) += M(xcbdb * s->real);
            *(here->BSIM4BPdpPtr +1) += M(xcbdb * s->imag);
            *(here->BSIM4BPdpPtr) -= M(gjbd - gbbdp + gIbtotd);
            *(here->BSIM4BPgpPtr ) += M(xcbgb * s->real);
            *(here->BSIM4BPgpPtr +1) += M(xcbgb * s->imag);
            *(here->BSIM4BPgpPtr) -= M(here->BSIM4gbgs + gIbtotg);
            *(here->BSIM4BPspPtr ) += M(xcbsb * s->real);
            *(here->BSIM4BPspPtr +1) += M(xcbsb * s->imag);
            *(here->BSIM4BPspPtr) -= M(gjbs - gbbsp + gIbtots);
            *(here->BSIM4BPbpPtr ) += M(xcbbb * s->real);
            *(here->BSIM4BPbpPtr +1) += M(xcbbb * s->imag);
            *(here->BSIM4BPbpPtr) += M(gjbd + gjbs - here->BSIM4gbbs
                                       - gIbtotb);
            ggidld = here->BSIM4ggidld;
            ggidlg = here->BSIM4ggidlg;
            ggidlb = here->BSIM4ggidlb;
            ggislg = here->BSIM4ggislg;
            ggisls = here->BSIM4ggisls;
            ggislb = here->BSIM4ggislb;

            /* stamp gidl */
            (*(here->BSIM4DPdpPtr) += M(ggidld));
            (*(here->BSIM4DPgpPtr) += M(ggidlg));
            (*(here->BSIM4DPspPtr) -= M((ggidlg + ggidld) + ggidlb));
            (*(here->BSIM4DPbpPtr) += M(ggidlb));
            (*(here->BSIM4BPdpPtr) -= M(ggidld));
            (*(here->BSIM4BPgpPtr) -= M(ggidlg));
            (*(here->BSIM4BPspPtr) += M((ggidlg + ggidld) + ggidlb));
            (*(here->BSIM4BPbpPtr) -= M(ggidlb));
            /* stamp gisl */
            (*(here->BSIM4SPdpPtr) -= M((ggisls + ggislg) + ggislb));
            (*(here->BSIM4SPgpPtr) += M(ggislg));
            (*(here->BSIM4SPspPtr) += M(ggisls));
            (*(here->BSIM4SPbpPtr) += M(ggislb));
            (*(here->BSIM4BPdpPtr) += M((ggislg + ggisls) + ggislb));
            (*(here->BSIM4BPgpPtr) -= M(ggislg));
            (*(here->BSIM4BPspPtr) -= M(ggisls));
            (*(here->BSIM4BPbpPtr) -= M(ggislb));

            if (here->BSIM4rbodyMod)
            {
                (*(here->BSIM4DPdbPtr ) += M(xcdbdb * s->real));
                (*(here->BSIM4DPdbPtr +1) += M(xcdbdb * s->imag));
                (*(here->BSIM4DPdbPtr) -= M(here->BSIM4gbd));
                (*(here->BSIM4SPsbPtr ) += M(xcsbsb * s->real));
                (*(here->BSIM4SPsbPtr +1) += M(xcsbsb * s->imag));
                (*(here->BSIM4SPsbPtr) -= M(here->BSIM4gbs));

                (*(here->BSIM4DBdpPtr ) += M(xcdbdb * s->real));
                (*(here->BSIM4DBdpPtr +1) += M(xcdbdb * s->imag));
                (*(here->BSIM4DBdpPtr) -= M(here->BSIM4gbd));
                (*(here->BSIM4DBdbPtr ) -= M(xcdbdb * s->real));
                (*(here->BSIM4DBdbPtr +1) -= M(xcdbdb * s->imag));
                (*(here->BSIM4DBdbPtr) += M(here->BSIM4gbd + here->BSIM4grbpd
                                            + here->BSIM4grbdb));
                (*(here->BSIM4DBbpPtr) -= M(here->BSIM4grbpd));
                (*(here->BSIM4DBbPtr) -= M(here->BSIM4grbdb));

                (*(here->BSIM4BPdbPtr) -= M(here->BSIM4grbpd));
                (*(here->BSIM4BPbPtr) -= M(here->BSIM4grbpb));
                (*(here->BSIM4BPsbPtr) -= M(here->BSIM4grbps));
                (*(here->BSIM4BPbpPtr) += M(here->BSIM4grbpd + here->BSIM4grbps
                                            + here->BSIM4grbpb));
                /* WDL: (-here->BSIM4gbbs) already added to BPbpPtr */

                (*(here->BSIM4SBspPtr ) += M(xcsbsb * s->real));
                (*(here->BSIM4SBspPtr +1) += M(xcsbsb * s->imag));
                (*(here->BSIM4SBspPtr) -= M(here->BSIM4gbs));
                (*(here->BSIM4SBbpPtr) -= M(here->BSIM4grbps));
                (*(here->BSIM4SBbPtr) -= M(here->BSIM4grbsb));
                (*(here->BSIM4SBsbPtr ) -= M(xcsbsb * s->real));
                (*(here->BSIM4SBsbPtr +1) -= M(xcsbsb * s->imag));
                (*(here->BSIM4SBsbPtr) += M(here->BSIM4gbs
                                            + here->BSIM4grbps + here->BSIM4grbsb));

                (*(here->BSIM4BdbPtr) -= M(here->BSIM4grbdb));
                (*(here->BSIM4BbpPtr) -= M(here->BSIM4grbpb));
                (*(here->BSIM4BsbPtr) -= M(here->BSIM4grbsb));
                (*(here->BSIM4BbPtr) += M(here->BSIM4grbsb + here->BSIM4grbdb
                                          + here->BSIM4grbpb));
            }

            if (here->BSIM4acnqsMod)
            {
                *(here->BSIM4QqPtr ) += M(s->real * ScalingFactor);
                *(here->BSIM4QqPtr +1) += M(s->imag * ScalingFactor);
                *(here->BSIM4QgpPtr ) -= M(xcqgb * s->real);
                *(here->BSIM4QgpPtr +1) -= M(xcqgb * s->imag);
                *(here->BSIM4QdpPtr ) -= M(xcqdb * s->real);
                *(here->BSIM4QdpPtr +1) -= M(xcqdb * s->imag);
                *(here->BSIM4QbpPtr ) -= M(xcqbb * s->real);
                *(here->BSIM4QbpPtr +1) -= M(xcqbb * s->imag);
                *(here->BSIM4QspPtr ) -= M(xcqsb * s->real);
                *(here->BSIM4QspPtr +1) -= M(xcqsb * s->imag);

                *(here->BSIM4GPqPtr) -= M(here->BSIM4gtau);
                *(here->BSIM4DPqPtr) += M(dxpart * here->BSIM4gtau);
                *(here->BSIM4SPqPtr) += M(sxpart * here->BSIM4gtau);

                *(here->BSIM4QqPtr) += M(here->BSIM4gtau);
                *(here->BSIM4QgpPtr) += M(xgtg);
                *(here->BSIM4QdpPtr) += M(xgtd);
                *(here->BSIM4QbpPtr) += M(xgtb);
                *(here->BSIM4QspPtr) += M(xgts);
            }
        }
    }
    return(OK);
}

