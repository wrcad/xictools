
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
 $Id: jfet2ic.cc,v 2.10 2002/12/03 00:58:31 stevew Exp $
 *========================================================================*/

/**********
Based on jfetic.c
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1985 Thomas L. Quarles

Modified to jfet2 for PS model definition ( Anthony E. Parker )
   Copyright 1994  Macquarie University, Sydney Australia.
**********/

#include "jfet2defs.h"

#define JFET2nextModel      next()
#define JFET2nextInstance   next()
#define JFET2instances      inst()


int
JFET2dev::getic(sGENmodel *genmod, sCKT *ckt)
{
    sJFET2model *model = static_cast<sJFET2model*>(genmod);
    sJFET2instance *here;
    /*
     * grab initial conditions out of rhs array.   User specified, so use
     * external nodes to get values
     */

    for( ; model ; model = model->JFET2nextModel) {
        for(here = model->JFET2instances; here ; here = here->JFET2nextInstance) {
            if(!here->JFET2icVDSGiven) {
                here->JFET2icVDS = 
                        *(ckt->CKTrhs + here->JFET2drainNode) - 
                        *(ckt->CKTrhs + here->JFET2sourceNode);
            }
            if(!here->JFET2icVGSGiven) {
                here->JFET2icVGS = 
                        *(ckt->CKTrhs + here->JFET2gateNode) - 
                        *(ckt->CKTrhs + here->JFET2sourceNode);
            }
        }
    }
    return(OK);
}
