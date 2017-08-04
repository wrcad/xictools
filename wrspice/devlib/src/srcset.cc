
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
#include "input.h"
#include "ifdata.h"


#define TSTALLOC_1(ptr, first, second) \
    if ((inst->ptr = ckt->alloc(inst->first, second)) == 0) { \
        return (E_NOMEM); }

#define TSTALLOC_2(ptr, first, second) \
    if ((inst->ptr = ckt->alloc(inst->first, second->number())) == 0) { \
        return (E_NOMEM); }


int
SRCdev::setup(sGENmodel *genmod, sCKT *ckt, int *states)
{
    sSRCmodel *model = static_cast<sSRCmodel*>(genmod);
    for ( ; model; model = model->next()) {
        sSRCinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {
            
            if (inst->SRCtype == SRC_V) {
                if (inst->SRCbranch == 0 &&
                        (inst->SRCposNode || inst->SRCnegNode)) {
                    sCKTnode *tmp;
                    int error = ckt->mkCur(&tmp, inst->GENname, "branch");
                    if (error)
                        return (error);
                    inst->SRCbranch = tmp->number();
                }
            }

            inst->SRCvalue = inst->SRCdcValue;

            if (inst->SRCdep == 0) {
                if (inst->SRCacGiven && !inst->SRCacMGiven)
                    inst->SRCacMag = 1;
                if (inst->SRCacGiven && !inst->SRCacPGiven)
                    inst->SRCacPhase = 0;
                double radians = inst->SRCacPhase * M_PI / 180.0;
                inst->SRCacReal = inst->SRCacMag * cos(radians);
                inst->SRCacImag = inst->SRCacMag * sin(radians);

                if (!inst->SRCdcGiven) {
                    // no DC value - either have a transient value, or none
                    if (inst->SRCtree) {
                        DVO.textOut(OUT_INFO,
                            "%s: no DC value, transient time 0 value used",
                            inst->GENname);
                    }
                    else {
                        DVO.textOut(OUT_INFO, "%s: has no value, DC 0 assumed",
                            inst->GENname);
                    }
                }
            }

            inst->GENstate = *states;

            if (inst->SRCtree) {
                *states += (inst->SRCtype == SRC_I);
                if (inst->SRCtree->num_vars()) {
                    *states += inst->SRCtree->num_vars();
                    if (!inst->SRCvalues)
                        inst->SRCvalues = new double[inst->SRCtree->num_vars()];
                    if (!inst->SRCderivs)
                        inst->SRCderivs = new double[inst->SRCtree->num_vars()];
                    if (!inst->SRCeqns)
                        inst->SRCeqns = new int[inst->SRCtree->num_vars()];
                }
                if (!inst->SRCacValues) {
                    inst->SRCacValues =
                        new double[inst->SRCtree->num_vars() + 1];
                }
            }
            else if (inst->SRCdep == SRC_CC) {
                inst->SRCcontBranch = ckt->findBranch(inst->SRCcontName);
                if (inst->SRCcontBranch == 0) {
                    DVO.textOut(OUT_FATAL,
                        "%s: unknown controlling source %s",
                        inst->GENname, inst->SRCcontName);
                    return (E_BADPARM);
                }
                if (!inst->SRCccCoeffGiven) {
                    inst->SRCcoeff.real = 1.0;
                    inst->SRCcoeff.imag = 0.0;
                }
            }
            else if (inst->SRCdep == SRC_VC) {
                if (!inst->SRCvcCoeffGiven) {
                    inst->SRCcoeff.real = 1.0;
                    inst->SRCcoeff.imag = 0.0;
                }
            }

            if (inst->SRCtype == SRC_V) {
                TSTALLOC(SRCposIbrptr, SRCposNode, SRCbranch)
                TSTALLOC(SRCnegIbrptr, SRCnegNode, SRCbranch)
                TSTALLOC(SRCibrNegptr, SRCbranch,  SRCnegNode)
                TSTALLOC(SRCibrPosptr, SRCbranch,  SRCposNode)

                // for pz analysis, an ac source is open, ac has precedence
                // over FUNC
                //
                if (inst->SRCacGiven) {
                    TSTALLOC(SRCibrIbrptr, SRCbranch,  SRCbranch)
                }

#ifdef USE_PRELOAD
                if (ckt->CKTpreload) {
                    // preload constants

                    if (inst->SRCposNode) {
                        ckt->preldset(inst->SRCposIbrptr, 1.0);
                        ckt->preldset(inst->SRCibrPosptr, 1.0);
                    }
                    if (inst->SRCnegNode) {
                        ckt->preldset(inst->SRCnegIbrptr, -1.0);
                        ckt->preldset(inst->SRCibrNegptr, -1.0);
                    }
                }
#endif

                if (inst->SRCdep == SRC_CC) {
                    TSTALLOC(SRCibrContBrptr, SRCbranch, SRCcontBranch)
                    if (!inst->SRCtree) {
                        inst->SRCdcFunc = SRC_ccvs;
                        inst->SRCtranFunc = SRC_ccvs;
#ifdef USE_PRELOAD
                        if (ckt->CKTpreload &&
                                (inst->SRCccCoeffGiven ||
                                !inst->SRCacTabName)) {
                            ckt->ldadd(inst->SRCibrContBrptr,
                                -inst->SRCcoeff.real);
                        }
#endif
                        continue;
                    }
                }
                else if (inst->SRCdep == SRC_VC) {
                    TSTALLOC(SRCibrContPosptr, SRCbranch, SRCcontPosNode)
                    TSTALLOC(SRCibrContNegptr, SRCbranch, SRCcontNegNode)
                    if (!inst->SRCtree) {
                        inst->SRCdcFunc = SRC_vcvs;
                        inst->SRCtranFunc = SRC_vcvs;
#ifdef USE_PRELOAD
                        if (ckt->CKTpreload &&
                                (inst->SRCvcCoeffGiven ||
                                !inst->SRCacTabName)) {
                            ckt->ldadd(inst->SRCibrContPosptr,
                                -inst->SRCcoeff.real);
                            ckt->ldadd(inst->SRCibrContNegptr,
                                inst->SRCcoeff.real);
                        }
#endif
                        continue;
                    }
                }
                else if (!inst->SRCtree) {
                    inst->SRCdcFunc = SRC_dc;
                    inst->SRCtranFunc = SRC_dc;
                    continue;
                }
            }
            else {

                if (inst->SRCdep == SRC_CC) {
                    TSTALLOC(SRCposContBrptr,SRCposNode, SRCcontBranch)
                    TSTALLOC(SRCnegContBrptr,SRCnegNode, SRCcontBranch)
                    if (!inst->SRCtree) {
                        inst->SRCdcFunc = SRC_cccs;
                        inst->SRCtranFunc = SRC_cccs;
#ifdef USE_PRELOAD
                        if (ckt->CKTpreload &&
                                (inst->SRCccCoeffGiven ||
                                !inst->SRCacTabName)) {
                            ckt->ldadd(inst->SRCposContBrptr,
                                inst->SRCcoeff.real);
                            ckt->ldadd(inst->SRCnegContBrptr,
                                -inst->SRCcoeff.real);
                        }
#endif
                        continue;
                    }
                }
                else if (inst->SRCdep == SRC_VC) {
                    TSTALLOC(SRCposContPosptr, SRCposNode, SRCcontPosNode)
                    TSTALLOC(SRCposContNegptr, SRCposNode, SRCcontNegNode)
                    TSTALLOC(SRCnegContPosptr, SRCnegNode, SRCcontPosNode)
                    TSTALLOC(SRCnegContNegptr, SRCnegNode, SRCcontNegNode)
                    if (!inst->SRCtree) {
                        inst->SRCdcFunc = SRC_vccs;
                        inst->SRCtranFunc = SRC_vccs;
#ifdef USE_PRELOAD
                        if (ckt->CKTpreload &&
                                (inst->SRCvcCoeffGiven ||
                                !inst->SRCacTabName)) {
                            ckt->ldadd(inst->SRCposContPosptr,
                                inst->SRCcoeff.real);
                            ckt->ldadd(inst->SRCposContNegptr,
                                -inst->SRCcoeff.real);
                            ckt->ldadd(inst->SRCnegContPosptr,
                                -inst->SRCcoeff.real);
                            ckt->ldadd(inst->SRCnegContNegptr,
                                inst->SRCcoeff.real);
                        }
#endif
                        continue;
                    }
                }
                else if (!inst->SRCtree) {
                    inst->SRCdcFunc = SRC_dc;
                    inst->SRCtranFunc = SRC_dc;
                    continue;
                }
                // save a space for the current
                (*states)++;
            }

            if (inst->SRCdcGiven)
                inst->SRCdcFunc = SRC_dc;
            else
                inst->SRCdcFunc = SRC_func;
            inst->SRCtranFunc = SRC_func;
            if (inst->SRCtree->num_vars() == 0)
                continue;

            // For each controlling variable set the entries
            // in the vector of the positions of the SMP

            int j = inst->SRCtree->num_vars();
            if (inst->SRCtype == SRC_I) j *= 2;

            if (!inst->SRCposptr)
                inst->SRCposptr = new double*[j];

            int i;
            for (j = 0, i = 0; i < inst->SRCtree->num_vars(); i++) {
                if (inst->SRCtree->vars()[i].type == IF_INSTANCE) {
                    int branch =
                            ckt->findBranch(inst->SRCtree->vars()[i].v.uValue);
                    if (branch == 0) {
                        DVO.textOut(OUT_FATAL,
                            "%s: unknown controlling source %s",
                            inst->GENname, inst->SRCtree->vars()[i].v.uValue);
                        return (E_BADPARM);
                    }

                    if (inst->SRCtype == SRC_V) {
                        TSTALLOC_1(SRCposptr[j++], SRCbranch, branch);
                    }
                    else if (inst->SRCtype == SRC_I) {
                        TSTALLOC_1(SRCposptr[j++], SRCposNode, branch);
                        TSTALLOC_1(SRCposptr[j++], SRCnegNode, branch);
                    }
                    else
                        return (E_BADPARM);
                    inst->SRCeqns[i] = branch;
                }
                else if (inst->SRCtree->vars()[i].type == IF_NODE) {
                    if (inst->SRCtype == SRC_V) {
                        TSTALLOC_2(SRCposptr[j++], SRCbranch,
                            inst->SRCtree->vars()[i].v.nValue);
                    }
                    else if(inst->SRCtype == SRC_I) {
                        TSTALLOC_2(SRCposptr[j++], SRCposNode,
                            inst->SRCtree->vars()[i].v.nValue);
                        TSTALLOC_2(SRCposptr[j++], SRCnegNode,
                            inst->SRCtree->vars()[i].v.nValue);
                    }
                    else
                        return (E_BADPARM);
                    inst->SRCeqns[i] =
                        inst->SRCtree->vars()[i].v.nValue->number();
                }
            }
        }
#ifdef USE_PRELOAD
        // Linear E,F,G,H dependent sources are entirely pre-loaded. 
        // Move these to the end, so we can avoid the no-ops when
        // loading.

        sSRCinstance *prv = 0, *itmp = 0, *nxt;
        for (inst = model->inst(); inst; inst = nxt) {
            nxt = inst->next();
            if (inst->SRCtree) {
                prv = inst;
                continue;
            }
            if (inst->SRCdep == SRC_CC) {
                if (inst->SRCccCoeffGiven || !inst->SRCacTabName) {
                    if (prv)
                        prv->GENnextInstance = nxt;
                    else
                        model->GENinstances = nxt;
                    inst->SRCdcFunc = SRC_none;
                    inst->SRCtranFunc = SRC_none;
                    inst->GENnextInstance = itmp;
                    itmp = inst;
                    continue;
                }
            }
            else if (inst->SRCdep == SRC_VC) {
                if (inst->SRCccCoeffGiven || !inst->SRCacTabName) {
                    if (prv)
                        prv->GENnextInstance = nxt;
                    else
                        model->GENinstances = nxt;
                    inst->SRCdcFunc = SRC_none;
                    inst->SRCtranFunc = SRC_none;
                    inst->GENnextInstance = itmp;
                    itmp = inst;
                    continue;
                }
            }
            prv = inst;
        }
        if (itmp) {
            if (prv)
                prv->GENnextInstance = itmp;
            else
                model->GENinstances = itmp;
        }
#endif
    }
    return (OK);
}


