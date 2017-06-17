
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
 $Id: b3sic.cc,v 2.9 2011/12/18 01:15:11 stevew Exp $
 *========================================================================*/

/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1998 Samuel Fung, Dennis Sinitsky and Stephen Tang
File: b3soigetic.c          98/5/01
Revision 3.0 2/5/20  Pin Su
BSIMSOI3.0 release
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

