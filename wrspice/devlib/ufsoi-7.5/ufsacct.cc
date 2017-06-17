
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
 $Id: ufsacct.cc,v 2.10 2016/10/16 01:33:19 stevew Exp $
 *========================================================================*/

/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1997 Min-Chie Jeng
File: ufsaccpt.c
**********/

#include "ufsdefs.h"
#include <stdio.h>

// Perform any actions needed when iteration completes


int
UFSdev::accept(sCKT *ckt, sGENmodel *genmod)
{
sUFSmodel *model = (sUFSmodel*)genmod;
double Percent;

    for (; model != NULL; model = model->UFSnextModel)
    {    if (model->UFSdebug == 3)
	 {   Percent = ckt->CKTtime / ckt->CKTfinalTime * 100.0;
	     fprintf(stderr, "Time = %g (%4.2g%%) \n", ckt->CKTtime, Percent);
	     return (OK);
	 }
    }
    return(OK);
}