int
SRCdev::resetup(sGENmodel *genmod, sCKT *ckt)
{
    sSRCmodel *model = static_cast<sSRCmodel*>(genmod);
    for ( ; model; model = model->next()) {
        sSRCinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {
            
            if (inst->SRCtype == SRC_V) {
                TSTALLOC(SRCposIbrptr, SRCposNode, SRCbranch)
                TSTALLOC(SRCnegIbrptr, SRCnegNode, SRCbranch)
                TSTALLOC(SRCibrNegptr, SRCbranch,  SRCnegNode)
                TSTALLOC(SRCibrPosptr, SRCbranch,  SRCposNode)

                // for pz analysis, an ac source is open, ac has precedence
                // over FUNC
                //
                if (inst->SRCacGiven) {
                    TSTALLOC(SRCibrIbrptr, SRCbranch,  SRCbranch)
                }

                if (inst->SRCdep == SRC_CC) {
                    TSTALLOC(SRCibrContBrptr, SRCbranch, SRCcontBranch)
                }
                else if (inst->SRCdep == SRC_VC) {
                    TSTALLOC(SRCibrContPosptr, SRCbranch, SRCcontPosNode)
                    TSTALLOC(SRCibrContNegptr, SRCbranch, SRCcontNegNode)
                }
            }
            else {
                if (inst->SRCdep == SRC_CC) {
                    TSTALLOC(SRCposContBrptr,SRCposNode, SRCcontBranch)
                    TSTALLOC(SRCnegContBrptr,SRCnegNode, SRCcontBranch)
                }
                else if (inst->SRCdep == SRC_VC) {
                    TSTALLOC(SRCposContPosptr, SRCposNode, SRCcontPosNode)
                    TSTALLOC(SRCposContNegptr, SRCposNode, SRCcontNegNode)
                    TSTALLOC(SRCnegContPosptr, SRCnegNode, SRCcontPosNode)
                    TSTALLOC(SRCnegContNegptr, SRCnegNode, SRCcontNegNode)
                }
            }
            if (!inst->SRCtree)
                continue;
            if (inst->SRCtree->num_vars() == 0)
                continue;

            // For each controlling variable set the entries
            // in the vector of the positions of the SMP

            for (int j = 0, i = 0; i < inst->SRCtree->num_vars(); i++) {
                if (inst->SRCtree->vars()[i].type == IF_INSTANCE) {
                    if (inst->SRCtype == SRC_V) {
                        TSTALLOC_1(SRCposptr[j++], SRCbranch,
                            inst->SRCeqns[i]);
                    }
                    else if (inst->SRCtype == SRC_I) {
                        TSTALLOC_1(SRCposptr[j++], SRCposNode,
                            inst->SRCeqns[i]);
                        TSTALLOC_1(SRCposptr[j++], SRCnegNode,
                            inst->SRCeqns[i]);
                    }
                }
                else if (inst->SRCtree->vars()[i].type == IF_NODE) {
                    if (inst->SRCtype == SRC_V) {
                        TSTALLOC_2(SRCposptr[j++], SRCbranch,
                            inst->SRCtree->vars()[i].v.nValue);
                    }
                    else if(inst->SRCtype == SRC_I) {
                        TSTALLOC_2(SRCposptr[j++], SRCposNode,
                            inst->SRCtree->vars()[i].v.nValue);
                        TSTALLOC_2(SRCposptr[j++], SRCnegNode,
                            inst->SRCtree->vars()[i].v.nValue);
                    }
                }
            }
        }
    }
    return (OK);
}


