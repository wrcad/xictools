
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
 $Id: ekvic.cc,v 2.9 2002/11/10 19:47:12 stevew Exp $
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
EKVdev::getic(sGENmodel *genmod, sCKT *ckt)
{
    sEKVmodel *model = static_cast<sEKVmodel*>(genmod);
    sEKVinstance *here;

    /*
     * grab initial conditions out of rhs array.   User specified, so use
     * external nodes to get values
     */

    for( ; model ; model = model->EKVnextModel) {
        for(here = model->EKVinstances; here ; here = here->EKVnextInstance) {
            if(!here->EKVicVBSGiven) {
                here->EKVicVBS = 
                    *(ckt->CKTrhs + here->EKVbNode) - 
                    *(ckt->CKTrhs + here->EKVsNode);
            }
            if(!here->EKVicVDSGiven) {
                here->EKVicVDS = 
                    *(ckt->CKTrhs + here->EKVdNode) - 
                    *(ckt->CKTrhs + here->EKVsNode);
            }
            if(!here->EKVicVGSGiven) {
                here->EKVicVGS = 
                    *(ckt->CKTrhs + here->EKVgNode) - 
                    *(ckt->CKTrhs + here->EKVsNode);
            }
        }
    }
    return(OK);
}

