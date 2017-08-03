
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

/***  B4SOI 12/16/2010 Released by Tanvir Morshed   ***/

/**********
 * Copyright 2010 Regents of the University of California.  All rights reserved.
 * Authors: 1998 Samuel Fung, Dennis Sinitsky and Stephen Tang
 * Authors: 1999-2004 Pin Su, Hui Wan, Wei Jin, b3soicheck.c
 * Authors: 2005- Hui Wan, Xuemei Xi, Ali Niknejad, Chenming Hu.
 * Authors: 2009- Wenwei Yang, Chung-Hsun Lin, Ali Niknejad, Chenming Hu.
 * Authors: 2010- Tanvir Morshed, Ali Niknejad, Chenming Hu.
 * File: b4soicheck.c
 * Modified by Hui Wan, Xuemei Xi 11/30/2005
 * Modified by Wenwei Yang, Chung-Hsun Lin, Darsen Lu 03/06/2009
 * Modified by Tanvir Morshed 09/22/2009
 * Modified by Tanvir Morshed 12/31/2009
 * Modified by Tanvir Morshed 12/16/2010
 **********/

#include <stdio.h>
#include "b4sdefs.h"


#define B4SOImodName GENmodName

// SRW -- Don't create the log file, and redirect error messages to
// the application's error handling.

// Changed all fprintf(fplog,...) to ChkFprintf
#define ChkFprintf if(fplog) fprintf

// Changed printf to these
#define FatalPrintf(...) DVO.printModDev(model, here, &name_shown), \
    DVO.textOut(OUT_FATAL, __VA_ARGS__)
#define WarnPrintf(...) DVO.printModDev(model, here, &name_shown), \
    DVO.textOut(OUT_WARNING, __VA_ARGS__)


