
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

/***  B4SOI 11/30/2005 Xuemei (Jane) Xi Release   ***/

/**********
 * Copyright 2005 Regents of the University of California.  All rights reserved.
 * Authors: 1998 Samuel Fung, Dennis Sinitsky and Stephen Tang
 * Authors: 1999-2004 Pin Su, Hui Wan, Wei Jin, b3soitemp.c
 * Authors: 2005- Hui Wan, Xuemei Xi, Ali Niknejad, Chenming Hu.
 * File: b4soitemp.c
 * Modified by Hui Wan, Xuemei Xi 11/30/2005
 **********/

#include "b4sdefs.h"


// SRW - replace all printfs
#define fprintf(foo, ...) DVO.printModDev(model, here), \
    DVO.textOut(OUT_WARNING, __VA_ARGS__)
#define printf(...) DVO.printModDev(model, here), \
    DVO.textOut(OUT_WARNING, __VA_ARGS__)

#define B4SOInextModel      next()
#define B4SOInextInstance   next()
#define B4SOIinstances      inst()
#define CKTtemp CKTcurTask->TSKtemp
#define B4SOImodName GENmodName
#define B4SOIname GENname
#define B4SOIcheckModel checkModel


#define Kb 1.3806226e-23
#define KboQ 8.617087e-5  /* Kb / q  where q = 1.60219e-19 */
#define EPSOX 3.453133e-11
#define EPSSI 1.03594e-10
#define PI 3.141592654
#define Charge_q 1.60219e-19
#define Eg300 1.115   /*  energy gap at 300K  */

#define MAX_EXPL 2.688117142e+43
#define MIN_EXPL 3.720075976e-44
#define EXPL_THRESHOLD 100.0
#define DELTA  1.0E-9
#define DEXP(A,B) {                                                         \
        if (A > EXPL_THRESHOLD) {                                           \
            B = MAX_EXPL*(1.0+(A)-EXPL_THRESHOLD);                          \
        } else if (A < -EXPL_THRESHOLD)  {                                  \
            B = MIN_EXPL;                                                   \
        } else   {                                                          \
            B = exp(A);                                                     \
        }                                                                   \
    }


