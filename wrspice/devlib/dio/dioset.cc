
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
 $Id: dioset.cc,v 1.5 2015/07/24 02:51:29 stevew Exp $
 *========================================================================*/

/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1985 Thomas L. Quarles
Modified: 2000 AlansFixes
Modified by Dietmar Warning 2003 and Paolo Nenzi 2003
**********/

#include "diodefs.h"

#define DIOnextModel      next()
#define DIOnextInstance   next()
#define DIOinstances      inst()
#define CKTnode sCKTnode
#define JOB sJOB
#define TSKtask sTASK
#define CKTnomTemp CKTcurTask->TSKnomTemp
#define DIOstate GENstate
#define DIOname GENname
#define CKTmkVolt(a, b, c, d) (a)->mkVolt(b, c, d)
#define CKTdltNNum(c, n)


namespace {
    int get_node_ptr(sCKT *ckt, sDIOinstance *inst)
    {
        TSTALLOC(DIOposPosPrimePtr,DIOposNode,DIOposPrimeNode)
        TSTALLOC(DIOnegPosPrimePtr,DIOnegNode,DIOposPrimeNode)
        TSTALLOC(DIOposPrimePosPtr,DIOposPrimeNode,DIOposNode)
        TSTALLOC(DIOposPrimeNegPtr,DIOposPrimeNode,DIOnegNode)
        TSTALLOC(DIOposPosPtr,DIOposNode,DIOposNode)
        TSTALLOC(DIOnegNegPtr,DIOnegNode,DIOnegNode)
        TSTALLOC(DIOposPrimePosPrimePtr,DIOposPrimeNode,DIOposPrimeNode)
        return (OK);
    }
}


