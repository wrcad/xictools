
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
 $Id: hsm1ic.cc,v 2.8 2002/10/17 19:58:12 stevew Exp $
 *========================================================================*/

/***********************************************************************
 HiSIM v1.1.0
 File: hsm1getic.c of HiSIM v1.1.0

 Copyright (C) 2002 STARC

 June 30, 2002: developed by Hiroshima University and STARC
 June 30, 2002: posted by Keiichi MORIKAWA, STARC Physical Design Group
***********************************************************************/

#include "hsm1defs.h"

#define HSM1nextModel      next()
#define HSM1nextInstance   next()
#define HSM1instances      inst()


int
HSM1dev::getic(sGENmodel *genmod, sCKT *ckt)
{
    sHSM1model *model = static_cast<sHSM1model*>(genmod);
    sHSM1instance *here;

  /*
   * grab initial conditions out of rhs array.   User specified, so use
   * external nodes to get values
   */

  for ( ;model ;model = model->HSM1nextModel ) {
    for ( here = model->HSM1instances; here ;here = here->HSM1nextInstance ) {
      if (!here->HSM1_icVBS_Given) {
        here->HSM1_icVBS = 
          *(ckt->CKTrhs + here->HSM1bNode) - 
          *(ckt->CKTrhs + here->HSM1sNode);
      }
      if (!here->HSM1_icVDS_Given) {
        here->HSM1_icVDS = 
          *(ckt->CKTrhs + here->HSM1dNode) - 
          *(ckt->CKTrhs + here->HSM1sNode);
      }
      if (!here->HSM1_icVGS_Given) {
        here->HSM1_icVGS = 
          *(ckt->CKTrhs + here->HSM1gNode) - 
          *(ckt->CKTrhs + here->HSM1sNode);
      }
    }
  }
  return(OK);
}

