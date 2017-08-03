
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

/**********
Copyright 1997 University of Florida.  All rights reserved.
Author: Min-Chie Jeng (For SPICE3E2)
File: ufsset.c
**********/

#include "ufsdefs.h"

#define CKTmkVolt(a, b, c, d) a->mkVolt(b, c, d)

// This function performs initial setup of the device and model


namespace {
    int get_node_ptr(sCKT *ckt, sUFSinstance *inst)
    {
        TSTALLOC(UFSGgPtr, UFSgNode, UFSgNode)     
        TSTALLOC(UFSGdpPtr, UFSgNode, UFSdNodePrime)     
        TSTALLOC(UFSGspPtr, UFSgNode, UFSsNodePrime)     
        TSTALLOC(UFSGbpPtr, UFSgNode, UFSbNodePrime)     
        TSTALLOC(UFSGgbPtr, UFSgNode, UFSbgNode)         

        TSTALLOC(UFSDPgPtr, UFSdNodePrime, UFSgNode)     
        TSTALLOC(UFSDPdpPtr, UFSdNodePrime, UFSdNodePrime)
        TSTALLOC(UFSDPspPtr, UFSdNodePrime, UFSsNodePrime)
        TSTALLOC(UFSDPbpPtr, UFSdNodePrime, UFSbNodePrime)
        TSTALLOC(UFSDPgbPtr, UFSdNodePrime, UFSbgNode)

        TSTALLOC(UFSSPgPtr, UFSsNodePrime, UFSgNode)     
        TSTALLOC(UFSSPdpPtr, UFSsNodePrime, UFSdNodePrime)
        TSTALLOC(UFSSPspPtr, UFSsNodePrime, UFSsNodePrime)
        TSTALLOC(UFSSPbpPtr, UFSsNodePrime, UFSbNodePrime)
        TSTALLOC(UFSSPgbPtr, UFSsNodePrime, UFSbgNode)

        TSTALLOC(UFSBPgPtr, UFSbNodePrime, UFSgNode)     
        TSTALLOC(UFSBPdpPtr, UFSbNodePrime, UFSdNodePrime)
        TSTALLOC(UFSBPspPtr, UFSbNodePrime, UFSsNodePrime)
        TSTALLOC(UFSBPbpPtr, UFSbNodePrime, UFSbNodePrime)
        TSTALLOC(UFSBPgbPtr, UFSbNodePrime, UFSbgNode)

        TSTALLOC(UFSGBgPtr, UFSbgNode, UFSgNode)         
        TSTALLOC(UFSGBdpPtr, UFSbgNode, UFSdNodePrime)
        TSTALLOC(UFSGBspPtr, UFSbgNode, UFSsNodePrime)
        TSTALLOC(UFSGBbpPtr, UFSbgNode, UFSbNodePrime)
        TSTALLOC(UFSGBgbPtr, UFSbgNode, UFSbgNode)

        TSTALLOC(UFSDdPtr, UFSdNode, UFSdNode)
        TSTALLOC(UFSDdpPtr, UFSdNode, UFSdNodePrime)
        TSTALLOC(UFSDPdPtr, UFSdNodePrime, UFSdNode)

        TSTALLOC(UFSSsPtr, UFSsNode, UFSsNode)
        TSTALLOC(UFSSspPtr, UFSsNode, UFSsNodePrime)
        TSTALLOC(UFSSPsPtr, UFSsNodePrime, UFSsNode)

        TSTALLOC(UFSBbPtr, UFSbNode, UFSbNode)
        TSTALLOC(UFSBbpPtr, UFSbNode, UFSbNodePrime)
        TSTALLOC(UFSBPbPtr, UFSbNodePrime, UFSbNode)

    /* Thermal pointers */
        TSTALLOC(UFSTtPtr, UFStNode, UFStNode)
        TSTALLOC(UFSGtPtr, UFSgNode, UFStNode)           
        TSTALLOC(UFSDPtPtr, UFSdNodePrime, UFStNode)
        TSTALLOC(UFSSPtPtr, UFSsNodePrime, UFStNode)
        TSTALLOC(UFSBPtPtr, UFSbNodePrime, UFStNode)
        TSTALLOC(UFSGBtPtr, UFSbgNode, UFStNode)
        TSTALLOC(UFSTgPtr, UFStNode, UFSgNode)           
        TSTALLOC(UFSTdpPtr, UFStNode, UFSdNodePrime)
        TSTALLOC(UFSTspPtr, UFStNode, UFSsNodePrime)
        TSTALLOC(UFSTbpPtr, UFStNode, UFSbNodePrime)
        TSTALLOC(UFSTgbPtr, UFStNode, UFSbgNode)
        return (OK);
    }
}


