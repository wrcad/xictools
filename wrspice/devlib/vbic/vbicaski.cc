
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
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1985 Mathew Lew and Thomas L. Quarles
Model Author: 1995 Colin McAndrew Motorola
Spice3 Implementation: 2003 Dietmar Warning DAnalyse GmbH
**********/

#include "vbicdefs.h"
#include "gencurrent.h"

// SRW - this function was rather extensively modified
//  1) call ckt->interp() for state table variables
//  2) use ac analysis functions for ac currents


// This routine gives access to the internal device 
// parameters for VBICs
//
int
VBICdev::askInst(const sCKT *ckt, const sGENinstance *geninst, int which,
    IFdata *data)
{
    const sVBICinstance *here = static_cast<const sVBICinstance*>(geninst);
    IFvalue *value = &data->v;

/*
    int itmp;
    double vr;
    double vi;
    double sr;
    double si;
    double vm;
*/

    // Need to override this for non-real returns.
    data->type = IF_REAL;

    switch(which) {
        case VBIC_AREA:
            value->rValue = here->VBICarea;
            return(OK);
        case VBIC_OFF:
            data->type = IF_INTEGER;
            value->iValue = here->VBICoff;
            return(OK);
        case VBIC_IC_VBE:
            value->rValue = here->VBICicVBE;
            return(OK);
        case VBIC_IC_VCE:
            value->rValue = here->VBICicVCE;
            return(OK);
        case VBIC_TEMP:
            value->rValue = here->VBICtemp - CONSTCtoK;
            return(OK);
        case VBIC_M:
            value->rValue = here->VBICm;
            return(OK);
        case VBIC_QUEST_COLLNODE:
            data->type = IF_INTEGER;
            value->iValue = here->VBICcollNode;
            return(OK);
        case VBIC_QUEST_BASENODE:
            data->type = IF_INTEGER;
            value->iValue = here->VBICbaseNode;
            return(OK);
        case VBIC_QUEST_EMITNODE:
            data->type = IF_INTEGER;
            value->iValue = here->VBICemitNode;
            return(OK);
        case VBIC_QUEST_SUBSNODE:
            data->type = IF_INTEGER;
            value->iValue = here->VBICsubsNode;
            return(OK);
        case VBIC_QUEST_COLLCXNODE:
            data->type = IF_INTEGER;
            value->iValue = here->VBICcollCXNode;
            return(OK);
        case VBIC_QUEST_BASEBXNODE:
            data->type = IF_INTEGER;
            value->iValue = here->VBICbaseBXNode;
            return(OK);
        case VBIC_QUEST_EMITEINODE:
            data->type = IF_INTEGER;
            value->iValue = here->VBICemitEINode;
            return(OK);
        case VBIC_QUEST_SUBSSINODE:
            data->type = IF_INTEGER;
            value->iValue = here->VBICsubsSINode;
            return(OK);
        case VBIC_QUEST_VBE:
//            value->rValue = *(ckt->CKTstate0 + here->VBICvbei);
            value->rValue = ckt->interp(here->VBICvbei);
            return(OK);
        case VBIC_QUEST_VBC:
//            value->rValue = *(ckt->CKTstate0 + here->VBICvbci);
            value->rValue = ckt->interp(here->VBICvbci);
            return(OK);
        case VBIC_QUEST_CC:
/*
            value->rValue = *(ckt->CKTstate0 + here->VBICitzf) -
                            *(ckt->CKTstate0 + here->VBICitzr) -
                            *(ckt->CKTstate0 + here->VBICibc);
*/
            value->rValue = ckt->interp(here->VBICitzf) -
                            ckt->interp(here->VBICitzr) -
                            ckt->interp(here->VBICibc);
            return(OK);
        case VBIC_QUEST_CB:
/*
            value->rValue = *(ckt->CKTstate0 + here->VBICibe) +
                            *(ckt->CKTstate0 + here->VBICibc) +
                            *(ckt->CKTstate0 + here->VBICibex) +
                            *(ckt->CKTstate0 + here->VBICibep) +
                            *(ckt->CKTstate0 + here->VBICiccp);
*/
            value->rValue = ckt->interp(here->VBICibe) +
                            ckt->interp(here->VBICibc) +
                            ckt->interp(here->VBICibex) +
                            ckt->interp(here->VBICibep) +
                            ckt->interp(here->VBICiccp);
            return(OK);
        case VBIC_QUEST_CE:
/*
            value->rValue = - *(ckt->CKTstate0 + here->VBICibe) -
                            *(ckt->CKTstate0 + here->VBICibex) -
                            *(ckt->CKTstate0 + here->VBICitzf) +
                            *(ckt->CKTstate0 + here->VBICitzr);
*/
            value->rValue = - ckt->interp(here->VBICibe) -
                            ckt->interp(here->VBICibex) -
                            ckt->interp(here->VBICitzf) +
                            ckt->interp(here->VBICitzr);
            return(OK);
        case VBIC_QUEST_CS:
/*
            value->rValue = *(ckt->CKTstate0 + here->VBICiccp) -
                            *(ckt->CKTstate0 + here->VBICibcp);
*/
            value->rValue = ckt->interp(here->VBICiccp) -
                            ckt->interp(here->VBICibcp);
            return(OK);
        case VBIC_QUEST_POWER:
/*
            value->rValue = fabs(*(ckt->CKTstate0 + here->VBICitzf) - *(ckt->CKTstate0 + here->VBICitzr)) 
                            * fabs(*(ckt->CKTstate0 + here->VBICvbei) - *(ckt->CKTstate0 + here->VBICvbci)) +
                            fabs(*(ckt->CKTstate0 + here->VBICibe) * *(ckt->CKTstate0 + here->VBICvbei)) +
                            fabs(*(ckt->CKTstate0 + here->VBICibex) * *(ckt->CKTstate0 + here->VBICvbex)) +
                            fabs(*(ckt->CKTstate0 + here->VBICibc) * *(ckt->CKTstate0 + here->VBICvbci)) +
                            fabs(*(ckt->CKTstate0 + here->VBICibcp) * *(ckt->CKTstate0 + here->VBICvbcp)) +
                            fabs(*(ckt->CKTstate0 + here->VBICiccp)) 
                            * fabs(*(ckt->CKTstate0 + here->VBICvbep) - *(ckt->CKTstate0 + here->VBICvbcp));
*/
            value->rValue =
                fabs(ckt->interp(here->VBICitzf) -
                    ckt->interp(here->VBICitzr)) *
                fabs(ckt->interp(here->VBICvbei) -
                    ckt->interp(here->VBICvbci)) +
                fabs(ckt->interp(here->VBICibe) *
                    ckt->interp( here->VBICvbei)) +
                fabs(ckt->interp(here->VBICibex) *
                    ckt->interp(here->VBICvbex)) +
                fabs(ckt->interp(here->VBICibc) *
                    ckt->interp(here->VBICvbci)) +
                fabs(ckt->interp(here->VBICibcp) *
                    ckt->interp(here->VBICvbcp)) +
                fabs(ckt->interp(here->VBICiccp)) 
                    * fabs(ckt->interp(here->VBICvbep) -
                ckt->interp(here->VBICvbcp));

            return(OK);
        case VBIC_QUEST_GM:
//            value->rValue = *(ckt->CKTstate0 + here->VBICitzf_Vbei);
            value->rValue = ckt->interp(here->VBICitzf_Vbei);
            return(OK);
        case VBIC_QUEST_GO:
//            value->rValue = *(ckt->CKTstate0 + here->VBICitzf_Vbci);
            value->rValue = ckt->interp(here->VBICitzf_Vbci);
            return(OK);
        case VBIC_QUEST_GPI:
//            value->rValue = *(ckt->CKTstate0 + here->VBICibe_Vbei);
            value->rValue = ckt->interp(here->VBICibe_Vbei);
            return(OK);
        case VBIC_QUEST_GMU:
//            value->rValue = *(ckt->CKTstate0 + here->VBICibc_Vbci);
            value->rValue = ckt->interp(here->VBICibc_Vbci);
            return(OK);
        case VBIC_QUEST_GX:
//            value->rValue = *(ckt->CKTstate0 + here->VBICirbi_Vrbi);
            value->rValue = ckt->interp(here->VBICirbi_Vrbi);
            return(OK);
        case VBIC_QUEST_CBE:
            value->rValue = here->VBICcapbe;
            return(OK);
        case VBIC_QUEST_CBEX:
            value->rValue = here->VBICcapbex;
            return(OK);
        case VBIC_QUEST_CBC:
            value->rValue = here->VBICcapbc;
            return(OK);
        case VBIC_QUEST_CBCX:
            value->rValue = here->VBICcapbcx;
            return(OK);
        case VBIC_QUEST_CBEP:
            value->rValue = here->VBICcapbep;
            return(OK);
        case VBIC_QUEST_CBCP:
            value->rValue = here->VBICcapbcp;
            return(OK);
        case VBIC_QUEST_QBE:
//            value->rValue = *(ckt->CKTstate0 + here->VBICqbe);
            value->rValue = ckt->interp(here->VBICqbe);
            return(OK);
        case VBIC_QUEST_QBC:
//            value->rValue = *(ckt->CKTstate0 + here->VBICqbc);
            value->rValue = ckt->interp(here->VBICqbc);

            return(OK);
/* SRW
        case VBIC_QUEST_SENS_DC:
            if(ckt->CKTsenInfo){
                value->rValue = *(ckt->CKTsenInfo->SEN_Sap[select->iValue + 1]+
                    here->VBICsenParmNo);
            }
            return(OK);
        case VBIC_QUEST_SENS_REAL:
            if(ckt->CKTsenInfo){
                value->rValue = *(ckt->CKTsenInfo->SEN_RHS[select->iValue + 1]+
                    here->VBICsenParmNo);
            }
            return(OK);
        case VBIC_QUEST_SENS_IMAG:
            if(ckt->CKTsenInfo){
                value->rValue = *(ckt->CKTsenInfo->SEN_iRHS[select->iValue + 1]+
                    here->VBICsenParmNo);
            }
            return(OK);
        case VBIC_QUEST_SENS_MAG:
            if(ckt->CKTsenInfo){
               vr = ckt->rhsOld(select->iValue + 1); 
               vi = ckt->irhsOld(select->iValue + 1); 
               vm = sqrt(vr*vr + vi*vi);
               if(vm == 0){
                 value->rValue = 0;
                 return(OK);
               }
               sr = *(ckt->CKTsenInfo->SEN_RHS[select->iValue + 1]+
                    here->VBICsenParmNo);
               si = *(ckt->CKTsenInfo->SEN_iRHS[select->iValue + 1]+
                    here->VBICsenParmNo);
                   value->rValue = (vr * sr + vi * si)/vm;
            }
            return(OK);
        case VBIC_QUEST_SENS_PH:
            if(ckt->CKTsenInfo){
               vr = ckt->rhsOld(select->iValue + 1); 
               vi = ckt->irhsOld(select->iValue + 1); 
               vm = vr*vr + vi*vi;
               if(vm == 0){
                 value->rValue = 0;
                 return(OK);
               }
               sr = *(ckt->CKTsenInfo->SEN_RHS[select->iValue + 1]+
                    here->VBICsenParmNo);
               si = *(ckt->CKTsenInfo->SEN_iRHS[select->iValue + 1]+
                    here->VBICsenParmNo);
       
                   value->rValue =  (vr * si - vi * sr)/vm;
            }
            return(OK);
        case VBIC_QUEST_SENS_CPLX:
            if(ckt->CKTsenInfo){
               itmp = select->iValue + 1;
               value->cValue.real= *(ckt->CKTsenInfo->SEN_RHS[itmp]+
                   here->VBICsenParmNo);
               value->cValue.imag= *(ckt->CKTsenInfo->SEN_iRHS[itmp]+
                   here->VBICsenParmNo);
            }
            return(OK);
*/
        default:
            return(E_BADPARM);
    }
    /* NOTREACHED */
}

