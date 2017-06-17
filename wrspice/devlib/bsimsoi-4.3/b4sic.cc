
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
 $Id: b4sic.cc,v 1.2 2011/12/17 06:19:04 stevew Exp $
 *========================================================================*/

/***  B4SOI 12/31/2009 Released by Tanvir Morshed   ***/

/**********
 * Copyright 2009 Regents of the University of California.  All rights reserved.
 * Authors: 1998 Samuel Fung, Dennis Sinitsky and Stephen Tang
 * Authors: 1999-2004 Pin Su, Hui Wan, Wei Jin, b3soigetic.c
 * Authors: 2005- Hui Wan, Xuemei Xi, Ali Niknejad, Chenming Hu.
 * Authors: 2009- Wenwei Yang, Chung-Hsun Lin, Ali Niknejad, Chenming Hu.
 * File: b4soigetic.c
 * Modified by Hui Wan, Xuemei Xi 11/30/2005
 * Modified by Wenwei Yang, Chung-Hsun Lin, Darsen Lu 03/06/2009
 * Modified by Tanvir Morshed 09/22/2009
 * Modified by Tanvir Morshed 12/31/2009
 **********/

#include "b4sdefs.h"


#define B4SOInextModel      next()
#define B4SOInextInstance   next()
#define B4SOIinstances      inst()


int
B4SOIdev::getic(sGENmodel *genmod, sCKT *ckt)
{
    sB4SOImodel *model = static_cast<sB4SOImodel*>(genmod);
    sB4SOIinstance *here;

    for (; model ; model = model->B4SOInextModel)
    {
        for (here = model->B4SOIinstances; here; here = here->B4SOInextInstance)
        {
            if(!here->B4SOIicVBSGiven)
            {
                here->B4SOIicVBS = *(ckt->CKTrhs + here->B4SOIbNode)
                                   - *(ckt->CKTrhs + here->B4SOIsNode);
            }
            if (!here->B4SOIicVDSGiven)
            {
                here->B4SOIicVDS = *(ckt->CKTrhs + here->B4SOIdNode)
                                   - *(ckt->CKTrhs + here->B4SOIsNode);
            }
            if (!here->B4SOIicVGSGiven)
            {
                here->B4SOIicVGS = *(ckt->CKTrhs + here->B4SOIgNode)
                                   - *(ckt->CKTrhs + here->B4SOIsNode);
            }
            if (!here->B4SOIicVESGiven)
            {
                here->B4SOIicVES = *(ckt->CKTrhs + here->B4SOIeNode)
                                   - *(ckt->CKTrhs + here->B4SOIsNode);
            }
            if (!here->B4SOIicVPSGiven)
            {
                here->B4SOIicVPS = *(ckt->CKTrhs + here->B4SOIpNode)
                                   - *(ckt->CKTrhs + here->B4SOIsNode);
            }
        }
    }
    return(OK);
}

