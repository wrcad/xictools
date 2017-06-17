
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
 $Id: diopzld.cc,v 1.1 2008/07/04 23:14:39 stevew Exp $
 *========================================================================*/

/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1985 Thomas L. Quarles
Modified by Dietmar Warning 2003
**********/

#include "diodefs.h"

#define DIOnextModel      next()
#define DIOnextInstance   next()
#define DIOinstances      inst()


int
DIOdev::pzLoad(sGENmodel *genmod, sCKT *ckt, IFcomplex *s)
{
    sDIOmodel *model = static_cast<sDIOmodel*>(genmod);
    sDIOinstance *here;

    double gspr;
    double geq;
    double xceq;

    /*  loop through all the diode models */
    for( ; model != NULL; model = model->DIOnextModel ) {

        /* loop through all the instances of the model */
        for (here = model->DIOinstances; here != NULL ;
                here=here->DIOnextInstance) {
//            if (here->DIOowner != ARCHme) continue;
            gspr=here->DIOtConductance*here->DIOarea*here->DIOm;

//            geq= *(ckt->CKTstate0 + here->DIOconduct);
            geq = here->DIOgd;
//            xceq= *(ckt->CKTstate0 + here->DIOcapCurrent);
            xceq = here->DIOcap * ckt->CKTomega;

            *(here->DIOposPosPtr ) += gspr;
            *(here->DIOnegNegPtr ) += geq + xceq * s->real;
            *(here->DIOnegNegPtr +1 ) += xceq * s->imag;
            *(here->DIOposPrimePosPrimePtr ) += geq + gspr + xceq * s->real;
            *(here->DIOposPrimePosPrimePtr +1 ) += xceq * s->imag;
            *(here->DIOposPosPrimePtr ) -= gspr;
            *(here->DIOnegPosPrimePtr ) -= geq + xceq * s->real;
            *(here->DIOnegPosPrimePtr +1 ) -= xceq * s->imag;
            *(here->DIOposPrimePosPtr ) -= gspr;
            *(here->DIOposPrimeNegPtr ) -= geq + xceq * s->real;
            *(here->DIOposPrimeNegPtr +1 ) -= xceq * s->imag;
        }
    }
    return(OK);

}

