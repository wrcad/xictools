
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
 $Id: b4spzld.cc,v 1.3 2011/12/17 06:19:04 stevew Exp $
 *========================================================================*/

/***  B4SOI  12/31/2009 Released by Tanvir Morshed  ***/

/**********
 * Copyright 2009 Regents of the University of California.  All rights reserved.
 * Authors: 1998 Samuel Fung, Dennis Sinitsky and Stephen Tang
 * Authors: 1999-2004 Pin Su, Hui Wan, Wei Jin, b3soipzld.c
 * Authors: 2005- Hui Wan, Xuemei Xi, Ali Niknejad, Chenming Hu.
 * Authors: 2009- Wenwei Yang, Chung-Hsun Lin, Ali Niknejad, Chenming Hu.
 * File: b4soipzld.c
 * Modified by Hui Wan, Xuemei Xi 11/30/2005
 * Modified by Wenwei Yang, Chung-Hsun Lin, Darsen Lu 03/06/2009
 * Modified by Tanvir Morshed 09/22/2009
 * Modified by Tanvir Morshed 12/31/2009
 **********/

#include "b4sdefs.h"


#define B4SOInextModel      next()
#define B4SOInextInstance   next()
#define B4SOIinstances      inst()


int
B4SOIdev::pzLoad(sGENmodel *genmod, sCKT*, IFcomplex *s)
{
    sB4SOImodel *model = static_cast<sB4SOImodel*>(genmod);
    sB4SOIinstance *here;

    double xcggb, xcgdb, xcgsb, xcbgb, xcbdb, xcbsb, xcddb, xcssb, xcdgb;
    double gdpr, gspr, gds, gbd, gbs, capbd=0.0, capbs=0.0, xcsgb, xcdsb, xcsdb;
    double cggb, cgdb, cgsb, cbgb, cbdb, cbsb, cddb, cdgb, cdsb;
    double GSoverlapCap, GDoverlapCap, GBoverlapCap=0.0;
    double FwdSum, RevSum, Gm, Gmbs;

    for (; model != NULL; model = model->B4SOInextModel)
    {
        for (here = model->B4SOIinstances; here!= NULL;
                here = here->B4SOInextInstance)
        {
            if (here->B4SOImode >= 0)
            {
                Gm = here->B4SOIgm;
                Gmbs = here->B4SOIgmbs;
                FwdSum = Gm + Gmbs;
                RevSum = 0.0;
                cggb = here->B4SOIcggb;
                cgsb = here->B4SOIcgsb;
                cgdb = here->B4SOIcgdb;

                cbgb = here->B4SOIcbgb;
                cbsb = here->B4SOIcbsb;
                cbdb = here->B4SOIcbdb;

                cdgb = here->B4SOIcdgb;
                cdsb = here->B4SOIcdsb;
                cddb = here->B4SOIcddb;
            }
            else
            {
                Gm = -here->B4SOIgm;
                Gmbs = -here->B4SOIgmbs;
                FwdSum = 0.0;
                RevSum = -Gm - Gmbs;
                cggb = here->B4SOIcggb;
                cgsb = here->B4SOIcgdb;
                cgdb = here->B4SOIcgsb;

                cbgb = here->B4SOIcbgb;
                cbsb = here->B4SOIcbdb;
                cbdb = here->B4SOIcbsb;

                cdgb = -(here->B4SOIcdgb + cggb + cbgb);
                cdsb = -(here->B4SOIcddb + cgsb + cbsb);
                cddb = -(here->B4SOIcdsb + cgdb + cbdb);
            }
            gdpr=here->B4SOIdrainConductance;
            gspr=here->B4SOIsourceConductance;
            gds= here->B4SOIgds;
            gbd= here->B4SOIgjdb;
            gbs= here->B4SOIgjsb;
#ifdef BULKCODE
            capbd= here->B4SOIcapbd;
            capbs= here->B4SOIcapbs;
#endif
            GSoverlapCap = here->B4SOIcgso;
            GDoverlapCap = here->B4SOIcgdo;
#ifdef BULKCODE
            GBoverlapCap = here->pParam->B4SOIcgbo;
#endif

            xcdgb = (cdgb - GDoverlapCap);
            xcddb = (cddb + capbd + GDoverlapCap);
            xcdsb = cdsb;
            xcsgb = -(cggb + cbgb + cdgb + GSoverlapCap);
            xcsdb = -(cgdb + cbdb + cddb);
            xcssb = (capbs + GSoverlapCap - (cgsb+cbsb+cdsb));
            xcggb = (cggb + GDoverlapCap + GSoverlapCap + GBoverlapCap);
            xcgdb = (cgdb - GDoverlapCap);
            xcgsb = (cgsb - GSoverlapCap);
            xcbgb = (cbgb - GBoverlapCap);
            xcbdb = (cbdb - capbd);
            xcbsb = (cbsb - capbs);


            *(here->B4SOIGgPtr ) += xcggb * s->real;
            *(here->B4SOIGgPtr +1) += xcggb * s->imag;
            *(here->B4SOIBbPtr ) += (-xcbgb-xcbdb-xcbsb) * s->real;
            *(here->B4SOIBbPtr +1) += (-xcbgb-xcbdb-xcbsb) * s->imag;
            *(here->B4SOIDPdpPtr ) += xcddb * s->real;
            *(here->B4SOIDPdpPtr +1) += xcddb * s->imag;
            *(here->B4SOISPspPtr ) += xcssb * s->real;
            *(here->B4SOISPspPtr +1) += xcssb * s->imag;
            *(here->B4SOIGbPtr ) += (-xcggb-xcgdb-xcgsb) * s->real;
            *(here->B4SOIGbPtr +1) += (-xcggb-xcgdb-xcgsb) * s->imag;
            *(here->B4SOIGdpPtr ) += xcgdb * s->real;
            *(here->B4SOIGdpPtr +1) += xcgdb * s->imag;
            *(here->B4SOIGspPtr ) += xcgsb * s->real;
            *(here->B4SOIGspPtr +1) += xcgsb * s->imag;
            *(here->B4SOIBgPtr ) += xcbgb * s->real;
            *(here->B4SOIBgPtr +1) += xcbgb * s->imag;
            *(here->B4SOIBdpPtr ) += xcbdb * s->real;
            *(here->B4SOIBdpPtr +1) += xcbdb * s->imag;
            *(here->B4SOIBspPtr ) += xcbsb * s->real;
            *(here->B4SOIBspPtr +1) += xcbsb * s->imag;
            *(here->B4SOIDPgPtr ) += xcdgb * s->real;
            *(here->B4SOIDPgPtr +1) += xcdgb * s->imag;
            *(here->B4SOIDPbPtr ) += (-xcdgb-xcddb-xcdsb) * s->real;
            *(here->B4SOIDPbPtr +1) += (-xcdgb-xcddb-xcdsb) * s->imag;
            *(here->B4SOIDPspPtr ) += xcdsb * s->real;
            *(here->B4SOIDPspPtr +1) += xcdsb * s->imag;
            *(here->B4SOISPgPtr ) += xcsgb * s->real;
            *(here->B4SOISPgPtr +1) += xcsgb * s->imag;
            *(here->B4SOISPbPtr ) += (-xcsgb-xcsdb-xcssb) * s->real;
            *(here->B4SOISPbPtr +1) += (-xcsgb-xcsdb-xcssb) * s->imag;
            *(here->B4SOISPdpPtr ) += xcsdb * s->real;
            *(here->B4SOISPdpPtr +1) += xcsdb * s->imag;
            *(here->B4SOIDdPtr) += gdpr;
            *(here->B4SOISsPtr) += gspr;
            *(here->B4SOIBbPtr) += gbd+gbs;
            *(here->B4SOIDPdpPtr) += gdpr+gds+gbd+RevSum;
            *(here->B4SOISPspPtr) += gspr+gds+gbs+FwdSum;
            *(here->B4SOIDdpPtr) -= gdpr;
            *(here->B4SOISspPtr) -= gspr;
            *(here->B4SOIBdpPtr) -= gbd;
            *(here->B4SOIBspPtr) -= gbs;
            *(here->B4SOIDPdPtr) -= gdpr;
            *(here->B4SOIDPgPtr) += Gm;
            *(here->B4SOIDPbPtr) -= gbd - Gmbs;
            *(here->B4SOIDPspPtr) -= gds + FwdSum;
            *(here->B4SOISPgPtr) -= Gm;
            *(here->B4SOISPsPtr) -= gspr;
            *(here->B4SOISPbPtr) -= gbs + Gmbs;
            *(here->B4SOISPdpPtr) -= gds + RevSum;

        }
    }
    return(OK);
}