int
B4SOIdev::checkModel(sB4SOImodel *model, sB4SOIinstance *here, sCKT*)
{
    struct b4soiSizeDependParam *pParam;
    int Fatal_Flag = 0;
    FILE *fplog;

    bool name_shown = false;

    fplog = 0;
    if (true)
//    if ((fplog = fopen("b4soiv1check.log", "w")) != NULL)
    {
        pParam = here->pParam;
        ChkFprintf(fplog, "B4SOIV3 Parameter Check\n");
        ChkFprintf(fplog, "Model = %s\n", (char*)model->B4SOImodName);
        ChkFprintf(fplog, "W = %g, L = %g\n", here->B4SOIw, here->B4SOIl);

        if (pParam->B4SOIlpe0 < -pParam->B4SOIleff)
        {
            ChkFprintf(fplog, "Fatal: Lpe0 = %g is less than -Leff.\n",
                       pParam->B4SOIlpe0);
            FatalPrintf("Lpe0 = %g is less than -Leff.\n",
                        pParam->B4SOIlpe0);
            Fatal_Flag = 1;
        }

        if((here->B4SOIsa > 0.0) && (here->B4SOIsb > 0.0) &&
                ((here->B4SOInf == 1.0) || ((here->B4SOInf > 1.0) &&
                                            (here->B4SOIsd > 0.0))) )
        {
            if (model->B4SOIsaref <= 0.0)
            {
                ChkFprintf(fplog, "Fatal: SAref = %g is not positive.\n",
                           model->B4SOIsaref);
                FatalPrintf("SAref = %g is not positive.\n",
                            model->B4SOIsaref);
                Fatal_Flag = 1;
            }
            if (model->B4SOIsbref <= 0.0)
            {
                ChkFprintf(fplog, "Fatal: SBref = %g is not positive.\n",
                           model->B4SOIsbref);
                FatalPrintf("SBref = %g is not positive.\n",
                            model->B4SOIsbref);
                Fatal_Flag = 1;
            }
        }

        if (pParam->B4SOIlpeb < -pParam->B4SOIleff) /* v4.0 for Vth */
        {
            ChkFprintf(fplog, "Fatal: Lpeb = %g is less than -Leff.\n",
                       pParam->B4SOIlpeb);
            FatalPrintf("Lpeb = %g is less than -Leff.\n",
                        pParam->B4SOIlpeb);
            Fatal_Flag = 1;
        }

        if (pParam->B4SOIfprout < 0.0) /* v4.0 for DITS */
        {
            ChkFprintf(fplog, "Fatal: fprout = %g is negative.\n",
                       pParam->B4SOIfprout);
            FatalPrintf("fprout = %g is negative.\n", pParam->B4SOIfprout);
            Fatal_Flag = 1;
        }
        if (pParam->B4SOIpdits < 0.0)  /* v4.0 for DITS */
        {
            ChkFprintf(fplog, "Fatal: pdits = %g is negative.\n",
                       pParam->B4SOIpdits);
            FatalPrintf("pdits = %g is negative.\n", pParam->B4SOIpdits);
            Fatal_Flag = 1;
        }
        if (model->B4SOIpditsl < 0.0)  /* v4.0 for DITS */
        {
            ChkFprintf(fplog, "Fatal: pditsl = %g is negative.\n",
                       model->B4SOIpditsl);
            FatalPrintf("pditsl = %g is negative.\n", model->B4SOIpditsl);
            Fatal_Flag = 1;
        }

        if (model->B4SOItox <= 0.0)
        {
            ChkFprintf(fplog, "Fatal: Tox = %g is not positive.\n",
                       model->B4SOItox);
            FatalPrintf("Tox = %g is not positive.\n", model->B4SOItox);
            Fatal_Flag = 1;
        }
        if (model->B4SOIleffeot <= 0.0)
        {
            ChkFprintf(fplog, "Fatal: leffeot = %g is not positive.\n",
                       model->B4SOIleffeot);
            FatalPrintf("Leffeot = %g is not positive.\n", model->B4SOIleffeot);
            Fatal_Flag = 1;
        }
        if (model->B4SOIweffeot <= 0.0)
        {
            ChkFprintf(fplog, "Fatal: weffeot = %g is not positive.\n",
                       model->B4SOIweffeot);
            FatalPrintf("Weffeot = %g is not positive.\n", model->B4SOIweffeot);
            Fatal_Flag = 1;
        }
        if (model->B4SOItoxp <= 0.0)
        {
            ChkFprintf(fplog, "Fatal: Toxp = %g is not positive.\n",
                       model->B4SOItoxp);
            FatalPrintf("Toxp = %g is not positive.\n", model->B4SOItoxp);
            Fatal_Flag = 1;
        }
        if (model->B4SOIepsrgate < 0.0)
        {
            ChkFprintf(fplog, "Fatal: Epsrgate = %g is not positive.\n",
                       model->B4SOIepsrgate);
            FatalPrintf("Epsrgate = %g is not positive.\n", model->B4SOIepsrgate);
            Fatal_Flag = 1;
        }

        if (model->B4SOItoxm <= 0.0)
        {
            ChkFprintf(fplog, "Fatal: Toxm = %g is not positive.\n",
                       model->B4SOItoxm);
            FatalPrintf("Toxm = %g is not positive.\n", model->B4SOItoxm);
            Fatal_Flag = 1;
        } /* v3.2 */

        if (here->B4SOInf < 1.0)
        {
            ChkFprintf(fplog, "Fatal: Number of finger = %g is smaller than one.\n", here->B4SOInf);
            FatalPrintf("Number of finger = %g is smaller than one.\n", here->B4SOInf);
            Fatal_Flag = 1;
        }


        /* v2.2.3 */
        if (model->B4SOItox - model->B4SOIdtoxcv <= 0.0)
        {
            ChkFprintf(fplog, "Fatal: Tox - dtoxcv = %g is not positive.\n",
                       model->B4SOItox - model->B4SOIdtoxcv);
            FatalPrintf("Tox - dtoxcv = %g is not positive.\n", model->B4SOItox - model->B4SOIdtoxcv);
            Fatal_Flag = 1;
        }


        if (model->B4SOItbox <= 0.0)
        {
            ChkFprintf(fplog, "Fatal: Tbox = %g is not positive.\n",
                       model->B4SOItbox);
            FatalPrintf("Tbox = %g is not positive.\n", model->B4SOItbox);
            Fatal_Flag = 1;
        }

        if (pParam->B4SOInpeak <= 0.0)
        {
            ChkFprintf(fplog, "Fatal: Nch = %g is not positive.\n",
                       pParam->B4SOInpeak);
            FatalPrintf("Nch = %g is not positive.\n",
                        pParam->B4SOInpeak);
            Fatal_Flag = 1;
        }
        if (pParam->B4SOIngate < 0.0)
        {
            ChkFprintf(fplog, "Fatal: Ngate = %g is not positive.\n",
                       pParam->B4SOIngate);
            FatalPrintf("Ngate = %g Ngate is not positive.\n",
                        pParam->B4SOIngate);
            Fatal_Flag = 1;
        }
        if (pParam->B4SOIngate > 1.e25)
        {
            ChkFprintf(fplog, "Fatal: Ngate = %g is too high.\n",
                       pParam->B4SOIngate);
            FatalPrintf("Ngate = %g Ngate is too high\n",
                        pParam->B4SOIngate);
            Fatal_Flag = 1;
        }

        if (pParam->B4SOIdvt1 < 0.0)
        {
            ChkFprintf(fplog, "Fatal: Dvt1 = %g is negative.\n",
                       pParam->B4SOIdvt1);
            FatalPrintf("Dvt1 = %g is negative.\n", pParam->B4SOIdvt1);
            Fatal_Flag = 1;
        }

        if (pParam->B4SOIdvt1w < 0.0)
        {
            ChkFprintf(fplog, "Fatal: Dvt1w = %g is negative.\n",
                       pParam->B4SOIdvt1w);
            FatalPrintf("Dvt1w = %g is negative.\n", pParam->B4SOIdvt1w);
            Fatal_Flag = 1;
        }

        if (pParam->B4SOIw0 == -pParam->B4SOIweff)
        {
            ChkFprintf(fplog, "Fatal: (W0 + Weff) = 0 cauing divided-by-zero.\n");
            FatalPrintf("(W0 + Weff) = 0 cauing divided-by-zero.\n");
            Fatal_Flag = 1;
        }

        if (pParam->B4SOIdsub < 0.0)
        {
            ChkFprintf(fplog, "Fatal: Dsub = %g is negative.\n", pParam->B4SOIdsub);
            FatalPrintf("Dsub = %g is negative.\n", pParam->B4SOIdsub);
            Fatal_Flag = 1;
        }
        if (pParam->B4SOIb1 == -pParam->B4SOIweff)
        {
            ChkFprintf(fplog, "Fatal: (B1 + Weff) = 0 causing divided-by-zero.\n");
            FatalPrintf("(B1 + Weff) = 0 causing divided-by-zero.\n");
            Fatal_Flag = 1;
        }
        if (pParam->B4SOIu0temp <= 0.0)
        {
            ChkFprintf(fplog, "Fatal: u0 at current temperature = %g is not positive.\n", pParam->B4SOIu0temp);
            FatalPrintf("u0 at current temperature = %g is not positive.\n",
                        pParam->B4SOIu0temp);
            Fatal_Flag = 1;
        }

        /* Check delta parameter */
        if (pParam->B4SOIdelta < 0.0)
        {
            ChkFprintf(fplog, "Fatal: Delta = %g is less than zero.\n",
                       pParam->B4SOIdelta);
            FatalPrintf("Delta = %g is less than zero.\n", pParam->B4SOIdelta);
            Fatal_Flag = 1;
        }

        if (pParam->B4SOIvsattemp <= 0.0)
        {
            ChkFprintf(fplog, "Fatal: Vsat at current temperature = %g is not positive.\n", pParam->B4SOIvsattemp);
            FatalPrintf("Vsat at current temperature = %g is not positive.\n",
                        pParam->B4SOIvsattemp);
            Fatal_Flag = 1;
        }
        /* Check Rout parameters */
        if (pParam->B4SOIpclm <= 0.0)
        {
            ChkFprintf(fplog, "Fatal: Pclm = %g is not positive.\n", pParam->B4SOIpclm);
            FatalPrintf("Pclm = %g is not positive.\n", pParam->B4SOIpclm);
            Fatal_Flag = 1;
        }

        if (pParam->B4SOIdrout < 0.0)
        {
            ChkFprintf(fplog, "Fatal: Drout = %g is negative.\n", pParam->B4SOIdrout);
            FatalPrintf("Drout = %g is negative.\n", pParam->B4SOIdrout);
            Fatal_Flag = 1;
        }
        if ( model->B4SOIunitLengthGateSidewallJctCapD > 0.0)     /* v4.0 */
        {
            if (here->B4SOIdrainPerimeter < pParam->B4SOIweff)
            {
                ChkFprintf(fplog, "Warning: Pd = %g is less than W.\n",
                           here->B4SOIdrainPerimeter);
// SRW
                if (here->B4SOIdrainPerimeterGiven)
                    WarnPrintf("Pd = %g is less than W.\n",
                               here->B4SOIdrainPerimeter);
                here->B4SOIdrainPerimeter =pParam->B4SOIweff;
            }
        }
        if ( model->B4SOIunitLengthGateSidewallJctCapS > 0.0)     /* v4.0 */
        {
            if (here->B4SOIsourcePerimeter < pParam->B4SOIweff)
            {
                ChkFprintf(fplog, "Warning: Ps = %g is less than W.\n",
                           here->B4SOIsourcePerimeter);
// SRW
                if (here->B4SOIsourcePerimeterGiven)
                    WarnPrintf("Ps = %g is less than W.\n",
                               here->B4SOIsourcePerimeter);
                here->B4SOIsourcePerimeter =pParam->B4SOIweff;
            }
        }
        /* Check capacitance parameters */
        if (pParam->B4SOIclc < 0.0)
        {
            ChkFprintf(fplog, "Fatal: Clc = %g is negative.\n", pParam->B4SOIclc);
            FatalPrintf("Clc = %g is negative.\n", pParam->B4SOIclc);
            Fatal_Flag = 1;
        }


        /* v3.2 */
        if (pParam->B4SOInoff < 0.1)
        {
            ChkFprintf(fplog, "Warning: Noff = %g is too small.\n",
                       pParam->B4SOInoff);
            WarnPrintf("Noff = %g is too small.\n", pParam->B4SOInoff);
        }
        if (pParam->B4SOInoff > 4.0)
        {
            ChkFprintf(fplog, "Warning: Noff = %g is too large.\n",
                       pParam->B4SOInoff);
            WarnPrintf("Noff = %g is too large.\n", pParam->B4SOInoff);
        }

        /* added for stress */

        /* Check stress effect parameters */
        if( (here->B4SOIsa > 0.0) && (here->B4SOIsb > 0.0) &&
                ((here->B4SOInf == 1.0) || ((here->B4SOInf > 1.0) &&
                                            (here->B4SOIsd > 0.0))) )
        {
            if (model->B4SOIlodk2 <= 0.0)
            {
                ChkFprintf(fplog, "Warning: LODK2 = %g is not positive.\n",model->B4SOIlodk2);
                WarnPrintf("LODK2 = %g is not positive.\n",model->B4SOIlodk2);
            }
            if (model->B4SOIlodeta0 <= 0.0)
            {
                ChkFprintf(fplog, "Warning: LODETA0 = %g is not positive.\n",model->B4SOIlodeta0);
                WarnPrintf("LODETA0 = %g is not positive.\n",model->B4SOIlodeta0);
            }
        }

        /* added for stress end */

        /* v2.2.3 */
        if (pParam->B4SOImoin < 5.0)
        {
            ChkFprintf(fplog, "Warning: Moin = %g is too small.\n",
                       pParam->B4SOImoin);
            WarnPrintf("Moin = %g is too small.\n", pParam->B4SOImoin);
        }
        if (pParam->B4SOImoin > 25.0)
        {
            ChkFprintf(fplog, "Warning: Moin = %g is too large.\n",
                       pParam->B4SOImoin);
            WarnPrintf("Moin = %g is too large.\n", pParam->B4SOImoin);
        }

        /* v3.0 */
        if (model->B4SOImoinFD < 5.0)
        {
            ChkFprintf(fplog, "Warning: MoinFD = %g is too small.\n",
                       model->B4SOImoinFD);
            WarnPrintf("MoinFD = %g is too small.\n", model->B4SOImoinFD);
        }


        if (model->B4SOIcapMod == 3)
        {
            if (pParam->B4SOIacde < 0.1) /* v3.1.1 */
            {
                ChkFprintf (fplog, "Warning: Acde = %g is too small.\n",
                            pParam->B4SOIacde);
                WarnPrintf("Acde = %g is too small.\n",
                           pParam->B4SOIacde);
            }
            if (pParam->B4SOIacde > 1.6)
            {
                ChkFprintf (fplog, "Warning: Acde = %g is too large.\n",
                            pParam->B4SOIacde);
                WarnPrintf("Acde = %g is too large.\n",
                           pParam->B4SOIacde);
            }
        }

        /* v4.2 always perform Fatal checks */
        if (pParam->B4SOInigc <= 0.0)
        {
            ChkFprintf(fplog, "Fatal: nigc = %g is non-positive.\n",
                       pParam->B4SOInigc);
            FatalPrintf("nigc = %g is non-positive.\n", pParam->B4SOInigc);
            Fatal_Flag = 1;
        }
        if (pParam->B4SOIpoxedge <= 0.0)
        {
            ChkFprintf(fplog, "Fatal: poxedge = %g is non-positive.\n",
                       pParam->B4SOIpoxedge);
            FatalPrintf("poxedge = %g is non-positive.\n", pParam->B4SOIpoxedge);
            Fatal_Flag = 1;
        }
        if (pParam->B4SOIpigcd <= 0.0)
        {
            ChkFprintf(fplog, "Fatal: pigcd = %g is non-positive.\n",
                       pParam->B4SOIpigcd);
            FatalPrintf("pigcd = %g is non-positive.\n", pParam->B4SOIpigcd);
            Fatal_Flag = 1;
        }

        if (model->B4SOItoxref < 0.0)
        {
            ChkFprintf(fplog, "Warning: TOXREF = %g is negative.\n",
                       model->B4SOItoxref);
            WarnPrintf("Toxref = %g is negative.\n", model->B4SOItoxref);
            Fatal_Flag = 1;
        }

        if (model->B4SOItoxqm <= 0.0)
        {
            ChkFprintf(fplog, "Fatal: Toxqm = %g is not positive.\n",
                       model->B4SOItoxqm);
            FatalPrintf("Toxqm = %g is not positive.\n", model->B4SOItoxqm);
            Fatal_Flag = 1;
        }

        if (model->B4SOIdeltavox <= 0.0)
        {
            ChkFprintf(fplog, "Fatal: Deltavox = %g is not positive.\n",
                       model->B4SOIdeltavox);
            FatalPrintf("Deltavox = %g is not positive.\n", model->B4SOIdeltavox);
        }

        /* v4.4 Tanvir */
        if (pParam->B4SOIrdsw < 0.0)
        {
            ChkFprintf(fplog, "Warning: Rdsw = %g is negative. Set to zero.\n",
                       pParam->B4SOIrdsw);
            WarnPrintf("Warning: Rdsw = %g is negative. Set to zero.\n",
                       pParam->B4SOIrdsw);
            pParam->B4SOIrdsw = 0.0;
            pParam->B4SOIrds0 = 0.0;
        }
        else if (pParam->B4SOIrds0 < 0.001)
        {
            ChkFprintf(fplog, "Warning: Rds at current temperature = %g is less than 0.001 ohm. Set to zero.\n",
                       pParam->B4SOIrds0);
            WarnPrintf("Warning: Rds at current temperature = %g is less than 0.001 ohm. Set to zero.\n",
                       pParam->B4SOIrds0);
            pParam->B4SOIrds0 = 0.0;
        } /* v4.4 */
        if ((model->B4SOIcfrcoeff < 1.0)||(model->B4SOIcfrcoeff > 2.0))
        {
            ChkFprintf(fplog, "Warning: CfrCoeff = %g is out of range.\n",
                       model->B4SOIcfrcoeff);
            WarnPrintf("Warning: CfrCoeff = %g is outside valid range [1,2], set to 1.\n", model->B4SOIcfrcoeff);
            model->B4SOIcfrcoeff = 1;
        } /* v4.4 */
        if (model->B4SOIparamChk ==1)
        {
            /* Check L and W parameters */
            if (pParam->B4SOIleff <= 5.0e-8)
            {
                ChkFprintf(fplog, "Warning: Leff = %g may be too small.\n",
                           pParam->B4SOIleff);
                WarnPrintf("Leff = %g may be too small.\n",
                           pParam->B4SOIleff);
            }

            if (pParam->B4SOIleffCV <= 5.0e-8)
            {
                ChkFprintf(fplog, "Warning: Leff for CV = %g may be too small.\n",
                           pParam->B4SOIleffCV);
                WarnPrintf("Leff for CV = %g may be too small.\n",
                           pParam->B4SOIleffCV);
            }

            if (pParam->B4SOIweff <= 1.0e-7)
            {
                ChkFprintf(fplog, "Warning: Weff = %g may be too small.\n",
                           pParam->B4SOIweff);
                WarnPrintf("Weff = %g may be too small.\n",
                           pParam->B4SOIweff);
            }

            if (pParam->B4SOIweffCV <= 1.0e-7)
            {
                ChkFprintf(fplog, "Warning: Weff for CV = %g may be too small.\n",
                           pParam->B4SOIweffCV);
                WarnPrintf("Weff for CV = %g may be too small.\n",
                           pParam->B4SOIweffCV);
            }

            /* Check threshold voltage parameters */
            if (pParam->B4SOIlpe0 < 0.0)
            {
                ChkFprintf(fplog, "Warning: Lpe0 = %g is negative.\n", pParam->B4SOIlpe0);
                WarnPrintf("Lpe0 = %g is negative.\n", pParam->B4SOIlpe0);
            }
            if (model->B4SOItox < 1.0e-9)
            {
                ChkFprintf(fplog, "Warning: Tox = %g is less than 10A.\n",
                           model->B4SOItox);
                WarnPrintf("Tox = %g is less than 10A.\n", model->B4SOItox);
            }

            if (pParam->B4SOInpeak <= 1.0e15)
            {
                ChkFprintf(fplog, "Warning: Nch = %g may be too small.\n",
                           pParam->B4SOInpeak);
                WarnPrintf("Nch = %g may be too small.\n",
                           pParam->B4SOInpeak);
            }
            else if (pParam->B4SOInpeak >= 1.0e21)
            {
                ChkFprintf(fplog, "Warning: Nch = %g may be too large.\n",
                           pParam->B4SOInpeak);
                WarnPrintf("Nch = %g may be too large.\n",
                           pParam->B4SOInpeak);
            }

            if (fabs(pParam->B4SOInsub) >= 1.0e21)
            {
                ChkFprintf(fplog, "Warning: Nsub = %g may be too large.\n",
                           pParam->B4SOInsub);
                WarnPrintf("Nsub = %g may be too large.\n",
                           pParam->B4SOInsub);
            }

            if ((pParam->B4SOIngate > 0.0) &&
                    (pParam->B4SOIngate <= 1.e18))
            {
                ChkFprintf(fplog, "Warning: Ngate = %g is less than 1.E18cm^-3.\n",
                           pParam->B4SOIngate);
                WarnPrintf("Ngate = %g is less than 1.E18cm^-3.\n",
                           pParam->B4SOIngate);
            }

            if (pParam->B4SOIdvt0 < 0.0)
            {
                ChkFprintf(fplog, "Warning: Dvt0 = %g is negative.\n",
                           pParam->B4SOIdvt0);
                WarnPrintf("Dvt0 = %g is negative.\n", pParam->B4SOIdvt0);
            }

            if (fabs(1.0e-6 / (pParam->B4SOIw0 + pParam->B4SOIweff)) > 10.0)
            {
                ChkFprintf(fplog, "Warning: (W0 + Weff) may be too small.\n");
                WarnPrintf("(W0 + Weff) may be too small.\n");
            }
            /* Check Nsd, Ngate and Npeak parameters*/      /* Bug Fix # 22 Jul09*/
            if (model->B4SOInsd > 1.0e23)
            {
                ChkFprintf(fplog, "Warning: Nsd = %g is too large, should be specified in cm^-3.\n",
                           model->B4SOInsd);
                WarnPrintf("Nsd = %g is too large, should be specified in cm^-3.\n", model->B4SOInsd);
            }
            if (model->B4SOIngate > 1.0e23)
            {
                ChkFprintf(fplog, "Warning: Ngate = %g is too large, should be specified in cm^-3.\n",
                           model->B4SOIngate);
                WarnPrintf("Ngate = %g is too large, should be specified in cm^-3.\n", model->B4SOIngate);
            }
            if (model->B4SOInpeak > 1.0e20)
            {
                ChkFprintf(fplog, "Warning: Npeak = %g is too large, should be less than 1.0e20, specified in cm^-3.\n",
                           model->B4SOInpeak);
                WarnPrintf("Npeak = %g is too large, should be less than 1.0e20, specified in cm^-3.\n", model->B4SOInpeak);
            }
            /* Check subthreshold parameters */
            if (pParam->B4SOInfactor < 0.0)
            {
                ChkFprintf(fplog, "Warning: Nfactor = %g is negative.\n",
                           pParam->B4SOInfactor);
                WarnPrintf("Nfactor = %g is negative.\n", pParam->B4SOInfactor);
            }
            if (pParam->B4SOIcdsc < 0.0)
            {
                ChkFprintf(fplog, "Warning: Cdsc = %g is negative.\n",
                           pParam->B4SOIcdsc);
                WarnPrintf("Cdsc = %g is negative.\n", pParam->B4SOIcdsc);
            }
            if (pParam->B4SOIcdscd < 0.0)
            {
                ChkFprintf(fplog, "Warning: Cdscd = %g is negative.\n",
                           pParam->B4SOIcdscd);
                WarnPrintf("Cdscd = %g is negative.\n", pParam->B4SOIcdscd);
            }
            /* Check DIBL parameters */
            if (pParam->B4SOIeta0 < 0.0)
            {
                ChkFprintf(fplog, "Warning: Eta0 = %g is negative.\n",
                           pParam->B4SOIeta0);
                WarnPrintf("Eta0 = %g is negative.\n", pParam->B4SOIeta0);
            }

            /* Check Abulk parameters */
            if (fabs(1.0e-6 / (pParam->B4SOIb1 + pParam->B4SOIweff)) > 10.0)
            {
                ChkFprintf(fplog, "Warning: (B1 + Weff) may be too small.\n");
                WarnPrintf("(B1 + Weff) may be too small.\n");
            }

            /* Check Saturation parameters */
            if (pParam->B4SOIa2 < 0.01)
            {
                ChkFprintf(fplog, "Warning: A2 = %g is too small. Set to 0.01.\n", pParam->B4SOIa2);
                WarnPrintf("A2 = %g is too small. Set to 0.01.\n",
                           pParam->B4SOIa2);
                pParam->B4SOIa2 = 0.01;
            }
            else if (pParam->B4SOIa2 > 1.0)
            {
                ChkFprintf(fplog, "Warning: A2 = %g is larger than 1. A2 is set to 1 and A1 is set to 0.\n",
                           pParam->B4SOIa2);
                WarnPrintf("A2 = %g is larger than 1. A2 is set to 1 and A1 is set to 0.\n",
                           pParam->B4SOIa2);
                pParam->B4SOIa2 = 1.0;
                pParam->B4SOIa1 = 0.0;

            }
            /*
                    if (pParam->B4SOIrdsw < 0.0)
                    {   ChkFprintf(fplog, "Warning: Rdsw = %g is negative. Set to zero.\n",
                                pParam->B4SOIrdsw);
                        WarnPrintf("Rdsw = %g is negative. Set to zero.\n",
                               pParam->B4SOIrdsw);
                        pParam->B4SOIrdsw = 0.0;
                        pParam->B4SOIrds0 = 0.0;
                    }
                    else if (pParam->B4SOIrds0 < 0.001)
                    {   ChkFprintf(fplog, "Warning: Rds at current temperature = %g is less than 0.001 ohm. Set to zero.\n",
                                pParam->B4SOIrds0);
                        WarnPrintf("Rds at current temperature = %g is less than 0.001 ohm. Set to zero.\n",
                               pParam->B4SOIrds0);
                        pParam->B4SOIrds0 = 0.0;
            	} v4.4 */
            if (pParam->B4SOIvsattemp < 1.0e3)
            {
                ChkFprintf(fplog, "Warning: Vsat at current temperature = %g may be too small.\n", pParam->B4SOIvsattemp);
                WarnPrintf("Vsat at current temperature = %g may be too small.\n", pParam->B4SOIvsattemp);
            }

            if (pParam->B4SOIpdibl1 < 0.0)
            {
                ChkFprintf(fplog, "Warning: Pdibl1 = %g is negative.\n",
                           pParam->B4SOIpdibl1);
                WarnPrintf("Pdibl1 = %g is negative.\n", pParam->B4SOIpdibl1);
            }
            if (pParam->B4SOIpdibl2 < 0.0)
            {
                ChkFprintf(fplog, "Warning: Pdibl2 = %g is negative.\n",
                           pParam->B4SOIpdibl2);
                WarnPrintf("Pdibl2 = %g is negative.\n", pParam->B4SOIpdibl2);
            }
            /* Check overlap capacitance parameters */
            if (model->B4SOIcgdo < 0.0)
            {
                ChkFprintf(fplog, "Warning: cgdo = %g is negative. Set to zero.\n", model->B4SOIcgdo);
                WarnPrintf("cgdo = %g is negative. Set to zero.\n", model->B4SOIcgdo);
                model->B4SOIcgdo = 0.0;
            }
            if (model->B4SOIcgso < 0.0)
            {
                ChkFprintf(fplog, "Warning: cgso = %g is negative. Set to zero.\n", model->B4SOIcgso);
                WarnPrintf("cgso = %g is negative. Set to zero.\n", model->B4SOIcgso);
                model->B4SOIcgso = 0.0;
            }
            if (model->B4SOIcgeo < 0.0)
            {
                ChkFprintf(fplog, "Warning: cgeo = %g is negative. Set to zero.\n", model->B4SOIcgeo);
                WarnPrintf("cgeo = %g is negative. Set to zero.\n", model->B4SOIcgeo);
                model->B4SOIcgeo = 0.0;
            }

            if (model->B4SOIntun < 0.0)
            {
                ChkFprintf(fplog, "Warning: Ntuns = %g is negative.\n",
                           model->B4SOIntun);
                WarnPrintf("Ntuns = %g is negative.\n", model->B4SOIntun);
            }

            if (model->B4SOIntund < 0.0)
            {
                ChkFprintf(fplog, "Warning: Ntund = %g is negative.\n",
                           model->B4SOIntund);
                WarnPrintf("Ntund = %g is negative.\n", model->B4SOIntund);
            }

            if (model->B4SOIndiode < 0.0)
            {
                ChkFprintf(fplog, "Warning: Ndiode = %g is negative.\n",
                           model->B4SOIndiode);
                WarnPrintf("Ndiode = %g is negative.\n", model->B4SOIndiode);
            }

            if (model->B4SOIndioded < 0.0)
            {
                ChkFprintf(fplog, "Warning: Ndioded = %g is negative.\n",
                           model->B4SOIndioded);
                WarnPrintf("Ndioded = %g is negative.\n", model->B4SOIndioded);
            }

            if (model->B4SOIisbjt < 0.0)
            {
                ChkFprintf(fplog, "Warning: Isbjt = %g is negative.\n",
                           model->B4SOIisbjt);
                WarnPrintf("Isbjt = %g is negative.\n", model->B4SOIisbjt);
            }
            if (model->B4SOIidbjt < 0.0)
            {
                ChkFprintf(fplog, "Warning: Idbjt = %g is negative.\n",
                           model->B4SOIidbjt);
                WarnPrintf("Idbjt = %g is negative.\n", model->B4SOIidbjt);
            }

            if (model->B4SOIisdif < 0.0)
            {
                ChkFprintf(fplog, "Warning: Isdif = %g is negative.\n",
                           model->B4SOIisdif);
                WarnPrintf("Isdif = %g is negative.\n", model->B4SOIisdif);
            }
            if (model->B4SOIiddif < 0.0)
            {
                ChkFprintf(fplog, "Warning: Iddif = %g is negative.\n",
                           model->B4SOIiddif);
                WarnPrintf("Iddif = %g is negative.\n", model->B4SOIiddif);
            }

            if (model->B4SOIisrec < 0.0)
            {
                ChkFprintf(fplog, "Warning: Isrec = %g is negative.\n",
                           model->B4SOIisrec);
                WarnPrintf("Isrec = %g is negative.\n", model->B4SOIisrec);
            }
            if (model->B4SOIidrec < 0.0)
            {
                ChkFprintf(fplog, "Warning: Idrec = %g is negative.\n",
                           model->B4SOIidrec);
                WarnPrintf("Idrec = %g is negative.\n", model->B4SOIidrec);
            }

            if (model->B4SOIistun < 0.0)
            {
                ChkFprintf(fplog, "Warning: Istun = %g is negative.\n",
                           model->B4SOIistun);
                WarnPrintf("Istun = %g is negative.\n", model->B4SOIistun);
            }
            if (model->B4SOIidtun < 0.0)
            {
                ChkFprintf(fplog, "Warning: Idtun = %g is negative.\n",
                           model->B4SOIidtun);
                WarnPrintf("Idtun = %g is negative.\n", model->B4SOIidtun);
            }

            if (model->B4SOItt < 0.0)
            {
                ChkFprintf(fplog, "Warning: Tt = %g is negative.\n",
                           model->B4SOItt);
                WarnPrintf("Tt = %g is negative.\n", model->B4SOItt);
            }

            if (model->B4SOIcsdmin < 0.0)
            {
                ChkFprintf(fplog, "Warning: Csdmin = %g is negative.\n",
                           model->B4SOIcsdmin);
                WarnPrintf("Csdmin = %g is negative.\n", model->B4SOIcsdmin);
            }

            if (model->B4SOIcsdesw < 0.0)
            {
                ChkFprintf(fplog, "Warning: Csdesw = %g is negative.\n",
                           model->B4SOIcsdesw);
                WarnPrintf("Csdesw = %g is negative.\n", model->B4SOIcsdesw);
            }

            if (model->B4SOIasd < 0.0)
            {
                ChkFprintf(fplog, "Warning: Asd = %g should be within (0, 1).\n",
                           model->B4SOIasd);
                WarnPrintf("Asd = %g should be within (0, 1).\n", model->B4SOIasd);
            }

            if (model->B4SOIrth0 < 0.0)
            {
                ChkFprintf(fplog, "Warning: Rth0 = %g is negative.\n",
                           model->B4SOIrth0);
                WarnPrintf("Rth0 = %g is negative.\n", model->B4SOIrth0);
            }

            if (model->B4SOIcth0 < 0.0)
            {
                ChkFprintf(fplog, "Warning: Cth0 = %g is negative.\n",
                           model->B4SOIcth0);
                WarnPrintf("Cth0 = %g is negative.\n", model->B4SOIcth0);
            }

            if (model->B4SOIrbody < 0.0)
            {
                ChkFprintf(fplog, "Warning: Rbody = %g is negative.\n",
                           model->B4SOIrbody);
                WarnPrintf("Rbody = %g is negative.\n", model->B4SOIrbody);
            }

            if (model->B4SOIrbsh < 0.0)
            {
                ChkFprintf(fplog, "Warning: Rbsh = %g is negative.\n",
                           model->B4SOIrbsh);
                WarnPrintf("Rbsh = %g is negative.\n", model->B4SOIrbsh);
            }


            /* v2.2 release */
            if (model->B4SOIwth0 < 0.0)
            {
                ChkFprintf(fplog, "Warning: WTH0 = %g is negative.\n",
                           model->B4SOIwth0);
                WarnPrintf("Wth0 = %g is negative.\n", model->B4SOIwth0);
            }
            if (model->B4SOIrhalo < 0.0)
            {
                ChkFprintf(fplog, "Warning: RHALO = %g is negative.\n",
                           model->B4SOIrhalo);
                WarnPrintf("Rhalo = %g is negative.\n", model->B4SOIrhalo);
            }
            if (model->B4SOIntox < 0.0)
            {
                ChkFprintf(fplog, "Warning: NTOX = %g is negative.\n",
                           model->B4SOIntox);
                WarnPrintf("Ntox = %g is negative.\n", model->B4SOIntox);
            }
            if (model->B4SOIebg < 0.0)
            {
                ChkFprintf(fplog, "Warning: EBG = %g is negative.\n",
                           model->B4SOIebg);
                WarnPrintf("Ebg = %g is negative.\n", model->B4SOIebg);
            }
            if (model->B4SOIvevb < 0.0)
            {
                ChkFprintf(fplog, "Warning: VEVB = %g is negative.\n",
                           model->B4SOIvevb);
                WarnPrintf("Vevb = %g is negative.\n", model->B4SOIvevb);
            }
            if (pParam->B4SOIalphaGB1 < 0.0)
            {
                ChkFprintf(fplog, "Warning: ALPHAGB1 = %g is negative.\n",
                           pParam->B4SOIalphaGB1);
                WarnPrintf("AlphaGB1 = %g is negative.\n", pParam->B4SOIalphaGB1);
            }
            if (pParam->B4SOIbetaGB1 < 0.0)
            {
                ChkFprintf(fplog, "Warning: BETAGB1 = %g is negative.\n",
                           pParam->B4SOIbetaGB1);
                WarnPrintf("BetaGB1 = %g is negative.\n", pParam->B4SOIbetaGB1);
            }
            if (model->B4SOIvgb1 < 0.0)
            {
                ChkFprintf(fplog, "Warning: VGB1 = %g is negative.\n",
                           model->B4SOIvgb1);
                WarnPrintf("Vgb1 = %g is negative.\n", model->B4SOIvgb1);
            }
            if (model->B4SOIvecb < 0.0)
            {
                ChkFprintf(fplog, "Warning: VECB = %g is negative.\n",
                           model->B4SOIvecb);
                WarnPrintf("Vecb = %g is negative.\n", model->B4SOIvecb);
            }
            if (pParam->B4SOIalphaGB2 < 0.0)
            {
                ChkFprintf(fplog, "Warning: ALPHAGB2 = %g is negative.\n",
                           pParam->B4SOIalphaGB2);
                WarnPrintf("AlphaGB2 = %g is negative.\n", pParam->B4SOIalphaGB2);
            }
            if (pParam->B4SOIbetaGB2 < 0.0)
            {
                ChkFprintf(fplog, "Warning: BETAGB2 = %g is negative.\n",
                           pParam->B4SOIbetaGB2);
                WarnPrintf("BetaGB2 = %g is negative.\n", pParam->B4SOIbetaGB2);
            }
            if (model->B4SOIvgb2 < 0.0)
            {
                ChkFprintf(fplog, "Warning: VGB2 = %g is negative.\n",
                           model->B4SOIvgb2);
                WarnPrintf("Vgb2 = %g is negative.\n", model->B4SOIvgb2);
            }
            if (model->B4SOIvoxh < 0.0)
            {
                ChkFprintf(fplog, "Warning: Voxh = %g is negative.\n",
                           model->B4SOIvoxh);
                WarnPrintf("Voxh = %g is negative.\n", model->B4SOIvoxh);
            }

            /* v2.0 release */
            if (model->B4SOIk1w1 < 0.0)
            {
                ChkFprintf(fplog, "Warning: K1W1 = %g is negative.\n",
                           model->B4SOIk1w1);
                WarnPrintf("K1w1 = %g is negative.\n", model->B4SOIk1w1);
            }
            if (model->B4SOIk1w2 < 0.0)
            {
                ChkFprintf(fplog, "Warning: K1W2 = %g is negative.\n",
                           model->B4SOIk1w2);
                WarnPrintf("K1w2 = %g is negative.\n", model->B4SOIk1w2);
            }
            if (model->B4SOIketas < 0.0)
            {
                ChkFprintf(fplog, "Warning: KETAS = %g is negative.\n",
                           model->B4SOIketas);
                WarnPrintf("Ketas = %g is negative.\n", model->B4SOIketas);
            }
            if (model->B4SOIdwbc < 0.0)
            {
                ChkFprintf(fplog, "Warning: DWBC = %g is negative.\n",
                           model->B4SOIdwbc);
                WarnPrintf("Dwbc = %g is negative.\n", model->B4SOIdwbc);
            }
            if (model->B4SOIbeta0 < 0.0)
            {
                ChkFprintf(fplog, "Warning: BETA0 = %g is negative.\n",
                           model->B4SOIbeta0);
                WarnPrintf("Beta0 = %g is negative.\n", model->B4SOIbeta0);
            }
            if (model->B4SOIbeta1 < 0.0)
            {
                ChkFprintf(fplog, "Warning: BETA1 = %g is negative.\n",
                           model->B4SOIbeta1);
                WarnPrintf("Beta1 = %g is negative.\n", model->B4SOIbeta1);
            }
            if (model->B4SOIbeta2 < 0.0)
            {
                ChkFprintf(fplog, "Warning: BETA2 = %g is negative.\n",
                           model->B4SOIbeta2);
                WarnPrintf("Beta2 = %g is negative.\n", model->B4SOIbeta2);
            }
            if (model->B4SOItii < 0.0)
            {
                ChkFprintf(fplog, "Warning: TII = %g is negative.\n",
                           model->B4SOItii);
                WarnPrintf("Tii = %g is negative.\n", model->B4SOItii);
            }
            if (model->B4SOIlii < 0.0)
            {
                ChkFprintf(fplog, "Warning: LII = %g is negative.\n",
                           model->B4SOIlii);
                WarnPrintf("Lii = %g is negative.\n", model->B4SOIlii);
            }
            if (model->B4SOIsii1 < 0.0)
            {
                ChkFprintf(fplog, "Warning: SII1 = %g is negative.\n",
                           model->B4SOIsii1);
                WarnPrintf("Sii1 = %g is negative.\n", model->B4SOIsii1);
            }
            if (model->B4SOIsii2 < 0.0)
            {
                ChkFprintf(fplog, "Warning: SII2 = %g is negative.\n",
                           model->B4SOIsii2);
                WarnPrintf("Sii2 = %g is negative.\n", model->B4SOIsii1);
            }
            if (model->B4SOIsiid < 0.0)
            {
                ChkFprintf(fplog, "Warning: SIID = %g is negative.\n",
                           model->B4SOIsiid);
                WarnPrintf("Siid = %g is negative.\n", model->B4SOIsiid);
            }
            if (model->B4SOIfbjtii < 0.0)
            {
                ChkFprintf(fplog, "Warning: FBJTII = %g is negative.\n",
                           model->B4SOIfbjtii);
                WarnPrintf("fbjtii = %g is negative.\n", model->B4SOIfbjtii);
            }
            if (model->B4SOIvrec0 < 0.0)    /* v4.0 */
            {
                ChkFprintf(fplog, "Warning: VREC0S = %g is negative.\n",
                           model->B4SOIvrec0);
                WarnPrintf("Vrec0s = %g is negative.\n", model->B4SOIvrec0);
            }
            if (model->B4SOIvrec0d < 0.0)   /* v4.0 */
            {
                ChkFprintf(fplog, "Warning: VREC0D = %g is negative.\n",
                           model->B4SOIvrec0d);
                WarnPrintf("Vrec0d = %g is negative.\n", model->B4SOIvrec0d);
            }
            if (model->B4SOIvtun0 < 0.0)    /* v4.0 */
            {
                ChkFprintf(fplog, "Warning: VTUN0S = %g is negative.\n",
                           model->B4SOIvtun0);
                WarnPrintf("Vtun0s = %g is negative.\n", model->B4SOIvtun0);
            }
            if (model->B4SOIvtun0d < 0.0)   /* v4.0 */
            {
                ChkFprintf(fplog, "Warning: VTUN0D = %g is negative.\n",
                           model->B4SOIvtun0d);
                WarnPrintf("Vtun0d = %g is negative.\n", model->B4SOIvtun0d);
            }
            if (model->B4SOInbjt < 0.0)
            {
                ChkFprintf(fplog, "Warning: NBJT = %g is negative.\n",
                           model->B4SOInbjt);
                WarnPrintf("Nbjt = %g is negative.\n", model->B4SOInbjt);
            }
            if (model->B4SOIaely < 0.0)
            {
                ChkFprintf(fplog, "Warning: AELY = %g is negative.\n",
                           model->B4SOIaely);
                WarnPrintf("Aely = %g is negative.\n", model->B4SOIaely);
            }
            if (model->B4SOIahli < 0.0)
            {
                ChkFprintf(fplog, "Warning: AHLIS = %g is negative.\n",
                           model->B4SOIahli);
                WarnPrintf("Ahlis = %g is negative.\n", model->B4SOIahli);
            }
            if (model->B4SOIahlid < 0.0)
            {
                ChkFprintf(fplog, "Warning: AHLID = %g is negative.\n",
                           model->B4SOIahlid);
                WarnPrintf("Ahlid = %g is negative.\n", model->B4SOIahlid);
            }
            if (model->B4SOIrbody < 0.0)
            {
                ChkFprintf(fplog, "Warning: RBODY = %g is negative.\n",
                           model->B4SOIrbody);
                WarnPrintf("Rbody = %g is negative.\n", model->B4SOIrbody);
            }
            if (model->B4SOIrbsh < 0.0)
            {
                ChkFprintf(fplog, "Warning: RBSH = %g is negative.\n",
                           model->B4SOIrbsh);
                WarnPrintf("Rbsh = %g is negative.\n", model->B4SOIrbsh);
            }
            /*       if (pParam->B4SOIntrecf < 0.0)
                   {   ChkFprintf(fplog, "Warning: NTRECF = %g is negative.\n",
                               pParam->B4SOIntrecf);
                       WarnPrintf("Ntrecf = %g is negative.\n", pParam->B4SOIntrecf);
                   }
                   if (pParam->B4SOIntrecr < 0.0)
                   {   ChkFprintf(fplog, "Warning: NTRECR = %g is negative.\n",
                               pParam->B4SOIntrecr);
                       WarnPrintf("Ntrecr = %g is negative.\n", pParam->B4SOIntrecr);
                   } v4.2 bugfix: QA Test uses negative temp co-efficients*/

            /* v3.0 bug fix */
            /*
                    if (model->B4SOIndif < 0.0)
                    {   ChkFprintf(fplog, "Warning: NDIF = %g is negative.\n",
                                model->B4SOIndif);
                        WarnPrintf("Ndif = %g is negative.\n", model->B4SOIndif);
                    }
            */

            /*        if (model->B4SOItcjswg < 0.0)
                    {   ChkFprintf(fplog, "Warning: TCJSWGS = %g is negative.\n",
                                model->B4SOItcjswg);
                        WarnPrintf("Tcjswg = %g is negative.\n", model->B4SOItcjswg);
                   }
                    if (model->B4SOItpbswg < 0.0)
                    {   ChkFprintf(fplog, "Warning: TPBSWGS = %g is negative.\n",
                                model->B4SOItpbswg);
                        WarnPrintf("Tpbswg = %g is negative.\n", model->B4SOItpbswg);
                   }
                    if (model->B4SOItcjswgd < 0.0)
                    {   ChkFprintf(fplog, "Warning: TCJSWGD = %g is negative.\n",
                                model->B4SOItcjswgd);
                        WarnPrintf("Tcjswgd = %g is negative.\n", model->B4SOItcjswgd);
                   }
                    if (model->B4SOItpbswgd < 0.0)
                    {   ChkFprintf(fplog, "Warning: TPBSWGD = %g is negative.\n",
                                model->B4SOItpbswgd);
                        WarnPrintf("Tpbswgd = %g is negative.\n", model->B4SOItpbswgd);
                   }   v4.2 bugfix: QA Test uses negative temp co-efficients*/
            if ((model->B4SOIacde < 0.1) || (model->B4SOIacde > 1.6))
            {
                ChkFprintf(fplog, "Warning: ACDE = %g is out of range.\n",
                           model->B4SOIacde);
                WarnPrintf("Acde = %g is out of range.\n", model->B4SOIacde);
            }
            if ((model->B4SOImoin < 5.0)||(model->B4SOImoin > 25.0))
            {
                ChkFprintf(fplog, "Warning: MOIN = %g is out of range.\n",
                           model->B4SOImoin);
                WarnPrintf("Moin = %g is out of range.\n", model->B4SOImoin);
            }
            if (model->B4SOIdlbg < 0.0)
            {
                ChkFprintf(fplog, "Warning: DLBG = %g is negative.\n",
                           model->B4SOIdlbg);
                WarnPrintf("dlbg = %g is negative.\n", model->B4SOIdlbg);
            }


            if (model->B4SOIagidl < 0.0)
            {
                ChkFprintf(fplog, "Warning: AGIDL = %g is negative.\n",
                           model->B4SOIagidl);
                WarnPrintf("Agidl = %g is negative.\n", model->B4SOIagidl);
            }
            if (model->B4SOIbgidl < 0.0)
            {
                ChkFprintf(fplog, "Warning: BGIDL = %g is negative.\n",
                           model->B4SOIbgidl);
                WarnPrintf("Bgidl = %g is negative.\n", model->B4SOIbgidl);
            }
            if (fabs(model->B4SOIcgidl) < 1e-9)
            {
                ChkFprintf(fplog, "Warning: CGIDL = %g is smaller than 1e-9.\n",
                           model->B4SOIcgidl);
                WarnPrintf("Cgidl = %g is smaller than 1e-9.\n",
                           model->B4SOIcgidl);
            }
            if (model->B4SOIegidl < 0.0)
            {
                ChkFprintf(fplog, "Warning: EGIDL = %g is negative.\n",
                           model->B4SOIegidl);
                WarnPrintf("Egidl = %g is negative.\n", model->B4SOIegidl);
            }

            if (model->B4SOIagisl < 0.0)
            {
                ChkFprintf(fplog, "Warning: AGISL = %g is negative.\n",
                           model->B4SOIagisl);
                WarnPrintf("Agidl = %g is negative.\n", model->B4SOIagidl);
            }
            if (model->B4SOIbgisl < 0.0)
            {
                ChkFprintf(fplog, "Warning: BGISL = %g is negative.\n",
                           model->B4SOIbgisl);
                WarnPrintf("Bgisl = %g is negative.\n", model->B4SOIbgisl);
            }
            if (fabs(model->B4SOIcgisl) < 1e-9)
            {
                ChkFprintf(fplog, "Warning: CGISL = %g is smaller than 1e-9.\n",
                           model->B4SOIcgisl);
                WarnPrintf("Cgisl = %g is smaller than 1e-9.\n",
                           model->B4SOIcgisl);
            }
            if (model->B4SOIegisl < 0.0)
            {
                ChkFprintf(fplog, "Warning: EGISL = %g is negative.\n",
                           model->B4SOIegisl);
                WarnPrintf("Egisl = %g is negative.\n", model->B4SOIegisl);
            }

            if (model->B4SOIesatii < 0.0)
            {
                ChkFprintf(fplog, "Warning: Esatii = %g should be within positive.\n",
                           model->B4SOIesatii);
                WarnPrintf("Esatii = %g should be within (0, 1).\n", model->B4SOIesatii);
            }

            if (!model->B4SOIvgstcvModGiven)
            {
                ChkFprintf(fplog, "Warning: The default vgstcvMod is changed in v4.2 from '0' to '1'.\n");
                WarnPrintf("The default vgstcvMod is changed in v4.2 from '0' to '1'.\n");
            }
            if (pParam->B4SOIxj > model->B4SOItsi)
            {
                ChkFprintf(fplog, "Warning: Xj = %g is thicker than Tsi = %g.\n",
                           pParam->B4SOIxj, model->B4SOItsi);
                WarnPrintf("Xj = %g is thicker than Tsi = %g.\n",
                           pParam->B4SOIxj, model->B4SOItsi);
            }

            if (model->B4SOIcapMod < 2)
            {
                ChkFprintf(fplog, "Warning: capMod < 2 is not supported by BSIM3SOI.\n");
                WarnPrintf("capMod < 2 is not supported by BSIM3SOI.\n");
            }
            if (model->B4SOIcapMod > 3)
            {
                ChkFprintf(fplog, "Warning: capMod > 3 is not supported by BSIMSOI4.2.\n");
                WarnPrintf("capMod > 3 is not supported by BSIMSOI4.2.\n");
            }

        }/* loop for the parameter check for warning messages */
        // SRW
        if (fplog)
            fclose(fplog);
    }
    else
    {
        ChkFprintf(stderr, "Warning: Can't open log file. Parameter checking skipped.\n");
    }

    return(Fatal_Flag);
}

