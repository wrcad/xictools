
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
Author: 1998 Samuel Fung, Dennis Sinitsky and Stephen Tang
File: b3soigetic.c          98/5/01
**********/

#include "b3sdefs.h"

#define B3SOInextModel      next()
#define B3SOInextInstance   next()
#define B3SOIinstances      inst()


int
B3SOIdev::getic(sGENmodel *genmod, sCKT *ckt)
{
    sB3SOImodel *model = static_cast<sB3SOImodel*>(genmod);
    sB3SOIinstance *here;

    for (; model ; model = model->B3SOInextModel)
    {
        for (here = model->B3SOIinstances; here; here = here->B3SOInextInstance)
        {
            if(!here->B3SOIicVBSGiven)
            {
                here->B3SOIicVBS = *(ckt->CKTrhs + here->B3SOIbNode)
                                   - *(ckt->CKTrhs + here->B3SOIsNode);
            }
            if (!here->B3SOIicVDSGiven)
            {
                here->B3SOIicVDS = *(ckt->CKTrhs + here->B3SOIdNode)
                                   - *(ckt->CKTrhs + here->B3SOIsNode);
            }
            if (!here->B3SOIicVGSGiven)
            {
                here->B3SOIicVGS = *(ckt->CKTrhs + here->B3SOIgNode)
                                   - *(ckt->CKTrhs + here->B3SOIsNode);
            }
            if (!here->B3SOIicVESGiven)
            {
                here->B3SOIicVES = *(ckt->CKTrhs + here->B3SOIeNode)
                                   - *(ckt->CKTrhs + here->B3SOIsNode);
            }
            if (!here->B3SOIicVPSGiven)
            {
                here->B3SOIicVPS = *(ckt->CKTrhs + here->B3SOIpNode)
                                   - *(ckt->CKTrhs + here->B3SOIsNode);
            }
        }
    }
    return(OK);
}


