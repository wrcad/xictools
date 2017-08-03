
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
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1998 Samuel Fung, Dennis Sinitsky and Stephen Tang
File: b3soiacld.c          98/5/01
Modified by Pin Su    99/4/30
Modified by Pin Su    99/9/27
Modified by Pin Su    02/5/20
Modified by Pin Su and Hui Wan 02/11/12
Modified by Pin Su    03/2/28
Modified by Pin Su    03/4/20
Modified by Pin Su and Hui Wan   03/07/30
**********/

#include "b3sdefs.h"
#include "gencurrent.h"

#define B3SOInextModel      next()
#define B3SOInextInstance   next()
#define B3SOIinstances      inst()


int
B3SOIdev::acLoad(sGENmodel *genmod, sCKT *ckt)
{
    sB3SOImodel *model = static_cast<sB3SOImodel*>(genmod);
    sB3SOIinstance *here;

    int selfheat;
    double xcggb, xcgdb, xcgsb, xcgeb, xcgT;
    double xcdgb, xcddb, xcdsb, xcdeb, xcdT;
    double xcsgb, xcsdb, xcssb, xcseb, xcsT;
    double xcbgb, xcbdb, xcbsb, xcbeb, xcbT;
    double xcegb, xceeb, xceT;
    double gdpr, gspr, gds;
    double cggb, cgdb, cgsb, cgT;
    double cdgb, cddb, cdsb, cdeb, cdT;
    double cbgb, cbdb, cbsb, cbeb, cbT;
    double ceeb, ceT;
    double GSoverlapCap, GDoverlapCap, GEoverlapCap, FwdSum, RevSum, Gm, Gmbs, GmT;
    double omega;
//    double dxpart, sxpart;
    double gbbg, gbbdp, gbbb, gbbp, gbbsp, gbbT;
    double gddpg, gddpdp, gddpsp, gddpb, gddpT;
    double gsspg, gsspdp, gsspsp, gsspb, gsspT;
    double gppb, gppp;
    double xcTt, cTt, /*gcTt,*/ gTtt, gTtg, gTtb, gTtdp, gTtsp;
    double EDextrinsicCap, ESextrinsicCap;
    double xcedb, xcesb;

    /* v3.0 */
    double Gme, gddpe, gsspe, gbbe, gTte;

    /* v3.1 added variables for RF */
    double T0;
    double gcrgd, gcrgg, gcrgs, gcrgb, gcrg;
    double xcgmgmb, xcgmdb, xcgmsb, xcgmeb, xcdgmb, xcsgmb, xcegmb;
    double geltd;
//double gcrgT;
    double gigg, gigd, gigs, gigb, gige, gigT;

    /* v3.1.1 bug fix */
    double gIstotg, gIstotd, gIstotb, gIstots;
    double gIdtotg, gIdtotd, gIdtotb, gIdtots;
    double gIgtotg, gIgtotd, gIgtotb, gIgtots;


    omega = ckt->CKTomega;
    for (; model != NULL; model = model->B3SOInextModel)
    {

        for (here = model->B3SOIinstances; here!= NULL;
                here = here->B3SOInextInstance)
        {
            selfheat = (model->B3SOIshMod == 1) && (here->B3SOIrth0 != 0.0);
            if (here->B3SOImode >= 0)
            {
                Gm = here->B3SOIgm;
                Gmbs = here->B3SOIgmbs;

                /* v3.0 */
                Gme = here->B3SOIgme;

                GmT = model->B3SOItype * here->B3SOIgmT;
                FwdSum = Gm + Gmbs + Gme; /* v3.0 */
                RevSum = 0.0;

                cbgb = here->B3SOIcbgb;
                cbsb = here->B3SOIcbsb;
                cbdb = here->B3SOIcbdb;
                cbeb = here->B3SOIcbeb;
                cbT  = model->B3SOItype * here->B3SOIcbT;

                ceeb = here->B3SOIceeb;
                ceT  = model->B3SOItype * here->B3SOIceT;

                cggb = here->B3SOIcggb;
                cgsb = here->B3SOIcgsb;
                cgdb = here->B3SOIcgdb;
                cgT  = model->B3SOItype * here->B3SOIcgT;

                cdgb = here->B3SOIcdgb;
                cdsb = here->B3SOIcdsb;
                cddb = here->B3SOIcddb;
                cdeb = here->B3SOIcdeb;
                cdT  = model->B3SOItype * here->B3SOIcdT;

                cTt = here->pParam->B3SOIcth;


                /* v3.1 bug fix */
                gigg = here->B3SOIgigg;
                gigb = here->B3SOIgigb;
                gige = here->B3SOIgige;
                gigs = here->B3SOIgigs;
                gigd = here->B3SOIgigd;
                gigT = model->B3SOItype * here->B3SOIgigT;

                gbbg  = -here->B3SOIgbgs;
                gbbdp = -here->B3SOIgbds;
                gbbb  = -here->B3SOIgbbs;
                gbbp  = -here->B3SOIgbps;
                gbbT  = -model->B3SOItype * here->B3SOIgbT;

                /* v3.0 */
                gbbe  = -here->B3SOIgbes;
                gbbsp = - ( gbbg + gbbdp + gbbb + gbbp + gbbe);

                gddpg  = -here->B3SOIgjdg;
                gddpdp = -here->B3SOIgjdd;
                gddpb  = -here->B3SOIgjdb;
                gddpT  = -model->B3SOItype * here->B3SOIgjdT;

                /* v3.0 */
                gddpe  = -here->B3SOIgjde;
                gddpsp = - ( gddpg + gddpdp + gddpb + gddpe);

                gsspg  = -here->B3SOIgjsg;
                gsspdp = -here->B3SOIgjsd;
                gsspb  = -here->B3SOIgjsb;
                gsspT  = -model->B3SOItype * here->B3SOIgjsT;

                /* v3.0 */
                gsspe  = 0.0;
                gsspsp = - (gsspg + gsspdp + gsspb + gsspe);


                gppb = -here->B3SOIgbpbs;
                gppp = -here->B3SOIgbpps;

                gTtg  = here->B3SOIgtempg;
                gTtb  = here->B3SOIgtempb;
                gTtdp = here->B3SOIgtempd;
                gTtt  = here->B3SOIgtempT;

                /* v3.0 */
                gTte  = here->B3SOIgtempe;
                gTtsp = - (gTtg + gTtb + gTtdp + gTte);


                /* v3.1.1 bug fix */
                if (model->B3SOIigcMod)
                {
                    gIstotg = here->B3SOIgIgsg + here->B3SOIgIgcsg;
                    gIstotd = here->B3SOIgIgcsd;
                    gIstots = here->B3SOIgIgss + here->B3SOIgIgcss;
                    gIstotb = here->B3SOIgIgcsb;

                    gIdtotg = here->B3SOIgIgdg + here->B3SOIgIgcdg;
                    gIdtotd = here->B3SOIgIgdd + here->B3SOIgIgcdd;
                    gIdtots = here->B3SOIgIgcds;
                    gIdtotb = here->B3SOIgIgcdb;

                    gIgtotg = gIstotg + gIdtotg;
                    gIgtotd = gIstotd + gIdtotd;
                    gIgtots = gIstots + gIdtots;
                    gIgtotb = gIstotb + gIdtotb;
                }
                else
                {
                    gIstotg = gIstotd = gIstots = gIstotb = 0.0;
                    gIdtotg = gIdtotd = gIdtots = gIdtotb = 0.0;
                    gIgtotg = gIgtotd = gIgtots = gIgtotb = 0.0;
                }


//                sxpart = 0.6;
//                dxpart = 0.4;

                /* v3.1 wanh added for RF */
                if (here->B3SOIrgateMod == 2)
                    T0 = *(ckt->CKTstates[0] + here->B3SOIvges)
                         - *(ckt->CKTstates[0] + here->B3SOIvgs);
                else if (here->B3SOIrgateMod == 3)
                    T0 = *(ckt->CKTstates[0] + here->B3SOIvgms)
                         - *(ckt->CKTstates[0] + here->B3SOIvgs);
                if (here->B3SOIrgateMod > 1)
                {
                    gcrgd = here->B3SOIgcrgd * T0;
                    gcrgg = here->B3SOIgcrgg * T0;
                    gcrgs = here->B3SOIgcrgs * T0;
                    gcrgb = here->B3SOIgcrgb * T0;
                    gcrgg -= here->B3SOIgcrg;
                    gcrg = here->B3SOIgcrg;
                }
                else
                    gcrg = gcrgd = gcrgg = gcrgs = gcrgb = 0.0;
                /* v3.1 wanh added for RF end*/

            }
            else
            {
                Gm = -here->B3SOIgm;
                Gmbs = -here->B3SOIgmbs;

                /* v3.0 */
                Gme = -here->B3SOIgme;

                GmT = -model->B3SOItype * here->B3SOIgmT;
                FwdSum = 0.0;
                RevSum = -Gm - Gmbs - Gme; /* v3.0 */

                cdgb = - (here->B3SOIcdgb + here->B3SOIcggb + here->B3SOIcbgb);
                cdsb = - (here->B3SOIcddb + here->B3SOIcgdb + here->B3SOIcbdb);
                cddb = - (here->B3SOIcdsb + here->B3SOIcgsb + here->B3SOIcbsb);
                cdeb = - (here->B3SOIcdeb + here->B3SOIcbeb + here->B3SOIceeb);
                cdT  = - model->B3SOItype * (here->B3SOIcgT + here->B3SOIcbT
                                             + here->B3SOIcdT + here->B3SOIceT);

                ceeb = here->B3SOIceeb;
                ceT  = model->B3SOItype * here->B3SOIceT;

                cggb = here->B3SOIcggb;
                cgsb = here->B3SOIcgdb;
                cgdb = here->B3SOIcgsb;
                cgT  = model->B3SOItype * here->B3SOIcgT;

                cbgb = here->B3SOIcbgb;
                cbsb = here->B3SOIcbdb;
                cbdb = here->B3SOIcbsb;
                cbeb = here->B3SOIcbeb;
                cbT  = model->B3SOItype * here->B3SOIcbT;

                cTt = here->pParam->B3SOIcth;


                /* v3.1 bug fix */
                gigg = here->B3SOIgigg;
                gigb = here->B3SOIgigb;
                gige = here->B3SOIgige;
                gigs = here->B3SOIgigd; /* v3.1.1 bug fix */
                gigd = here->B3SOIgigs; /* v3.1.1 bug fix */
                gigT = model->B3SOItype * here->B3SOIgigT;

                gbbg  = -here->B3SOIgbgs;
                gbbb  = -here->B3SOIgbbs;
                gbbp  = -here->B3SOIgbps;
                gbbsp = -here->B3SOIgbds;
                gbbT  = -model->B3SOItype * here->B3SOIgbT;

                /* v3.0 */
                gbbe  = -here->B3SOIgbes;
                gbbdp = - ( gbbg + gbbsp + gbbb + gbbp + gbbe);

                gddpg  = -here->B3SOIgjsg;
                gddpsp = -here->B3SOIgjsd;
                gddpb  = -here->B3SOIgjsb;
                gddpT  = -model->B3SOItype * here->B3SOIgjsT;

                /* v3.0 */
                gddpe  = 0.0;
                gddpdp = - (gddpg + gddpsp + gddpb + gddpe );

                gsspg  = -here->B3SOIgjdg;
                gsspsp = -here->B3SOIgjdd;
                gsspb  = -here->B3SOIgjdb;
                gsspT  = -model->B3SOItype * here->B3SOIgjdT;

                /* v3.0 */
                gsspe  = -here->B3SOIgjde;
                gsspdp = - ( gsspg + gsspsp + gsspb + gsspe );

                gppb = -here->B3SOIgbpbs;
                gppp = -here->B3SOIgbpps;

                gTtg  = here->B3SOIgtempg;
                gTtb  = here->B3SOIgtempb;
                gTtsp = here->B3SOIgtempd;
                gTtt  = here->B3SOIgtempT;

                /* v3.0 */
                gTte = here->B3SOIgtempe;
                gTtdp = - (gTtg + gTtb + gTtsp + gTte);


                /* v3.1.1 bug fix */
                if (model->B3SOIigcMod)
                {
                    gIstotg = here->B3SOIgIgsg + here->B3SOIgIgcdg;
                    gIstotd = here->B3SOIgIgcds;
                    gIstots = here->B3SOIgIgss + here->B3SOIgIgcdd;
                    gIstotb = here->B3SOIgIgcdb;

                    gIdtotg = here->B3SOIgIgdg + here->B3SOIgIgcsg;
                    gIdtotd = here->B3SOIgIgdd + here->B3SOIgIgcss;
                    gIdtots = here->B3SOIgIgcsd;
                    gIdtotb = here->B3SOIgIgcsb;

                    gIgtotg = gIstotg + gIdtotg;
                    gIgtotd = gIstotd + gIdtotd;
                    gIgtots = gIstots + gIdtots;
                    gIgtotb = gIstotb + gIdtotb;
                }
                else
                {
                    gIstotg = gIstotd = gIstots = gIstotb = 0.0;
                    gIdtotg = gIdtotd = gIdtots = gIdtotb = 0.0;
                    gIgtotg = gIgtotd = gIgtots = gIgtotb = 0.0;
                }


//                sxpart = 0.4;
//                dxpart = 0.6;

                /* v3.1 wanh added for RF */
                if (here->B3SOIrgateMod == 2)
                    T0 = *(ckt->CKTstates[0] + here->B3SOIvges)
                         - *(ckt->CKTstates[0] + here->B3SOIvgs);
                else if (here->B3SOIrgateMod == 3)
                    T0 = *(ckt->CKTstates[0] + here->B3SOIvgms)
                         - *(ckt->CKTstates[0] + here->B3SOIvgs);
                if (here->B3SOIrgateMod > 1)
                {
                    gcrgd = here->B3SOIgcrgs * T0;
                    gcrgg = here->B3SOIgcrgg * T0;
                    gcrgs = here->B3SOIgcrgd * T0;
                    gcrgb = here->B3SOIgcrgb * T0;
                    gcrgg -= here->B3SOIgcrg;
                    gcrg = here->B3SOIgcrg;
                }
                else
                    gcrg = gcrgd = gcrgg = gcrgs = gcrgb = 0.0;

                /* v3.1 wanh added for RF end*/

            }

            gdpr=here->B3SOIdrainConductance;
            gspr=here->B3SOIsourceConductance;
            gds= here->B3SOIgds;

            GSoverlapCap = here->B3SOIcgso;
            GDoverlapCap = here->B3SOIcgdo;
            GEoverlapCap = here->pParam->B3SOIcgeo;

            EDextrinsicCap = here->B3SOIgcde;
            ESextrinsicCap = here->B3SOIgcse;


            /* v3.1 wanh added for RF */
            if (here->B3SOIrgateMod == 3)
            {
                xcgmgmb = (GDoverlapCap + GSoverlapCap + GEoverlapCap ) * omega;
                xcgmdb = -GDoverlapCap * omega;
                xcgmsb = -GSoverlapCap * omega;
                xcgmeb = -GEoverlapCap * omega;

                xcdgmb = xcgmdb;
                xcsgmb = xcgmsb;
                xcegmb = xcgmeb;

                xcedb = -EDextrinsicCap * omega;
                xcdeb = (cdeb - EDextrinsicCap) * omega;
                xcddb = (cddb + GDoverlapCap + EDextrinsicCap) * omega;
                xceeb = (ceeb + GEoverlapCap + EDextrinsicCap + ESextrinsicCap)
                        * omega;
                xcesb = -ESextrinsicCap * omega;
                xcssb = (GSoverlapCap + ESextrinsicCap - (cgsb + cbsb + cdsb))
                        * omega;

                xcseb = -(cbeb + cdeb + ceeb + ESextrinsicCap) * omega;

                xcegb = 0; /* v3.1 wanh change for RF */
                xceT  =  ceT * omega;
                xcggb = here->B3SOIcggb * omega;
                xcgdb = cgdb * omega;
                xcgsb = cgsb * omega;
                xcgeb = 0;
                xcgT  = cgT * omega;

                xcdgb = cdgb * omega;
                xcdsb = cdsb * omega;
                xcdT  = cdT * omega;

                xcsgb = -(cggb + cbgb + cdgb) * omega;
                xcsdb = -(cgdb + cbdb + cddb) * omega;
                xcsT  = -(cgT + cbT + cdT + ceT) * omega;

                xcbgb = cbgb * omega;
                xcbdb = cbdb * omega;
                xcbsb = cbsb * omega;
                xcbeb = cbeb * omega;
                xcbT  = cbT * omega;

                xcTt = cTt * omega;
            }

            else
            {
                xcedb = -EDextrinsicCap * omega;
                xcdeb = (cdeb - EDextrinsicCap) * omega;
                xcddb = (cddb + GDoverlapCap + EDextrinsicCap) * omega;
                xceeb = (ceeb + GEoverlapCap + EDextrinsicCap + ESextrinsicCap)
                        * omega;
                xcesb = -ESextrinsicCap * omega;
                xcssb = (GSoverlapCap + ESextrinsicCap - (cgsb + cbsb + cdsb))
                        * omega;

                xcseb = -(cbeb + cdeb + ceeb + ESextrinsicCap) * omega;

                xcegb = (- GEoverlapCap) * omega;
                xceT  =  ceT * omega;
                xcggb = (cggb + GDoverlapCap + GSoverlapCap + GEoverlapCap)
                        * omega;
                xcgdb = (cgdb - GDoverlapCap ) * omega;
                xcgsb = (cgsb - GSoverlapCap) * omega;
                xcgeb = (- GEoverlapCap) * omega;
                xcgT  = cgT * omega;

                xcdgb = (cdgb - GDoverlapCap) * omega;
                xcdsb = cdsb * omega;
                xcdT  = cdT * omega;

                xcsgb = -(cggb + cbgb + cdgb + GSoverlapCap) * omega;
                xcsdb = -(cgdb + cbdb + cddb) * omega;
                xcsT  = -(cgT + cbT + cdT + ceT) * omega;

                xcbgb = cbgb * omega;
                xcbdb = cbdb * omega;
                xcbsb = cbsb * omega;
                xcbeb = cbeb * omega;
                xcbT  = cbT * omega;

                xcTt = cTt * omega;

                /* v3.1 wanh added */
                xcdgmb = xcsgmb = xcegmb = 0.0;
                xcgmgmb = xcgmdb = xcgmsb = xcgmeb =0.0;
                /* v3.1 wanh added end */
            }

            /* v3.1 wanh added for RF */
            geltd = here->B3SOIgrgeltd;
            if (here->B3SOIrgateMod == 1)
            {
                *(here->B3SOIGEgePtr) += geltd;
                *(here->B3SOIGgePtr) -= geltd;
                *(here->B3SOIGEgPtr) -= geltd;
                *(here->B3SOIGgPtr) += geltd + gigg + gIgtotg; /* v3.1.1 bug fix */
                *(here->B3SOIGdpPtr) += gigd + gIgtotd; /* v3.1.1 bug fix */
                *(here->B3SOIGspPtr) += gigs + gIgtots; /* v3.1.1 bug fix */
                if (here->B3SOIsoiMod != 2) /* v3.2 */
                    *(here->B3SOIGbPtr) -= -gigb - gIgtotb; /* v3.1.1 bug fix */
            }

            else if (here->B3SOIrgateMod == 2)
            {
                *(here->B3SOIGEgePtr) += gcrg;
                *(here->B3SOIGEgPtr) += gcrgg;
                *(here->B3SOIGEdpPtr) += gcrgd;
                *(here->B3SOIGEspPtr) += gcrgs;
                if (here->B3SOIsoiMod != 2) /* v3.2 */
                    *(here->B3SOIGEbPtr) += gcrgb;

                *(here->B3SOIGgePtr) -= gcrg;
                *(here->B3SOIGgPtr) -= gcrgg - gigg - gIgtotg; /* v3.1.1 bug fix */
                *(here->B3SOIGdpPtr) -= gcrgd - gigd - gIgtotd; /* v3.1.1 bug fix */
                *(here->B3SOIGspPtr) -= gcrgs - gigs - gIgtots; /* v3.1.1 bug fix */
                if (here->B3SOIsoiMod != 2) /* v3.2 */
                    *(here->B3SOIGbPtr) -= gcrgb - gigb - gIgtotb; /* v3.1.1 bug fix */
            }

            else if (here->B3SOIrgateMod == 3)
            {
                *(here->B3SOIGEgePtr) += geltd;
                *(here->B3SOIGEgmPtr) -= geltd;
                *(here->B3SOIGMgePtr) -= geltd;
                *(here->B3SOIGMgmPtr) += geltd + gcrg;
                *(here->B3SOIGMgmPtr +1) += xcgmgmb;

                *(here->B3SOIGMdpPtr) += gcrgd;
                *(here->B3SOIGMdpPtr +1) += xcgmdb;
                *(here->B3SOIGMgPtr) += gcrgg;
                *(here->B3SOIGMspPtr) += gcrgs;
                *(here->B3SOIGMspPtr +1) += xcgmsb;
                if (here->B3SOIsoiMod != 2) /* v3.2 */
                    *(here->B3SOIGMbPtr) += gcrgb;
                *(here->B3SOIGMePtr +1) += xcgmeb;

                *(here->B3SOIDPgmPtr +1) += xcdgmb;
                *(here->B3SOIGgmPtr) -= gcrg;
                *(here->B3SOISPgmPtr +1) += xcsgmb;
                *(here->B3SOIEgmPtr +1) += xcegmb;

                *(here->B3SOIGgPtr) -= gcrgg - gigg - gIgtotg; /* v3.1.1 bug fix */
                *(here->B3SOIGdpPtr) -= gcrgd - gigd - gIgtotd; /* v3.1.1 bug fix */
                *(here->B3SOIGspPtr) -= gcrgs - gigs - gIgtots; /* v3.1.1 bug fix */
                if (here->B3SOIsoiMod != 2) /* v3.2 */
                    *(here->B3SOIGbPtr) -= gcrgb - gigb - gIgtotb; /* v3.1.1 bug fix */
            }
            else
            {
                *(here->B3SOIGgPtr)  += gigg + gIgtotg; /* v3.1.1 bug fix */
                *(here->B3SOIGdpPtr) += gigd + gIgtotd; /* v3.1.1 bug fix */
                *(here->B3SOIGspPtr) += gigs + gIgtots; /* v3.1.1 bug fix */
                if (here->B3SOIsoiMod != 2) /* v3.2 */
                    *(here->B3SOIGbPtr)  += gigb + gIgtotb; /* v3.1.1 bug fix */
            }
            /* v3.1 wanh added for RF end*/

            *(here->B3SOIEdpPtr +1) += xcedb;
            *(here->B3SOIEspPtr +1) += xcesb;
            *(here->B3SOIDPePtr +1) += xcdeb;
            *(here->B3SOISPePtr +1) += xcseb;
            *(here->B3SOIEgPtr  +1) += xcegb;
            *(here->B3SOIGePtr  +1) += xcgeb;

            *(here->B3SOIEePtr  +1) += xceeb;

            *(here->B3SOIGgPtr  +1) += xcggb;
            *(here->B3SOIGdpPtr +1) += xcgdb;
            *(here->B3SOIGspPtr +1) += xcgsb;

            *(here->B3SOIDPgPtr +1) += xcdgb;
            *(here->B3SOIDPdpPtr +1) += xcddb;
            *(here->B3SOIDPspPtr +1) += xcdsb;

            *(here->B3SOISPgPtr +1) += xcsgb;
            *(here->B3SOISPdpPtr +1) += xcsdb;
            *(here->B3SOISPspPtr +1) += xcssb;

            /* v3.1 */
            if (here->B3SOIsoiMod != 2) /* v3.2 */
            {
                *(here->B3SOIBePtr +1) += xcbeb;
                *(here->B3SOIBgPtr +1) += xcbgb;
                *(here->B3SOIBdpPtr +1) += xcbdb;
                *(here->B3SOIBspPtr +1) += xcbsb;

                *(here->B3SOIEbPtr  +1) -= xcegb + xceeb + xcedb + xcesb;
                *(here->B3SOIGbPtr +1) -= xcggb + xcgdb + xcgsb + xcgeb;
                /*               *(here->B3SOIDPbPtr +1) -= xcdgb + xcddb + xcdsb + xcdeb; */

                *(here->B3SOIDPbPtr +1) -= xcdgb + xcddb + xcdsb + xcdeb
                                           + xcdgmb;     /* v3.2 bug fix */
                /*               *(here->B3SOISPbPtr +1) -= xcsgb + xcsdb + xcssb + xcseb; */

                *(here->B3SOISPbPtr +1) -= xcsgb + xcsdb + xcssb + xcseb
                                           + xcsgmb;     /* v3.2 bug fix */
                *(here->B3SOIBbPtr +1) -= xcbgb + xcbdb + xcbsb + xcbeb;
            }
            /* v3.1 */


            if (selfheat)
            {
                *(here->B3SOITemptempPtr + 1) += xcTt;
                *(here->B3SOIDPtempPtr + 1) += xcdT;
                *(here->B3SOISPtempPtr + 1) += xcsT;
                *(here->B3SOIBtempPtr + 1) += xcbT;
                *(here->B3SOIEtempPtr + 1) += xceT;
                *(here->B3SOIGtempPtr + 1) += xcgT;
            }


            /* v3.0 */
            if (here->B3SOIsoiMod != 0) /* v3.2 */
            {
                *(here->B3SOIDPePtr) += Gme + gddpe;
                *(here->B3SOISPePtr) += gsspe - Gme;

                if (here->B3SOIsoiMod != 2) /* v3.2 */
                {
                    *(here->B3SOIGePtr) += gige;
                    *(here->B3SOIBePtr) -= gige;
                }
            }

            *(here->B3SOIEePtr) += 0.0;

            *(here->B3SOIDPgPtr) += Gm + gddpg - gIdtotg; /* v3.1.1 bug fix */
            *(here->B3SOIDPdpPtr) += gdpr + gds + gddpdp + RevSum - gIdtotd; /* v3.1.1 bug fix */
            *(here->B3SOIDPspPtr) -= gds + FwdSum - gddpsp + gIdtots; /* v3.1.1 bug fix */
            *(here->B3SOIDPdPtr) -= gdpr;

            *(here->B3SOISPgPtr) -= Gm - gsspg + gIstotg; /* v3.1.1 bug fix */
            *(here->B3SOISPdpPtr) -= gds + RevSum - gsspdp + gIstotd; /* v3.1.1 bug fix */
            *(here->B3SOISPspPtr) += gspr + gds + FwdSum + gsspsp - gIstots; /* v3.1.1 bug fix */
            *(here->B3SOISPsPtr) -= gspr;


            /* v3.1 */
            if (here->B3SOIsoiMod != 2) /* v3.2 */
            {
                *(here->B3SOIBePtr) += gbbe; /* v3.0 */
                *(here->B3SOIBgPtr)  += gbbg - gigg; /* v3.1 bug fix */
                *(here->B3SOIBdpPtr) += gbbdp - gigd; /* v3.1 bug fix */
                *(here->B3SOIBspPtr) += gbbsp - gigs; /* v3.1 bug fix */
                *(here->B3SOIBbPtr) += gbbb - gigb; /* v3.1 bug fix */
                *(here->B3SOISPbPtr) -= Gmbs - gsspb + gIstotb; /* v3.1.1 bug fix */
                *(here->B3SOIDPbPtr) -= (-gddpb - Gmbs) + gIdtotb; /* v3.1.1 bug fix */
            }
            /* v3.1 */



            if (selfheat)
            {
                *(here->B3SOIDPtempPtr) += GmT + gddpT;
                *(here->B3SOISPtempPtr) += -GmT + gsspT;
                *(here->B3SOIBtempPtr) += gbbT - gigT; /* v3.1 bug fix */
                *(here->B3SOIGtempPtr) += gigT; /* v3.1 bug fix */
                *(here->B3SOITemptempPtr) += gTtt + 1/here->pParam->B3SOIrth;
                *(here->B3SOITempgPtr) += gTtg;
                *(here->B3SOITempbPtr) += gTtb;
                *(here->B3SOITempdpPtr) += gTtdp;
                *(here->B3SOITempspPtr) += gTtsp;

                /* v3.0 */
                if (here->B3SOIsoiMod != 0) /* v3.2 */
                    *(here->B3SOITempePtr) += gTte;
            }


            *(here->B3SOIDdPtr) += gdpr;
            *(here->B3SOIDdpPtr) -= gdpr;
            *(here->B3SOISsPtr) += gspr;
            *(here->B3SOISspPtr) -= gspr;


            if (here->B3SOIbodyMod == 1)
            {
                (*(here->B3SOIBpPtr) -= gppp);
                (*(here->B3SOIPbPtr) += gppb);
                (*(here->B3SOIPpPtr) += gppp);
            }

            if (here->B3SOIdebugMod != 0)
            {
                *(here->B3SOIVbsPtr) += 1;
                *(here->B3SOIIdsPtr) += 1;
                *(here->B3SOIIcPtr) += 1;
                *(here->B3SOIIbsPtr) += 1;
                *(here->B3SOIIbdPtr) += 1;
                *(here->B3SOIIiiPtr) += 1;
                *(here->B3SOIIgidlPtr) += 1;
                *(here->B3SOIItunPtr) += 1;
                *(here->B3SOIIbpPtr) += 1;
                *(here->B3SOICbgPtr) += 1;
                *(here->B3SOICbbPtr) += 1;
                *(here->B3SOICbdPtr) += 1;
                *(here->B3SOIQbfPtr) += 1;
                *(here->B3SOIQjsPtr) += 1;
                *(here->B3SOIQjdPtr) += 1;

            }
// SRW
            if (here->B3SOIadjoint)
            {
                B3SOIadj *adj = here->B3SOIadjoint;
                adj->matrix->clear();

                if (here->B3SOIrgateMod == 1)
                {
                    *(adj->B3SOIGEgePtr) += geltd;
                    *(adj->B3SOIGgePtr) -= geltd;
                    *(adj->B3SOIGEgPtr) -= geltd;
                    *(adj->B3SOIGgPtr) += geltd + gigg + gIgtotg; /* v3.1.1 bug fix */
                    *(adj->B3SOIGdpPtr) += gigd + gIgtotd; /* v3.1.1 bug fix */
                    *(adj->B3SOIGspPtr) += gigs + gIgtots; /* v3.1.1 bug fix */
                    if (here->B3SOIsoiMod != 2) /* v3.2 */
                        *(adj->B3SOIGbPtr) -= -gigb - gIgtotb; /* v3.1.1 bug fix */
                }

                else if (here->B3SOIrgateMod == 2)
                {
                    *(adj->B3SOIGEgePtr) += gcrg;
                    *(adj->B3SOIGEgPtr) += gcrgg;
                    *(adj->B3SOIGEdpPtr) += gcrgd;
                    *(adj->B3SOIGEspPtr) += gcrgs;
                    if (here->B3SOIsoiMod != 2) /* v3.2 */
                        *(adj->B3SOIGEbPtr) += gcrgb;

                    *(adj->B3SOIGgePtr) -= gcrg;
                    *(adj->B3SOIGgPtr) -= gcrgg - gigg - gIgtotg; /* v3.1.1 bug fix */
                    *(adj->B3SOIGdpPtr) -= gcrgd - gigd - gIgtotd; /* v3.1.1 bug fix */
                    *(adj->B3SOIGspPtr) -= gcrgs - gigs - gIgtots; /* v3.1.1 bug fix */
                    if (here->B3SOIsoiMod != 2) /* v3.2 */
                        *(adj->B3SOIGbPtr) -= gcrgb - gigb - gIgtotb; /* v3.1.1 bug fix */
                }

                else if (here->B3SOIrgateMod == 3)
                {
                    *(adj->B3SOIGEgePtr) += geltd;
                    *(adj->B3SOIGEgmPtr) -= geltd;
                    *(adj->B3SOIGMgePtr) -= geltd;
                    *(adj->B3SOIGMgmPtr) += geltd + gcrg;
                    *(adj->B3SOIGMgmPtr +1) += xcgmgmb;

                    *(adj->B3SOIGMdpPtr) += gcrgd;
                    *(adj->B3SOIGMdpPtr +1) += xcgmdb;
                    *(adj->B3SOIGMgPtr) += gcrgg;
                    *(adj->B3SOIGMspPtr) += gcrgs;
                    *(adj->B3SOIGMspPtr +1) += xcgmsb;
                    if (here->B3SOIsoiMod != 2) /* v3.2 */
                        *(adj->B3SOIGMbPtr) += gcrgb;
                    *(adj->B3SOIGMePtr +1) += xcgmeb;

                    *(adj->B3SOIDPgmPtr +1) += xcdgmb;
                    *(adj->B3SOIGgmPtr) -= gcrg;
                    *(adj->B3SOISPgmPtr +1) += xcsgmb;
                    *(adj->B3SOIEgmPtr +1) += xcegmb;

                    *(adj->B3SOIGgPtr) -= gcrgg - gigg - gIgtotg; /* v3.1.1 bug fix */
                    *(adj->B3SOIGdpPtr) -= gcrgd - gigd - gIgtotd; /* v3.1.1 bug fix */
                    *(adj->B3SOIGspPtr) -= gcrgs - gigs - gIgtots; /* v3.1.1 bug fix */
                    if (here->B3SOIsoiMod != 2) /* v3.2 */
                        *(adj->B3SOIGbPtr) -= gcrgb - gigb - gIgtotb; /* v3.1.1 bug fix */
                }
                else
                {
                    *(adj->B3SOIGgPtr)  += gigg + gIgtotg; /* v3.1.1 bug fix */
                    *(adj->B3SOIGdpPtr) += gigd + gIgtotd; /* v3.1.1 bug fix */
                    *(adj->B3SOIGspPtr) += gigs + gIgtots; /* v3.1.1 bug fix */
                    if (here->B3SOIsoiMod != 2) /* v3.2 */
                        *(adj->B3SOIGbPtr)  += gigb + gIgtotb; /* v3.1.1 bug fix */
                }
                /* v3.1 wanh added for RF end*/

                *(adj->B3SOIEdpPtr +1) += xcedb;
                *(adj->B3SOIEspPtr +1) += xcesb;
                *(adj->B3SOIDPePtr +1) += xcdeb;
                *(adj->B3SOISPePtr +1) += xcseb;
                *(adj->B3SOIEgPtr  +1) += xcegb;
                *(adj->B3SOIGePtr  +1) += xcgeb;

                *(adj->B3SOIEePtr  +1) += xceeb;

                *(adj->B3SOIGgPtr  +1) += xcggb;
                *(adj->B3SOIGdpPtr +1) += xcgdb;
                *(adj->B3SOIGspPtr +1) += xcgsb;

                *(adj->B3SOIDPgPtr +1) += xcdgb;
                *(adj->B3SOIDPdpPtr +1) += xcddb;
                *(adj->B3SOIDPspPtr +1) += xcdsb;

                *(adj->B3SOISPgPtr +1) += xcsgb;
                *(adj->B3SOISPdpPtr +1) += xcsdb;
                *(adj->B3SOISPspPtr +1) += xcssb;

                /* v3.1 */
                if (here->B3SOIsoiMod != 2) /* v3.2 */
                {
                    *(adj->B3SOIBePtr +1) += xcbeb;
                    *(adj->B3SOIBgPtr +1) += xcbgb;
                    *(adj->B3SOIBdpPtr +1) += xcbdb;
                    *(adj->B3SOIBspPtr +1) += xcbsb;

                    *(adj->B3SOIEbPtr  +1) -= xcegb + xceeb + xcedb + xcesb;
                    *(adj->B3SOIGbPtr +1) -= xcggb + xcgdb + xcgsb + xcgeb;
                    /*               *(adj->B3SOIDPbPtr +1) -= xcdgb + xcddb + xcdsb + xcdeb; */

                    *(adj->B3SOIDPbPtr +1) -= xcdgb + xcddb + xcdsb + xcdeb
                                              + xcdgmb;     /* v3.2 bug fix */
                    /*               *(adj->B3SOISPbPtr +1) -= xcsgb + xcsdb + xcssb + xcseb; */

                    *(adj->B3SOISPbPtr +1) -= xcsgb + xcsdb + xcssb + xcseb
                                              + xcsgmb;     /* v3.2 bug fix */
                    *(adj->B3SOIBbPtr +1) -= xcbgb + xcbdb + xcbsb + xcbeb;
                }
                /* v3.1 */


                if (selfheat)
                {
                    *(adj->B3SOITemptempPtr + 1) += xcTt;
                    *(adj->B3SOIDPtempPtr + 1) += xcdT;
                    *(adj->B3SOISPtempPtr + 1) += xcsT;
                    *(adj->B3SOIBtempPtr + 1) += xcbT;
                    *(adj->B3SOIEtempPtr + 1) += xceT;
                    *(adj->B3SOIGtempPtr + 1) += xcgT;
                }


                /* v3.0 */
                if (here->B3SOIsoiMod != 0) /* v3.2 */
                {
                    *(adj->B3SOIDPePtr) += Gme + gddpe;
                    *(adj->B3SOISPePtr) += gsspe - Gme;

                    if (here->B3SOIsoiMod != 2) /* v3.2 */
                    {
                        *(adj->B3SOIGePtr) += gige;
                        *(adj->B3SOIBePtr) -= gige;
                    }
                }

                *(adj->B3SOIEePtr) += 0.0;

                *(adj->B3SOIDPgPtr) += Gm + gddpg - gIdtotg; /* v3.1.1 bug fix */
                *(adj->B3SOIDPdpPtr) += gdpr + gds + gddpdp + RevSum - gIdtotd; /* v3.1.1 bug fix */
                *(adj->B3SOIDPspPtr) -= gds + FwdSum - gddpsp + gIdtots; /* v3.1.1 bug fix */
                *(adj->B3SOIDPdPtr) -= gdpr;

                *(adj->B3SOISPgPtr) -= Gm - gsspg + gIstotg; /* v3.1.1 bug fix */
                *(adj->B3SOISPdpPtr) -= gds + RevSum - gsspdp + gIstotd; /* v3.1.1 bug fix */
                *(adj->B3SOISPspPtr) += gspr + gds + FwdSum + gsspsp - gIstots; /* v3.1.1 bug fix */
                *(adj->B3SOISPsPtr) -= gspr;


                /* v3.1 */
                if (here->B3SOIsoiMod != 2) /* v3.2 */
                {
                    *(adj->B3SOIBePtr) += gbbe; /* v3.0 */
                    *(adj->B3SOIBgPtr)  += gbbg - gigg; /* v3.1 bug fix */
                    *(adj->B3SOIBdpPtr) += gbbdp - gigd; /* v3.1 bug fix */
                    *(adj->B3SOIBspPtr) += gbbsp - gigs; /* v3.1 bug fix */
                    *(adj->B3SOIBbPtr) += gbbb - gigb; /* v3.1 bug fix */
                    *(adj->B3SOISPbPtr) -= Gmbs - gsspb + gIstotb; /* v3.1.1 bug fix */
                    *(adj->B3SOIDPbPtr) -= (-gddpb - Gmbs) + gIdtotb; /* v3.1.1 bug fix */
                }
                /* v3.1 */



                if (selfheat)
                {
                    *(adj->B3SOIDPtempPtr) += GmT + gddpT;
                    *(adj->B3SOISPtempPtr) += -GmT + gsspT;
                    *(adj->B3SOIBtempPtr) += gbbT - gigT; /* v3.1 bug fix */
                    *(adj->B3SOIGtempPtr) += gigT; /* v3.1 bug fix */
                    *(adj->B3SOITemptempPtr) += gTtt + 1/here->pParam->B3SOIrth;
                    *(adj->B3SOITempgPtr) += gTtg;
                    *(adj->B3SOITempbPtr) += gTtb;
                    *(adj->B3SOITempdpPtr) += gTtdp;
                    *(adj->B3SOITempspPtr) += gTtsp;

                    /* v3.0 */
                    if (here->B3SOIsoiMod != 0) /* v3.2 */
                        *(adj->B3SOITempePtr) += gTte;
                }


                *(adj->B3SOIDdPtr) += gdpr;
                *(adj->B3SOIDdpPtr) -= gdpr;
                *(adj->B3SOISsPtr) += gspr;
                *(adj->B3SOISspPtr) -= gspr;


                if (here->B3SOIbodyMod == 1)
                {
                    (*(adj->B3SOIBpPtr) -= gppp);
                    (*(adj->B3SOIPbPtr) += gppb);
                    (*(adj->B3SOIPpPtr) += gppp);
                }
            }
// SRW - end

        }
    }
    return(OK);
}

