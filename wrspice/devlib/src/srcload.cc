
/*========================================================================*
 *                                                                        *
 *  Distributed by Whiteley Research Inc., Sunnyvale, California, USA     *
 *                       http://wrcad.com                                 *
 *  Copyright (C) 2017 Whiteley Research Inc., all rights reserved.       *
 *  Author: Stephen R. Whiteley, except as indicated.                     *
 *                                                                        *
 *  As fully as possible recognizing licensing terms and conditions       *
 *  imposed by earlier work from which this work was derived, if any,     *
 *  this work is released under the Apache License, Version 2.0 (the      *
 *  "License").  You may not use this file except in compliance with      *
 *  the License, and compliance with inherited licenses which are         *
 *  specified in a sub-header below this one if applicable.  A copy       *
 *  of the License is provided with this distribution, or you may         *
 *  obtain a copy of the License at                                       *
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

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1987 Kanwar Jit Singh
         1992 Stephen R. Whiteley
****************************************************************************/

#include "srcdefs.h"
#include "dctdefs.h"


int
SRCdev::loadTest(sGENinstance *in_inst, sCKT*)
{
#ifdef USE_PRELOAD
    sSRCinstance *inst = (sSRCinstance*)in_inst;
    if (inst->SRCdcFunc == SRC_none && inst->SRCtranFunc == SRC_none)
        return (~OK);
#endif
    return (OK);
}


