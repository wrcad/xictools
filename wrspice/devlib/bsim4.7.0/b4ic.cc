
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

/**** BSIM4.7.0 Released by Darsen Lu 04/08/2011 ****/

/**********
 * Copyright 2006 Regents of the University of California. All rights reserved.
 * File: b4getic.c of BSIM4.7.0.
 * Author: 2000 Weidong Liu
 * Authors: 2001- Xuemei Xi, Mohan Dunga, Ali Niknejad, Chenming Hu.
 * Authors: 2006- Mohan Dunga, Ali Niknejad, Chenming Hu
 * Authors: 2007- Mohan Dunga, Wenwei Yang, Ali Niknejad, Chenming Hu
 * Project Director: Prof. Chenming Hu.
 **********/

#include "b4defs.h"


#define BSIM4nextModel      next()
#define BSIM4nextInstance   next()
#define BSIM4instances      inst()


int
BSIM4dev::getic(sGENmodel *genmod, sCKT *ckt)
{
    sBSIM4model *model = static_cast<sBSIM4model*>(genmod);
    sBSIM4instance *here;

    for (; model ; model = model->BSIM4nextModel)
    {
        for (here = model->BSIM4instances; here; here = here->BSIM4nextInstance)
        {
            if (!here->BSIM4icVDSGiven)
            {
                here->BSIM4icVDS = *(ckt->CKTrhs + here->BSIM4dNode)
                                   - *(ckt->CKTrhs + here->BSIM4sNode);
            }
            if (!here->BSIM4icVGSGiven)
            {
                here->BSIM4icVGS = *(ckt->CKTrhs + here->BSIM4gNodeExt)
                                   - *(ckt->CKTrhs + here->BSIM4sNode);
            }
            if(!here->BSIM4icVBSGiven)
            {
                here->BSIM4icVBS = *(ckt->CKTrhs + here->BSIM4bNode)
                                   - *(ckt->CKTrhs + here->BSIM4sNode);
            }
        }
    }
    return(OK);
}

