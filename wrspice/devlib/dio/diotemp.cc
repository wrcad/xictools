
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

/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1985 Thomas L. Quarles
Modified: 2000 AlansFixes
Modified by Dietmar Warning 2003 and Paolo Nenzi 2003
**********/

#include "diodefs.h"
#include <stdio.h>

#define DIOnextModel      next()
#define DIOnextInstance   next()
#define DIOinstances      inst()
#define CKTtemp CKTcurTask->TSKtemp
#define CKTnomTemp CKTcurTask->TSKnomTemp
#define CKTreltol CKTcurTask->TSKreltol
#define DIOmodName GENmodName
#define DIOname GENname
#define DIOcheckModel checkModel

#define MALLOC(x) new char[x]
#define FREE(x) delete x

int
DIOdev::temperature(sGENmodel *genmod, sCKT *ckt)
{
    sDIOmodel *model = static_cast<sDIOmodel*>(genmod);
    sDIOinstance *here;

    double xfc, xfcs;
    double vte;
    double cbv;
    double xbv;
    double xcbv;
    double tol;
    double vt;
    double vtnom;
    double difference;
    double factor;
    int iter;
    char *emsg;

    /*  loop through all the diode models */
    for( ; model != NULL; model = model->DIOnextModel ) {
        if(!model->DIOnomTempGiven) {
            model->DIOnomTemp = ckt->CKTnomTemp;
        }
        vtnom = CONSTKoverQ * model->DIOnomTemp;
        /* limit grading coeff to max of .9 */
        if(model->DIOgradingCoeff>.9) {
/* SRW
            (*(SPfrontEnd->IFerror))(ERR_WARNING,
*/
            DVO.textOut(OUT_WARNING,
                    "%s: grading coefficient too large, limited to 0.9",
                    model->DIOmodName);
            model->DIOgradingCoeff=.9;
        }
        /* limit activation energy to min of .1 */
        if(model->DIOactivationEnergy<.1) {
/* SRW
            (*(SPfrontEnd->IFerror))(ERR_WARNING,
*/
            DVO.textOut(OUT_WARNING,
                    "%s: activation energy too small, limited to 0.1",
                    model->DIOmodName);
            model->DIOactivationEnergy=.1;
        }
        /* limit depletion cap coeff to max of .95 */
        if(model->DIOdepletionCapCoeff>.95) {
/* SRW
            (*(SPfrontEnd->IFerror))(ERR_WARNING,
*/
            DVO.textOut(OUT_WARNING,
                    "%s: coefficient Fc too large, limited to 0.95",
                    model->DIOmodName);
            model->DIOdepletionCapCoeff=.95;
        }
        /* limit sidewall depletion cap coeff to max of .95 */
        if(model->DIOdepletionSWcapCoeff>.95) {
/* SRW
            (*(SPfrontEnd->IFerror))(ERR_WARNING,
*/
            DVO.textOut(OUT_WARNING,
                    "%s: coefficient Fcs too large, limited to 0.95",
                    model->DIOmodName);
            model->DIOdepletionSWcapCoeff=.95;
        }
        if((!model->DIOresistGiven) || (model->DIOresist==0)) {
            model->DIOconductance = 0.0;
        } else {
            model->DIOconductance = 1/model->DIOresist;
        }
        xfc=log(1-model->DIOdepletionCapCoeff);
        xfcs=log(1-model->DIOdepletionSWcapCoeff);
        
        for(here=model->DIOinstances;here;here=here->DIOnextInstance) {
            double egfet1,arg1,fact1,pbfact1,pbo,gmaold,pboSW,gmaSWold;
            double fact2,pbfact,arg,egfet,gmanew,gmaSWnew;
            
//            if (here->DIOowner != ARCHme) continue;

            /* loop through all the instances */
            
            if(!here->DIOdtempGiven) here->DIOdtemp = 0.0;
            
            if(!here->DIOtempGiven) 
                here->DIOtemp = ckt->CKTtemp + here->DIOdtemp;          
                
           /* Junction grading temperature adjust */
           difference = here->DIOtemp - model->DIOnomTemp;
           factor = 1.0 + (model->DIOgradCoeffTemp1 * difference)
                        + (model->DIOgradCoeffTemp2 * difference * difference);
           here->DIOtGradingCoeff = model->DIOgradingCoeff * factor;
           
           /* limit temperature adjusted grading coeff 
        * to max of .9 
        */
          if(here->DIOtGradingCoeff>.9) {
/* SRW
              (*(SPfrontEnd->IFerror))(ERR_WARNING,
*/
              DVO.textOut(OUT_WARNING,
                    "%s: temperature adjusted grading coefficient too large, limited to 0.9",
                    here->DIOname);
              here->DIOtGradingCoeff=.9;
              }         
                
            vt = CONSTKoverQ * here->DIOtemp;
            /* this part gets really ugly - I won't even try to
         * explain these equations */
            fact2 = here->DIOtemp/REFTEMP;
            egfet = 1.16-(7.02e-4*here->DIOtemp*here->DIOtemp)/
                    (here->DIOtemp+1108);
            arg = -egfet/(2*CONSTboltz*here->DIOtemp) +
                    1.1150877/(CONSTboltz*(REFTEMP+REFTEMP));
            pbfact = -2*vt*(1.5*log(fact2)+CHARGE*arg);
            egfet1 = 1.16 - (7.02e-4*model->DIOnomTemp*model->DIOnomTemp)/
                    (model->DIOnomTemp+1108);
            arg1 = -egfet1/(CONSTboltz*2*model->DIOnomTemp) + 
                    1.1150877/(2*CONSTboltz*REFTEMP);
            fact1 = model->DIOnomTemp/REFTEMP;
            pbfact1 = -2 * vtnom*(1.5*log(fact1)+CHARGE*arg1);

            if (model->DIOtlevc == 1) {
                here->DIOtJctCap = model->DIOjunctionCap*(1.0 +
                    model->DIOcta*(here->DIOtemp - model->DIOnomTemp));
                here->DIOtJctSWCap = model->DIOjunctionSWCap*(1.0 +
                    model->DIOctp*(here->DIOtemp - model->DIOnomTemp));
                here->DIOtJctPot = model->DIOjunctionPot -
                    model->DIOtpb*(here->DIOtemp - model->DIOnomTemp);
                here->DIOtJctSWPot = model->DIOjunctionSWPot -
                    model->DIOtphp*(here->DIOtemp - model->DIOnomTemp);
            }
            else {
                pbo = (model->DIOjunctionPot-pbfact1)/fact1;
                gmaold = (model->DIOjunctionPot -pbo)/pbo;
                here->DIOtJctCap = model->DIOjunctionCap/
                        (1+here->DIOtGradingCoeff*
                        (400e-6*(model->DIOnomTemp-REFTEMP)-gmaold) );
                here->DIOtJctPot = pbfact+fact2*pbo;
                gmanew = (here->DIOtJctPot-pbo)/pbo;
                here->DIOtJctCap *= 1+here->DIOtGradingCoeff*
                        (400e-6*(here->DIOtemp-REFTEMP)-gmanew);
                pboSW = (model->DIOjunctionSWPot-pbfact1)/fact1;
                gmaSWold = (model->DIOjunctionSWPot -pboSW)/pboSW;
                here->DIOtJctSWCap = model->DIOjunctionSWCap/
                        (1+model->DIOgradingSWCoeff*
                        (400e-6*(model->DIOnomTemp-REFTEMP)-gmaSWold) );
                here->DIOtJctSWPot = pbfact+fact2*pboSW;
                gmaSWnew = (here->DIOtJctSWPot-pboSW)/pboSW;
                here->DIOtJctSWCap *= 1+model->DIOgradingSWCoeff*
                        (400e-6*(here->DIOtemp-REFTEMP)-gmaSWnew);
            }

            here->DIOtSatCur = model->DIOsatCur *  exp( 
                    ((here->DIOtemp/model->DIOnomTemp)-1) *
                    model->DIOactivationEnergy/(model->DIOemissionCoeff*vt) +
                    model->DIOsaturationCurrentExp/model->DIOemissionCoeff*
                    log(here->DIOtemp/model->DIOnomTemp) );
            here->DIOtSatSWCur = model->DIOsatSWCur *  exp( 
                    ((here->DIOtemp/model->DIOnomTemp)-1) *
                    model->DIOactivationEnergy/(model->DIOemissionCoeff*vt) +
                    model->DIOsaturationCurrentExp/model->DIOemissionCoeff*
                    log(here->DIOtemp/model->DIOnomTemp) );

            /* the defintion of f1, just recompute after temperature adjusting
             * all the variables used in it */
            here->DIOtF1=here->DIOtJctPot*
                    (1-exp((1-here->DIOtGradingCoeff)*xfc))/
                    (1-here->DIOtGradingCoeff);
            /* same for Depletion Capacitance */
            here->DIOtDepCap=model->DIOdepletionCapCoeff*
                    here->DIOtJctPot;
            /* and Vcrit */
            vte=model->DIOemissionCoeff*vt;
            
            here->DIOtVcrit=vte*
                          log(vte/(CONSTroot2*here->DIOtSatCur*here->DIOarea));
            
            /* and now to copute the breakdown voltage, again, using
             * temperature adjusted basic parameters */
            if (model->DIObreakdownVoltageGiven){

                double bvt;
                if (model->DIOtlev == 0)
                    bvt = model->DIObreakdownVoltage - 
                        model->DIOtcv*(here->DIOtemp - model->DIOnomTemp);
                else
                    bvt = model->DIObreakdownVoltage*(1.0 -
                        model->DIOtcv*(here->DIOtemp - model->DIOnomTemp));

                cbv=model->DIObreakdownCurrent*here->DIOarea*here->DIOm;
                if (cbv < here->DIOtSatCur*here->DIOarea*here->DIOm * bvt/vt) {
                    cbv=here->DIOtSatCur*here->DIOarea*here->DIOm * bvt/vt;
                    emsg = MALLOC(100);
                    if(emsg == (char *)NULL) return(E_NOMEM);
                    (void)sprintf(emsg,
                    "%%s: breakdown current increased to %g to resolve",
                            cbv);
/* SRW
                    (*(SPfrontEnd->IFerror))(ERR_WARNING,emsg,&(here->DIOname));
*/
                    DVO.textOut(OUT_WARNING,emsg,here->DIOname);
                    FREE(emsg);
/* SRW
                    (*(SPfrontEnd->IFerror))(ERR_WARNING,
*/
                    DVO.textOut(OUT_WARNING,
                    "incompatibility with specified saturation current",(IFuid*)NULL);
                    xbv=bvt;
                } else {
                    tol=ckt->CKTreltol*cbv;
                    xbv=bvt-vt*log(1+cbv/
                            (here->DIOtSatCur*here->DIOarea*here->DIOm));
                    iter=0;
                    for(iter=0 ; iter < 25 ; iter++) {
                        xbv=bvt-vt*log(cbv/
                                (here->DIOtSatCur*here->DIOarea*here->DIOm)+1-xbv/vt);
                        xcbv=here->DIOtSatCur*here->DIOarea*here->DIOm * 
                             (exp((bvt-xbv)/vt)-1+xbv/vt);
                        if (fabs(xcbv-cbv) <= tol) goto matched;
                    }
                    emsg = MALLOC(100);
                    if(emsg == (char *)NULL) return(E_NOMEM);
                    (void)sprintf(emsg,
                    "%%s: unable to match forward and reverse diode regions: bv = %g, ibv = %g",
                            xbv,xcbv);
/* SRW
                    (*(SPfrontEnd->IFerror))(ERR_WARNING,emsg,&here->DIOname);
*/
                    DVO.textOut(OUT_WARNING,emsg,here->DIOname);
                    FREE(emsg);
                }
                matched:
                here->DIOtBrkdwnV = xbv;
            }
            
            /* transit time temperature adjust */
            difference = here->DIOtemp - model->DIOnomTemp;
            factor = 1.0 + (model->DIOtranTimeTemp1 * difference) 
                         + (model->DIOtranTimeTemp2 * difference * difference);
            here->DIOtTransitTime = model->DIOtransitTime * factor;     
        
            /* Series resistance temperature adjust */
            here->DIOtConductance = model->DIOconductance;
            if(model->DIOresistGiven && model->DIOresist!=0.0) {
                difference = here->DIOtemp - model->DIOnomTemp;
                factor = 1.0 + (model->DIOresistTemp1) * difference
                             + (model->DIOresistTemp2 * difference * difference);
                here->DIOtConductance = model->DIOconductance / factor;
            }
            
            here->DIOtF2=exp((1+here->DIOtGradingCoeff)*xfc);
            here->DIOtF3=1-model->DIOdepletionCapCoeff*
                    (1+here->DIOtGradingCoeff);
            here->DIOtF2SW=exp((1+model->DIOgradingSWCoeff)*xfcs);
            here->DIOtF3SW=1-model->DIOdepletionSWcapCoeff*
                    (1+model->DIOgradingSWCoeff);
        
        } /* instance */
                
    } /* model */
    return(OK);
}

