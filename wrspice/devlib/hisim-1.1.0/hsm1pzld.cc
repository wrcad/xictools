
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

/***********************************************************************
 HiSIM v1.1.0
 File: hsm1pzld.c of HiSIM v1.1.0

 Copyright (C) 2002 STARC

 June 30, 2002: developed by Hiroshima University and STARC
 June 30, 2002: posted by Keiichi MORIKAWA, STARC Physical Design Group
***********************************************************************/

#include "hsm1defs.h"

#define HSM1nextModel      next()
#define HSM1nextInstance   next()
#define HSM1instances      inst()


int
HSM1dev::pzLoad(sGENmodel *genmod, sCKT*, IFcomplex *s)
{
    sHSM1model *model = static_cast<sHSM1model*>(genmod);
    sHSM1instance *here;

  double xcggb, xcgdb, xcgsb, xcgbb, xcbgb, xcbdb, xcbsb, xcbbb;
  double xcdgb, xcddb, xcdsb, xcdbb, xcsgb, xcsdb, xcssb, xcsbb;
  double gdpr, gspr, gds, gbd, gbs, capbd, capbs, FwdSum, RevSum, gm, gmbs;
  double cggb, cgdb, cgsb, cbgb, cbdb, cbsb, cddb, cdgb, cdsb;
  double cgso, cgdo, cgbo;
  double gbspsp, gbbdp, gbbsp, gbspg, gbspb;
  double gbspdp, gbdpdp, gbdpg, gbdpb, gbdpsp;
  
  for ( ;model != NULL ;model = model->HSM1nextModel ) {
    for ( here = model->HSM1instances ;here!= NULL ;
          here = here->HSM1nextInstance ) {
      if ( here->HSM1_mode >= 0 ) {
        gm = here->HSM1_gm;
        gmbs = here->HSM1_gmbs;
        FwdSum = gm + gmbs;
        RevSum = 0.0;
        
        gbbdp = -here->HSM1_gbds;
        gbbsp = here->HSM1_gbds + here->HSM1_gbgs + here->HSM1_gbbs;
        
        gbdpg = here->HSM1_gbgs;
        gbdpdp = here->HSM1_gbds;
        gbdpb = here->HSM1_gbbs;
        gbdpsp = -(gbdpg + gbdpdp + gbdpb);
        
        gbspg = 0.0;
        gbspdp = 0.0;
        gbspb = 0.0;
        gbspsp = 0.0;
        
        cggb = here->HSM1_cggb;
        cgsb = here->HSM1_cgsb;
        cgdb = here->HSM1_cgdb;
        
        cbgb = here->HSM1_cbgb;
        cbsb = here->HSM1_cbsb;
        cbdb = here->HSM1_cbdb;
        
        cdgb = here->HSM1_cdgb;
        cdsb = here->HSM1_cdsb;
        cddb = here->HSM1_cddb;
        
      }
      else {
        gm = -here->HSM1_gm;
        gmbs = -here->HSM1_gmbs;
        FwdSum = 0.0;
        RevSum = -(gm + gmbs);
        
        gbbsp = -here->HSM1_gbds;
        gbbdp = here->HSM1_gbds + here->HSM1_gbgs + here->HSM1_gbbs;
        
        gbdpg = 0.0;
        gbdpsp = 0.0;
        gbdpb = 0.0;
        gbdpdp = 0.0;
        
        gbspg = here->HSM1_gbgs;
        gbspsp = here->HSM1_gbds;
        gbspb = here->HSM1_gbbs;
        gbspdp = -(gbspg + gbspsp + gbspb);
        
        cggb = here->HSM1_cggb;
        cgsb = here->HSM1_cgdb;
        cgdb = here->HSM1_cgsb;
          
        cbgb = here->HSM1_cbgb;
        cbsb = here->HSM1_cbdb;
        cbdb = here->HSM1_cbsb;
        
        cdgb = -(here->HSM1_cdgb + cggb + cbgb);
        cdsb = -(here->HSM1_cddb + cgsb + cbsb);
        cddb = -(here->HSM1_cdsb + cgdb + cbdb);
      }
      
      gdpr = here->HSM1drainConductance;
      gspr = here->HSM1sourceConductance;
      gds = here->HSM1_gds;
      gbd = here->HSM1_gbd;
      gbs = here->HSM1_gbs;
      capbd = here->HSM1_capbd;
      capbs = here->HSM1_capbs;
      
      cgso = here->HSM1_cgso;
      cgdo = here->HSM1_cgdo;
      cgbo = here->HSM1_cgbo;
      
      xcdgb = (cdgb - cgdo);
      xcddb = (cddb + capbd + cgdo);
      xcdsb = cdsb;
      xcdbb = -(xcdgb + xcddb + xcdsb);
      xcsgb = -(cggb + cbgb + cdgb + cgso);
      xcsdb = -(cgdb + cbdb + cddb);
      xcssb = (capbs + cgso - (cgsb + cbsb + cdsb));
      xcsbb = -(xcsgb + xcsdb + xcssb); 
      xcggb = (cggb + cgdo + cgso + cgbo);
      xcgdb = (cgdb - cgdo);
      xcgsb = (cgsb - cgso);
      xcgbb = -(xcggb + xcgdb + xcgsb);
      xcbgb = (cbgb - cgbo);
      xcbdb = (cbdb - capbd);
      xcbsb = (cbsb - capbs);
      xcbbb = -(xcbgb + xcbdb + xcbsb);
      
      *(here->HSM1GgPtr ) += xcggb * s->real;
      *(here->HSM1GgPtr +1) += xcggb * s->imag;
      *(here->HSM1BbPtr ) += xcbbb * s->real;
      *(here->HSM1BbPtr +1) += xcbbb * s->imag;
      *(here->HSM1DPdpPtr ) += xcddb * s->real;
      *(here->HSM1DPdpPtr +1) += xcddb * s->imag;
      *(here->HSM1SPspPtr ) += xcssb * s->real;
      *(here->HSM1SPspPtr +1) += xcssb * s->imag;
      
      *(here->HSM1GbPtr ) += xcgbb * s->real;
      *(here->HSM1GbPtr +1) += xcgbb * s->imag;
      *(here->HSM1GdpPtr ) += xcgdb * s->real;
      *(here->HSM1GdpPtr +1) += xcgdb * s->imag;
      *(here->HSM1GspPtr ) += xcgsb * s->real;
      *(here->HSM1GspPtr +1) += xcgsb * s->imag;
      
      *(here->HSM1BgPtr ) += xcbgb * s->real;
      *(here->HSM1BgPtr +1) += xcbgb * s->imag;
      *(here->HSM1BdpPtr ) += xcbdb * s->real;
      *(here->HSM1BdpPtr +1) += xcbdb * s->imag;
      *(here->HSM1BspPtr ) += xcbsb * s->real;
      *(here->HSM1BspPtr +1) += xcbsb * s->imag;
      
      *(here->HSM1DPgPtr ) += xcdgb * s->real;
      *(here->HSM1DPgPtr +1) += xcdgb * s->imag;
      *(here->HSM1DPbPtr ) += xcdbb * s->real;
      *(here->HSM1DPbPtr +1) += xcdbb * s->imag;
      *(here->HSM1DPspPtr ) += xcdsb * s->real;
      *(here->HSM1DPspPtr +1) += xcdsb * s->imag;
      
      *(here->HSM1SPgPtr ) += xcsgb * s->real;
      *(here->HSM1SPgPtr +1) += xcsgb * s->imag;
      *(here->HSM1SPbPtr ) += xcsbb * s->real;
      *(here->HSM1SPbPtr +1) += xcsbb * s->imag;
      *(here->HSM1SPdpPtr ) += xcsdb * s->real;
      *(here->HSM1SPdpPtr +1) += xcsdb * s->imag;
      
      *(here->HSM1DdPtr) += gdpr;
      *(here->HSM1DdpPtr) -= gdpr;
      *(here->HSM1DPdPtr) -= gdpr;
      
      *(here->HSM1SsPtr) += gspr;
      *(here->HSM1SspPtr) -= gspr;
      *(here->HSM1SPsPtr) -= gspr;
      
      *(here->HSM1BgPtr) -= here->HSM1_gbgs;
      *(here->HSM1BbPtr) += gbd + gbs - here->HSM1_gbbs;
      *(here->HSM1BdpPtr) -= gbd - gbbdp;
      *(here->HSM1BspPtr) -= gbs - gbbsp;
      
      *(here->HSM1DPgPtr) += gm + gbdpg;
      *(here->HSM1DPdpPtr) += gdpr + gds + gbd + RevSum + gbdpdp;
      *(here->HSM1DPspPtr) -= gds + FwdSum - gbdpsp;
      *(here->HSM1DPbPtr) -= gbd - gmbs - gbdpb;
      
      *(here->HSM1SPgPtr) -= gm - gbspg;
      *(here->HSM1SPspPtr) += gspr + gds + gbs + FwdSum + gbspsp;
      *(here->HSM1SPbPtr) -= gbs + gmbs - gbspb;
      *(here->HSM1SPdpPtr) -= gds + RevSum - gbspdp;
      
      /*        
       *(here->HSM1GgPtr) -= xgtg;
       *(here->HSM1GbPtr) -= xgtb;
       *(here->HSM1GdpPtr) -= xgtd;
       *(here->HSM1GspPtr) -= xgts;
       */
      
    }
  }
  return(OK);
}

