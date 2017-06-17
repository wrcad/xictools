
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
 $Id: b2conv.cc,v 1.0 1998/01/30 05:27:36 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Hong June Park, Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/

#include "b2defs.h"


int
B2dev::convTest(sGENmodel *genmod, sCKT *ckt)
{
    double cbd;
    double cbhat;
    double cbs;
    double cd;
    double cdhat;
    double delvbd;
    double delvbs;
    double delvds;
    double delvgd;
    double delvgs;
    double tol;
    double vbd;
    double vbs;
    double vds;
    double vgd;
    double vgdo;
    double vgs;

    sB2model *model = static_cast<sB2model*>(genmod);
    for ( ; model; model = model->next()) {
        sB2instance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {

            vbs = model->B2type * ( 
                *(ckt->CKTrhsOld+inst->B2bNode) -
                *(ckt->CKTrhsOld+inst->B2sNodePrime));
            vgs = model->B2type * ( 
                *(ckt->CKTrhsOld+inst->B2gNode) -
                *(ckt->CKTrhsOld+inst->B2sNodePrime));
            vds = model->B2type * ( 
                *(ckt->CKTrhsOld+inst->B2dNodePrime) -
                *(ckt->CKTrhsOld+inst->B2sNodePrime));
            vbd=vbs-vds;
            vgd=vgs-vds;
            vgdo = *(ckt->CKTstate0 + inst->B2vgs) - 
                *(ckt->CKTstate0 + inst->B2vds);
            delvbs = vbs - *(ckt->CKTstate0 + inst->B2vbs);
            delvbd = vbd - *(ckt->CKTstate0 + inst->B2vbd);
            delvgs = vgs - *(ckt->CKTstate0 + inst->B2vgs);
            delvds = vds - *(ckt->CKTstate0 + inst->B2vds);
            delvgd = vgd-vgdo;

            if (inst->B2mode >= 0) {
                cdhat=
                    *(ckt->CKTstate0 + inst->B2cd) -
                    *(ckt->CKTstate0 + inst->B2gbd) * delvbd +
                    *(ckt->CKTstate0 + inst->B2gmbs) * delvbs +
                    *(ckt->CKTstate0 + inst->B2gm) * delvgs + 
                    *(ckt->CKTstate0 + inst->B2gds) * delvds ;
            }
            else {
                cdhat=
                    *(ckt->CKTstate0 + inst->B2cd) -
                    ( *(ckt->CKTstate0 + inst->B2gbd) -
                      *(ckt->CKTstate0 + inst->B2gmbs)) * delvbd -
                    *(ckt->CKTstate0 + inst->B2gm) * delvgd +
                    *(ckt->CKTstate0 + inst->B2gds) * delvds;
            }
            cbhat=
                *(ckt->CKTstate0 + inst->B2cbs) +
                *(ckt->CKTstate0 + inst->B2cbd) +
                *(ckt->CKTstate0 + inst->B2gbd) * delvbd +
                *(ckt->CKTstate0 + inst->B2gbs) * delvbs ;

            cd = *(ckt->CKTstate0 + inst->B2cd);
            cbs = *(ckt->CKTstate0 + inst->B2cbs);
            cbd = *(ckt->CKTstate0 + inst->B2cbd);
            //
            //  check convergence
            //
            if ( (inst->B2off == 0)  || (!(ckt->CKTmode & MODEINITFIX)) ){
                tol=ckt->CKTcurTask->TSKreltol*SPMAX(FABS(cdhat),FABS(cd))+
                    ckt->CKTcurTask->TSKabstol;
                if (FABS(cdhat-cd) >= tol) { 
                    ckt->CKTnoncon++;
                    ckt->CKTtroubleElt = inst;
                    return(OK);
                } 
                tol=ckt->CKTcurTask->TSKreltol*SPMAX(FABS(cbhat),FABS(cbs+cbd))+
                    ckt->CKTcurTask->TSKabstol;
                if (FABS(cbhat-(cbs+cbd)) > tol) {
                    ckt->CKTnoncon++;
                    ckt->CKTtroubleElt = inst;
                    return(OK);
                }
            }
        }
    }
    return(OK);
}

