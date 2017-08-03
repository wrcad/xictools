
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA, http://wrcad.com            *
 *  Copyright (C) 2017 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
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

/**** BSIM3v3.3.0, Released by Xuemei Xi 07/29/2005 ****/

/**********
 * Copyright 2004 Regents of the University of California. All rights reserved.
 * File: b3getic.c of BSIM3v3.3.0
 * Author: 1995 Min-Chie Jeng and Mansun Chan.
 * Author: 1997-1999 Weidong Liu.
 * Author: 2001  Xuemei Xi
 **********/

#include "b3defs.h"

#define BSIM3nextModel      next()
#define BSIM3nextInstance   next()
#define BSIM3instances      inst()


int
BSIM3dev::getic(sGENmodel *genmod, sCKT *ckt)
{
    sBSIM3model *model = static_cast<sBSIM3model*>(genmod);
    sBSIM3instance *here;

    for (; model ; model = model->BSIM3nextModel)
    {
        for (here = model->BSIM3instances; here; here = here->BSIM3nextInstance)
        {
            if(!here->BSIM3icVBSGiven)
            {
                here->BSIM3icVBS = *(ckt->CKTrhs + here->BSIM3bNode)
                                   - *(ckt->CKTrhs + here->BSIM3sNode);
            }
            if (!here->BSIM3icVDSGiven)
            {
                here->BSIM3icVDS = *(ckt->CKTrhs + here->BSIM3dNode)
                                   - *(ckt->CKTrhs + here->BSIM3sNode);
            }
            if (!here->BSIM3icVGSGiven)
            {
                here->BSIM3icVGS = *(ckt->CKTrhs + here->BSIM3gNode)
                                   - *(ckt->CKTrhs + here->BSIM3sNode);
            }
        }
    }
    return(OK);
}