int
SRCdev::unsetup(sGENmodel *genmod, sCKT*)
{
    sSRCmodel *model = static_cast<sSRCmodel*>(genmod);
    for ( ; model; model = model->next()) {
        sSRCinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {
            inst->SRCbranch = 0;
        }
    }
    return (OK);
}
// End of ESRCdev functions.


sSRCinstance::~sSRCinstance()
{
    delete [] SRCacTabName;
    delete [] SRCvalues;
    delete [] SRCderivs;
    delete [] SRCeqns;
    delete [] SRCacValues;
    delete [] SRCposptr;
    delete [] SRCtimeResp;
    delete [] SRCfreqResp;
    delete [] SRCworkArea;
    delete SRCtree;
}


//
// FFT stuff
//

namespace {
    // FFT routine from "Numerical Recipies in C", W. H. Press, et al.

#define SWAP(a, b) temp=(a); (a)=(b); (b)=temp;

    void four1(double *data, int nn, int isign)
    {
        int n = nn << 1;
        int i, j = 1;
        for (i = 1; i < n; i += 2) {
            if (j > i) {
                double temp;
                SWAP(data[i-1], data[j-1]);
                SWAP(data[j], data[i]);
            }
            int m = n >> 1;
            while (m >= 2 && j > m) {
                j -= m;
                m >>= 1;
            }
            j += m;
        }
        int mmax = 2;
        while (n > mmax) {
            int istep = 2*mmax;
            double theta = 2*M_PI/(isign*mmax);
            double wtemp = sin(0.5*theta);
            double wpr = -2.0*wtemp*wtemp;
            double wpi = sin(theta);
            double wr = 1.0;
            double wi = 0.0;
            for (int m = 1; m < mmax; m += 2) {
                for (i = m; i <= n; i += istep) {
                    j = i + mmax;
                    double tempr = wr*data[j-1] - wi*data[j];
                    double tempi = wr*data[j] + wi*data[j-1];
                    data[j-1] = data[i-1] - tempr;
                    data[j] = data[i] - tempi;
                    data[i-1] += tempr;
                    data[i] += tempi;
                }
                wr = (wtemp = wr)*wpr - wi*wpi + wr;
                wi = wi*wpr + wtemp*wpi + wi;
            }
            mmax = istep;
        }
    }
}


