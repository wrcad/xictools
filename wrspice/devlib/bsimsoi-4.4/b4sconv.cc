
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
 $Id: b4sconv.cc,v 1.1 2011/12/17 04:54:06 stevew Exp $
 *========================================================================*/

/***  B4SOI 12/16/2010 Released by Tanvir Morshed   ***/

/**********
 * Copyright 2010 Regents of the University of California.  All rights reserved.
 * Authors: 1998 Samuel Fung, Dennis Sinitsky and Stephen Tang
 * Authors: 1999-2004 Pin Su, Hui Wan, Wei Jin, b3soicvtest.c
 * Authors: 2005- Hui Wan, Xuemei Xi, Ali Niknejad, Chenming Hu.
 * Authors: 2009- Wenwei Yang, Chung-Hsun Lin, Ali Niknejad, Chenming Hu.
 * File: b4soicvtest.c
 * Modified by Hui Wan, Xuemei Xi 11/30/2005
 * Modified by Wenwei Yang, Chung-Hsun Lin, Darsen Lu 03/06/2009
 * Modified by Tanvir Morshed 09/22/2009
 * Modified by Tanvir Morshed 12/31/2009
 **********/

#include "b4sdefs.h"


#define B4SOInextModel      next()
#define B4SOInextInstance   next()
#define B4SOIinstances      inst()
#define CKTreltol CKTcurTask->TSKreltol
#define CKTabstol CKTcurTask->TSKabstol
#define MAX SPMAX


int
B4SOIdev::convTest(sGENmodel *genmod, sCKT *ckt)
{
    sB4SOImodel *model = static_cast<sB4SOImodel*>(genmod);
    sB4SOIinstance *here;

    double delvbd, delvbs, delvds, delvgd, delvgs, vbd, vbs, vds;
    double cbd, cbhat, cbs, cd, cdhat, tol, vgd, vgdo, vgs;

    /*  loop through all the B4SOI device models */
    for (; model != NULL; model = model->B4SOInextModel)
    {
        /* loop through all the instances of the model */
        for (here = model->B4SOIinstances; here != NULL ;
                here=here->B4SOInextInstance)
        {
// SRW, check this here to avoid computations below.
            if (here->B4SOIoff && (ckt->CKTmode & MODEINITFIX))
                continue;

            vbs = model->B4SOItype
                  * (*(ckt->CKTrhsOld+here->B4SOIbNode)
                     - *(ckt->CKTrhsOld+here->B4SOIsNodePrime));
            vgs = model->B4SOItype
                  * (*(ckt->CKTrhsOld+here->B4SOIgNode)
                     - *(ckt->CKTrhsOld+here->B4SOIsNodePrime));
            vds = model->B4SOItype
                  * (*(ckt->CKTrhsOld+here->B4SOIdNodePrime)
                     - *(ckt->CKTrhsOld+here->B4SOIsNodePrime));
            vbd = vbs - vds;
            vgd = vgs - vds;
            vgdo = *(ckt->CKTstate0 + here->B4SOIvgs)
                   - *(ckt->CKTstate0 + here->B4SOIvds);
            delvbs = vbs - *(ckt->CKTstate0 + here->B4SOIvbs);
            delvbd = vbd - *(ckt->CKTstate0 + here->B4SOIvbd);
            delvgs = vgs - *(ckt->CKTstate0 + here->B4SOIvgs);
            delvds = vds - *(ckt->CKTstate0 + here->B4SOIvds);
            delvgd = vgd-vgdo;

            cd = here->B4SOIcd;
            if (here->B4SOImode >= 0)
            {
                cdhat = cd - here->B4SOIgjdb * delvbd
                        + here->B4SOIgmbs * delvbs + here->B4SOIgm * delvgs
                        + here->B4SOIgds * delvds;
            }
            else
            {
                cdhat = cd - (here->B4SOIgjdb - here->B4SOIgmbs) * delvbd
                        - here->B4SOIgm * delvgd + here->B4SOIgds * delvds;
            }

            /*
             *  check convergence
             */
// SRW              if ((here->B4SOIoff == 0)  || (!(ckt->CKTmode & MODEINITFIX)))
            {
                tol = ckt->CKTreltol * MAX(FABS(cdhat), FABS(cd))
                      + ckt->CKTabstol;
                if (FABS(cdhat - cd) >= tol)
                {
                    ckt->CKTnoncon++;
                    return(OK);
                }
                cbs = here->B4SOIcjs;
                cbd = here->B4SOIcjd;
                cbhat = cbs + cbd + here->B4SOIgjdb * delvbd
                        + here->B4SOIgjsb * delvbs;
                tol = ckt->CKTreltol * MAX(FABS(cbhat), FABS(cbs + cbd))
                      + ckt->CKTabstol;
                if (FABS(cbhat - (cbs + cbd)) > tol)
                {
                    ckt->CKTnoncon++;
                    return(OK);
                }
            }
        }
    }
    return(OK);
}

