
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
 $Id: ufsic.cc,v 2.8 2002/09/30 13:01:48 stevew Exp $
 *========================================================================*/

/**********
Copyright 1997 University of Florida.  All rights reserved.
Author: Min-Chie Jeng (For SPICE3E2)
File: ufsgetic.c
**********/

#include "ufsdefs.h"

// This function sets initial conditions for the device, if not specified
// in the input file.  Below, it obtains the voltave across the device
// resulting from the initial operating point analysis.  This function
// is called only for devices that have charge/flux storage.


int
UFSdev::getic(sGENmodel *genmod, sCKT *ckt)
{
sUFSmodel *model = (sUFSmodel*)genmod;
sUFSinstance *here;

    for (; model ; model = model->UFSnextModel) 
    {    for (here = model->UFSinstances; here; here = here->UFSnextInstance)
	 {    if(!here->UFSicVBSGiven) 
	      {  here->UFSicVBS = *(ckt->CKTrhs + here->UFSbNode) 
				- *(ckt->CKTrhs + here->UFSsNode);
              }
              if (!here->UFSicVDSGiven) 
	      {   here->UFSicVDS = *(ckt->CKTrhs + here->UFSdNode) 
				 - *(ckt->CKTrhs + here->UFSsNode);
              }
              if (!here->UFSicVGFSGiven) 
	      {   here->UFSicVGFS = *(ckt->CKTrhs + here->UFSgNode) 
				  - *(ckt->CKTrhs + here->UFSsNode);
              }
              if (!here->UFSicVGBSGiven) 
	      {   here->UFSicVGBS = *(ckt->CKTrhs + here->UFSbgNode) 
				  - *(ckt->CKTrhs + here->UFSsNode);
              }
         }
    }
    return(OK);
}