int
DIOdev::setup(sGENmodel *genmod, sCKT *ckt, int *states)
{
    sDIOmodel *model = static_cast<sDIOmodel*>(genmod);
    sDIOinstance *here;

    int error;
    CKTnode *tmp;

    /*  loop through all the diode models */
    for( ; model != NULL; model = model->DIOnextModel ) {

        if(!model->DIOemissionCoeffGiven) {
            model->DIOemissionCoeff = 1;
        }
        if(!model->DIOsatCurGiven) {
            model->DIOsatCur = 1e-14;
        }
        if(!model->DIOsatSWCurGiven) {
            model->DIOsatSWCur = 0.0;
        }

        if(!model->DIObreakdownCurrentGiven) {
            model->DIObreakdownCurrent = 1e-3;
        }
        if(!model->DIOjunctionPotGiven){
            model->DIOjunctionPot = 1;
        }
        if(!model->DIOgradingCoeffGiven) {
            model->DIOgradingCoeff = .5;
        } 
        if(!model->DIOgradCoeffTemp1Given) {
            model->DIOgradCoeffTemp1 = 0.0;
        }
        if(!model->DIOgradCoeffTemp2Given) {
            model->DIOgradCoeffTemp2 = 0.0;
        }       
        if(!model->DIOdepletionCapCoeffGiven) {
            model->DIOdepletionCapCoeff = .5;
        } 
        if(!model->DIOdepletionSWcapCoeffGiven) {
            model->DIOdepletionSWcapCoeff = .5;
        }
        if(!model->DIOtransitTimeGiven) {
            model->DIOtransitTime = 0;
        }
        if(!model->DIOtranTimeTemp1Given) {
            model->DIOtranTimeTemp1 = 0.0;
        }
        if(!model->DIOtranTimeTemp2Given) {
            model->DIOtranTimeTemp2 = 0.0;
        }
        if(!model->DIOjunctionCapGiven) {
            model->DIOjunctionCap = 0;
        }
        if(!model->DIOjunctionSWCapGiven) {
            model->DIOjunctionSWCap = 0;
        }
        if(!model->DIOjunctionSWPotGiven){
            model->DIOjunctionSWPot = 1;
        }
        if(!model->DIOgradingSWCoeffGiven) {
            model->DIOgradingSWCoeff = .33;
        } 
        if(!model->DIOforwardKneeCurrentGiven) {
            model->DIOforwardKneeCurrent = 1e-3;
        } 
        if(!model->DIOreverseKneeCurrentGiven) {
            model->DIOreverseKneeCurrent = 1e-3;
        } 
        if(!model->DIOactivationEnergyGiven) {
            model->DIOactivationEnergy = 1.11;
        } 
        if(!model->DIOsaturationCurrentExpGiven) {
            model->DIOsaturationCurrentExp = 3;
        }
        if(!model->DIOfNcoefGiven) {
            model->DIOfNcoef = 0.0;
        }
        if(!model->DIOfNexpGiven) {
            model->DIOfNexp = 1.0;
        }
        if(!model->DIOresistTemp1Given) {
            model->DIOresistTemp1 = 0.0;
        }
        if(!model->DIOresistTemp2Given) {
            model->DIOresistTemp2 = 0.0;
        }
        if(!model->DIOpjGiven) {
            model->DIOpj = 0.0;
        }
        if(!model->DIOareaGiven) {
            model->DIOarea = 0.0;
        }
        if(!model->DIOctaGiven) {
            model->DIOcta = 0.0;
        }
        if(!model->DIOctpGiven) {
            model->DIOctp = 0.0;
        }
        if(!model->DIOtcvGiven) {
            model->DIOtcv = 0.0;
        }
        if(!model->DIOtpb) {
            model->DIOtpb = 0.0;
        }
        if(!model->DIOtphp) {
            model->DIOtphp = 0.0;
        }
        if(!model->DIOtlev) {
            model->DIOtlev = 0;
        }
        if(!model->DIOtlevc) {
            model->DIOtlevc = 0;
        }

        /* loop through all the instances of the model */
        for (here = model->DIOinstances; here != NULL ;
                here=here->DIOnextInstance) {
//            if (here->DIOowner != ARCHme) goto matrixpointers;

            if(!here->DIOareaGiven) {
                if (model->DIOareaGiven)
                    here->DIOarea = model->DIOarea;
                else
                    here->DIOarea = 1;
            }
            if(!here->DIOpjGiven) {
                if (model->DIOpjGiven)
                    here->DIOpj = model->DIOpj;
                else
                    here->DIOpj = 0;
            }
            if(!here->DIOmGiven) {
                here->DIOm = 1;
            }

            here->DIOstate = *states;
//            *states += 5;
            *states += DIOnumStates;
/* SRW
            if(ckt->CKTsenInfo && (ckt->CKTsenInfo->SENmode & TRANSEN) ){
                *states += 2 * (ckt->CKTsenInfo->SENparms);
            }
            
matrixpointers:
*/
            if(model->DIOresist == 0) {
                here->DIOposPrimeNode = here->DIOposNode;
            } else if(here->DIOposPrimeNode == 0) {
                
/* SRW
                CKTnode *tmpNode;
               IFuid tmpName;
*/
                
                error = CKTmkVolt(ckt,&tmp,here->DIOname,"internal");
                if(error) return(error);
                here->DIOposPrimeNode = tmp->number();
/* SRW
                if (ckt->CKTcopyNodesets) {
                  if (CKTinst2Node(ckt,here,1,&tmpNode,&tmpName)==OK) {
                     if (tmpNode->nsGiven) {
                       tmp->nodeset=tmpNode->nodeset; 
                       tmp->nsGiven=tmpNode->nsGiven; 
                     }
                  }
                }
*/
            }

/* macro to make elements with built in test for out of memory */
/* SRW
#define TSTALLOC(ptr,first,second) \
if((here->ptr = SMPmakeElt(matrix,here->first,here->second))==(double *)NULL){\
    return(E_NOMEM);\
}
*/
            error = get_node_ptr(ckt, here);
            if (error != OK)
                return (error);
        }
    }
    return(OK);
}

int
DIOdev::unsetup(sGENmodel *genmod, sCKT*)
{
    sDIOmodel *model = static_cast<sDIOmodel*>(genmod);
    sDIOinstance *here;

    for ( ; model != NULL;
            model = model->DIOnextModel)
    {
        for (here = model->DIOinstances; here != NULL;
                here=here->DIOnextInstance)
        {

            if (here->DIOposPrimeNode
                    && here->DIOposPrimeNode != here->DIOposNode)
            {
                CKTdltNNum(ckt, here->DIOposPrimeNode);
                here->DIOposPrimeNode = 0;
            }
        }
    }
    return OK;
}


// SRW - reset the matrix element pointers.
//
int
DIOdev::resetup(sGENmodel *inModel, sCKT *ckt)
{
    for (sDIOmodel *model = (sDIOmodel*)inModel; model;
            model = model->next()) {
        for (sDIOinstance *here = model->inst(); here;
                here = here->next()) {
            int error = get_node_ptr(ckt, here);
            if (error != OK)
                return (error);
        }
    }
    return (OK);
}