// Set the impulse response from the ac table.  The impulse response is
// in our fft format.  Also allocate the convolution vector and do general
// setup.  Return false if error
//
bool
sSRCinstance::SRCgetFreqResp(sCKT *ckt)
{
    double fmax = 0.5/ckt->CKTstep;

    int len = (int)(ckt->CKTfinalTime/ckt->CKTstep) + 1;
    int j;
    for (j = 1; j < len; j <<= 1) ;
    SRCfreqRespLen = j;
    SRCfreqResp = new IFcomplex[SRCfreqRespLen];
    SRCdeltaFreq = 2*fmax/j;

    int i;
    int n = j/2;
    SRCfreqResp[0] = SRCacTab->tablEval(0);
    for (i = 1; i < n; i++) {
        IFcomplex c = SRCacTab->tablEval(SRCdeltaFreq*i);
        SRCfreqResp[i] = c;
        c.imag = -c.imag;
        SRCfreqResp[j - i] = c;
    }
    SRCfreqResp[n] = SRCacTab->tablEval(fmax);
    SRCtimeResp = new IFcomplex[SRCfreqRespLen];
    SRCworkArea = new IFcomplex[SRCfreqRespLen];
    return (true);
}


// Compute the source response at the present time.  The algorithm is:
// - Take the input as a delta-function sample at time t.
// - Compute the time-shifted response to this input by multiplying
//   the impulse response by exp(j*omega*t).
// - Perform inverse fft.
// - Interpolate the transform plus history at time t to get the response.
// This is not done here:
// - Add the result to the previous responses.
//
bool
sSRCinstance::SRCdo_fft(sCKT *ckt, double ain, double *aout)
{
    if (!SRCfreqResp) {
        if (!SRCacTabName)
            return (false);
        if (!SRCacTab && !IP.tablFind(SRCacTabName, &SRCacTab, ckt))
            return (false);
        if (ckt->CKTmode & (MODEDCOP | MODEDCTRANCURVE)) {
            // dc analysis, return dc value
            IFcomplex c = SRCacTab->tablEval(0);
            *aout = c.real*ain;
            return (true);
        }
        if (!SRCgetFreqResp(ckt))
            return (false);
    }

    // do multiply
    int j = SRCfreqRespLen;
    int n = j/2;
    double tfact = 2*M_PI*ckt->CKTtime*SRCdeltaFreq;

    ain *= ckt->CKTdeltaOld[0]/(j*ckt->CKTstep);
    IFcomplex *c = SRCworkArea;
    IFcomplex *d = SRCfreqResp;
    c[0].real = ain*d[0].real;
    c[0].imag = ain*d[0].imag;
    for (int i = 1; i <= n; i++) {
        double omega = i*tfact;
        IFcomplex tmp;
        tmp.real = cos(omega)*ain;
        tmp.imag = sin(omega)*ain;
        c[i].real = tmp.real*d[i].real - tmp.imag*d[i].imag;
        c[i].imag = tmp.imag*d[i].real + tmp.real*d[i].imag;
        if (i == n)
            break;
        tmp.imag = -tmp.imag;
        c[j - i].real = tmp.real*d[j - i].real - tmp.imag*d[j - i].imag;
        c[j - i].imag = tmp.imag*d[j - i].real + tmp.real*d[j - i].imag;
    }

    // compute ifft
    four1((double*)c, j, -1);

    // interpolate result
    double dt = ckt->CKTstep;
    n = (int)(ckt->CKTtime/dt);
    double tx = n*dt;
    d = SRCtimeResp + n;
    c += n;
    *aout = c->real + d->real +
        ((c+1)->real + (d+1)->real - c->real - d->real)*(ckt->CKTtime - tx)/dt;
    return (true);
}

