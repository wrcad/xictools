
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
 $Id: b3ic.cc,v 2.1 1999/01/07 01:40:41 stevew Exp $
 *========================================================================*/

/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1995 Min-Chie Jeng and Mansun Chan.
* Revision 3.2 1998/6/16  18:00:00  Weidong 
* BSIM3v3.2 release
**********/

#include "b3defs.h"


int
B3dev::getic(sGENmodel *genmod, sCKT *ckt)
{
    sB3model *model = static_cast<sB3model*>(genmod);
    for ( ; model; model = model->next()) {

        sB3instance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {

            if (!inst->B3icVBSGiven) {
                inst->B3icVBS = *(ckt->CKTrhs + inst->B3bNode)
                    - *(ckt->CKTrhs + inst->B3sNode);
            }
            if (!inst->B3icVDSGiven) {
                inst->B3icVDS = *(ckt->CKTrhs + inst->B3dNode)
                    - *(ckt->CKTrhs + inst->B3sNode);
            }
            if (!inst->B3icVGSGiven) {
                inst->B3icVGS = *(ckt->CKTrhs + inst->B3gNode)
                    - *(ckt->CKTrhs + inst->B3sNode);
            }
        }
    }
    return (OK);
}