int
B4SOIdev::temperature(sGENmodel *genmod, sCKT *ckt)
{
    sB4SOImodel *model = static_cast<sB4SOImodel*>(genmod);
    sB4SOIinstance *here;

    struct b4soiSizeDependParam *pSizeDependParamKnot, *pLastKnot, *pParam;
    double tmp, tmp1, tmp2, Eg, Eg0, ni, T0, T1, T2, T3, T4, T5/*, T6*/;
    double /*Lnew, Wnew,*/ Ldrn, Wdrn;
    double Temp, TempRatio, Inv_L, Inv_W, Inv_LW, /*Dw, Dl,*/ Vtm0, Tnom/*, TRatio*/;
    double SDphi, SDgamma;
    double Inv_saref, Inv_sbref, Inv_sa, Inv_sb, rho, dvth0_lod;
    double W_tmp, Inv_ODeff, OD_offset, dk2_lod, deta0_lod, kvsat=0.0;
    int Size_Not_Found, i;
    double PowWeffWr, T10; /*v4.0 */



    /* v2.0 release */
    double tmp3, T7/*, T8, T9*/;


    /*  loop through all the B4SOI device models */
    for (; model != NULL; model = model->B4SOInextModel)
    {
        here = 0; // SRW
        Temp = ckt->CKTtemp;
        if (model->B4SOIGatesidewallJctSPotential < 0.1)       /* v4.0 */
            model->B4SOIGatesidewallJctSPotential = 0.1;
        if (model->B4SOIGatesidewallJctDPotential < 0.1)       /* v4.0 */
            model->B4SOIGatesidewallJctDPotential = 0.1;
        model->pSizeDependParamKnot = NULL;
        pLastKnot = NULL;

        Tnom = model->B4SOItnom;
        TempRatio = Temp / Tnom;

        model->B4SOIvcrit = CONSTvt0 * log(CONSTvt0 / (CONSTroot2 * 1.0e-14));
        model->B4SOIfactor1 = sqrt(EPSSI / EPSOX * model->B4SOItox);

        Vtm0 = KboQ * Tnom;
        Eg0 = 1.16 - 7.02e-4 * Tnom * Tnom / (Tnom + 1108.0);
        model->B4SOIeg0 = Eg0;
        model->B4SOIvtm = KboQ * Temp;

        Eg = 1.16 - 7.02e-4 * Temp * Temp / (Temp + 1108.0);
        /* ni is in cm^-3 */
        ni = 1.45e10 * (Temp / 300.15) * sqrt(Temp / 300.15)
             * exp(21.5565981 - Eg / (2.0 * model->B4SOIvtm));


        /* loop through all the instances of the model */
        /* MCJ: Length and Width not initialized */
        for (here = model->B4SOIinstances; here != NULL;
                here = here->B4SOInextInstance)
        {
            here->B4SOIrbodyext = here->B4SOIbodySquares *
                                  model->B4SOIrbsh;
            pSizeDependParamKnot = model->pSizeDependParamKnot;
            Size_Not_Found = 1;
            while ((pSizeDependParamKnot != NULL) && Size_Not_Found)
            {
                if ((here->B4SOIl == pSizeDependParamKnot->Length)
                        && (here->B4SOIw == pSizeDependParamKnot->Width)
                        && (here->B4SOIrth0 == pSizeDependParamKnot->Rth0)
                        && (here->B4SOIcth0 == pSizeDependParamKnot->Cth0)
                        && (here->B4SOInf == pSizeDependParamKnot->NFinger)) /*4.0*/
                {
                    Size_Not_Found = 0;
                    here->pParam = pSizeDependParamKnot;
                    pParam = here->pParam; /* v2.2.3 bug fix */
                }
                else
                {
                    pLastKnot = pSizeDependParamKnot;
                    pSizeDependParamKnot = pSizeDependParamKnot->pNext;
                }
            }

            if (Size_Not_Found)
            {
                pParam = (struct b4soiSizeDependParam *)malloc(
                             sizeof(struct b4soiSizeDependParam));
                if (pLastKnot == NULL)
                    model->pSizeDependParamKnot = pParam;
                else
                    pLastKnot->pNext = pParam;
                pParam->pNext = NULL;
                here->pParam = pParam;

                Ldrn = here->B4SOIl;
                Wdrn = here->B4SOIw / here->B4SOInf; /* v4.0 */
                pParam->Length = here->B4SOIl;
                pParam->Width = here->B4SOIw;
                pParam->NFinger = here->B4SOInf; /* v4.0 */
                pParam->Rth0 = here->B4SOIrth0;
                pParam->Cth0 = here->B4SOIcth0;

                T0 = pow(Ldrn, model->B4SOILln);
                T1 = pow(Wdrn, model->B4SOILwn);
                tmp1 = model->B4SOILl / T0 + model->B4SOILw / T1
                       + model->B4SOILwl / (T0 * T1);
                pParam->B4SOIdl = model->B4SOILint + tmp1;

                /* v2.2.3 */
                tmp1 = model->B4SOILlc / T0 + model->B4SOILwc / T1
                       + model->B4SOILwlc / (T0 * T1);
                pParam->B4SOIdlc = model->B4SOIdlc + tmp1;

                /* v3.0 */
                pParam->B4SOIdlcig = model->B4SOIdlcig + tmp1;


                T2 = pow(Ldrn, model->B4SOIWln);
                T3 = pow(Wdrn, model->B4SOIWwn);
                tmp2 = model->B4SOIWl / T2 + model->B4SOIWw / T3
                       + model->B4SOIWwl / (T2 * T3);
                pParam->B4SOIdw = model->B4SOIWint + tmp2;

                /* v2.2.3 */
                tmp2 = model->B4SOIWlc / T2 + model->B4SOIWwc / T3
                       + model->B4SOIWwlc / (T2 * T3);
                pParam->B4SOIdwc = model->B4SOIdwc + tmp2;


                pParam->B4SOIleff = here->B4SOIl - 2.0 * pParam->B4SOIdl;
                if (pParam->B4SOIleff <= 0.0)
                {
                    IFuid namarray[2];
                    namarray[0] = model->B4SOImodName;
                    namarray[1] = here->B4SOIname;
                    /* SRW
                                          (*(SPfrontEnd->IFerror))(ERR_FATAL,
                                          "B4SOI: mosfet %s, model %s: Effective channel length <= 0",
                                           namarray);
                    */
                    DVO.textOut(OUT_FATAL,
                             "B4SOI: mosfet %s, model %s: Effective channel length <= 0",
                             namarray[0], namarray[1]);
                    return(E_BADPARM);
                }

                pParam->B4SOIweff = here->B4SOIw / here->B4SOInf  /* v4.0 */
                                    - here->B4SOInbc * model->B4SOIdwbc
                                    - (2.0 - here->B4SOInbc) * pParam->B4SOIdw;
                if (pParam->B4SOIweff <= 0.0)
                {
                    IFuid namarray[2];
                    namarray[0] = model->B4SOImodName;
                    namarray[1] = here->B4SOIname;
                    /* SRW
                                          (*(SPfrontEnd->IFerror))(ERR_FATAL,
                                          "B4SOI: mosfet %s, model %s: Effective channel width <= 0",
                                           namarray);
                    */
                    DVO.textOut(OUT_FATAL,
                             "B4SOI: mosfet %s, model %s: Effective channel width <= 0",
                             namarray[0], namarray[1]);
                    return(E_BADPARM);
                }

                pParam->B4SOIwdiod = pParam->B4SOIweff / here->B4SOInseg
                                     + here->B4SOIpdbcp;
                pParam->B4SOIwdios = pParam->B4SOIweff / here->B4SOInseg
                                     + here->B4SOIpsbcp;

                pParam->B4SOIleffCV = here->B4SOIl - 2.0 * pParam->B4SOIdlc;
                if (pParam->B4SOIleffCV <= 0.0)
                {
                    IFuid namarray[2];
                    namarray[0] = model->B4SOImodName;
                    namarray[1] = here->B4SOIname;
                    /* SRW
                                          (*(SPfrontEnd->IFerror))(ERR_FATAL,
                                          "B4SOI: mosfet %s, model %s: Effective channel length for C-V <= 0",
                                           namarray);
                    */
                    DVO.textOut(OUT_FATAL,
                             "B4SOI: mosfet %s, model %s: Effective channel length for C-V <= 0",
                             namarray[0], namarray[1]);
                    return(E_BADPARM);
                }

                pParam->B4SOIweffCV = here->B4SOIw /here->B4SOInf /* v4.0 */
                                      - here->B4SOInbc * model->B4SOIdwbc
                                      - (2.0 - here->B4SOInbc) * pParam->B4SOIdwc;
                if (pParam->B4SOIweffCV <= 0.0)
                {
                    IFuid namarray[2];
                    namarray[0] = model->B4SOImodName;
                    namarray[1] = here->B4SOIname;
                    /* SRW
                                          (*(SPfrontEnd->IFerror))(ERR_FATAL,
                                          "B4SOI: mosfet %s, model %s: Effective channel width for C-V <= 0",
                                           namarray);
                    */
                    DVO.textOut(OUT_FATAL,
                             "B4SOI: mosfet %s, model %s: Effective channel width for C-V <= 0",
                             namarray[0], namarray[1]);
                    return(E_BADPARM);
                }

                pParam->B4SOIwdiodCV = pParam->B4SOIweffCV / here->B4SOInseg
                                       + here->B4SOIpdbcp;
                pParam->B4SOIwdiosCV = pParam->B4SOIweffCV / here->B4SOInseg
                                       + here->B4SOIpsbcp;

                pParam->B4SOIleffCVb = here->B4SOIl - 2.0 * pParam->B4SOIdlc
                                       - model->B4SOIdlcb;
                if (pParam->B4SOIleffCVb <= 0.0)
                {
                    IFuid namarray[2];
                    namarray[0] = model->B4SOImodName;
                    namarray[1] = here->B4SOIname;
                    /* SRW
                                         (*(SPfrontEnd->IFerror))(ERR_FATAL,
                                         "B4SOI: mosfet %s, model %s: Effective channel length for C-V (body) <= 0",
                                         namarray);
                    */
                    DVO.textOut(OUT_FATAL,
                             "B4SOI: mosfet %s, model %s: Effective channel length for C-V (body) <= 0",
                             namarray[0], namarray[1]);
                    return(E_BADPARM);
                }

                pParam->B4SOIleffCVbg = pParam->B4SOIleffCVb + 2 * model->B4SOIdlbg;
                if (pParam->B4SOIleffCVbg <= 0.0)
                {
                    IFuid namarray[2];
                    namarray[0] = model->B4SOImodName;
                    namarray[1] = here->B4SOIname;
                    /* SRW
                                         (*(SPfrontEnd->IFerror))(ERR_FATAL,
                                         "B4SOI: mosfet %s, model %s: Effective channel length for C-V (backgate) <= 0",
                                         namarray);
                    */
                    DVO.textOut(OUT_FATAL,
                             "B4SOI: mosfet %s, model %s: Effective channel length for C-V (backgate) <= 0",
                             namarray[0], namarray[1]);
                    return(E_BADPARM);
                }


                /* Not binned - START */
                pParam->B4SOIgamma1 = model->B4SOIgamma1;
                pParam->B4SOIgamma2 = model->B4SOIgamma2;
                pParam->B4SOIvbx = model->B4SOIvbx;
                pParam->B4SOIvbm = model->B4SOIvbm;
                pParam->B4SOIxt = model->B4SOIxt;
                /* Not binned - END */

                /* CV model */
                pParam->B4SOIcf = model->B4SOIcf;
                pParam->B4SOIclc = model->B4SOIclc;
                pParam->B4SOIcle = model->B4SOIcle;

                pParam->B4SOIabulkCVfactor = 1.0 + pow((pParam->B4SOIclc
                                                        / pParam->B4SOIleff),
                                                       pParam->B4SOIcle);

                /* Added for binning - START */
                if (model->B4SOIbinUnit == 1)
                {
                    Inv_L = 1.0e-6 / pParam->B4SOIleff;
                    Inv_W = 1.0e-6 / pParam->B4SOIweff;
                    Inv_LW = 1.0e-12 / (pParam->B4SOIleff
                                        * pParam->B4SOIweff);
                }
                else
                {
                    Inv_L = 1.0 / pParam->B4SOIleff;
                    Inv_W = 1.0 / pParam->B4SOIweff;
                    Inv_LW = 1.0 / (pParam->B4SOIleff
                                    * pParam->B4SOIweff);
                }
                pParam->B4SOInpeak = model->B4SOInpeak
                                     + model->B4SOIlnpeak * Inv_L
                                     + model->B4SOIwnpeak * Inv_W
                                     + model->B4SOIpnpeak * Inv_LW;
                pParam->B4SOInsub = model->B4SOInsub
                                    + model->B4SOIlnsub * Inv_L
                                    + model->B4SOIwnsub * Inv_W
                                    + model->B4SOIpnsub * Inv_LW;
                pParam->B4SOIngate = model->B4SOIngate
                                     + model->B4SOIlngate * Inv_L
                                     + model->B4SOIwngate * Inv_W
                                     + model->B4SOIpngate * Inv_LW;
                pParam->B4SOIvth0 = model->B4SOIvth0
                                    + model->B4SOIlvth0 * Inv_L
                                    + model->B4SOIwvth0 * Inv_W
                                    + model->B4SOIpvth0 * Inv_LW;
                pParam->B4SOIk1 = model->B4SOIk1
                                  + model->B4SOIlk1 * Inv_L
                                  + model->B4SOIwk1 * Inv_W
                                  + model->B4SOIpk1 * Inv_LW;
                pParam->B4SOIk2 = model->B4SOIk2
                                  + model->B4SOIlk2 * Inv_L
                                  + model->B4SOIwk2 * Inv_W
                                  + model->B4SOIpk2 * Inv_LW;
                pParam->B4SOIk1w1 = model->B4SOIk1w1
                                    + model->B4SOIlk1w1 * Inv_L
                                    + model->B4SOIwk1w1 * Inv_W
                                    + model->B4SOIpk1w1 * Inv_LW;
                pParam->B4SOIk1w2 = model->B4SOIk1w2
                                    + model->B4SOIlk1w2 * Inv_L
                                    + model->B4SOIwk1w2 * Inv_W
                                    + model->B4SOIpk1w2 * Inv_LW;
                pParam->B4SOIk3 = model->B4SOIk3
                                  + model->B4SOIlk3 * Inv_L
                                  + model->B4SOIwk3 * Inv_W
                                  + model->B4SOIpk3 * Inv_LW;
                pParam->B4SOIk3b = model->B4SOIk3b
                                   + model->B4SOIlk3b * Inv_L
                                   + model->B4SOIwk3b * Inv_W
                                   + model->B4SOIpk3b * Inv_LW;
                pParam->B4SOIkb1 = model->B4SOIkb1
                                   + model->B4SOIlkb1 * Inv_L
                                   + model->B4SOIwkb1 * Inv_W
                                   + model->B4SOIpkb1 * Inv_LW;
                pParam->B4SOIw0 = model->B4SOIw0
                                  + model->B4SOIlw0 * Inv_L
                                  + model->B4SOIww0 * Inv_W
                                  + model->B4SOIpw0 * Inv_LW;
                pParam->B4SOIlpe0 = model->B4SOIlpe0
                                    + model->B4SOIllpe0 * Inv_L
                                    + model->B4SOIwlpe0 * Inv_W
                                    + model->B4SOIplpe0 * Inv_LW;
                pParam->B4SOIlpeb = model->B4SOIlpeb
                                    + model->B4SOIllpeb * Inv_L
                                    + model->B4SOIwlpeb * Inv_W
                                    + model->B4SOIplpeb * Inv_LW; /* v4.0 */
                pParam->B4SOIdvt0 = model->B4SOIdvt0
                                    + model->B4SOIldvt0 * Inv_L
                                    + model->B4SOIwdvt0 * Inv_W
                                    + model->B4SOIpdvt0 * Inv_LW;
                pParam->B4SOIdvt1 = model->B4SOIdvt1
                                    + model->B4SOIldvt1 * Inv_L
                                    + model->B4SOIwdvt1 * Inv_W
                                    + model->B4SOIpdvt1 * Inv_LW;
                pParam->B4SOIdvt2 = model->B4SOIdvt2
                                    + model->B4SOIldvt2 * Inv_L
                                    + model->B4SOIwdvt2 * Inv_W
                                    + model->B4SOIpdvt2 * Inv_LW;
                pParam->B4SOIdvt0w = model->B4SOIdvt0w
                                     + model->B4SOIldvt0w * Inv_L
                                     + model->B4SOIwdvt0w * Inv_W
                                     + model->B4SOIpdvt0w * Inv_LW;
                pParam->B4SOIdvt1w = model->B4SOIdvt1w
                                     + model->B4SOIldvt1w * Inv_L
                                     + model->B4SOIwdvt1w * Inv_W
                                     + model->B4SOIpdvt1w * Inv_LW;
                pParam->B4SOIdvt2w = model->B4SOIdvt2w
                                     + model->B4SOIldvt2w * Inv_L
                                     + model->B4SOIwdvt2w * Inv_W
                                     + model->B4SOIpdvt2w * Inv_LW;
                pParam->B4SOIu0 = model->B4SOIu0
                                  + model->B4SOIlu0 * Inv_L
                                  + model->B4SOIwu0 * Inv_W
                                  + model->B4SOIpu0 * Inv_LW;
                pParam->B4SOIua = model->B4SOIua
                                  + model->B4SOIlua * Inv_L
                                  + model->B4SOIwua * Inv_W
                                  + model->B4SOIpua * Inv_LW;
                pParam->B4SOIub = model->B4SOIub
                                  + model->B4SOIlub * Inv_L
                                  + model->B4SOIwub * Inv_W
                                  + model->B4SOIpub * Inv_LW;
                pParam->B4SOIuc = model->B4SOIuc
                                  + model->B4SOIluc * Inv_L
                                  + model->B4SOIwuc * Inv_W
                                  + model->B4SOIpuc * Inv_LW;
                pParam->B4SOIvsat = model->B4SOIvsat
                                    + model->B4SOIlvsat * Inv_L
                                    + model->B4SOIwvsat * Inv_W
                                    + model->B4SOIpvsat * Inv_LW;
                pParam->B4SOIa0 = model->B4SOIa0
                                  + model->B4SOIla0 * Inv_L
                                  + model->B4SOIwa0 * Inv_W
                                  + model->B4SOIpa0 * Inv_LW;
                pParam->B4SOIags = model->B4SOIags
                                   + model->B4SOIlags * Inv_L
                                   + model->B4SOIwags * Inv_W
                                   + model->B4SOIpags * Inv_LW;
                pParam->B4SOIb0 = model->B4SOIb0
                                  + model->B4SOIlb0 * Inv_L
                                  + model->B4SOIwb0 * Inv_W
                                  + model->B4SOIpb0 * Inv_LW;
                pParam->B4SOIb1 = model->B4SOIb1
                                  + model->B4SOIlb1 * Inv_L
                                  + model->B4SOIwb1 * Inv_W
                                  + model->B4SOIpb1 * Inv_LW;
                pParam->B4SOIketa = model->B4SOIketa
                                    + model->B4SOIlketa * Inv_L
                                    + model->B4SOIwketa * Inv_W
                                    + model->B4SOIpketa * Inv_LW;
                pParam->B4SOIketas = model->B4SOIketas
                                     + model->B4SOIlketas * Inv_L
                                     + model->B4SOIwketas * Inv_W
                                     + model->B4SOIpketas * Inv_LW;
                pParam->B4SOIa1 = model->B4SOIa1
                                  + model->B4SOIla1 * Inv_L
                                  + model->B4SOIwa1 * Inv_W
                                  + model->B4SOIpa1 * Inv_LW;
                pParam->B4SOIa2 = model->B4SOIa2
                                  + model->B4SOIla2 * Inv_L
                                  + model->B4SOIwa2 * Inv_W
                                  + model->B4SOIpa2 * Inv_LW;
                pParam->B4SOIrdsw = model->B4SOIrdsw
                                    + model->B4SOIlrdsw * Inv_L
                                    + model->B4SOIwrdsw * Inv_W
                                    + model->B4SOIprdsw * Inv_LW;
                pParam->B4SOIrsw = model->B4SOIrsw    /* v4.0 */
                                   + model->B4SOIlrsw * Inv_L
                                   + model->B4SOIwrsw * Inv_W
                                   + model->B4SOIprsw * Inv_LW;
                pParam->B4SOIrdw = model->B4SOIrdw    /* v4.0 */
                                   + model->B4SOIlrdw * Inv_L
                                   + model->B4SOIwrdw * Inv_W
                                   + model->B4SOIprdw * Inv_LW;
                pParam->B4SOIprwb = model->B4SOIprwb
                                    + model->B4SOIlprwb * Inv_L
                                    + model->B4SOIwprwb * Inv_W
                                    + model->B4SOIpprwb * Inv_LW;
                pParam->B4SOIprwg = model->B4SOIprwg
                                    + model->B4SOIlprwg * Inv_L
                                    + model->B4SOIwprwg * Inv_W
                                    + model->B4SOIpprwg * Inv_LW;
                pParam->B4SOIwr = model->B4SOIwr
                                  + model->B4SOIlwr * Inv_L
                                  + model->B4SOIwwr * Inv_W
                                  + model->B4SOIpwr * Inv_LW;
                pParam->B4SOInfactor = model->B4SOInfactor
                                       + model->B4SOIlnfactor * Inv_L
                                       + model->B4SOIwnfactor * Inv_W
                                       + model->B4SOIpnfactor * Inv_LW;
                pParam->B4SOIdwg = model->B4SOIdwg
                                   + model->B4SOIldwg * Inv_L
                                   + model->B4SOIwdwg * Inv_W
                                   + model->B4SOIpdwg * Inv_LW;
                pParam->B4SOIdwb = model->B4SOIdwb
                                   + model->B4SOIldwb * Inv_L
                                   + model->B4SOIwdwb * Inv_W
                                   + model->B4SOIpdwb * Inv_LW;
                pParam->B4SOIvoff = model->B4SOIvoff
                                    + model->B4SOIlvoff * Inv_L
                                    + model->B4SOIwvoff * Inv_W
                                    + model->B4SOIpvoff * Inv_LW;
                pParam->B4SOIeta0 = model->B4SOIeta0
                                    + model->B4SOIleta0 * Inv_L
                                    + model->B4SOIweta0 * Inv_W
                                    + model->B4SOIpeta0 * Inv_LW;
                pParam->B4SOIetab = model->B4SOIetab
                                    + model->B4SOIletab * Inv_L
                                    + model->B4SOIwetab * Inv_W
                                    + model->B4SOIpetab * Inv_LW;
                pParam->B4SOIdsub = model->B4SOIdsub
                                    + model->B4SOIldsub * Inv_L
                                    + model->B4SOIwdsub * Inv_W
                                    + model->B4SOIpdsub * Inv_LW;
                pParam->B4SOIcit = model->B4SOIcit
                                   + model->B4SOIlcit * Inv_L
                                   + model->B4SOIwcit * Inv_W
                                   + model->B4SOIpcit * Inv_LW;
                pParam->B4SOIcdsc = model->B4SOIcdsc
                                    + model->B4SOIlcdsc * Inv_L
                                    + model->B4SOIwcdsc * Inv_W
                                    + model->B4SOIpcdsc * Inv_LW;
                pParam->B4SOIcdscb = model->B4SOIcdscb
                                     + model->B4SOIlcdscb * Inv_L
                                     + model->B4SOIwcdscb * Inv_W
                                     + model->B4SOIpcdscb * Inv_LW;
                pParam->B4SOIcdscd = model->B4SOIcdscd
                                     + model->B4SOIlcdscd * Inv_L
                                     + model->B4SOIwcdscd * Inv_W
                                     + model->B4SOIpcdscd * Inv_LW;
                pParam->B4SOIpclm = model->B4SOIpclm
                                    + model->B4SOIlpclm * Inv_L
                                    + model->B4SOIwpclm * Inv_W
                                    + model->B4SOIppclm * Inv_LW;
                pParam->B4SOIpdibl1 = model->B4SOIpdibl1
                                      + model->B4SOIlpdibl1 * Inv_L
                                      + model->B4SOIwpdibl1 * Inv_W
                                      + model->B4SOIppdibl1 * Inv_LW;
                pParam->B4SOIpdibl2 = model->B4SOIpdibl2
                                      + model->B4SOIlpdibl2 * Inv_L
                                      + model->B4SOIwpdibl2 * Inv_W
                                      + model->B4SOIppdibl2 * Inv_LW;
                pParam->B4SOIpdiblb = model->B4SOIpdiblb
                                      + model->B4SOIlpdiblb * Inv_L
                                      + model->B4SOIwpdiblb * Inv_W
                                      + model->B4SOIppdiblb * Inv_LW;
                pParam->B4SOIdrout = model->B4SOIdrout
                                     + model->B4SOIldrout * Inv_L
                                     + model->B4SOIwdrout * Inv_W
                                     + model->B4SOIpdrout * Inv_LW;
                pParam->B4SOIpvag = model->B4SOIpvag
                                    + model->B4SOIlpvag * Inv_L
                                    + model->B4SOIwpvag * Inv_W
                                    + model->B4SOIppvag * Inv_LW;
                pParam->B4SOIdelta = model->B4SOIdelta
                                     + model->B4SOIldelta * Inv_L
                                     + model->B4SOIwdelta * Inv_W
                                     + model->B4SOIpdelta * Inv_LW;
                pParam->B4SOIalpha0 = model->B4SOIalpha0
                                      + model->B4SOIlalpha0 * Inv_L
                                      + model->B4SOIwalpha0 * Inv_W
                                      + model->B4SOIpalpha0 * Inv_LW;
                pParam->B4SOIfbjtii = model->B4SOIfbjtii
                                      + model->B4SOIlfbjtii * Inv_L
                                      + model->B4SOIwfbjtii * Inv_W
                                      + model->B4SOIpfbjtii * Inv_LW;
                pParam->B4SOIbeta0 = model->B4SOIbeta0
                                     + model->B4SOIlbeta0 * Inv_L
                                     + model->B4SOIwbeta0 * Inv_W
                                     + model->B4SOIpbeta0 * Inv_LW;
                pParam->B4SOIbeta1 = model->B4SOIbeta1
                                     + model->B4SOIlbeta1 * Inv_L
                                     + model->B4SOIwbeta1 * Inv_W
                                     + model->B4SOIpbeta1 * Inv_LW;
                pParam->B4SOIbeta2 = model->B4SOIbeta2
                                     + model->B4SOIlbeta2 * Inv_L
                                     + model->B4SOIwbeta2 * Inv_W
                                     + model->B4SOIpbeta2 * Inv_LW;
                pParam->B4SOIvdsatii0 = model->B4SOIvdsatii0
                                        + model->B4SOIlvdsatii0 * Inv_L
                                        + model->B4SOIwvdsatii0 * Inv_W
                                        + model->B4SOIpvdsatii0 * Inv_LW;
                pParam->B4SOIlii = model->B4SOIlii
                                   + model->B4SOIllii * Inv_L
                                   + model->B4SOIwlii * Inv_W
                                   + model->B4SOIplii * Inv_LW;
                pParam->B4SOIesatii = model->B4SOIesatii
                                      + model->B4SOIlesatii * Inv_L
                                      + model->B4SOIwesatii * Inv_W
                                      + model->B4SOIpesatii * Inv_LW;
                pParam->B4SOIsii0 = model->B4SOIsii0
                                    + model->B4SOIlsii0 * Inv_L
                                    + model->B4SOIwsii0 * Inv_W
                                    + model->B4SOIpsii0 * Inv_LW;
                pParam->B4SOIsii1 = model->B4SOIsii1
                                    + model->B4SOIlsii1 * Inv_L
                                    + model->B4SOIwsii1 * Inv_W
                                    + model->B4SOIpsii1 * Inv_LW;
                pParam->B4SOIsii2 = model->B4SOIsii2
                                    + model->B4SOIlsii2 * Inv_L
                                    + model->B4SOIwsii2 * Inv_W
                                    + model->B4SOIpsii2 * Inv_LW;
                pParam->B4SOIsiid = model->B4SOIsiid
                                    + model->B4SOIlsiid * Inv_L
                                    + model->B4SOIwsiid * Inv_W
                                    + model->B4SOIpsiid * Inv_LW;
                pParam->B4SOIagidl = model->B4SOIagidl
                                     + model->B4SOIlagidl * Inv_L
                                     + model->B4SOIwagidl * Inv_W
                                     + model->B4SOIpagidl * Inv_LW;
                pParam->B4SOIbgidl = model->B4SOIbgidl
                                     + model->B4SOIlbgidl * Inv_L
                                     + model->B4SOIwbgidl * Inv_W
                                     + model->B4SOIpbgidl * Inv_LW;
                pParam->B4SOIcgidl = model->B4SOIcgidl
                                     + model->B4SOIlcgidl * Inv_L
                                     + model->B4SOIwcgidl * Inv_W
                                     + model->B4SOIpcgidl * Inv_LW;
                pParam->B4SOIegidl = model->B4SOIegidl
                                     + model->B4SOIlegidl * Inv_L
                                     + model->B4SOIwegidl * Inv_W
                                     + model->B4SOIpegidl * Inv_LW;
                pParam->B4SOIntun = model->B4SOIntun  /* v4.0 */
                                    + model->B4SOIlntun * Inv_L
                                    + model->B4SOIwntun * Inv_W
                                    + model->B4SOIpntun * Inv_LW;
                pParam->B4SOIntund = model->B4SOIntund        /* v4.0 */
                                     + model->B4SOIlntund * Inv_L
                                     + model->B4SOIwntund * Inv_W
                                     + model->B4SOIpntund * Inv_LW;
                pParam->B4SOIndiode = model->B4SOIndiode      /* v4.0 */
                                      + model->B4SOIlndiode * Inv_L
                                      + model->B4SOIwndiode * Inv_W
                                      + model->B4SOIpndiode * Inv_LW;
                pParam->B4SOIndioded = model->B4SOIndioded    /* v4.0 */
                                       + model->B4SOIlndioded * Inv_L
                                       + model->B4SOIwndioded * Inv_W
                                       + model->B4SOIpndioded * Inv_LW;
                pParam->B4SOInrecf0 = model->B4SOInrecf0      /* v4.0 */
                                      + model->B4SOIlnrecf0 * Inv_L
                                      + model->B4SOIwnrecf0 * Inv_W
                                      + model->B4SOIpnrecf0 * Inv_LW;
                pParam->B4SOInrecf0d = model->B4SOInrecf0d    /* v4.0 */
                                       + model->B4SOIlnrecf0d * Inv_L
                                       + model->B4SOIwnrecf0d * Inv_W
                                       + model->B4SOIpnrecf0d * Inv_LW;
                pParam->B4SOInrecr0 = model->B4SOInrecr0      /* v4.0 */
                                      + model->B4SOIlnrecr0 * Inv_L
                                      + model->B4SOIwnrecr0 * Inv_W
                                      + model->B4SOIpnrecr0 * Inv_LW;
                pParam->B4SOInrecr0d = model->B4SOInrecr0d    /* v4.0 */
                                       + model->B4SOIlnrecr0d * Inv_L
                                       + model->B4SOIwnrecr0d * Inv_W
                                       + model->B4SOIpnrecr0d * Inv_LW;
                pParam->B4SOIisbjt = model->B4SOIisbjt
                                     + model->B4SOIlisbjt * Inv_L
                                     + model->B4SOIwisbjt * Inv_W
                                     + model->B4SOIpisbjt * Inv_LW;
                pParam->B4SOIidbjt = model->B4SOIidbjt
                                     + model->B4SOIlidbjt * Inv_L
                                     + model->B4SOIwidbjt * Inv_W
                                     + model->B4SOIpidbjt * Inv_LW;
                pParam->B4SOIisdif = model->B4SOIisdif
                                     + model->B4SOIlisdif * Inv_L
                                     + model->B4SOIwisdif * Inv_W
                                     + model->B4SOIpisdif * Inv_LW;
                pParam->B4SOIiddif = model->B4SOIiddif
                                     + model->B4SOIliddif * Inv_L
                                     + model->B4SOIwiddif * Inv_W
                                     + model->B4SOIpiddif * Inv_LW;
                pParam->B4SOIisrec = model->B4SOIisrec
                                     + model->B4SOIlisrec * Inv_L
                                     + model->B4SOIwisrec * Inv_W
                                     + model->B4SOIpisrec * Inv_LW;
                pParam->B4SOIistun = model->B4SOIistun
                                     + model->B4SOIlistun * Inv_L
                                     + model->B4SOIwistun * Inv_W
                                     + model->B4SOIpistun * Inv_LW;
                pParam->B4SOIidrec = model->B4SOIidrec
                                     + model->B4SOIlidrec * Inv_L
                                     + model->B4SOIwidrec * Inv_W
                                     + model->B4SOIpidrec * Inv_LW;
                pParam->B4SOIidtun = model->B4SOIidtun
                                     + model->B4SOIlidtun * Inv_L
                                     + model->B4SOIwidtun * Inv_W
                                     + model->B4SOIpidtun * Inv_LW;
                pParam->B4SOIvrec0 = model->B4SOIvrec0        /* v4.0 */
                                     + model->B4SOIlvrec0 * Inv_L
                                     + model->B4SOIwvrec0 * Inv_W
                                     + model->B4SOIpvrec0 * Inv_LW;
                pParam->B4SOIvrec0d = model->B4SOIvrec0d      /* v4.0 */
                                      + model->B4SOIlvrec0d * Inv_L
                                      + model->B4SOIwvrec0d * Inv_W
                                      + model->B4SOIpvrec0d * Inv_LW;
                pParam->B4SOIvtun0 = model->B4SOIvtun0        /* v4.0 */
                                     + model->B4SOIlvtun0 * Inv_L
                                     + model->B4SOIwvtun0 * Inv_W
                                     + model->B4SOIpvtun0 * Inv_LW;
                pParam->B4SOIvtun0d = model->B4SOIvtun0d      /* v4.0 */
                                      + model->B4SOIlvtun0d * Inv_L
                                      + model->B4SOIwvtun0d * Inv_W
                                      + model->B4SOIpvtun0d * Inv_LW;
                pParam->B4SOInbjt = model->B4SOInbjt
                                    + model->B4SOIlnbjt * Inv_L
                                    + model->B4SOIwnbjt * Inv_W
                                    + model->B4SOIpnbjt * Inv_LW;
                pParam->B4SOIlbjt0 = model->B4SOIlbjt0
                                     + model->B4SOIllbjt0 * Inv_L
                                     + model->B4SOIwlbjt0 * Inv_W
                                     + model->B4SOIplbjt0 * Inv_LW;
                pParam->B4SOIvabjt = model->B4SOIvabjt
                                     + model->B4SOIlvabjt * Inv_L
                                     + model->B4SOIwvabjt * Inv_W
                                     + model->B4SOIpvabjt * Inv_LW;
                pParam->B4SOIaely = model->B4SOIaely
                                    + model->B4SOIlaely * Inv_L
                                    + model->B4SOIwaely * Inv_W
                                    + model->B4SOIpaely * Inv_LW;
                pParam->B4SOIahli = model->B4SOIahli  /* v4.0 */
                                    + model->B4SOIlahli * Inv_L
                                    + model->B4SOIwahli * Inv_W
                                    + model->B4SOIpahli * Inv_LW;
                pParam->B4SOIahlid = model->B4SOIahlid        /* v4.0 */
                                     + model->B4SOIlahlid * Inv_L
                                     + model->B4SOIwahlid * Inv_W
                                     + model->B4SOIpahlid * Inv_LW;


                /* v3.1 */
                pParam->B4SOIxj = model->B4SOIxj
                                  + model->B4SOIlxj * Inv_L
                                  + model->B4SOIwxj * Inv_W
                                  + model->B4SOIpxj * Inv_LW;
                pParam->B4SOIalphaGB1 = model->B4SOIalphaGB1
                                        + model->B4SOIlalphaGB1 * Inv_L
                                        + model->B4SOIwalphaGB1 * Inv_W
                                        + model->B4SOIpalphaGB1 * Inv_LW;
                pParam->B4SOIalphaGB2 = model->B4SOIalphaGB2
                                        + model->B4SOIlalphaGB2 * Inv_L
                                        + model->B4SOIwalphaGB2 * Inv_W
                                        + model->B4SOIpalphaGB2 * Inv_LW;
                pParam->B4SOIbetaGB1 = model->B4SOIbetaGB1
                                       + model->B4SOIlbetaGB1* Inv_L
                                       + model->B4SOIwbetaGB1 * Inv_W
                                       + model->B4SOIpbetaGB1 * Inv_LW;
                pParam->B4SOIbetaGB2 = model->B4SOIbetaGB2
                                       + model->B4SOIlbetaGB2 * Inv_L
                                       + model->B4SOIwbetaGB2 * Inv_W
                                       + model->B4SOIpbetaGB2 * Inv_LW;
                pParam->B4SOIndif = model->B4SOIndif
                                    + model->B4SOIlndif * Inv_L
                                    + model->B4SOIwndif * Inv_W
                                    + model->B4SOIpndif * Inv_LW;
                pParam->B4SOIntrecf = model->B4SOIntrecf
                                      + model->B4SOIlntrecf* Inv_L
                                      + model->B4SOIwntrecf * Inv_W
                                      + model->B4SOIpntrecf * Inv_LW;
                pParam->B4SOIntrecr = model->B4SOIntrecr
                                      + model->B4SOIlntrecr * Inv_L
                                      + model->B4SOIwntrecr * Inv_W
                                      + model->B4SOIpntrecr * Inv_LW;
                pParam->B4SOIxbjt = model->B4SOIxbjt
                                    + model->B4SOIlxbjt * Inv_L
                                    + model->B4SOIwxbjt * Inv_W
                                    + model->B4SOIpxbjt * Inv_LW;
                pParam->B4SOIxdif = model->B4SOIxdif
                                    + model->B4SOIlxdif* Inv_L
                                    + model->B4SOIwxdif * Inv_W
                                    + model->B4SOIpxdif * Inv_LW;
                pParam->B4SOIxrec = model->B4SOIxrec
                                    + model->B4SOIlxrec * Inv_L
                                    + model->B4SOIwxrec * Inv_W
                                    + model->B4SOIpxrec * Inv_LW;
                pParam->B4SOIxtun = model->B4SOIxtun
                                    + model->B4SOIlxtun * Inv_L
                                    + model->B4SOIwxtun * Inv_W
                                    + model->B4SOIpxtun * Inv_LW;
                pParam->B4SOIxdifd = model->B4SOIxdifd
                                     + model->B4SOIlxdifd* Inv_L
                                     + model->B4SOIwxdifd * Inv_W
                                     + model->B4SOIpxdifd * Inv_LW;
                pParam->B4SOIxrecd = model->B4SOIxrecd
                                     + model->B4SOIlxrecd * Inv_L
                                     + model->B4SOIwxrecd * Inv_W
                                     + model->B4SOIpxrecd * Inv_LW;
                pParam->B4SOIxtund = model->B4SOIxtund
                                     + model->B4SOIlxtund * Inv_L
                                     + model->B4SOIwxtund * Inv_W
                                     + model->B4SOIpxtund * Inv_LW;
                pParam->B4SOIcgdl = model->B4SOIcgdl
                                    + model->B4SOIlcgdl * Inv_L
                                    + model->B4SOIwcgdl * Inv_W
                                    + model->B4SOIpcgdl * Inv_LW;
                pParam->B4SOIcgsl = model->B4SOIcgsl
                                    + model->B4SOIlcgsl * Inv_L
                                    + model->B4SOIwcgsl * Inv_W
                                    + model->B4SOIpcgsl * Inv_LW;
                pParam->B4SOIckappa = model->B4SOIckappa
                                      + model->B4SOIlckappa * Inv_L
                                      + model->B4SOIwckappa * Inv_W
                                      + model->B4SOIpckappa * Inv_LW;
                pParam->B4SOIute = model->B4SOIute
                                   + model->B4SOIlute * Inv_L
                                   + model->B4SOIwute * Inv_W
                                   + model->B4SOIpute * Inv_LW;
                pParam->B4SOIkt1 = model->B4SOIkt1
                                   + model->B4SOIlkt1 * Inv_L
                                   + model->B4SOIwkt1 * Inv_W
                                   + model->B4SOIpkt1 * Inv_LW;
                pParam->B4SOIkt2 = model->B4SOIkt2
                                   + model->B4SOIlkt2 * Inv_L
                                   + model->B4SOIwkt2 * Inv_W
                                   + model->B4SOIpkt2 * Inv_LW;
                pParam->B4SOIkt1l = model->B4SOIkt1l
                                    + model->B4SOIlkt1l * Inv_L
                                    + model->B4SOIwkt1l * Inv_W
                                    + model->B4SOIpkt1l * Inv_LW;
                pParam->B4SOIua1 = model->B4SOIua1
                                   + model->B4SOIlua1 * Inv_L
                                   + model->B4SOIwua1 * Inv_W
                                   + model->B4SOIpua1 * Inv_LW;
                pParam->B4SOIub1 = model->B4SOIub1
                                   + model->B4SOIlub1* Inv_L
                                   + model->B4SOIwub1 * Inv_W
                                   + model->B4SOIpub1 * Inv_LW;
                pParam->B4SOIuc1 = model->B4SOIuc1
                                   + model->B4SOIluc1 * Inv_L
                                   + model->B4SOIwuc1 * Inv_W
                                   + model->B4SOIpuc1 * Inv_LW;
                pParam->B4SOIat = model->B4SOIat
                                  + model->B4SOIlat * Inv_L
                                  + model->B4SOIwat * Inv_W
                                  + model->B4SOIpat * Inv_LW;
                pParam->B4SOIprt = model->B4SOIprt
                                   + model->B4SOIlprt * Inv_L
                                   + model->B4SOIwprt * Inv_W
                                   + model->B4SOIpprt * Inv_LW;


                /* v3.0 */
                pParam->B4SOInigc = model->B4SOInigc
                                    + model->B4SOIlnigc * Inv_L
                                    + model->B4SOIwnigc * Inv_W
                                    + model->B4SOIpnigc * Inv_LW;
                pParam->B4SOIaigc = model->B4SOIaigc
                                    + model->B4SOIlaigc * Inv_L
                                    + model->B4SOIwaigc * Inv_W
                                    + model->B4SOIpaigc * Inv_LW;
                pParam->B4SOIbigc = model->B4SOIbigc
                                    + model->B4SOIlbigc * Inv_L
                                    + model->B4SOIwbigc * Inv_W
                                    + model->B4SOIpbigc * Inv_LW;
                pParam->B4SOIcigc = model->B4SOIcigc
                                    + model->B4SOIlcigc * Inv_L
                                    + model->B4SOIwcigc * Inv_W
                                    + model->B4SOIpcigc * Inv_LW;
                pParam->B4SOIaigsd = model->B4SOIaigsd
                                     + model->B4SOIlaigsd * Inv_L
                                     + model->B4SOIwaigsd * Inv_W
                                     + model->B4SOIpaigsd * Inv_LW;
                pParam->B4SOIbigsd = model->B4SOIbigsd
                                     + model->B4SOIlbigsd * Inv_L
                                     + model->B4SOIwbigsd * Inv_W
                                     + model->B4SOIpbigsd * Inv_LW;
                pParam->B4SOIcigsd = model->B4SOIcigsd
                                     + model->B4SOIlcigsd * Inv_L
                                     + model->B4SOIwcigsd * Inv_W
                                     + model->B4SOIpcigsd * Inv_LW;
                pParam->B4SOIpigcd = model->B4SOIpigcd
                                     + model->B4SOIlpigcd * Inv_L
                                     + model->B4SOIwpigcd * Inv_W
                                     + model->B4SOIppigcd * Inv_LW;
                pParam->B4SOIpoxedge = model->B4SOIpoxedge
                                       + model->B4SOIlpoxedge * Inv_L
                                       + model->B4SOIwpoxedge * Inv_W
                                       + model->B4SOIppoxedge * Inv_LW;
                /* v3.0 */

                /* v3.1 added for RF */
                pParam->B4SOIxrcrg1 = model->B4SOIxrcrg1
                                      + model->B4SOIlxrcrg1 * Inv_L
                                      + model->B4SOIwxrcrg1 * Inv_W
                                      + model->B4SOIpxrcrg1 * Inv_LW;
                pParam->B4SOIxrcrg2 = model->B4SOIxrcrg2
                                      + model->B4SOIlxrcrg2 * Inv_L
                                      + model->B4SOIwxrcrg2 * Inv_W
                                      + model->B4SOIpxrcrg2 * Inv_LW;
                /* v3.1 added for RF end */


                /* CV model */
                pParam->B4SOIvsdfb = model->B4SOIvsdfb
                                     + model->B4SOIlvsdfb * Inv_L
                                     + model->B4SOIwvsdfb * Inv_W
                                     + model->B4SOIpvsdfb * Inv_LW;
                pParam->B4SOIvsdth = model->B4SOIvsdth
                                     + model->B4SOIlvsdth * Inv_L
                                     + model->B4SOIwvsdth * Inv_W
                                     + model->B4SOIpvsdth * Inv_LW;
                pParam->B4SOIdelvt = model->B4SOIdelvt
                                     + model->B4SOIldelvt * Inv_L
                                     + model->B4SOIwdelvt * Inv_W
                                     + model->B4SOIpdelvt * Inv_LW;
                pParam->B4SOIacde = model->B4SOIacde
                                    + model->B4SOIlacde * Inv_L
                                    + model->B4SOIwacde * Inv_W
                                    + model->B4SOIpacde * Inv_LW;
                pParam->B4SOIacde = pParam->B4SOIacde *
                                    pow((pParam->B4SOInpeak / 2.0e16), -0.25);
                /* v3.2 bug fix */

                pParam->B4SOImoin = model->B4SOImoin
                                    + model->B4SOIlmoin * Inv_L
                                    + model->B4SOIwmoin * Inv_W
                                    + model->B4SOIpmoin * Inv_LW;
                pParam->B4SOInoff = model->B4SOInoff
                                    + model->B4SOIlnoff * Inv_L
                                    + model->B4SOIwnoff * Inv_W
                                    + model->B4SOIpnoff * Inv_LW; /* v3.2 */

                pParam->B4SOIdvtp0 = model->B4SOIdvtp0
                                     + model->B4SOIldvtp0 * Inv_L
                                     + model->B4SOIwdvtp0 * Inv_W
                                     + model->B4SOIpdvtp0 * Inv_LW; /* v4.0 */
                pParam->B4SOIdvtp1 = model->B4SOIdvtp1
                                     + model->B4SOIldvtp1 * Inv_L
                                     + model->B4SOIwdvtp1 * Inv_W
                                     + model->B4SOIpdvtp1 * Inv_LW; /* v4.0 */
                pParam->B4SOIminv = model->B4SOIminv
                                    + model->B4SOIlminv * Inv_L
                                    + model->B4SOIwminv * Inv_W
                                    + model->B4SOIpminv * Inv_LW; /* v4.0 */
                pParam->B4SOIfprout = model->B4SOIfprout
                                      + model->B4SOIlfprout * Inv_L
                                      + model->B4SOIwfprout * Inv_W
                                      + model->B4SOIpfprout * Inv_LW; /* v4.0 */
                pParam->B4SOIpdits = model->B4SOIpdits
                                     + model->B4SOIlpdits * Inv_L
                                     + model->B4SOIwpdits * Inv_W
                                     + model->B4SOIppdits * Inv_LW; /* v4.0 */
                pParam->B4SOIpditsd = model->B4SOIpditsd
                                      + model->B4SOIlpditsd * Inv_L
                                      + model->B4SOIwpditsd * Inv_W
                                      + model->B4SOIppditsd * Inv_LW; /* v4.0 */

                /* Added for binning - END */

                /* v4.0 add mstar for Vgsteff */
                pParam->B4SOImstar = 0.5 + atan(pParam->B4SOIminv) / PI;

                T0 = (TempRatio - 1.0);

                pParam->B4SOIuatemp = pParam->B4SOIua;  /*  save ua, ub, and uc for b4soild.c */
                pParam->B4SOIubtemp = pParam->B4SOIub;
                pParam->B4SOIuctemp = pParam->B4SOIuc;
                pParam->B4SOIrds0denom = pow(pParam->B4SOIweff * 1E6, pParam->B4SOIwr);


                /* v2.2 release */
                pParam->B4SOIrth = here->B4SOIrth0
                                   / (pParam->B4SOIweff + model->B4SOIwth0)
                                   * here->B4SOInseg;
                pParam->B4SOIcth = here->B4SOIcth0
                                   * (pParam->B4SOIweff + model->B4SOIwth0)
                                   / here->B4SOInseg;

                /* v2.2.2 adding layout-dependent Frbody multiplier */
                pParam->B4SOIrbody = here->B4SOIfrbody *model->B4SOIrbody
                                     * model->B4SOIrhalo
                                     / (2 * model->B4SOIrbody
                                        + model->B4SOIrhalo * pParam->B4SOIleff)
                                     * pParam->B4SOIweff / here->B4SOInseg
                                     / here->B4SOInf    /* v4.0 */;

                pParam->B4SOIoxideRatio = pow(model->B4SOItoxref
                                              /model->B4SOItoxqm, model->B4SOIntox)
                                          /model->B4SOItoxqm/model->B4SOItoxqm;
                /* v2.2 release */


                pParam->B4SOIua = pParam->B4SOIua + pParam->B4SOIua1 * T0;
                pParam->B4SOIub = pParam->B4SOIub + pParam->B4SOIub1 * T0;
                pParam->B4SOIuc = pParam->B4SOIuc + pParam->B4SOIuc1 * T0;
                if (pParam->B4SOIu0 > 1.0)
                    pParam->B4SOIu0 = pParam->B4SOIu0 / 1.0e4;

                pParam->B4SOIu0temp = pParam->B4SOIu0
                                      * pow(TempRatio, pParam->B4SOIute);
                pParam->B4SOIvsattemp = pParam->B4SOIvsat - pParam->B4SOIat
                                        * T0;
                pParam->B4SOIrds0 = (pParam->B4SOIrdsw
                                     + pParam->B4SOIprt * T0)
                                    / pow(pParam->B4SOIweff * 1E6,
                                          pParam->B4SOIwr);

                if(model->B4SOIrdsMod)   /* v4.0 */
                {
                    PowWeffWr = pParam->B4SOIrds0denom * here->B4SOInf;
                    T10 = pParam->B4SOIprt * T0;
                    /* External Rd(V) */
                    T1 = pParam->B4SOIrdw + T10;
                    T2 = model->B4SOIrdwmin + T10;
                    if (T1 < 0.0)
                    {
                        T1 = 0.0;
                        printf("Warning: Rdw at current temperature is negative; set to 0.\n");
                    }
                    if (T2 < 0.0)
                    {
                        T2 = 0.0;
                        printf("Warning: Rdwmin at current temperature is negative; set to 0.\n");
                    }

                    pParam->B4SOIrd0 = T1 / PowWeffWr;
                    pParam->B4SOIrdwmin = T2 / PowWeffWr;

                    /* External Rs(V) */
                    T3 = pParam->B4SOIrsw + T10;
                    T4 = model->B4SOIrswmin + T10;
                    if (T3 < 0.0)
                    {
                        T3 = 0.0;
                        printf("Warning: Rsw at current temperature is negative; set to 0.\n");
                    }
                    if (T4 < 0.0)
                    {
                        T4 = 0.0;
                        printf("Warning: Rswmin at current temperature is negative; set to 0.\n");
                    }
                    pParam->B4SOIrs0 = T3 / PowWeffWr;
                    pParam->B4SOIrswmin = T4 / PowWeffWr;
                }

                if (B4SOIcheckModel(model, here, ckt))
                {
                    IFuid namarray[2];
                    namarray[0] = model->B4SOImodName;
                    namarray[1] = here->B4SOIname;
                    /* SRW
                                          (*(SPfrontEnd->IFerror)) (ERR_FATAL,
                                          "Fatal error(s) detected during B4SOIV3 parameter checking for %s in model %s",
                                          namarray);
                    */
                    DVO.textOut(OUT_FATAL,
                             "Fatal error(s) detected during BSIMSOIV4 parameter checking for %s in model %s",
                             namarray[0], namarray[1]);
                    return(E_BADPARM);
                }


                pParam->B4SOIcgdo = (model->B4SOIcgdo + pParam->B4SOIcf)
                                    * pParam->B4SOIwdiodCV;
                pParam->B4SOIcgso = (model->B4SOIcgso + pParam->B4SOIcf)
                                    * pParam->B4SOIwdiosCV;

                pParam->B4SOIcgeo = model->B4SOIcgeo * pParam->B4SOIleffCV
                                    * here->B4SOInf;    /* v4.0 */


                if (!model->B4SOInpeakGiven && model->B4SOIgamma1Given)
                {
                    T0 = pParam->B4SOIgamma1 * model->B4SOIcox;
                    pParam->B4SOInpeak = 3.021E22 * T0 * T0;
                }


                T4 = Eg300 / model->B4SOIvtm * (TempRatio - 1.0);
                /* source side */
                T7 = pParam->B4SOIxbjt * T4 / pParam->B4SOIndiode;
                DEXP(T7, T0);
                T7 = pParam->B4SOIxdif * T4 / pParam->B4SOIndiode;
                DEXP(T7, T1);
                T7 = pParam->B4SOIxrec * T4 / pParam->B4SOInrecf0;
                DEXP(T7, T2);

                pParam->B4SOIahli0s = pParam->B4SOIahli * T0;
                pParam->B4SOIjbjts = pParam->B4SOIisbjt * T0;
                pParam->B4SOIjdifs = pParam->B4SOIisdif * T1;
                pParam->B4SOIjrecs = pParam->B4SOIisrec * T2;
                T7 = pParam->B4SOIxtun * (TempRatio - 1);
                DEXP(T7, T0);
                pParam->B4SOIjtuns = pParam->B4SOIistun * T0;

                /* drain side */
                pParam->B4SOIjtund = pParam->B4SOIidtun * T0;

                T7 = pParam->B4SOIxbjt * T4 / pParam->B4SOIndioded;
                DEXP(T7, T0);
                T7 = pParam->B4SOIxdifd * T4 / pParam->B4SOIndioded;
                DEXP(T7, T1);
                T7 = pParam->B4SOIxrecd * T4 / pParam->B4SOInrecf0d;
                DEXP(T7, T2);

                pParam->B4SOIahli0d = pParam->B4SOIahlid * T0;
                pParam->B4SOIjbjtd = pParam->B4SOIidbjt * T0;
                pParam->B4SOIjdifd = pParam->B4SOIiddif * T1;
                pParam->B4SOIjrecd = pParam->B4SOIidrec * T2;
                T7 = pParam->B4SOIxtund * (TempRatio - 1);
                DEXP(T7, T0);
                pParam->B4SOIjtund = pParam->B4SOIistun * T0;

                if (pParam->B4SOInsub > 0)
                    pParam->B4SOIvfbb = -model->B4SOItype * model->B4SOIvtm *
                                        log(pParam->B4SOInpeak/ pParam->B4SOInsub);
                else
                    pParam->B4SOIvfbb = -model->B4SOItype * model->B4SOIvtm *
                                        log(-pParam->B4SOInpeak* pParam->B4SOInsub/ni/ni);

                if (!model->B4SOIvsdfbGiven)
                {
                    if (pParam->B4SOInsub > 0)
                        pParam->B4SOIvsdfb = -model->B4SOItype *
                                             (model->B4SOIvtm*log(1e20 *
                                                     pParam->B4SOInsub / ni /ni) - 0.3);
                    else if (pParam->B4SOInsub < 0)
                        pParam->B4SOIvsdfb = -model->B4SOItype *
                                             (model->B4SOIvtm*log(-1e20 /
                                                     pParam->B4SOInsub) + 0.3);
                }

                /* Phi  & Gamma */
                SDphi = 2.0*model->B4SOIvtm*log(fabs(pParam->B4SOInsub) / ni);
                SDgamma = 5.753e-12 * sqrt(fabs(pParam->B4SOInsub))
                          / model->B4SOIcbox;

                if (!model->B4SOIvsdthGiven)
                {
                    if ( ((pParam->B4SOInsub > 0) && (model->B4SOItype > 0)) ||
                            ((pParam->B4SOInsub < 0) && (model->B4SOItype < 0)) )
                        pParam->B4SOIvsdth = pParam->B4SOIvsdfb + SDphi +
                                             SDgamma * sqrt(SDphi);
                    else
                        pParam->B4SOIvsdth = pParam->B4SOIvsdfb - SDphi -
                                             SDgamma * sqrt(SDphi);
                }

                if (!model->B4SOIcsdminGiven)
                {
                    /* Cdmin */
                    tmp = sqrt(2.0 * EPSSI * SDphi / (Charge_q *
                                                      FABS(pParam->B4SOInsub) * 1.0e6));
                    tmp1 = EPSSI / tmp;
                    model->B4SOIcsdmin = tmp1 * model->B4SOIcbox /
                                         (tmp1 + model->B4SOIcbox);
                }


                pParam->B4SOIphi = 2.0 * model->B4SOIvtm
                                   * log(pParam->B4SOInpeak / ni);

                pParam->B4SOIsqrtPhi = sqrt(pParam->B4SOIphi);
                pParam->B4SOIphis3 = pParam->B4SOIsqrtPhi * pParam->B4SOIphi;

                pParam->B4SOIXdep0 = sqrt(2.0 * EPSSI / (Charge_q
                                          * pParam->B4SOInpeak * 1.0e6))
                                     * pParam->B4SOIsqrtPhi;
                pParam->B4SOIsqrtXdep0 = sqrt(pParam->B4SOIXdep0);
                pParam->B4SOIlitl = sqrt(3.0 * pParam->B4SOIxj
                                         * model->B4SOItox);
                pParam->B4SOIvbi = model->B4SOIvtm * log(1.0e20
                                   * pParam->B4SOInpeak / (ni * ni));
                pParam->B4SOIcdep0 = sqrt(Charge_q * EPSSI
                                          * pParam->B4SOInpeak * 1.0e6 / 2.0
                                          / pParam->B4SOIphi);

                /* v3.0 */
                if (pParam->B4SOIngate > 0.0)
                {
                    pParam->B4SOIvfbsd = Vtm0 * log(pParam->B4SOIngate
                                                    / 1.0e20);
                }
                else
                    pParam->B4SOIvfbsd = 0.0;

                pParam->B4SOIToxRatio = exp(model->B4SOIntox
                                            * log(model->B4SOItoxref /model->B4SOItoxqm))
                                        /model->B4SOItoxqm /model->B4SOItoxqm;
                pParam->B4SOIToxRatioEdge = exp(model->B4SOIntox
                                                * log(model->B4SOItoxref
                                                      / (model->B4SOItoxqm * pParam->B4SOIpoxedge)))
                                            / model->B4SOItoxqm / model->B4SOItoxqm
                                            / pParam->B4SOIpoxedge / pParam->B4SOIpoxedge;
                pParam->B4SOIAechvb = (model->B4SOItype == NMOS) ? 4.97232e-7 : 3.42537e-7;
                pParam->B4SOIBechvb = (model->B4SOItype == NMOS) ? 7.45669e11 : 1.16645e12;
                pParam->B4SOIAechvbEdge = pParam->B4SOIAechvb * pParam->B4SOIweff/here->B4SOInseg
                                          * pParam->B4SOIdlcig * pParam->B4SOIToxRatioEdge; /* v3.1 bug fix */
                pParam->B4SOIBechvbEdge = -pParam->B4SOIBechvb
                                          * model->B4SOItoxqm * pParam->B4SOIpoxedge;
                pParam->B4SOIAechvb *= pParam->B4SOIweff/here->B4SOInseg
                                       * pParam->B4SOIleff
                                       * pParam->B4SOIToxRatio
                                       + here->B4SOIagbcpd; /* v4.0 */

                pParam->B4SOIBechvb *= -model->B4SOItoxqm;
                /* v3.0 */


                if (model->B4SOIk1Given || model->B4SOIk2Given)
                {
                    if (!model->B4SOIk1Given)
                    {
                        fprintf(stdout, "Warning: k1 should be specified with k2.\n");
                        pParam->B4SOIk1 = 0.53;
                    }
                    if (!model->B4SOIk2Given)
                    {
                        fprintf(stdout, "Warning: k2 should be specified with k1.\n");
                        pParam->B4SOIk2 = -0.0186;
                    }
                    if (model->B4SOIxtGiven)
                        fprintf(stdout, "Warning: xt is ignored because k1 or k2 is given.\n");
                    if (model->B4SOIvbxGiven)
                        fprintf(stdout, "Warning: vbx is ignored because k1 or k2 is given.\n");
                    if (model->B4SOIvbmGiven)
                        fprintf(stdout, "Warning: vbm is ignored because k1 or k2 is given.\n");
                    if (model->B4SOIgamma1Given)
                        fprintf(stdout, "Warning: gamma1 is ignored because k1 or k2 is given.\n");
                    if (model->B4SOIgamma2Given)
                        fprintf(stdout, "Warning: gamma2 is ignored because k1 or k2 is given.\n");
                }
                else
                {
                    if (!model->B4SOIvbxGiven)
                        pParam->B4SOIvbx = pParam->B4SOIphi - 7.7348e-4
                                           * pParam->B4SOInpeak
                                           * pParam->B4SOIxt * pParam->B4SOIxt;
                    if (pParam->B4SOIvbx > 0.0)
                        pParam->B4SOIvbx = -pParam->B4SOIvbx;
                    if (pParam->B4SOIvbm > 0.0)
                        pParam->B4SOIvbm = -pParam->B4SOIvbm;

                    if (!model->B4SOIgamma1Given)
                        pParam->B4SOIgamma1 = 5.753e-12
                                              * sqrt(pParam->B4SOInpeak)
                                              / model->B4SOIcox;
                    if (!model->B4SOIgamma2Given)
                        pParam->B4SOIgamma2 = 5.753e-12
                                              * sqrt(pParam->B4SOInsub)
                                              / model->B4SOIcox;

                    T0 = pParam->B4SOIgamma1 - pParam->B4SOIgamma2;
                    T1 = sqrt(pParam->B4SOIphi - pParam->B4SOIvbx)
                         - pParam->B4SOIsqrtPhi;
                    T2 = sqrt(pParam->B4SOIphi * (pParam->B4SOIphi
                                                  - pParam->B4SOIvbm)) - pParam->B4SOIphi;
                    pParam->B4SOIk2 = T0 * T1 / (2.0 * T2 + pParam->B4SOIvbm);
                    pParam->B4SOIk1 = pParam->B4SOIgamma2 - 2.0
                                      * pParam->B4SOIk2 * sqrt(pParam->B4SOIphi
                                              - pParam->B4SOIvbm);
                }

                if (pParam->B4SOIk2 < 0.0)
                {
                    T0 = 0.5 * pParam->B4SOIk1 / pParam->B4SOIk2;
                    pParam->B4SOIvbsc = 0.9 * (pParam->B4SOIphi - T0 * T0);
                    if (pParam->B4SOIvbsc > -3.0)
                        pParam->B4SOIvbsc = -3.0;
                    else if (pParam->B4SOIvbsc < -30.0)
                        pParam->B4SOIvbsc = -30.0;
                }
                else
                {
                    pParam->B4SOIvbsc = -30.0;
                }
                if (pParam->B4SOIvbsc > pParam->B4SOIvbm)
                    pParam->B4SOIvbsc = pParam->B4SOIvbm;

                if ((T0 = pParam->B4SOIweff + pParam->B4SOIk1w2) < 1e-8)
                    T0 = 1e-8;
                pParam->B4SOIk1eff = pParam->B4SOIk1 * (1 + pParam->B4SOIk1w1/T0);

                if (model->B4SOIvth0Given)
                {
                    pParam->B4SOIvfb = model->B4SOItype * pParam->B4SOIvth0
                                       - pParam->B4SOIphi - pParam->B4SOIk1eff
                                       * pParam->B4SOIsqrtPhi;
                }
                else
                {
                    pParam->B4SOIvfb = -1.0;
                    pParam->B4SOIvth0 = model->B4SOItype * (pParam->B4SOIvfb
                                                            + pParam->B4SOIphi + pParam->B4SOIk1eff
                                                            * pParam->B4SOIsqrtPhi);
                }

                /* v4.0 */
                pParam->B4SOIk1ox = pParam->B4SOIk1eff * model->B4SOItox
                                    / model->B4SOItoxm;

                T1 = sqrt(EPSSI / EPSOX * model->B4SOItox
                          * pParam->B4SOIXdep0);
                T0 = exp(-0.5 * pParam->B4SOIdsub * pParam->B4SOIleff / T1);
                pParam->B4SOItheta0vb0 = (T0 + 2.0 * T0 * T0);

                T0 = exp(-0.5 * pParam->B4SOIdrout * pParam->B4SOIleff / T1);
                T2 = (T0 + 2.0 * T0 * T0);
                pParam->B4SOIthetaRout = pParam->B4SOIpdibl1 * T2
                                         + pParam->B4SOIpdibl2;

                /* stress effect */
                T0 = pow(Ldrn, model->B4SOIllodku0);
                W_tmp = Wdrn + model->B4SOIwlod;
                T1 = pow(W_tmp, model->B4SOIwlodku0);
                tmp1 = model->B4SOIlku0 / T0 + model->B4SOIwku0 / T1
                       + model->B4SOIpku0 / (T0 * T1);
                pParam->B4SOIku0 = 1.0 + tmp1;

                T0 = pow(Ldrn, model->B4SOIllodvth);
                T1 = pow(W_tmp, model->B4SOIwlodvth);
                tmp1 = model->B4SOIlkvth0 / T0 + model->B4SOIwkvth0 / T1
                       + model->B4SOIpkvth0 / (T0 * T1);
                pParam->B4SOIkvth0 = 1.0 + tmp1;
                pParam->B4SOIkvth0 = sqrt( pParam->B4SOIkvth0
                                           * pParam->B4SOIkvth0 + DELTA);

                /*T0 = (TRatio - 1.0);*/
                T0 = (TempRatio - 1.0);  /* bug fix v4.1 */
                pParam->B4SOIku0temp = pParam->B4SOIku0 * (1.0
                                       + model->B4SOItku0 * T0) + DELTA;

                Inv_saref = 1.0 / (model->B4SOIsaref + 0.5 * Ldrn);
                Inv_sbref = 1.0 / (model->B4SOIsbref + 0.5 * Ldrn);
                pParam->B4SOIinv_od_ref = Inv_saref + Inv_sbref;
                pParam->B4SOIrho_ref = model->B4SOIku0 / pParam->B4SOIku0temp
                                       * pParam->B4SOIinv_od_ref;
                /* stress effect end */

            }

            here->B4SOIcsbox = model->B4SOIcbox*here->B4SOIsourceArea;
            here->B4SOIcsmin = model->B4SOIcsdmin*here->B4SOIsourceArea;
            here->B4SOIcdbox = model->B4SOIcbox*here->B4SOIdrainArea;
            here->B4SOIcdmin = model->B4SOIcsdmin*here->B4SOIdrainArea;

            if ( ((pParam->B4SOInsub > 0) && (model->B4SOItype > 0)) ||
                    ((pParam->B4SOInsub < 0) && (model->B4SOItype < 0)) )
            {
                T0 = pParam->B4SOIvsdth - pParam->B4SOIvsdfb;
                pParam->B4SOIsdt1 = pParam->B4SOIvsdfb + model->B4SOIasd * T0;
                T1 = here->B4SOIcsbox - here->B4SOIcsmin;
                T2 = T1 / T0 / T0;
                pParam->B4SOIst2 = T2 / model->B4SOIasd;
                pParam->B4SOIst3 = T2 /( 1 - model->B4SOIasd);
                here->B4SOIst4 =  T0 * T1 * (1 + model->B4SOIasd) / 3
                                  - here->B4SOIcsmin * pParam->B4SOIvsdfb;

                T1 = here->B4SOIcdbox - here->B4SOIcdmin;
                T2 = T1 / T0 / T0;
                pParam->B4SOIdt2 = T2 / model->B4SOIasd;
                pParam->B4SOIdt3 = T2 /( 1 - model->B4SOIasd);
                here->B4SOIdt4 =  T0 * T1 * (1 + model->B4SOIasd) / 3
                                  - here->B4SOIcdmin * pParam->B4SOIvsdfb;
            }
            else
            {
                T0 = pParam->B4SOIvsdfb - pParam->B4SOIvsdth;
                pParam->B4SOIsdt1 = pParam->B4SOIvsdth + model->B4SOIasd * T0;
                T1 = here->B4SOIcsmin - here->B4SOIcsbox;
                T2 = T1 / T0 / T0;
                pParam->B4SOIst2 = T2 / model->B4SOIasd;
                pParam->B4SOIst3 = T2 /( 1 - model->B4SOIasd);
                here->B4SOIst4 =  T0 * T1 * (1 + model->B4SOIasd) / 3
                                  - here->B4SOIcsbox * pParam->B4SOIvsdth;

                T1 = here->B4SOIcdmin - here->B4SOIcdbox;
                T2 = T1 / T0 / T0;
                pParam->B4SOIdt2 = T2 / model->B4SOIasd;
                pParam->B4SOIdt3 = T2 /( 1 - model->B4SOIasd);
                here->B4SOIdt4 =  T0 * T1 * (1 + model->B4SOIasd) / 3
                                  - here->B4SOIcdbox * pParam->B4SOIvsdth;
            }

            /* v2.2.2 bug fix */
            T0 = model->B4SOIcsdesw * log(1 + model->B4SOItsi /
                                          model->B4SOItbox);
            T1 = here->B4SOIsourcePerimeter - here->B4SOIw;
            if (T1 > 0.0)
                here->B4SOIcsesw = T0 * T1;
            else
                here->B4SOIcsesw = 0.0;
            T1 = here->B4SOIdrainPerimeter - here->B4SOIw;
            if (T1 > 0.0)
                here->B4SOIcdesw = T0 * T1;
            else
                here->B4SOIcdesw = 0.0;


            here->B4SOIphi = pParam->B4SOIphi;
            /* process source/drain series resistance */
            here->B4SOIdrainConductance = model->B4SOIsheetResistance
                                          * here->B4SOIdrainSquares;
            if (here->B4SOIdrainConductance > 0.0)
                here->B4SOIdrainConductance = 1.0
                                              / here->B4SOIdrainConductance;
            else
                here->B4SOIdrainConductance = 0.0;

            here->B4SOIsourceConductance = model->B4SOIsheetResistance
                                           * here->B4SOIsourceSquares;
            if (here->B4SOIsourceConductance > 0.0)
                here->B4SOIsourceConductance = 1.0
                                               / here->B4SOIsourceConductance;
            else
                here->B4SOIsourceConductance = 0.0;
            here->B4SOIcgso = pParam->B4SOIcgso;
            here->B4SOIcgdo = pParam->B4SOIcgdo;


            /* v2.0 release */
            if (model->B4SOIln < 1e-15) model->B4SOIln = 1e-15;
            T0 = -0.5 * pParam->B4SOIleff * pParam->B4SOIleff / model->B4SOIln / model->B4SOIln;
            DEXP(T0,T1);
            pParam->B4SOIarfabjt = T1;

            T0 = pParam->B4SOIlbjt0 * (1.0 / pParam->B4SOIleff + 1.0 / model->B4SOIln);
            pParam->B4SOIlratio = pow(T0,pParam->B4SOInbjt);
            pParam->B4SOIlratiodif = 1.0 + model->B4SOIldif0 * pow(T0,pParam->B4SOIndif);

            if ((pParam->B4SOIvearly = pParam->B4SOIvabjt + pParam->B4SOIaely * pParam->B4SOIleff) < 1)
                pParam->B4SOIvearly = 1;

            /* vfbzb calculation for capMod 3 */
            tmp = sqrt(pParam->B4SOIXdep0);
            tmp1 = pParam->B4SOIvbi - pParam->B4SOIphi;
            tmp2 = model->B4SOIfactor1 * tmp;

            T0 = -0.5 * pParam->B4SOIdvt1w * pParam->B4SOIweff
                 * pParam->B4SOIleff / tmp2;
            if (T0 > -EXPL_THRESHOLD)
            {
                T1 = exp(T0);
                T2 = T1 * (1.0 + 2.0 * T1);
            }
            else
            {
                T1 = MIN_EXPL;
                T2 = T1 * (1.0 + 2.0 * T1);
            }
            T0 = pParam->B4SOIdvt0w * T2;
            T2 = T0 * tmp1;

            T0 = -0.5 * pParam->B4SOIdvt1 * pParam->B4SOIleff / tmp2;
            if (T0 > -EXPL_THRESHOLD)
            {
                T1 = exp(T0);
                T3 = T1 * (1.0 + 2.0 * T1);
            }
            else
            {
                T1 = MIN_EXPL;
                T3 = T1 * (1.0 + 2.0 * T1);
            }
            T3 = pParam->B4SOIdvt0 * T3 * tmp1;

            /* v2.2.3 */
            T4 = (model->B4SOItox - model->B4SOIdtoxcv) * pParam->B4SOIphi
                 / (pParam->B4SOIweff + pParam->B4SOIw0);

            T0 = sqrt(1.0 + pParam->B4SOIlpe0 / pParam->B4SOIleff); /*v4.0*/
            T5 = pParam->B4SOIk1ox * (T0 - 1.0) * pParam->B4SOIsqrtPhi
                 + (pParam->B4SOIkt1 + pParam->B4SOIkt1l / pParam->B4SOIleff)
                 * (TempRatio - 1.0);   /* v4.0 */

            tmp3 = model->B4SOItype * pParam->B4SOIvth0
                   - T2 - T3 + pParam->B4SOIk3 * T4 + T5;
            pParam->B4SOIvfbzb = tmp3 - pParam->B4SOIphi - pParam->B4SOIk1
                                 * pParam->B4SOIsqrtPhi;
            /* End of vfbzb */


            /* v3.2 */
            pParam->B4SOIqsi = Charge_q * model->B4SOInpeak
                               * (1.0 + pParam->B4SOIlpe0 / pParam->B4SOIleff)
                               * 1e6 * model->B4SOItsi;


            /* v3.1 added for RF */
            here->B4SOIgrgeltd = model->B4SOIrshg * (model->B4SOIxgw
                                 + pParam->B4SOIweff / here->B4SOInseg
                                 / 3.0 / model->B4SOIngcon) /
                                 (model->B4SOIngcon * here->B4SOInf *
                                  (here->B4SOIl - model->B4SOIxgl));
            if (here->B4SOIgrgeltd > 0.0)
                here->B4SOIgrgeltd = 1.0 / here->B4SOIgrgeltd;
            else
            {
                here->B4SOIgrgeltd = 1.0e3; /* mho */
                if (here->B4SOIrgateMod !=0)
                    printf("Warning: The gate conductance reset to 1.0e3 mho.\n");
            }
            /* v3.1 added for RF end */

            /* v4.0 rbodyMod */
            if (here->B4SOIrbodyMod)
            {
                if (here->B4SOIrbdb < 1.0e-3)
                    here->B4SOIgrbdb = 1.0e3; /* in mho */
                else
                    here->B4SOIgrbdb = model->B4SOIgbmin
                                       + 1.0 / here->B4SOIrbdb;
                if (here->B4SOIrbsb < 1.0e-3)
                    here->B4SOIgrbsb = 1.0e3;
                else
                    here->B4SOIgrbsb = model->B4SOIgbmin
                                       + 1.0 / here->B4SOIrbsb;
            }
            /* v4.0 rbodyMod end */

            /*  v4.0 stress effect */
            if( (here->B4SOIsa > 0.0) && (here->B4SOIsb > 0.0) &&
                    ( (here->B4SOInf == 1.0) ||
                      ((here->B4SOInf > 1.0) && (here->B4SOIsd > 0.0))
                    )
              )
            {
                Inv_sa = 0;
                Inv_sb = 0;

                if (model->B4SOIwlod < 0.0)
                {
                    fprintf(stderr, "Warning: WLOD = %g is less than 0. Set to 0.0\n",model->B4SOIwlod);
                    model->B4SOIwlod = 0.0;
                }

                if (model->B4SOIkvsat < -1.0 )
                {
                    fprintf(stderr, "Warning: KVSAT = %g is too small; Reset to -1.0.\n",model->B4SOIkvsat);
                    here->B4SOIkvsat = kvsat = -1.0;
                }
                else if (model->B4SOIkvsat > 1.0)
                {
                    fprintf(stderr, "Warning: KVSAT = %g is too big; Reset to 1.0.\n",model->B4SOIkvsat);
                    here->B4SOIkvsat = kvsat = 1.0;
                }
                else here->B4SOIkvsat = model->B4SOIkvsat;

                for(i = 0; i < here->B4SOInf; i++)
                {
                    T0 = 1.0 / here->B4SOInf / (here->B4SOIsa
                                                + 0.5*Ldrn + i * (here->B4SOIsd +Ldrn));
                    T1 = 1.0 / here->B4SOInf / (here->B4SOIsb
                                                + 0.5*Ldrn + i * (here->B4SOIsd +Ldrn));
                    Inv_sa += T0;
                    Inv_sb += T1;
                }

                Inv_ODeff = Inv_sa + Inv_sb;
                here->B4SOIInv_ODeff = Inv_ODeff;
                rho = model->B4SOIku0 / pParam->B4SOIku0temp * Inv_ODeff;
                T0 = (1.0 + rho)/(1.0 + pParam->B4SOIrho_ref);
                here->B4SOIu0temp = pParam->B4SOIu0temp * T0;

                T1 = (1.0 + kvsat * rho)/(1.0 + kvsat * pParam->B4SOIrho_ref);
                here->B4SOIvsattemp = pParam->B4SOIvsattemp * T1;

                OD_offset = Inv_ODeff - pParam->B4SOIinv_od_ref;
                dvth0_lod = model->B4SOIkvth0
                            / pParam->B4SOIkvth0 * OD_offset;
                dk2_lod = model->B4SOIstk2
                          / pow(pParam->B4SOIkvth0, model->B4SOIlodk2)
                          * OD_offset;
                deta0_lod = model->B4SOIsteta0
                            / pow(pParam->B4SOIkvth0, model->B4SOIlodeta0)
                            * OD_offset;
                here->B4SOIvth0 = pParam->B4SOIvth0 + dvth0_lod;

                here->B4SOIk2 = pParam->B4SOIk2 + dk2_lod;

                here->B4SOIeta0 = pParam->B4SOIeta0 + deta0_lod;
            }
            else
            {
                here->B4SOIu0temp = pParam->B4SOIu0temp;
                here->B4SOIvth0 = pParam->B4SOIvth0;
                here->B4SOIvsattemp = pParam->B4SOIvsattemp;
                here->B4SOIk2 = pParam->B4SOIk2;
                here->B4SOIeta0 = pParam->B4SOIeta0;
                here->B4SOIInv_ODeff = 0;
            } /* v4.0 stress effect end */

            here->B4SOIk2ox = here->B4SOIk2 * model->B4SOItox
                              / model->B4SOItoxm;       /* v4.0 */
            here->B4SOIvth0 += here->B4SOIdelvto;     /* v4.0 */
            here->B4SOIvfb = pParam->B4SOIvfb + model->B4SOItype * here->B4SOIdelvto;
            here->B4SOIvfbzb = pParam->B4SOIvfbzb + model->B4SOItype * here->B4SOIdelvto;

            pParam->B4SOIldeb = sqrt(EPSSI * Vtm0 /
                                     (Charge_q * pParam->B4SOInpeak * 1.0e6)) / 3.0;
        }
    }
    return(OK);
}