int
SRCdev::load(sGENinstance *in_inst, sCKT *ckt)
{
    sSRCinstance *inst = (sSRCinstance*)in_inst;
#ifndef USE_PRELOAD
    if (inst->SRCtype == SRC_V) {
        if (inst->SRCposNode) {
            ckt->ldset(inst->SRCposIbrptr, 1.0);
            ckt->ldset(inst->SRCibrPosptr, 1.0);
        }
        if (inst->SRCnegNode) {
            ckt->ldset(inst->SRCnegIbrptr, -1.0);
            ckt->ldset(inst->SRCibrNegptr, -1.0);
        }
    }
#endif
    bool is_dc = ckt->CKTmode & (MODEDCOP | MODEDCTRANCURVE);
    SRCfuncType ft = is_dc ? inst->SRCdcFunc : inst->SRCtranFunc;

    switch (ft) {
    case SRC_none:
        // We've hit the sources that don't require loading, which
        // were moved to the end of the list in setup.
#ifdef USE_PRELOAD
        // Tell caller to quit loading this device model type.
        return (LOAD_SKIP_FLAG);
#else
        return (OK);
#endif
    case SRC_dc:
        {
            double rhs = ckt->CKTsrcFact * inst->SRCdcValue;
            if (inst->SRCtype == SRC_V)
                ckt->rhsadd(inst->SRCbranch, rhs);
            else {
                ckt->rhsadd(inst->SRCposNode, -rhs);
                ckt->rhsadd(inst->SRCnegNode, rhs);
                inst->SRCvalue = rhs;  // constant, no interpolation
            }
        }
        break;
    case SRC_ccvs:
        {
            if (!inst->SRCccCoeffGiven && inst->SRCacTabName) {
                if ((ckt->CKTmode & MODEINITPRED) && inst->SRCworkArea) {
                    // starting new time point, accumualte last result
                    IFcomplex *c = inst->SRCworkArea;
                    IFcomplex *d = inst->SRCtimeResp;
                    for (int i = 0; i < inst->SRCfreqRespLen; i++) {
                        d->real += c->real;
                        d->imag += c->imag;
                        c++;
                        d++;
                    }
                }
                double ain = *(ckt->CKTrhsOld + (inst->SRCcontBranch));
                inst->SRCdo_fft(ckt, ain, &inst->SRCvalue);
                ckt->rhsadd(inst->SRCbranch, -inst->SRCvalue);
            }
#ifndef USE_PRELOAD
            else {
                ckt->ldadd(inst->SRCibrContBrptr, -inst->SRCcoeff.real);
            }
#endif
        }
        break;
    case SRC_vcvs:
        {
            if (!inst->SRCvcCoeffGiven && inst->SRCacTabName) {
                if ((ckt->CKTmode & MODEINITPRED) && inst->SRCworkArea) {
                    // starting new time point, accumualte last result
                    IFcomplex *c = inst->SRCworkArea;
                    IFcomplex *d = inst->SRCtimeResp;
                    for (int i = 0; i < inst->SRCfreqRespLen; i++) {
                        d->real += c->real;
                        d->imag += c->imag;
                        c++;
                        d++;
                    }
                }
                double ain = *(ckt->CKTrhsOld + (inst->SRCcontPosNode)) -
                    *(ckt->CKTrhsOld + (inst->SRCcontNegNode));
                inst->SRCdo_fft(ckt, ain, &inst->SRCvalue);
                ckt->rhsadd(inst->SRCbranch, inst->SRCvalue);
            }
#ifndef USE_PRELOAD
            else {
                ckt->ldadd(inst->SRCibrContPosptr, -inst->SRCcoeff.real);
                ckt->ldadd(inst->SRCibrContNegptr, inst->SRCcoeff.real);
            }
#endif
        }
        break;
    case SRC_cccs:
        {
            if (!inst->SRCccCoeffGiven && inst->SRCacTabName) {
                if ((ckt->CKTmode & MODEINITPRED) && inst->SRCworkArea) {
                    // starting new time point, accumualte last result
                    IFcomplex *c = inst->SRCworkArea;
                    IFcomplex *d = inst->SRCtimeResp;
                    for (int i = 0; i < inst->SRCfreqRespLen; i++) {
                        d->real += c->real;
                        d->imag += c->imag;
                        c++;
                        d++;
                    }
                }
                double ain = *(ckt->CKTrhsOld + (inst->SRCcontBranch));
                inst->SRCdo_fft(ckt, ain, &inst->SRCvalue);
                ckt->rhsadd(inst->SRCposNode, -inst->SRCvalue);
                ckt->rhsadd(inst->SRCnegNode, inst->SRCvalue);
            }
#ifndef USE_PRELOAD
            else {
                ckt->ldadd(inst->SRCposContBrptr, inst->SRCcoeff.real);
                ckt->ldadd(inst->SRCnegContBrptr, -inst->SRCcoeff.real);
            }
#endif
        }
        break;
    case SRC_vccs:
        {
            if (!inst->SRCvcCoeffGiven && inst->SRCacTabName) {
                if ((ckt->CKTmode & MODEINITPRED) && inst->SRCworkArea) {
                    // starting new time point, accumualte last result
                    IFcomplex *c = inst->SRCworkArea;
                    IFcomplex *d = inst->SRCtimeResp;
                    for (int i = 0; i < inst->SRCfreqRespLen; i++) {
                        d->real += c->real;
                        d->imag += c->imag;
                        c++;
                        d++;
                    }
                }
                double ain = *(ckt->CKTrhsOld + (inst->SRCcontPosNode)) -
                    *(ckt->CKTrhsOld + (inst->SRCcontNegNode));
                inst->SRCdo_fft(ckt, ain, &inst->SRCvalue);
                ckt->rhsadd(inst->SRCposNode, -inst->SRCvalue);
                ckt->rhsadd(inst->SRCnegNode, inst->SRCvalue);
            }
#ifndef USE_PRELOAD
            else {
                ckt->ldadd(inst->SRCposContPosptr, inst->SRCcoeff.real);
                ckt->ldadd(inst->SRCposContNegptr, -inst->SRCcoeff.real);
                ckt->ldadd(inst->SRCnegContPosptr, -inst->SRCcoeff.real);
                ckt->ldadd(inst->SRCnegContNegptr, inst->SRCcoeff.real);
            }
#endif
        }
        break;
    case SRC_func:
        {
            int numvars = inst->SRCtree->num_vars();
            if (!numvars) {
                double rhs = 0;

                sDCTAN *dct = dynamic_cast<sDCTAN*>(ckt->CKTcurJob);
#ifdef ALLPRMS
                // If sweeping the DC value (the default parameter)
                // take the source as a DC source.
                if (dct &&
                        ((dct->JOBdc.elt(0) == inst &&
                            dct->JOBdc.param(0) == SRC_DC) ||
                        (dct->JOBdc.elt(1) == inst &&
                            dct->JOBdc.param(1) == SRC_DC)))
#else
                if (dct && (dct->JOBdc.elt(0) == inst ||
                        dct->JOBdc.elt(1) == inst))
#endif
                    rhs = ckt->CKTsrcFact * inst->SRCdcValue;
                else {
                    BEGIN_EVAL
                    int ret = inst->SRCtree->eval(&rhs, 0, 0);
                    END_EVAL
                    if (ret == OK) {
                        // for test in niiter.c
                        inst->SRCdcValue = rhs;
                        if (ckt->CKTmode & MODEINITSMSIG)
                            // Store the rhs for small signal analysis
                            inst->SRCacValues[0] = rhs; 
                        else if (ckt->CKTmode & (MODEDCOP | MODETRANOP))
                            rhs *= ckt->CKTsrcFact;
                    }
                    else
                        return (ret);
                }
                if (inst->SRCtype == SRC_V)
                    ckt->rhsadd(inst->SRCbranch, rhs);
                else {
                    ckt->rhsadd(inst->SRCposNode, -rhs);
                    ckt->rhsadd(inst->SRCnegNode, rhs);
                    // interpolate current for SRCask()
                    *(ckt->CKTstate0 + inst->GENstate) = rhs;
                }
                break;
            }

            if (ckt->CKTmode & (MODEINITFLOAT|MODEINITFIX)) {
                for (int i = 0; i < numvars; i++)
                    inst->SRCvalues[i] = *(ckt->CKTrhsOld + inst->SRCeqns[i]);
            }
            else if (ckt->CKTmode & MODEINITPRED) {
                for (int i = 0; i < numvars; i++)
                    inst->SRCvalues[i] = DEV.pred(ckt, inst->GENstate + i);
            }
            else if (ckt->CKTmode & MODEINITJCT) {
                for (int i = 0; i < numvars; i++)
                    inst->SRCvalues[i] = 0;
            }
            else {
                for (int i = 0; i < numvars; i++)
                    inst->SRCvalues[i] = *(ckt->CKTstate1 + inst->GENstate + i);
            }

            int i;
            for (i = 0; i < numvars; i++)
                *(ckt->CKTstate0 + inst->GENstate + i) = inst->SRCvalues[i];

            double rhs = 0;
            BEGIN_EVAL
            int ret = inst->SRCtree->eval(&rhs, inst->SRCvalues,
                inst->SRCderivs);
            END_EVAL
            if (ret == OK) {

                inst->SRCprev = rhs;   // for convergence test

                if (inst->SRCtype == SRC_V) {
                    for (i = 0; i < numvars; i++) {
                        double deriv = inst->SRCderivs[i];
                        rhs -= (inst->SRCvalues[i] * deriv);
                        ckt->ldadd(inst->SRCposptr[i], -deriv);
                    }
                    ckt->rhsadd(inst->SRCbranch, rhs);
                }
                else {
                    int j;
                    for (j = 0,i = 0; i < numvars; i++) {
                        double deriv = inst->SRCderivs[i];
                        rhs -= (inst->SRCvalues[i] * deriv);
                        ckt->ldadd(inst->SRCposptr[j++], deriv);
                        ckt->ldadd(inst->SRCposptr[j++], -deriv);
                    }
                    ckt->rhsadd(inst->SRCposNode, -rhs);
                    ckt->rhsadd(inst->SRCnegNode, rhs);
                    // interpolate current for SRCask()
                    *(ckt->CKTstate0 + inst->GENstate +
                        inst->SRCtree->num_vars()) = inst->SRCprev;
                }

                // Store the rhs for small signal analysis
                if (ckt->CKTmode & MODEINITSMSIG) {
                    for (i = 0; i < numvars; i++)
                        inst->SRCacValues[i] = inst->SRCderivs[i]; 
                    inst->SRCacValues[numvars] = rhs; 
                }
            }
            else
                return (ret);
        }
    }
    return (OK);
}


#ifdef NEW_FASTLIN

int
SRCdev::loadRHS(sGENinstance *in_inst, sCKT *ckt)
{
    return (load(in_inst, ckt));
}

#endif

