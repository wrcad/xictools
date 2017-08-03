
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
Author: 1998 Samuel Fung, Dennis Sinitsky and Stephen Tang
File: b3soicheck.c          98/5/01
Modified by Pin Su and Jan Feng        99/2/15
Modified by Pin Su 99/4/30
Modified by Pin Su 00/3/1
Modified by Pin Su and Hui Wan 02/3/5
Modified by Pin Su 02/5/20
Revision 3.0 2/5/20  Pin Su
BSIMSOI3.0 release
**********/

#include <stdio.h>
#include "b3sdefs.h"


#define B3SOImodName GENmodName

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
B3SOIdev::checkModel(sB3SOImodel *model, sB3SOIinstance *here, sCKT*)
{
    struct b3soiSizeDependParam *pParam;
    int Fatal_Flag = 0;
    FILE *fplog;

    bool name_shown = false;

    fplog = 0;
    if (true)
//    if ((fplog = fopen("b3soiv1check.log", "w")) != NULL)
    {
        pParam = here->pParam;
// SRW
        ChkFprintf(fplog, "BSIMSOI-3.0 Parameter Check\n");
        ChkFprintf(fplog, "Model = %s\n", (char*)model->B3SOImodName);
        ChkFprintf(fplog, "W = %g, L = %g\n", here->B3SOIw, here->B3SOIl);


        if (pParam->B3SOInlx < -pParam->B3SOIleff)
        {
            ChkFprintf(fplog, "Fatal: Nlx = %g is less than -Leff.\n",
                       pParam->B3SOInlx);
            FatalPrintf("Fatal: Nlx = %g is less than -Leff.\n",
                        pParam->B3SOInlx);
            Fatal_Flag = 1;
        }

        if (model->B3SOItox <= 0.0)
        {
            ChkFprintf(fplog, "Fatal: Tox = %g is not positive.\n",
                       model->B3SOItox);
            FatalPrintf("Fatal: Tox = %g is not positive.\n", model->B3SOItox);
            Fatal_Flag = 1;
        }

        /* v2.2.3 */
        if (model->B3SOItox - model->B3SOIdtoxcv <= 0.0)
        {
            ChkFprintf(fplog, "Fatal: Tox - dtoxcv = %g is not positive.\n",
                       model->B3SOItox - model->B3SOIdtoxcv);
            FatalPrintf("Fatal: Tox - dtoxcv = %g is not positive.\n", model->B3SOItox - model->B3SOIdtoxcv);
            Fatal_Flag = 1;
        }


        if (model->B3SOItbox <= 0.0)
        {
            ChkFprintf(fplog, "Fatal: Tbox = %g is not positive.\n",
                       model->B3SOItbox);
            FatalPrintf("Fatal: Tbox = %g is not positive.\n", model->B3SOItbox);
            Fatal_Flag = 1;
        }

        if (pParam->B3SOInpeak <= 0.0)
        {
            ChkFprintf(fplog, "Fatal: Nch = %g is not positive.\n",
                       pParam->B3SOInpeak);
            FatalPrintf("Fatal: Nch = %g is not positive.\n",
                        pParam->B3SOInpeak);
            Fatal_Flag = 1;
        }
        if (pParam->B3SOIngate < 0.0)
        {
            ChkFprintf(fplog, "Fatal: Ngate = %g is not positive.\n",
                       pParam->B3SOIngate);
            FatalPrintf("Fatal: Ngate = %g Ngate is not positive.\n",
                        pParam->B3SOIngate);
            Fatal_Flag = 1;
        }
        if (pParam->B3SOIngate > 1.e25)
        {
            ChkFprintf(fplog, "Fatal: Ngate = %g is too high.\n",
                       pParam->B3SOIngate);
            FatalPrintf("Fatal: Ngate = %g Ngate is too high\n",
                        pParam->B3SOIngate);
            Fatal_Flag = 1;
        }

        if (pParam->B3SOIdvt1 < 0.0)
        {
            ChkFprintf(fplog, "Fatal: Dvt1 = %g is negative.\n",
                       pParam->B3SOIdvt1);
            FatalPrintf("Fatal: Dvt1 = %g is negative.\n", pParam->B3SOIdvt1);
            Fatal_Flag = 1;
        }

        if (pParam->B3SOIdvt1w < 0.0)
        {
            ChkFprintf(fplog, "Fatal: Dvt1w = %g is negative.\n",
                       pParam->B3SOIdvt1w);
            FatalPrintf("Fatal: Dvt1w = %g is negative.\n", pParam->B3SOIdvt1w);
            Fatal_Flag = 1;
        }

        if (pParam->B3SOIw0 == -pParam->B3SOIweff)
        {
            ChkFprintf(fplog, "Fatal: (W0 + Weff) = 0 cauing divided-by-zero.\n");
            FatalPrintf("Fatal: (W0 + Weff) = 0 cauing divided-by-zero.\n");
            Fatal_Flag = 1;
        }

        if (pParam->B3SOIdsub < 0.0)
        {
            ChkFprintf(fplog, "Fatal: Dsub = %g is negative.\n", pParam->B3SOIdsub);
            FatalPrintf("Fatal: Dsub = %g is negative.\n", pParam->B3SOIdsub);
            Fatal_Flag = 1;
        }
        if (pParam->B3SOIb1 == -pParam->B3SOIweff)
        {
            ChkFprintf(fplog, "Fatal: (B1 + Weff) = 0 causing divided-by-zero.\n");
            FatalPrintf("Fatal: (B1 + Weff) = 0 causing divided-by-zero.\n");
            Fatal_Flag = 1;
        }
        if (pParam->B3SOIu0temp <= 0.0)
        {
            ChkFprintf(fplog, "Fatal: u0 at current temperature = %g is not positive.\n", pParam->B3SOIu0temp);
            FatalPrintf("Fatal: u0 at current temperature = %g is not positive.\n",
                        pParam->B3SOIu0temp);
            Fatal_Flag = 1;
        }

        /* Check delta parameter */
        if (pParam->B3SOIdelta < 0.0)
        {
            ChkFprintf(fplog, "Fatal: Delta = %g is less than zero.\n",
                       pParam->B3SOIdelta);
            FatalPrintf("Fatal: Delta = %g is less than zero.\n", pParam->B3SOIdelta);
            Fatal_Flag = 1;
        }

        if (pParam->B3SOIvsattemp <= 0.0)
        {
            ChkFprintf(fplog, "Fatal: Vsat at current temperature = %g is not positive.\n", pParam->B3SOIvsattemp);
            FatalPrintf("Fatal: Vsat at current temperature = %g is not positive.\n",
                        pParam->B3SOIvsattemp);
            Fatal_Flag = 1;
        }
        /* Check Rout parameters */
        if (pParam->B3SOIpclm <= 0.0)
        {
            ChkFprintf(fplog, "Fatal: Pclm = %g is not positive.\n", pParam->B3SOIpclm);
            FatalPrintf("Fatal: Pclm = %g is not positive.\n", pParam->B3SOIpclm);
            Fatal_Flag = 1;
        }

        if (pParam->B3SOIdrout < 0.0)
        {
            ChkFprintf(fplog, "Fatal: Drout = %g is negative.\n", pParam->B3SOIdrout);
            FatalPrintf("Fatal: Drout = %g is negative.\n", pParam->B3SOIdrout);
            Fatal_Flag = 1;
        }
        if ( model->B3SOIunitLengthGateSidewallJctCap > 0.0)
        {
            if (here->B3SOIdrainPerimeter < pParam->B3SOIweff)
            {
                ChkFprintf(fplog, "Warning: Pd = %g is less than W.\n",
                           here->B3SOIdrainPerimeter);
// SRW
                if (here->B3SOIdrainPerimeterGiven)
                    WarnPrintf("Warning: Pd = %g is less than W.\n",
                               here->B3SOIdrainPerimeter);
                here->B3SOIdrainPerimeter =pParam->B3SOIweff;
            }
            if (here->B3SOIsourcePerimeter < pParam->B3SOIweff)
            {
                ChkFprintf(fplog, "Warning: Ps = %g is less than W.\n",
                           here->B3SOIsourcePerimeter);
// SRW
                if (here->B3SOIsourcePerimeterGiven)
                    WarnPrintf("Warning: Ps = %g is less than W.\n",
                               here->B3SOIsourcePerimeter);
                here->B3SOIsourcePerimeter =pParam->B3SOIweff;
            }
        }
        /* Check capacitance parameters */
        if (pParam->B3SOIclc < 0.0)
        {
            ChkFprintf(fplog, "Fatal: Clc = %g is negative.\n", pParam->B3SOIclc);
            FatalPrintf("Fatal: Clc = %g is negative.\n", pParam->B3SOIclc);
            Fatal_Flag = 1;
        }

        /* v2.2.3 */
        if (pParam->B3SOImoin < 5.0)
        {
            ChkFprintf(fplog, "Warning: Moin = %g is too small.\n",
                       pParam->B3SOImoin);
            WarnPrintf("Warning: Moin = %g is too small.\n", pParam->B3SOImoin);
        }
        if (pParam->B3SOImoin > 25.0)
        {
            ChkFprintf(fplog, "Warning: Moin = %g is too large.\n",
                       pParam->B3SOImoin);
            WarnPrintf("Warning: Moin = %g is too large.\n", pParam->B3SOImoin);
        }

        /* v3.0 */
        if (model->B3SOImoinFD < 5.0)
        {
            ChkFprintf(fplog, "Warning: MoinFD = %g is too small.\n",
                       model->B3SOImoinFD);
            WarnPrintf("Warning: MoinFD = %g is too small.\n", model->B3SOImoinFD);
        }


        if (model->B3SOIcapMod == 3)
        {
            if (pParam->B3SOIacde < 0.4)
            {
                ChkFprintf (fplog, "Warning: Acde = %g is too small.\n",
                            pParam->B3SOIacde);
                WarnPrintf ("Warning: Acde = %g is too small.\n",
                            pParam->B3SOIacde);
            }
            if (pParam->B3SOIacde > 1.6)
            {
                ChkFprintf (fplog, "Warning: Acde = %g is too large.\n",
                            pParam->B3SOIacde);
                WarnPrintf ("Warning: Acde = %g is too large.\n",
                            pParam->B3SOIacde);
            }
        }
        /* v2.2.3 */

        if (model->B3SOIparamChk ==1)
        {
            /* Check L and W parameters */
            if (pParam->B3SOIleff <= 5.0e-8)
            {
                ChkFprintf(fplog, "Warning: Leff = %g may be too small.\n",
                           pParam->B3SOIleff);
                WarnPrintf("Warning: Leff = %g may be too small.\n",
                           pParam->B3SOIleff);
            }

            if (pParam->B3SOIleffCV <= 5.0e-8)
            {
                ChkFprintf(fplog, "Warning: Leff for CV = %g may be too small.\n",
                           pParam->B3SOIleffCV);
                WarnPrintf("Warning: Leff for CV = %g may be too small.\n",
                           pParam->B3SOIleffCV);
            }

            if (pParam->B3SOIweff <= 1.0e-7)
            {
                ChkFprintf(fplog, "Warning: Weff = %g may be too small.\n",
                           pParam->B3SOIweff);
                WarnPrintf("Warning: Weff = %g may be too small.\n",
                           pParam->B3SOIweff);
            }

            if (pParam->B3SOIweffCV <= 1.0e-7)
            {
                ChkFprintf(fplog, "Warning: Weff for CV = %g may be too small.\n",
                           pParam->B3SOIweffCV);
                WarnPrintf("Warning: Weff for CV = %g may be too small.\n",
                           pParam->B3SOIweffCV);
            }

            /* Check threshold voltage parameters */
            if (pParam->B3SOInlx < 0.0)
            {
                ChkFprintf(fplog, "Warning: Nlx = %g is negative.\n", pParam->B3SOInlx);
                WarnPrintf("Warning: Nlx = %g is negative.\n", pParam->B3SOInlx);
            }
            if (model->B3SOItox < 1.0e-9)
            {
                ChkFprintf(fplog, "Warning: Tox = %g is less than 10A.\n",
                           model->B3SOItox);
                WarnPrintf("Warning: Tox = %g is less than 10A.\n", model->B3SOItox);
            }

            if (pParam->B3SOInpeak <= 1.0e15)
            {
                ChkFprintf(fplog, "Warning: Nch = %g may be too small.\n",
                           pParam->B3SOInpeak);
                WarnPrintf("Warning: Nch = %g may be too small.\n",
                           pParam->B3SOInpeak);
            }
            else if (pParam->B3SOInpeak >= 1.0e21)
            {
                ChkFprintf(fplog, "Warning: Nch = %g may be too large.\n",
                           pParam->B3SOInpeak);
                WarnPrintf("Warning: Nch = %g may be too large.\n",
                           pParam->B3SOInpeak);
            }

            if (fabs(pParam->B3SOInsub) >= 1.0e21)
            {
                ChkFprintf(fplog, "Warning: Nsub = %g may be too large.\n",
                           pParam->B3SOInsub);
                WarnPrintf("Warning: Nsub = %g may be too large.\n",
                           pParam->B3SOInsub);
            }

            if ((pParam->B3SOIngate > 0.0) &&
                    (pParam->B3SOIngate <= 1.e18))
            {
                ChkFprintf(fplog, "Warning: Ngate = %g is less than 1.E18cm^-3.\n",
                           pParam->B3SOIngate);
                WarnPrintf("Warning: Ngate = %g is less than 1.E18cm^-3.\n",
                           pParam->B3SOIngate);
            }

            if (pParam->B3SOIdvt0 < 0.0)
            {
                ChkFprintf(fplog, "Warning: Dvt0 = %g is negative.\n",
                           pParam->B3SOIdvt0);
                WarnPrintf("Warning: Dvt0 = %g is negative.\n", pParam->B3SOIdvt0);
            }

            if (fabs(1.0e-6 / (pParam->B3SOIw0 + pParam->B3SOIweff)) > 10.0)
            {
                ChkFprintf(fplog, "Warning: (W0 + Weff) may be too small.\n");
                WarnPrintf("Warning: (W0 + Weff) may be too small.\n");
            }

            /* Check subthreshold parameters */
            if (pParam->B3SOInfactor < 0.0)
            {
                ChkFprintf(fplog, "Warning: Nfactor = %g is negative.\n",
                           pParam->B3SOInfactor);
                WarnPrintf("Warning: Nfactor = %g is negative.\n", pParam->B3SOInfactor);
            }
            if (pParam->B3SOIcdsc < 0.0)
            {
                ChkFprintf(fplog, "Warning: Cdsc = %g is negative.\n",
                           pParam->B3SOIcdsc);
                WarnPrintf("Warning: Cdsc = %g is negative.\n", pParam->B3SOIcdsc);
            }
            if (pParam->B3SOIcdscd < 0.0)
            {
                ChkFprintf(fplog, "Warning: Cdscd = %g is negative.\n",
                           pParam->B3SOIcdscd);
                WarnPrintf("Warning: Cdscd = %g is negative.\n", pParam->B3SOIcdscd);
            }
            /* Check DIBL parameters */
            if (pParam->B3SOIeta0 < 0.0)
            {
                ChkFprintf(fplog, "Warning: Eta0 = %g is negative.\n",
                           pParam->B3SOIeta0);
                WarnPrintf("Warning: Eta0 = %g is negative.\n", pParam->B3SOIeta0);
            }

            /* Check Abulk parameters */
            if (fabs(1.0e-6 / (pParam->B3SOIb1 + pParam->B3SOIweff)) > 10.0)
            {
                ChkFprintf(fplog, "Warning: (B1 + Weff) may be too small.\n");
                WarnPrintf("Warning: (B1 + Weff) may be too small.\n");
            }

            /* Check Saturation parameters */
            if (pParam->B3SOIa2 < 0.01)
            {
                ChkFprintf(fplog, "Warning: A2 = %g is too small. Set to 0.01.\n", pParam->B3SOIa2);
                WarnPrintf("Warning: A2 = %g is too small. Set to 0.01.\n",
                           pParam->B3SOIa2);
                pParam->B3SOIa2 = 0.01;
            }
            else if (pParam->B3SOIa2 > 1.0)
            {
                ChkFprintf(fplog, "Warning: A2 = %g is larger than 1. A2 is set to 1 and A1 is set to 0.\n",
                           pParam->B3SOIa2);
                WarnPrintf("Warning: A2 = %g is larger than 1. A2 is set to 1 and A1 is set to 0.\n",
                           pParam->B3SOIa2);
                pParam->B3SOIa2 = 1.0;
                pParam->B3SOIa1 = 0.0;

            }

            if (pParam->B3SOIrdsw < 0.0)
            {
                ChkFprintf(fplog, "Warning: Rdsw = %g is negative. Set to zero.\n",
                           pParam->B3SOIrdsw);
                WarnPrintf("Warning: Rdsw = %g is negative. Set to zero.\n",
                           pParam->B3SOIrdsw);
                pParam->B3SOIrdsw = 0.0;
                pParam->B3SOIrds0 = 0.0;
            }
            else if ((pParam->B3SOIrds0 > 0.0) && (pParam->B3SOIrds0 < 0.001))
            {
                ChkFprintf(fplog, "Warning: Rds at current temperature = %g is less than 0.001 ohm. Set to zero.\n",
                           pParam->B3SOIrds0);
                WarnPrintf("Warning: Rds at current temperature = %g is less than 0.001 ohm. Set to zero.\n",
                           pParam->B3SOIrds0);
                pParam->B3SOIrds0 = 0.0;
            }
            if (pParam->B3SOIvsattemp < 1.0e3)
            {
                ChkFprintf(fplog, "Warning: Vsat at current temperature = %g may be too small.\n", pParam->B3SOIvsattemp);
                WarnPrintf("Warning: Vsat at current temperature = %g may be too small.\n", pParam->B3SOIvsattemp);
            }

            if (pParam->B3SOIpdibl1 < 0.0)
            {
                ChkFprintf(fplog, "Warning: Pdibl1 = %g is negative.\n",
                           pParam->B3SOIpdibl1);
                WarnPrintf("Warning: Pdibl1 = %g is negative.\n", pParam->B3SOIpdibl1);
            }
            if (pParam->B3SOIpdibl2 < 0.0)
            {
                ChkFprintf(fplog, "Warning: Pdibl2 = %g is negative.\n",
                           pParam->B3SOIpdibl2);
                WarnPrintf("Warning: Pdibl2 = %g is negative.\n", pParam->B3SOIpdibl2);
            }
            /* Check overlap capacitance parameters */
            if (model->B3SOIcgdo < 0.0)
            {
                ChkFprintf(fplog, "Warning: cgdo = %g is negative. Set to zero.\n", model->B3SOIcgdo);
                WarnPrintf("Warning: cgdo = %g is negative. Set to zero.\n", model->B3SOIcgdo);
                model->B3SOIcgdo = 0.0;
            }
            if (model->B3SOIcgso < 0.0)
            {
                ChkFprintf(fplog, "Warning: cgso = %g is negative. Set to zero.\n", model->B3SOIcgso);
                WarnPrintf("Warning: cgso = %g is negative. Set to zero.\n", model->B3SOIcgso);
                model->B3SOIcgso = 0.0;
            }
            if (model->B3SOIcgeo < 0.0)
            {
                ChkFprintf(fplog, "Warning: cgeo = %g is negative. Set to zero.\n", model->B3SOIcgeo);
                WarnPrintf("Warning: cgeo = %g is negative. Set to zero.\n", model->B3SOIcgeo);
                model->B3SOIcgeo = 0.0;
            }

            if (model->B3SOIntun < 0.0)
            {
                ChkFprintf(fplog, "Warning: Ntun = %g is negative.\n",
                           model->B3SOIntun);
                WarnPrintf("Warning: Ntun = %g is negative.\n", model->B3SOIntun);
            }

            if (model->B3SOIndiode < 0.0)
            {
                ChkFprintf(fplog, "Warning: Ndiode = %g is negative.\n",
                           model->B3SOIndiode);
                WarnPrintf("Warning: Ndiode = %g is negative.\n", model->B3SOIndiode);
            }

            if (model->B3SOIisbjt < 0.0)
            {
                ChkFprintf(fplog, "Warning: Isbjt = %g is negative.\n",
                           model->B3SOIisbjt);
                WarnPrintf("Warning: Isbjt = %g is negative.\n", model->B3SOIisbjt);
            }

            if (model->B3SOIisdif < 0.0)
            {
                ChkFprintf(fplog, "Warning: Isdif = %g is negative.\n",
                           model->B3SOIisdif);
                WarnPrintf("Warning: Isdif = %g is negative.\n", model->B3SOIisdif);
            }

            if (model->B3SOIisrec < 0.0)
            {
                ChkFprintf(fplog, "Warning: Isrec = %g is negative.\n",
                           model->B3SOIisrec);
                WarnPrintf("Warning: Isrec = %g is negative.\n", model->B3SOIisrec);
            }

            if (model->B3SOIistun < 0.0)
            {
                ChkFprintf(fplog, "Warning: Istun = %g is negative.\n",
                           model->B3SOIistun);
                WarnPrintf("Warning: Istun = %g is negative.\n", model->B3SOIistun);
            }

            if (model->B3SOItt < 0.0)
            {
                ChkFprintf(fplog, "Warning: Tt = %g is negative.\n",
                           model->B3SOItt);
                WarnPrintf("Warning: Tt = %g is negative.\n", model->B3SOItt);
            }

            if (model->B3SOIcsdmin < 0.0)
            {
                ChkFprintf(fplog, "Warning: Csdmin = %g is negative.\n",
                           model->B3SOIcsdmin);
                WarnPrintf("Warning: Csdmin = %g is negative.\n", model->B3SOIcsdmin);
            }

            if (model->B3SOIcsdesw < 0.0)
            {
                ChkFprintf(fplog, "Warning: Csdesw = %g is negative.\n",
                           model->B3SOIcsdesw);
                WarnPrintf("Warning: Csdesw = %g is negative.\n", model->B3SOIcsdesw);
            }

            if (model->B3SOIasd < 0.0)
            {
                ChkFprintf(fplog, "Warning: Asd = %g should be within (0, 1).\n",
                           model->B3SOIasd);
                WarnPrintf("Warning: Asd = %g should be within (0, 1).\n", model->B3SOIasd);
            }

            if (model->B3SOIrth0 < 0.0)
            {
                ChkFprintf(fplog, "Warning: Rth0 = %g is negative.\n",
                           model->B3SOIrth0);
                WarnPrintf("Warning: Rth0 = %g is negative.\n", model->B3SOIrth0);
            }

            if (model->B3SOIcth0 < 0.0)
            {
                ChkFprintf(fplog, "Warning: Cth0 = %g is negative.\n",
                           model->B3SOIcth0);
                WarnPrintf("Warning: Cth0 = %g is negative.\n", model->B3SOIcth0);
            }

            if (model->B3SOIrbody < 0.0)
            {
                ChkFprintf(fplog, "Warning: Rbody = %g is negative.\n",
                           model->B3SOIrbody);
                WarnPrintf("Warning: Rbody = %g is negative.\n", model->B3SOIrbody);
            }

            if (model->B3SOIrbsh < 0.0)
            {
                ChkFprintf(fplog, "Warning: Rbsh = %g is negative.\n",
                           model->B3SOIrbsh);
                WarnPrintf("Warning: Rbsh = %g is negative.\n", model->B3SOIrbsh);
            }


            /* v3.0 */
            if (pParam->B3SOInigc <= 0.0)
            {
                ChkFprintf(fplog, "Fatal: nigc = %g is non-positive.\n",
                           pParam->B3SOInigc);
                FatalPrintf("Fatal: nigc = %g is non-positive.\n", pParam->B3SOInigc);
                Fatal_Flag = 1;
            }
            if (pParam->B3SOIpoxedge <= 0.0)
            {
                ChkFprintf(fplog, "Fatal: poxedge = %g is non-positive.\n",
                           pParam->B3SOIpoxedge);
                FatalPrintf("Fatal: poxedge = %g is non-positive.\n", pParam->B3SOIpoxedge);
                Fatal_Flag = 1;
            }
            if (pParam->B3SOIpigcd <= 0.0)
            {
                ChkFprintf(fplog, "Fatal: pigcd = %g is non-positive.\n",
                           pParam->B3SOIpigcd);
                FatalPrintf("Fatal: pigcd = %g is non-positive.\n", pParam->B3SOIpigcd);
                Fatal_Flag = 1;
            }


            /* v2.2 release */
            if (model->B3SOIwth0 < 0.0)
            {
                ChkFprintf(fplog, "Warning: WTH0 = %g is negative.\n",
                           model->B3SOIwth0);
                WarnPrintf("Warning:  Wth0 = %g is negative.\n", model->B3SOIwth0);
            }
            if (model->B3SOIrhalo < 0.0)
            {
                ChkFprintf(fplog, "Warning: RHALO = %g is negative.\n",
                           model->B3SOIrhalo);
                WarnPrintf("Warning:  Rhalo = %g is negative.\n", model->B3SOIrhalo);
            }
            if (model->B3SOIntox < 0.0)
            {
                ChkFprintf(fplog, "Warning: NTOX = %g is negative.\n",
                           model->B3SOIntox);
                WarnPrintf("Warning:  Ntox = %g is negative.\n", model->B3SOIntox);
            }
            if (model->B3SOItoxref < 0.0)
            {
                ChkFprintf(fplog, "Warning: TOXREF = %g is negative.\n",
                           model->B3SOItoxref);
                WarnPrintf("Warning:  Toxref = %g is negative.\n", model->B3SOItoxref);
                Fatal_Flag = 1;
            }
            if (model->B3SOIebg < 0.0)
            {
                ChkFprintf(fplog, "Warning: EBG = %g is negative.\n",
                           model->B3SOIebg);
                WarnPrintf("Warning:  Ebg = %g is negative.\n", model->B3SOIebg);
            }
            if (model->B3SOIvevb < 0.0)
            {
                ChkFprintf(fplog, "Warning: VEVB = %g is negative.\n",
                           model->B3SOIvevb);
                WarnPrintf("Warning:  Vevb = %g is negative.\n", model->B3SOIvevb);
            }
            if (model->B3SOIalphaGB1 < 0.0)
            {
                ChkFprintf(fplog, "Warning: ALPHAGB1 = %g is negative.\n",
                           model->B3SOIalphaGB1);
                WarnPrintf("Warning:  AlphaGB1 = %g is negative.\n", model->B3SOIalphaGB1);
            }
            if (model->B3SOIbetaGB1 < 0.0)
            {
                ChkFprintf(fplog, "Warning: BETAGB1 = %g is negative.\n",
                           model->B3SOIbetaGB1);
                WarnPrintf("Warning:  BetaGB1 = %g is negative.\n", model->B3SOIbetaGB1);
            }
            if (model->B3SOIvgb1 < 0.0)
            {
                ChkFprintf(fplog, "Warning: VGB1 = %g is negative.\n",
                           model->B3SOIvgb1);
                WarnPrintf("Warning:  Vgb1 = %g is negative.\n", model->B3SOIvgb1);
            }
            if (model->B3SOIvecb < 0.0)
            {
                ChkFprintf(fplog, "Warning: VECB = %g is negative.\n",
                           model->B3SOIvecb);
                WarnPrintf("Warning:  Vecb = %g is negative.\n", model->B3SOIvecb);
            }
            if (model->B3SOIalphaGB2 < 0.0)
            {
                ChkFprintf(fplog, "Warning: ALPHAGB2 = %g is negative.\n",
                           model->B3SOIalphaGB2);
                WarnPrintf("Warning:  AlphaGB2 = %g is negative.\n", model->B3SOIalphaGB2);
            }
            if (model->B3SOIbetaGB2 < 0.0)
            {
                ChkFprintf(fplog, "Warning: BETAGB2 = %g is negative.\n",
                           model->B3SOIbetaGB2);
                WarnPrintf("Warning:  BetaGB2 = %g is negative.\n", model->B3SOIbetaGB2);
            }
            if (model->B3SOIvgb2 < 0.0)
            {
                ChkFprintf(fplog, "Warning: VGB2 = %g is negative.\n",
                           model->B3SOIvgb2);
                WarnPrintf("Warning:  Vgb2 = %g is negative.\n", model->B3SOIvgb2);
            }
            if (model->B3SOItoxqm <= 0.0)
            {
                ChkFprintf(fplog, "Fatal: Toxqm = %g is not positive.\n",
                           model->B3SOItoxqm);
                FatalPrintf("Fatal: Toxqm = %g is not positive.\n", model->B3SOItoxqm);
                Fatal_Flag = 1;
            }
            if (model->B3SOIvoxh < 0.0)
            {
                ChkFprintf(fplog, "Warning: Voxh = %g is negative.\n",
                           model->B3SOIvoxh);
                WarnPrintf("Warning:  Voxh = %g is negative.\n", model->B3SOIvoxh);
            }
            if (model->B3SOIdeltavox <= 0.0)
            {
                ChkFprintf(fplog, "Fatal: Deltavox = %g is not positive.\n",
                           model->B3SOIdeltavox);
                FatalPrintf("Fatal: Deltavox = %g is not positive.\n", model->B3SOIdeltavox);
            }


            /* v2.0 release */
            if (model->B3SOIk1w1 < 0.0)
            {
                ChkFprintf(fplog, "Warning: K1W1 = %g is negative.\n",
                           model->B3SOIk1w1);
                WarnPrintf("Warning:  K1w1 = %g is negative.\n", model->B3SOIk1w1);
            }
            if (model->B3SOIk1w2 < 0.0)
            {
                ChkFprintf(fplog, "Warning: K1W2 = %g is negative.\n",
                           model->B3SOIk1w2);
                WarnPrintf("Warning:  K1w2 = %g is negative.\n", model->B3SOIk1w2);
            }
            if (model->B3SOIketas < 0.0)
            {
                ChkFprintf(fplog, "Warning: KETAS = %g is negative.\n",
                           model->B3SOIketas);
                WarnPrintf("Warning:  Ketas = %g is negative.\n", model->B3SOIketas);
            }
            if (model->B3SOIdwbc < 0.0)
            {
                ChkFprintf(fplog, "Warning: DWBC = %g is negative.\n",
                           model->B3SOIdwbc);
                WarnPrintf("Warning:  Dwbc = %g is negative.\n", model->B3SOIdwbc);
            }
            if (model->B3SOIbeta0 < 0.0)
            {
                ChkFprintf(fplog, "Warning: BETA0 = %g is negative.\n",
                           model->B3SOIbeta0);
                WarnPrintf("Warning:  Beta0 = %g is negative.\n", model->B3SOIbeta0);
            }
            if (model->B3SOIbeta1 < 0.0)
            {
                ChkFprintf(fplog, "Warning: BETA1 = %g is negative.\n",
                           model->B3SOIbeta1);
                WarnPrintf("Warning:  Beta1 = %g is negative.\n", model->B3SOIbeta1);
            }
            if (model->B3SOIbeta2 < 0.0)
            {
                ChkFprintf(fplog, "Warning: BETA2 = %g is negative.\n",
                           model->B3SOIbeta2);
                WarnPrintf("Warning:  Beta2 = %g is negative.\n", model->B3SOIbeta2);
            }
            if (model->B3SOItii < 0.0)
            {
                ChkFprintf(fplog, "Warning: TII = %g is negative.\n",
                           model->B3SOItii);
                WarnPrintf("Warning:  Tii = %g is negative.\n", model->B3SOItii);
            }
            if (model->B3SOIlii < 0.0)
            {
                ChkFprintf(fplog, "Warning: LII = %g is negative.\n",
                           model->B3SOIlii);
                WarnPrintf("Warning:  Lii = %g is negative.\n", model->B3SOIlii);
            }
            if (model->B3SOIsii1 < 0.0)
            {
                ChkFprintf(fplog, "Warning: SII1 = %g is negative.\n",
                           model->B3SOIsii1);
                WarnPrintf("Warning:  Sii1 = %g is negative.\n", model->B3SOIsii1);
            }
            if (model->B3SOIsii2 < 0.0)
            {
                ChkFprintf(fplog, "Warning: SII2 = %g is negative.\n",
                           model->B3SOIsii2);
                WarnPrintf("Warning:  Sii2 = %g is negative.\n", model->B3SOIsii1);
            }
            if (model->B3SOIsiid < 0.0)
            {
                ChkFprintf(fplog, "Warning: SIID = %g is negative.\n",
                           model->B3SOIsiid);
                WarnPrintf("Warning:  Siid = %g is negative.\n", model->B3SOIsiid);
            }
            if (model->B3SOIfbjtii < 0.0)
            {
                ChkFprintf(fplog, "Warning: FBJTII = %g is negative.\n",
                           model->B3SOIfbjtii);
                WarnPrintf("Warning:  fbjtii = %g is negative.\n", model->B3SOIfbjtii);
            }
            if (model->B3SOIvrec0 < 0.0)
            {
                ChkFprintf(fplog, "Warning: VREC0 = %g is negative.\n",
                           model->B3SOIvrec0);
                WarnPrintf("Warning:  Vrec0 = %g is negative.\n", model->B3SOIvrec0);
            }
            if (model->B3SOIvtun0 < 0.0)
            {
                ChkFprintf(fplog, "Warning: VTUN0 = %g is negative.\n",
                           model->B3SOIvtun0);
                WarnPrintf("Warning:  Vtun0 = %g is negative.\n", model->B3SOIvtun0);
            }
            if (model->B3SOInbjt < 0.0)
            {
                ChkFprintf(fplog, "Warning: NBJT = %g is negative.\n",
                           model->B3SOInbjt);
                WarnPrintf("Warning:  Nbjt = %g is negative.\n", model->B3SOInbjt);
            }
            if (model->B3SOIaely < 0.0)
            {
                ChkFprintf(fplog, "Warning: AELY = %g is negative.\n",
                           model->B3SOIaely);
                WarnPrintf("Warning:  Aely = %g is negative.\n", model->B3SOIaely);
            }
            if (model->B3SOIahli < 0.0)
            {
                ChkFprintf(fplog, "Warning: AHLI = %g is negative.\n",
                           model->B3SOIahli);
                WarnPrintf("Warning:  Ahli = %g is negative.\n", model->B3SOIahli);
            }
            if (model->B3SOIrbody < 0.0)
            {
                ChkFprintf(fplog, "Warning: RBODY = %g is negative.\n",
                           model->B3SOIrbody);
                WarnPrintf("Warning:  Rbody = %g is negative.\n", model->B3SOIrbody);
            }
            if (model->B3SOIrbsh < 0.0)
            {
                ChkFprintf(fplog, "Warning: RBSH = %g is negative.\n",
                           model->B3SOIrbsh);
                WarnPrintf("Warning:  Rbsh = %g is negative.\n", model->B3SOIrbsh);
            }
            if (model->B3SOIntrecf < 0.0)
            {
                ChkFprintf(fplog, "Warning: NTRECF = %g is negative.\n",
                           model->B3SOIntrecf);
                WarnPrintf("Warning:  Ntrecf = %g is negative.\n", model->B3SOIntrecf);
            }
            if (model->B3SOIntrecr < 0.0)
            {
                ChkFprintf(fplog, "Warning: NTRECR = %g is negative.\n",
                           model->B3SOIntrecr);
                WarnPrintf("Warning:  Ntrecr = %g is negative.\n", model->B3SOIntrecr);
            }

            /* v3.0 bug fix */
            /*
                    if (model->B3SOIndif < 0.0)
                    {   ChkFprintf(fplog, "Warning: NDIF = %g is negative.\n",
                                model->B3SOIndif);
                        WarnPrintf("Warning:  Ndif = %g is negative.\n", model->B3SOIndif);
                    }
            */

            if (model->B3SOItcjswg < 0.0)
            {
                ChkFprintf(fplog, "Warning: TCJSWG = %g is negative.\n",
                           model->B3SOItcjswg);
                WarnPrintf("Warning:  Tcjswg = %g is negative.\n", model->B3SOItcjswg);
            }
            if (model->B3SOItpbswg < 0.0)
            {
                ChkFprintf(fplog, "Warning: TPBSWG = %g is negative.\n",
                           model->B3SOItpbswg);
                WarnPrintf("Warning:  Tpbswg = %g is negative.\n", model->B3SOItpbswg);
            }
            if ((model->B3SOIacde < 0.4) || (model->B3SOIacde > 1.6))
            {
                ChkFprintf(fplog, "Warning: ACDE = %g is out of range.\n",
                           model->B3SOIacde);
                WarnPrintf("Warning:  Acde = %g is out of range.\n", model->B3SOIacde);
            }
            if ((model->B3SOImoin < 5.0)||(model->B3SOImoin > 25.0))
            {
                ChkFprintf(fplog, "Warning: MOIN = %g is out of range.\n",
                           model->B3SOImoin);
                WarnPrintf("Warning:  Moin = %g is out of range.\n", model->B3SOImoin);
            }
            if (model->B3SOIdlbg < 0.0)
            {
                ChkFprintf(fplog, "Warning: DLBG = %g is negative.\n",
                           model->B3SOIdlbg);
                WarnPrintf("Warning:  dlbg = %g is negative.\n", model->B3SOIdlbg);
            }


            if (model->B3SOIagidl < 0.0)
            {
                ChkFprintf(fplog, "Warning: AGIDL = %g is negative.\n",
                           model->B3SOIagidl);
                WarnPrintf("Warning:  Agidl = %g is negative.\n", model->B3SOIagidl);
            }
            if (model->B3SOIbgidl < 0.0)
            {
                ChkFprintf(fplog, "Warning: BGIDL = %g is negative.\n",
                           model->B3SOIbgidl);
                WarnPrintf("Warning:  Bgidl = %g is negative.\n", model->B3SOIbgidl);
            }
            if (model->B3SOIngidl < 0.0)
            {
                ChkFprintf(fplog, "Warning: NGIDL = %g is negative.\n",
                           model->B3SOIngidl);
                WarnPrintf("Warning:  Ngidl = %g is negative.\n", model->B3SOIngidl);
            }
            if (model->B3SOIesatii < 0.0)
            {
                ChkFprintf(fplog, "Warning: Esatii = %g should be within positive.\n",
                           model->B3SOIesatii);
                WarnPrintf("Warning: Esatii = %g should be within (0, 1).\n", model->B3SOIesatii);
            }


            if (model->B3SOIxj > model->B3SOItsi)
            {
                ChkFprintf(fplog, "Warning: Xj = %g is thicker than Tsi = %g.\n",
                           model->B3SOIxj, model->B3SOItsi);
                WarnPrintf("Warning: Xj = %g is thicker than Tsi = %g.\n",
                           model->B3SOIxj, model->B3SOItsi);
            }

            if (model->B3SOIcapMod < 2)
// SRW
            {
                ChkFprintf(fplog, "Warning: capMod < 2 is not supported by BSIMSOI-3.0.\n");
                WarnPrintf("Warning: Warning: capMod < 2 is not supported by BSIMSOI-3.0.\n");
            }

        }/* loop for the parameter check for warning messages */
// SRW
        if (fplog)
            fclose(fplog);
    }
    else
    {
        fprintf(stderr, "Warning: Can't open log file. Parameter checking skipped.\n");
    }

    return(Fatal_Flag);
}

