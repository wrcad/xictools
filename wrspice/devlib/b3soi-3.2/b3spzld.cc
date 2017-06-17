
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
 $Id: b3spzld.cc,v 1.4 2011/12/18 01:15:13 stevew Exp $
 *========================================================================*/

/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1998 Samuel Fung, Dennis Sinitsky and Stephen Tang
File: b3soipzld.c          98/5/01
**********/

#include "b3sdefs.h"

#define B3SOInextModel      next()
#define B3SOInextInstance   next()
#define B3SOIinstances      inst()


int
B3SOIdev::pzLoad(sGENmodel *genmod, sCKT*, IFcomplex *s)
{
    sB3SOImodel *model = static_cast<sB3SOImodel*>(genmod);
    sB3SOIinstance *here;

    double xcggb, xcgdb, xcgsb, xcbgb, xcbdb, xcbsb, xcddb, xcssb, xcdgb;
    double gdpr, gspr, gds, gbd, gbs, capbd=0.0, capbs=0.0, xcsgb, xcdsb, xcsdb;
    double cggb, cgdb, cgsb, cbgb, cbdb, cbsb, cddb, cdgb, cdsb;
    double GSoverlapCap, GDoverlapCap, GBoverlapCap=0.0;
    double FwdSum, RevSum, Gm, Gmbs;

    for (; model != NULL; model = model->B3SOInextModel)
    {
        for (here = model->B3SOIinstances; here!= NULL;
                here = here->B3SOInextInstance)
        {
            if (here->B3SOImode >= 0)
            {
                Gm = here->B3SOIgm;
                Gmbs = here->B3SOIgmbs;
                FwdSum = Gm + Gmbs;
                RevSum = 0.0;
                cggb = here->B3SOIcggb;
                cgsb = here->B3SOIcgsb;
                cgdb = here->B3SOIcgdb;

                cbgb = here->B3SOIcbgb;
                cbsb = here->B3SOIcbsb;
                cbdb = here->B3SOIcbdb;

                cdgb = here->B3SOIcdgb;
                cdsb = here->B3SOIcdsb;
                cddb = here->B3SOIcddb;
            }
            else
            {
                Gm = -here->B3SOIgm;
                Gmbs = -here->B3SOIgmbs;
                FwdSum = 0.0;
                RevSum = -Gm - Gmbs;
                cggb = here->B3SOIcggb;
                cgsb = here->B3SOIcgdb;
                cgdb = here->B3SOIcgsb;

                cbgb = here->B3SOIcbgb;
                cbsb = here->B3SOIcbdb;
                cbdb = here->B3SOIcbsb;

                cdgb = -(here->B3SOIcdgb + cggb + cbgb);
                cdsb = -(here->B3SOIcddb + cgsb + cbsb);
                cddb = -(here->B3SOIcdsb + cgdb + cbdb);
            }
            gdpr=here->B3SOIdrainConductance;
            gspr=here->B3SOIsourceConductance;
            gds= here->B3SOIgds;
            gbd= here->B3SOIgjdb;
            gbs= here->B3SOIgjsb;
#ifdef BULKCODE
            capbd= here->B3SOIcapbd;
            capbs= here->B3SOIcapbs;
#endif
            GSoverlapCap = here->B3SOIcgso;
            GDoverlapCap = here->B3SOIcgdo;
#ifdef BULKCODE
            GBoverlapCap = here->pParam->B3SOIcgbo;
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


            *(here->B3SOIGgPtr ) += xcggb * s->real;
            *(here->B3SOIGgPtr +1) += xcggb * s->imag;
            *(here->B3SOIBbPtr ) += (-xcbgb-xcbdb-xcbsb) * s->real;
            *(here->B3SOIBbPtr +1) += (-xcbgb-xcbdb-xcbsb) * s->imag;
            *(here->B3SOIDPdpPtr ) += xcddb * s->real;
            *(here->B3SOIDPdpPtr +1) += xcddb * s->imag;
            *(here->B3SOISPspPtr ) += xcssb * s->real;
            *(here->B3SOISPspPtr +1) += xcssb * s->imag;
            *(here->B3SOIGbPtr ) += (-xcggb-xcgdb-xcgsb) * s->real;
            *(here->B3SOIGbPtr +1) += (-xcggb-xcgdb-xcgsb) * s->imag;
            *(here->B3SOIGdpPtr ) += xcgdb * s->real;
            *(here->B3SOIGdpPtr +1) += xcgdb * s->imag;
            *(here->B3SOIGspPtr ) += xcgsb * s->real;
            *(here->B3SOIGspPtr +1) += xcgsb * s->imag;
            *(here->B3SOIBgPtr ) += xcbgb * s->real;
            *(here->B3SOIBgPtr +1) += xcbgb * s->imag;
            *(here->B3SOIBdpPtr ) += xcbdb * s->real;
            *(here->B3SOIBdpPtr +1) += xcbdb * s->imag;
            *(here->B3SOIBspPtr ) += xcbsb * s->real;
            *(here->B3SOIBspPtr +1) += xcbsb * s->imag;
            *(here->B3SOIDPgPtr ) += xcdgb * s->real;
            *(here->B3SOIDPgPtr +1) += xcdgb * s->imag;
            *(here->B3SOIDPbPtr ) += (-xcdgb-xcddb-xcdsb) * s->real;
            *(here->B3SOIDPbPtr +1) += (-xcdgb-xcddb-xcdsb) * s->imag;
            *(here->B3SOIDPspPtr ) += xcdsb * s->real;
            *(here->B3SOIDPspPtr +1) += xcdsb * s->imag;
            *(here->B3SOISPgPtr ) += xcsgb * s->real;
            *(here->B3SOISPgPtr +1) += xcsgb * s->imag;
            *(here->B3SOISPbPtr ) += (-xcsgb-xcsdb-xcssb) * s->real;
            *(here->B3SOISPbPtr +1) += (-xcsgb-xcsdb-xcssb) * s->imag;
            *(here->B3SOISPdpPtr ) += xcsdb * s->real;
            *(here->B3SOISPdpPtr +1) += xcsdb * s->imag;
            *(here->B3SOIDdPtr) += gdpr;
            *(here->B3SOISsPtr) += gspr;
            *(here->B3SOIBbPtr) += gbd+gbs;
            *(here->B3SOIDPdpPtr) += gdpr+gds+gbd+RevSum;
            *(here->B3SOISPspPtr) += gspr+gds+gbs+FwdSum;
            *(here->B3SOIDdpPtr) -= gdpr;
            *(here->B3SOISspPtr) -= gspr;
            *(here->B3SOIBdpPtr) -= gbd;
            *(here->B3SOIBspPtr) -= gbs;
            *(here->B3SOIDPdPtr) -= gdpr;
            *(here->B3SOIDPgPtr) += Gm;
            *(here->B3SOIDPbPtr) -= gbd - Gmbs;
            *(here->B3SOIDPspPtr) -= gds + FwdSum;
            *(here->B3SOISPgPtr) -= Gm;
            *(here->B3SOISPsPtr) -= gspr;
            *(here->B3SOISPbPtr) -= gbs + Gmbs;
            *(here->B3SOISPdpPtr) -= gds + RevSum;

        }
    }
    return(OK);
}

