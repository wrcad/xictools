
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
 $Id: b4ic.cc,v 2.9 2011/12/18 01:15:28 stevew Exp $
 *========================================================================*/

/**********
 * Copyright 2001 Regents of the University of California. All rights reserved.
 * File: b4getic.c of BSIM4.2.1.
 * Author: 2000 Weidong Liu
 * Authors: Xuemei Xi, Kanyu M. Cao, Hui Wan, Mansun Chan, Chenming Hu.
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

