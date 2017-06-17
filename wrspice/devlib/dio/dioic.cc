
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
 $Id: dioic.cc,v 1.1 2008/07/04 23:14:39 stevew Exp $
 *========================================================================*/

/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1985 Thomas L. Quarles
**********/

#include "diodefs.h"

#define DIOnextModel      next()
#define DIOnextInstance   next()
#define DIOinstances      inst()


int
DIOdev::getic(sGENmodel *genmod, sCKT *ckt)
{
    sDIOmodel *model = static_cast<sDIOmodel*>(genmod);
    sDIOinstance *here;

    /*
     * grab initial conditions out of rhs array.   User specified, so use
     * external nodes to get values
     */

    for( ; model ; model = model->DIOnextModel) {
        for(here = model->DIOinstances; here ; here = here->DIOnextInstance) {
//	    if (here->DIOowner != ARCHme) continue;

            if(!here->DIOinitCondGiven) {
                here->DIOinitCond = 
                        *(ckt->CKTrhs + here->DIOposNode) - 
                        *(ckt->CKTrhs + here->DIOnegNode);
            }
        }
    }
    return(OK);
}