int
UFSdev::setup(sGENmodel *genmod, sCKT *ckt, int *states)
{
(void)ckt;
sUFSmodel *model = (sUFSmodel*)genmod;
sUFSinstance *here;
struct ufsAPI_ModelData *pModel;
struct ufsAPI_InstData *pInst;
struct ufsAPI_EnvData Env;
int error;
/*
CKTnode *tmp;
*/
sCKTnode *tmp;
//double tmp1, tmp2, T1;

    Env.Temperature = ckt->CKTtemp;
    Env.Tnom = ckt->CKTnomTemp;
    /*  loop through all the UFS device models */
    for(; model != NULL; model = model->UFSnextModel )
    {   /* Default value Processing for UFS MOSFET Models */
        if (!model->UFSparamChkGiven) 
            model->UFSparamChk = 0;
        if (!model->UFSdebugGiven) 
            model->UFSdebug = 0;

	pModel = model->pModel;
	ufsDefaultModelParam(pModel, &Env);
	ufsInitModel(pModel, &Env);

        /* loop through all the instances of the model */
        for (here = model->UFSinstances; here != NULL ;
             here = here->UFSnextInstance) 
	{   /* allocate a chunk of the state vector */
            here->UFSstates = *states;
            *states += UFSnumStates;

            /* perform the parameter defaulting */
            if (!here->UFSicVBSGiven)
                here->UFSicVBS = 0;
            if (!here->UFSicVDSGiven)
                here->UFSicVDS = 0;
            if (!here->UFSicVGFSGiven)
                here->UFSicVGFS = 0;
            if (!here->UFSicVGBSGiven)
                here->UFSicVGBS = 0;

	    pInst = here->pInst;
	    ufsDefaultInstParam(pModel, pInst, &Env);
// SRW
    if (!pInst->WidthGiven)
        pInst->Width = ckt->mos_default_w();
    if (!pInst->LengthGiven)
        pInst->Length = ckt->mos_default_l();
    if (!pInst->DrainAreaGiven)
        pInst->DrainArea = ckt->mos_default_ad();
    if (!pInst->SourceAreaGiven)
        pInst->SourceArea = ckt->mos_default_as();

	    ufsInitInst(pModel, pInst, &Env);

            /* process drain series resistance */
            if ((pInst->DrainConductance > 0.0)
		&& (here->UFSdNodePrime == 0))
	    {   error = CKTmkVolt(ckt,&tmp,here->UFSname,"drain");
                if(error) return(error);
                here->UFSdNodePrime = tmp->number();
            }
	    else
	    {   here->UFSdNodePrime = here->UFSdNode;
            }
                   
            /* process source series resistance */
            if ((pInst->SourceConductance > 0.0)
		&& (here->UFSsNodePrime == 0)) 
	    {   error = CKTmkVolt(ckt, &tmp, here->UFSname, "source");
                if(error) return(error);
                here->UFSsNodePrime = tmp->number();
            }
	    else 
	    {   here->UFSsNodePrime = here->UFSsNode;
            }

            if ((pModel->Selft > 0) && (here->UFStNode == 0)) 
	    {   error = CKTmkVolt(ckt, &tmp, here->UFSname, "temp_node");
                if(error) return(error);
                here->UFStNode = tmp->number();
            }
	    else 
	    {   here->UFStNode = 0;
            }

            /* set the substrate (backgate) node to ground if not provided */
            if (here->UFSbgNode < 0)
	        here->UFSbgNode = 0;

            /* if bulk (nfdmod=2), tie the body node to back gate */                     /* 6.0bulk */
	    if (pModel->NfdMod == 2)                                                     /* 6.0bulk */ 
      	        {/* if 3, 4 or 5 nodes - don't care if B is specified */                 /* 6.0bulk */
                      /* check for well/substrate resistance */                          /* 6.0bulk */
	              if ((pInst->BodyConductance > 0.0) && (here->UFSbNodePrime == 0))  /* 6.0bulk */
	              {   error = CKTmkVolt(ckt,&tmp,here->UFSname,"body");              /* 6.0bulk */
                          if(error) return(error);                                       /* 6.0bulk */
                          here->UFSbNodePrime = tmp->number();                             /* 6.0bulk */
 	        	  here->UFSbNode = here->UFSbgNode;                              /* 6.0bulk */
                      }                                                                  /* 6.0bulk */
	              else                                                               /* 6.0bulk */
	              {   here->UFSbNodePrime = here->UFSbgNode;                         /* 6.0bulk */
 	        	  here->UFSbNode = here->UFSbgNode;                              /* 6.0bulk */
                      }                                                                  /* 6.0bulk */
	              pInst->BulkContact = 1;                                            /* 6.0bulk */
        	}                                                                        /* 6.0bulk */
            else /* set up for the SOI structure */
	    {
               /* process body series resistance */
               if (here->UFSbNode < 0)
               {   /* Floating body: 3 or 4 nodes */
	           error = CKTmkVolt(ckt,&tmp,here->UFSname,"body");
                   if(error) return(error);
                   here->UFSbNodePrime = tmp->number();
	           here->UFSbNode = here->UFSbNodePrime;
	           pInst->BulkContact = 0;   /*check vbs limit/convergence*/
               }
               else
	       {   /* With body contact: 5 nodes */
	           if ((pInst->BodyConductance > 0.0) && (here->UFSbNodePrime == 0)) 
	           {   error = CKTmkVolt(ckt,&tmp,here->UFSname,"body");
                       if(error) return(error);
                       here->UFSbNodePrime = tmp->number();
                   }
	           else 
	           {   here->UFSbNodePrime = here->UFSbNode;
                   }
	           pInst->BulkContact = 1;
               }
            }

        /* set Sparse Matrix Pointers */

/* macro to make elements with built in test for out of memory */
/*
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

#undef inst
}


int
UFSdev::unsetup(sGENmodel *genmod, sCKT*)
{
    sUFSmodel *model = static_cast<sUFSmodel*>(genmod);
    for ( ; model; model = model->next()) {
        sUFSinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {
            if (inst->UFSdNodePrime != inst->UFSdNode)
                inst->UFSdNodePrime = 0;
            if (inst->UFSsNodePrime != inst->UFSsNode)
                inst->UFSsNodePrime = 0;
            inst->UFSbNodePrime = 0;
            inst->UFStNode = 0;
        }
    }
    return (OK);
}


// SRW - reset the matrix element pointers.
//
int
UFSdev::resetup(sGENmodel *inModel, sCKT *ckt)
{
    for (sUFSmodel *model = (sUFSmodel*)inModel; model;
            model = model->next()) {
        for (sUFSinstance *here = model->inst(); here;
                here = here->next()) {
            int error = get_node_ptr(ckt, here);
            if (error != OK)
                return (error);
        }
    }
    return (OK);
}

