
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

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1988 Jaijeet S Roychowdhury
         1993 Stephen R. Whiteley
****************************************************************************/
/********** new in 3f2
Sydney University mods Copyright(c) 1989 Anthony E. Parker, David J. Skellern
    Laboratory for Communication Science Engineering
    Sydney University Department of Electrical Engineering, Australia
**********/

#define DISTO
#include "jfetdefs.h"
#include "distdefs.h"

int
JFETdev::dSetup(sJFETmodel *model, sCKT *ckt)
{
    sJFETinstance *inst;
    sGENinstance *geninst;
    double gmin = ckt->CKTcurTask->TSKgmin;
    double beta;
    double betap;
    double lcapgd1;
    double lcapgd2;
    double lcapgd3;
    double lcapgs2;
    double lcapgs3;
    double lcapgs1;
//    double cd;
//    double cdrain;
    double temp;
    double cg;
    double cgd;
    double csat;
    double czgd;
    double czgdf2;
    double czgs;
    double czgsf2;
    double evgd;
    double evgs;
//    double fcpb2;
//    double gdpr;
    double gds1;
    double gds2;
    double gds3;
    double lggd1;
    double lggd2;
    double lggd3;
    double lggs1;
    double lggs2;
    double lggs3;
    double gm1;
    double gm2;
    double gm3;
    double gmds;
    double gm2ds;
    double gmds2;
//    double gspr;
    double sarg;
    double twob;
    double twop;
    double vds;
    double vgd;
    double vgs;
    double vgst;

    for ( ; model != 0; model = (sJFETmodel*)model->GENnextModel) {
        for (geninst = model->GENinstances; geninst != 0; 
                geninst = geninst->GENnextInstance) {
            inst = (sJFETinstance*)geninst;

            //
            //  dc model parameters 
            //
            beta = model->JFETbeta * inst->JFETarea;
//            gdpr = model->JFETdrainConduct*inst->JFETarea;
//            gspr = model->JFETsourceConduct*inst->JFETarea;
            csat = inst->JFETtSatCur*inst->JFETarea;
            //
            //    initialization
            //
            vgs= model->JFETtype*(*(ckt->CKTrhsOld + inst->JFETgateNode)
                - *(ckt->CKTrhsOld + inst->JFETsourcePrimeNode));
            vgd= model->JFETtype*(*(ckt->CKTrhsOld + inst->JFETgateNode)
                - *(ckt->CKTrhsOld + inst->JFETdrainPrimeNode));
            //
            //   determine dc current and derivatives 
            //
            vds=vgs-vgd;

            if (vds < 0.0) {
                vds = -vds;
                temp = vgs;
                vgs = vgd;
                vgd = temp;   // so now these have become the local variables
                inst->JFETmode = -1;
            }
            else
                inst->JFETmode = 1;

            if (vgs <= -5*inst->JFETtemp*CONSTKoverQ) {
                lggs1 = -csat/vgs+gmin;
                lggs2 = lggs3 = 0;
                cg = lggs1*vgs;
            }
            else {
                evgs = exp(vgs/(inst->JFETtemp*CONSTKoverQ));
                lggs1 = csat*evgs/(inst->JFETtemp*CONSTKoverQ)+gmin;
                lggs2 = (lggs1-gmin)/((inst->JFETtemp*CONSTKoverQ)*2);
                lggs3 = lggs2/(3*(inst->JFETtemp*CONSTKoverQ));
                cg = csat*(evgs-1)+gmin*vgs;
            }
            if (vgd <= -5*(inst->JFETtemp*CONSTKoverQ)) {
                lggd1 = -csat/vgd+gmin;
                lggd2 = lggd3 = 0;
                cgd = lggd1*vgd;
            }
            else {
                evgd = exp(vgd/(inst->JFETtemp*CONSTKoverQ));
                lggd1 = csat*evgd/(inst->JFETtemp*CONSTKoverQ)+gmin;
                lggd2 = (lggd1-gmin)/((inst->JFETtemp*CONSTKoverQ)*2);
                lggd3 = lggd2/(3*(inst->JFETtemp*CONSTKoverQ));
                cgd = csat*(evgd-1)+gmin*vgd;
            }
            cg = cg+cgd;
            //
            //   compute drain current and derivatives
            //
            vgst = vgs-model->JFETthreshold;
            //
            //   cutoff region 
            //
            if (vgst <= 0) {
//                cdrain = 0;
                gm1 = gm2 = gm3 = 0;
                gds1 = gds2 = gds3 = 0;
                gmds = gm2ds = gmds2 = 0;
            }
            else {
                betap = beta*(1+model->JFETlModulation*vds);
                twob = betap+betap;
                if (vgst <= vds) {
                     //
                     //   normal mode, saturation region 
                     //

                     // note - for cdrain, all the g's refer to the
                     // derivatives which have not been divided to
                     // become Taylor coeffs. A notational 
                     // inconsistency but simplifies processing later.
                     //
//                    cdrain = betap*vgst*vgst;
                    gm1 = twob*vgst;
                    gm2 = twob;
                    gm3 = 0;
                    gds1 = model->JFETlModulation*beta*vgst*vgst;
                    gds2 = gds3 = gmds2 = 0;
                    gm2ds = 2*model->JFETlModulation*beta;
                    gmds = gm2ds*vgst;
                }
                else {
                    //
                    //   normal mode, linear region 
                    //
//                    cdrain = betap*vds*(vgst+vgst-vds);
                    gm1 = twob*vds;
                    gm2 = 0;
                    gm3 = 0;
                    gmds = (beta+beta)*(1+2*model->JFETlModulation*vds);
                    gm2ds = 0;
                    gds2 = 2*beta*(2*model->JFETlModulation*vgst - 1 -
                        3*model->JFETlModulation*vds);
                    gds1 = beta*(2*(vgst-vds) + 4*vgst*vds*
                        model->JFETlModulation -
                        3*model->JFETlModulation*vds*vds);
                    gmds2 = 4*beta*model->JFETlModulation;
                    gds3 = -6*beta*model->JFETlModulation;
                }
            }
            //
            //   compute equivalent drain current source 
            //
//            cd = cdrain-cgd;
            // 
            //    charge storage elements 
            //
            czgs = inst->JFETtCGS*inst->JFETarea;
            czgd = inst->JFETtCGD*inst->JFETarea;
            twop = inst->JFETtGatePot+inst->JFETtGatePot;
//            fcpb2 = inst->JFETcorDepCap*inst->JFETcorDepCap;
            czgsf2 = czgs/model->JFETf2;
            czgdf2 = czgd/model->JFETf2;
            if (vgs < inst->JFETcorDepCap) {
                sarg = sqrt(1-vgs/inst->JFETtGatePot);
                lcapgs1 = czgs/sarg;
                lcapgs2 = lcapgs1/(inst->JFETtGatePot*4*sarg*sarg);
                lcapgs3 = lcapgs2/(inst->JFETtGatePot*2*sarg*sarg);
            }
            else {
                lcapgs1 = czgsf2*(model->JFETf3+vgs/twop);
                lcapgs2 = czgsf2/twop*0.5;
                lcapgs3 = 0;
            }
            if (vgd < inst->JFETcorDepCap) {
                sarg = sqrt(1-vgd/inst->JFETtGatePot);
                lcapgd1 = czgd/sarg;
                lcapgd2 = lcapgd1/(inst->JFETtGatePot*4*sarg*sarg);
                lcapgd3 = lcapgd2/(inst->JFETtGatePot*2*sarg*sarg);
            }
            else {
                lcapgd1 = czgdf2*(model->JFETf3+vgd/twop);
                lcapgd2 = czgdf2/twop*0.5;
                lcapgd3 = 0;
            }
            //
            //   process to get Taylor coefficients, taking into
            // account type and mode.
            //

            if (inst->JFETmode == 1) {
                // normal mode - no source-drain interchange
                inst->cdr_x = gm1;
                inst->cdr_y = gds1;
                inst->cdr_x2 = gm2;
                inst->cdr_y2 = gds2;
                inst->cdr_xy = gmds;
                inst->cdr_x3 = gm3;
                inst->cdr_y3 = gds3;
                inst->cdr_x2y = gm2ds;
                inst->cdr_xy2 = gmds2;

                inst->ggs1 = lggs1;
                inst->ggd1 = lggd1;
                inst->ggs2 = lggs2;
                inst->ggd2 = lggd2;
                inst->ggs3 = lggs3;
                inst->ggd3 = lggd3;
                inst->capgs1 = lcapgs1;
                inst->capgd1 = lcapgd1;
                inst->capgs2 = lcapgs2;
                inst->capgd2 = lcapgd2;
                inst->capgs3 = lcapgs3;
                inst->capgd3 = lcapgd3;
            }
            else {
                //
                // inverse mode - source and drain interchanged
                //
                inst->cdr_x = -gm1;
                inst->cdr_y = gm1 + gds1;
                inst->cdr_x2 = -gm2;
                inst->cdr_y2 = -(gm2 + gds2 + 2*gmds);
                inst->cdr_xy = gm2 + gmds;
                inst->cdr_x3 = -gm3;
                inst->cdr_y3 = gm3 + gds3 + 3*(gm2ds + gmds2 ) ;
                inst->cdr_x2y = gm3 + gm2ds;
                inst->cdr_xy2 = -(gm3 + 2*gm2ds + gmds2);

                inst->ggs1 = lggd1;
                inst->ggd1 = lggs1;

                inst->ggs2 = lggd2;
                inst->ggd2 = lggs2;

                inst->ggs3 = lggd3;
                inst->ggd3 = lggs3;

                inst->capgs1 = lcapgd1;
                inst->capgd1 = lcapgs1;

                inst->capgs2 = lcapgd2;
                inst->capgd2 = lcapgs2;

                inst->capgs3 = lcapgd3;
                inst->capgd3 = lcapgs3;
            }

            // now to adjust for type and multiply by factors to convert to
            // Taylor coeffs

            inst->cdr_x2 = 0.5*model->JFETtype*inst->cdr_x2;
            inst->cdr_y2 = 0.5*model->JFETtype*inst->cdr_y2;
            inst->cdr_xy = model->JFETtype*inst->cdr_xy;
            inst->cdr_x3 = inst->cdr_x3/6.;
            inst->cdr_y3 = inst->cdr_y3/6.;
            inst->cdr_x2y = 0.5*inst->cdr_x2y;
            inst->cdr_xy2 = 0.5*inst->cdr_xy2;


            inst->ggs2 = model->JFETtype*lggs2;
            inst->ggd2 = model->JFETtype*lggd2;

            inst->capgs2 = model->JFETtype*lcapgs2;
            inst->capgd2 = model->JFETtype*lcapgd2;

        }
    }
    return(OK);
}
