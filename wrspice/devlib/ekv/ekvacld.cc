
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

/*
 * Author: 2000 Wladek Grabinski; EKV v2.6 Model Upgrade
 * Author: 1997 Eckhard Brass;    EKV v2.5 Model Implementation
 *     (C) 1990 Regents of the University of California. Spice3 Format
 */

#include "ekvdefs.h"

#define EKVnextModel      next()
#define EKVnextInstance   next()
#define EKVinstances      inst()


int
EKVdev::acLoad(sGENmodel *genmod, sCKT *ckt)
{
    sEKVmodel *model = static_cast<sEKVmodel*>(genmod);
    sEKVinstance *here;

    int xnrm;
    int xrev;
    double xgs;
    double xgd;
    double xgb;
    double xbd;
    double xbs;
    double capgs;
    double capgd;
    double capgb;
    double GateBulkOverlapCap;
    double GateDrainOverlapCap;
    double GateSourceOverlapCap;
    double EffectiveLength;
    double EffectiveWidth;

    for( ; model != NULL; model = model->EKVnextModel) {
        for(here = model->EKVinstances; here!= NULL;
            here = here->EKVnextInstance) {

            if (here->EKVmode < 0) {
                xnrm=0;
                xrev=1;
            } else {
                xnrm=1;
                xrev=0;
            }
            /*
             *     meyer's model parameters
             */
            EffectiveLength=here->EKVl+model->EKVdl;
            EffectiveWidth =here->EKVw+model->EKVdw;

            GateSourceOverlapCap = model->EKVgateSourceOverlapCapFactor * 
                EffectiveWidth;
            GateDrainOverlapCap = model->EKVgateDrainOverlapCapFactor * 
                EffectiveWidth;
            GateBulkOverlapCap = model->EKVgateBulkOverlapCapFactor * 
                EffectiveLength;
            capgs = ( *(ckt->CKTstate0+here->EKVcapgs)+ 
                *(ckt->CKTstate0+here->EKVcapgs) +
                GateSourceOverlapCap );
            capgd = ( *(ckt->CKTstate0+here->EKVcapgd)+ 
                *(ckt->CKTstate0+here->EKVcapgd) +
                GateDrainOverlapCap );
            capgb = ( *(ckt->CKTstate0+here->EKVcapgb)+ 
                *(ckt->CKTstate0+here->EKVcapgb) +
                GateBulkOverlapCap );
            xgs = capgs * ckt->CKTomega;
            xgd = capgd * ckt->CKTomega;
            xgb = capgb * ckt->CKTomega;
            xbd  = here->EKVcapbd * ckt->CKTomega;
            xbs  = here->EKVcapbs * ckt->CKTomega;
            /*
             *    load matrix
             */

            *(here->EKVGgPtr +1) += xgd+xgs+xgb;
            *(here->EKVBbPtr +1) += xgb+xbd+xbs;
            *(here->EKVDPdpPtr +1) += xgd+xbd;
            *(here->EKVSPspPtr +1) += xgs+xbs;
            *(here->EKVGbPtr +1) -= xgb;
            *(here->EKVGdpPtr +1) -= xgd;
            *(here->EKVGspPtr +1) -= xgs;
            *(here->EKVBgPtr +1) -= xgb;
            *(here->EKVBdpPtr +1) -= xbd;
            *(here->EKVBspPtr +1) -= xbs;
            *(here->EKVDPgPtr +1) -= xgd;
            *(here->EKVDPbPtr +1) -= xbd;
            *(here->EKVSPgPtr +1) -= xgs;
            *(here->EKVSPbPtr +1) -= xbs;
            *(here->EKVDdPtr) += here->EKVdrainConductance;
            *(here->EKVSsPtr) += here->EKVsourceConductance;
            *(here->EKVBbPtr) += here->EKVgbd+here->EKVgbs;
            *(here->EKVDPdpPtr) += here->EKVdrainConductance+
                here->EKVgds+here->EKVgbd+
                xrev*(here->EKVgm+here->EKVgmbs);
            *(here->EKVSPspPtr) += here->EKVsourceConductance+
                here->EKVgds+here->EKVgbs+
                xnrm*(here->EKVgm+here->EKVgmbs);
            *(here->EKVDdpPtr) -= here->EKVdrainConductance;
            *(here->EKVSspPtr) -= here->EKVsourceConductance;
            *(here->EKVBdpPtr) -= here->EKVgbd;
            *(here->EKVBspPtr) -= here->EKVgbs;
            *(here->EKVDPdPtr) -= here->EKVdrainConductance;
            *(here->EKVDPgPtr) += (xnrm-xrev)*here->EKVgm;
            *(here->EKVDPbPtr) += -here->EKVgbd+(xnrm-xrev)*here->EKVgmbs;
            *(here->EKVDPspPtr) -= here->EKVgds+
                xnrm*(here->EKVgm+here->EKVgmbs);
            *(here->EKVSPgPtr) -= (xnrm-xrev)*here->EKVgm;
            *(here->EKVSPsPtr) -= here->EKVsourceConductance;
            *(here->EKVSPbPtr) -= here->EKVgbs+(xnrm-xrev)*here->EKVgmbs;
            *(here->EKVSPdpPtr) -= here->EKVgds+
                xrev*(here->EKVgm+here->EKVgmbs);

        }
    }
    return(OK);
}
