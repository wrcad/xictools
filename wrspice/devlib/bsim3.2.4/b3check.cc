
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
 $Id: b3check.cc,v 2.14 2011/12/18 01:15:21 stevew Exp $
 *========================================================================*/

/**********
 * Copyright 2001 Regents of the University of California. All rights reserved.
 * File: b3check.c of BSIM3v3.2.4
 * Author: 1995 Min-Chie Jeng
 * Author: 1997-1999 Weidong Liu.
 * Author: 2001 Xuemei Xi
 * Modified by Xuemei Xi, 10/05, 12/14, 2001.
 **********/

#include <stdio.h>
#include "b3defs.h"


#define BSIM3modName GENmodName

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
BSIM3dev::checkModel(sBSIM3model *model, sBSIM3instance *here, sCKT*)
{
    struct bsim3SizeDependParam *pParam;
    int Fatal_Flag = 0;
    FILE *fplog;

    bool name_shown = false;

    fplog = 0;
    if (true)
//    if ((fplog = fopen("b3v3check.log", "w")) != NULL)
    {
        pParam = here->pParam;
        ChkFprintf(fplog, "BSIM3v3.2.4 Parameter Checking.\n");

// SRW -- modified version checking
        if (DEV.checkVersion(B3VERSION, model->BSIM3version))
        {
            ChkFprintf(fplog, "Warning: This model is %s, given version is %s.\n", B3VERSION, model->BSIM3version);
            WarnPrintf("Warning: This model is %s, given version is %s.\n", B3VERSION, model->BSIM3version);
        }
        ChkFprintf(fplog, "Model = %s\n", (char*)model->BSIM3modName);

        if (pParam->BSIM3nlx < -pParam->BSIM3leff)
        {
            ChkFprintf(fplog, "Fatal: Nlx = %g is less than -Leff.\n",
                       pParam->BSIM3nlx);
            FatalPrintf("Fatal: Nlx = %g is less than -Leff.\n",
                        pParam->BSIM3nlx);
            Fatal_Flag = 1;
        }

        if (model->BSIM3tox <= 0.0)
        {
            ChkFprintf(fplog, "Fatal: Tox = %g is not positive.\n",
                       model->BSIM3tox);
            FatalPrintf("Fatal: Tox = %g is not positive.\n", model->BSIM3tox);
            Fatal_Flag = 1;
        }

        if (model->BSIM3toxm <= 0.0)
        {
            ChkFprintf(fplog, "Fatal: Toxm = %g is not positive.\n",
                       model->BSIM3toxm);
            FatalPrintf("Fatal: Toxm = %g is not positive.\n", model->BSIM3toxm);
            Fatal_Flag = 1;
        }

        if (pParam->BSIM3npeak <= 0.0)
        {
            ChkFprintf(fplog, "Fatal: Nch = %g is not positive.\n",
                       pParam->BSIM3npeak);
            FatalPrintf("Fatal: Nch = %g is not positive.\n",
                        pParam->BSIM3npeak);
            Fatal_Flag = 1;
        }
        if (pParam->BSIM3nsub <= 0.0)
        {
            ChkFprintf(fplog, "Fatal: Nsub = %g is not positive.\n",
                       pParam->BSIM3nsub);
            FatalPrintf("Fatal: Nsub = %g is not positive.\n",
                        pParam->BSIM3nsub);
            Fatal_Flag = 1;
        }
        if (pParam->BSIM3ngate < 0.0)
        {
            ChkFprintf(fplog, "Fatal: Ngate = %g is not positive.\n",
                       pParam->BSIM3ngate);
            FatalPrintf("Fatal: Ngate = %g Ngate is not positive.\n",
                        pParam->BSIM3ngate);
            Fatal_Flag = 1;
        }
        if (pParam->BSIM3ngate > 1.e25)
        {
            ChkFprintf(fplog, "Fatal: Ngate = %g is too high.\n",
                       pParam->BSIM3ngate);
            FatalPrintf("Fatal: Ngate = %g Ngate is too high\n",
                        pParam->BSIM3ngate);
            Fatal_Flag = 1;
        }
        if (pParam->BSIM3xj <= 0.0)
        {
            ChkFprintf(fplog, "Fatal: Xj = %g is not positive.\n",
                       pParam->BSIM3xj);
            FatalPrintf("Fatal: Xj = %g is not positive.\n", pParam->BSIM3xj);
            Fatal_Flag = 1;
        }

        if (pParam->BSIM3dvt1 < 0.0)
        {
            ChkFprintf(fplog, "Fatal: Dvt1 = %g is negative.\n",
                       pParam->BSIM3dvt1);
            FatalPrintf("Fatal: Dvt1 = %g is negative.\n", pParam->BSIM3dvt1);
            Fatal_Flag = 1;
        }

        if (pParam->BSIM3dvt1w < 0.0)
        {
            ChkFprintf(fplog, "Fatal: Dvt1w = %g is negative.\n",
                       pParam->BSIM3dvt1w);
            FatalPrintf("Fatal: Dvt1w = %g is negative.\n", pParam->BSIM3dvt1w);
            Fatal_Flag = 1;
        }

        if (pParam->BSIM3w0 == -pParam->BSIM3weff)
        {
            ChkFprintf(fplog, "Fatal: (W0 + Weff) = 0 causing divided-by-zero.\n");
            FatalPrintf("Fatal: (W0 + Weff) = 0 causing divided-by-zero.\n");
            Fatal_Flag = 1;
        }

        if (pParam->BSIM3dsub < 0.0)
        {
            ChkFprintf(fplog, "Fatal: Dsub = %g is negative.\n", pParam->BSIM3dsub);
            FatalPrintf("Fatal: Dsub = %g is negative.\n", pParam->BSIM3dsub);
            Fatal_Flag = 1;
        }
        if (pParam->BSIM3b1 == -pParam->BSIM3weff)
        {
            ChkFprintf(fplog, "Fatal: (B1 + Weff) = 0 causing divided-by-zero.\n");
            FatalPrintf("Fatal: (B1 + Weff) = 0 causing divided-by-zero.\n");
            Fatal_Flag = 1;
        }
        if (pParam->BSIM3u0temp <= 0.0)
        {
            ChkFprintf(fplog, "Fatal: u0 at current temperature = %g is not positive.\n", pParam->BSIM3u0temp);
            FatalPrintf("Fatal: u0 at current temperature = %g is not positive.\n",
                        pParam->BSIM3u0temp);
            Fatal_Flag = 1;
        }

        /* Check delta parameter */
        if (pParam->BSIM3delta < 0.0)
        {
            ChkFprintf(fplog, "Fatal: Delta = %g is less than zero.\n",
                       pParam->BSIM3delta);
            FatalPrintf("Fatal: Delta = %g is less than zero.\n", pParam->BSIM3delta);
            Fatal_Flag = 1;
        }

        if (pParam->BSIM3vsattemp <= 0.0)
        {
            ChkFprintf(fplog, "Fatal: Vsat at current temperature = %g is not positive.\n", pParam->BSIM3vsattemp);
            FatalPrintf("Fatal: Vsat at current temperature = %g is not positive.\n",
                        pParam->BSIM3vsattemp);
            Fatal_Flag = 1;
        }
        /* Check Rout parameters */
        if (pParam->BSIM3pclm <= 0.0)
        {
            ChkFprintf(fplog, "Fatal: Pclm = %g is not positive.\n", pParam->BSIM3pclm);
            FatalPrintf("Fatal: Pclm = %g is not positive.\n", pParam->BSIM3pclm);
            Fatal_Flag = 1;
        }

        if (pParam->BSIM3drout < 0.0)
        {
            ChkFprintf(fplog, "Fatal: Drout = %g is negative.\n", pParam->BSIM3drout);
            FatalPrintf("Fatal: Drout = %g is negative.\n", pParam->BSIM3drout);
            Fatal_Flag = 1;
        }

        if (pParam->BSIM3pscbe2 <= 0.0)
        {
            ChkFprintf(fplog, "Warning: Pscbe2 = %g is not positive.\n",
                       pParam->BSIM3pscbe2);
            WarnPrintf("Warning: Pscbe2 = %g is not positive.\n", pParam->BSIM3pscbe2);
        }

        if (model->BSIM3unitLengthSidewallJctCap > 0.0 ||
                model->BSIM3unitLengthGateSidewallJctCap > 0.0)
        {
            if (here->BSIM3drainPerimeter < pParam->BSIM3weff)
            {
                ChkFprintf(fplog, "Warning: Pd = %g is less than W.\n",
                           here->BSIM3drainPerimeter);
// SRW
                if (here->BSIM3drainPerimeterGiven)
                    WarnPrintf("Warning: Pd = %g is less than W.\n",
                               here->BSIM3drainPerimeter);
            }
            if (here->BSIM3sourcePerimeter < pParam->BSIM3weff)
            {
                ChkFprintf(fplog, "Warning: Ps = %g is less than W.\n",
                           here->BSIM3sourcePerimeter);
// SRW
                if (here->BSIM3sourcePerimeterGiven)
                    WarnPrintf("Warning: Ps = %g is less than W.\n",
                               here->BSIM3sourcePerimeter);
            }
        }

        if (pParam->BSIM3noff < 0.1)
        {
            ChkFprintf(fplog, "Warning: Noff = %g is too small.\n",
                       pParam->BSIM3noff);
            WarnPrintf("Warning: Noff = %g is too small.\n", pParam->BSIM3noff);
        }
        if (pParam->BSIM3noff > 4.0)
        {
            ChkFprintf(fplog, "Warning: Noff = %g is too large.\n",
                       pParam->BSIM3noff);
            WarnPrintf("Warning: Noff = %g is too large.\n", pParam->BSIM3noff);
        }

        if (pParam->BSIM3voffcv < -0.5)
        {
            ChkFprintf(fplog, "Warning: Voffcv = %g is too small.\n",
                       pParam->BSIM3voffcv);
            WarnPrintf("Warning: Voffcv = %g is too small.\n", pParam->BSIM3voffcv);
        }
        if (pParam->BSIM3voffcv > 0.5)
        {
            ChkFprintf(fplog, "Warning: Voffcv = %g is too large.\n",
                       pParam->BSIM3voffcv);
            WarnPrintf("Warning: Voffcv = %g is too large.\n", pParam->BSIM3voffcv);
        }

        if (model->BSIM3ijth < 0.0)
        {
            ChkFprintf(fplog, "Fatal: Ijth = %g cannot be negative.\n",
                       model->BSIM3ijth);
            FatalPrintf("Fatal: Ijth = %g cannot be negative.\n", model->BSIM3ijth);
            Fatal_Flag = 1;
        }

        /* Check capacitance parameters */
        if (pParam->BSIM3clc < 0.0)
        {
            ChkFprintf(fplog, "Fatal: Clc = %g is negative.\n", pParam->BSIM3clc);
            FatalPrintf("Fatal: Clc = %g is negative.\n", pParam->BSIM3clc);
            Fatal_Flag = 1;
        }

        if (pParam->BSIM3moin < 5.0)
        {
            ChkFprintf(fplog, "Warning: Moin = %g is too small.\n",
                       pParam->BSIM3moin);
            WarnPrintf("Warning: Moin = %g is too small.\n", pParam->BSIM3moin);
        }
        if (pParam->BSIM3moin > 25.0)
        {
            ChkFprintf(fplog, "Warning: Moin = %g is too large.\n",
                       pParam->BSIM3moin);
            WarnPrintf("Warning: Moin = %g is too large.\n", pParam->BSIM3moin);
        }

        if(model->BSIM3capMod ==3)
        {
            if (pParam->BSIM3acde < 0.4)
            {
                ChkFprintf(fplog, "Warning:  Acde = %g is too small.\n",
                           pParam->BSIM3acde);
                WarnPrintf("Warning: Acde = %g is too small.\n", pParam->BSIM3acde);
            }
            if (pParam->BSIM3acde > 1.6)
            {
                ChkFprintf(fplog, "Warning:  Acde = %g is too large.\n",
                           pParam->BSIM3acde);
                WarnPrintf("Warning: Acde = %g is too large.\n", pParam->BSIM3acde);
            }
        }

        if (model->BSIM3paramChk ==1)
        {
            /* Check L and W parameters */
            if (pParam->BSIM3leff <= 5.0e-8)
            {
                ChkFprintf(fplog, "Warning: Leff = %g may be too small.\n",
                           pParam->BSIM3leff);
                WarnPrintf("Warning: Leff = %g may be too small.\n",
                           pParam->BSIM3leff);
            }

            if (pParam->BSIM3leffCV <= 5.0e-8)
            {
                ChkFprintf(fplog, "Warning: Leff for CV = %g may be too small.\n",
                           pParam->BSIM3leffCV);
                WarnPrintf("Warning: Leff for CV = %g may be too small.\n",
                           pParam->BSIM3leffCV);
            }

            if (pParam->BSIM3weff <= 1.0e-7)
            {
                ChkFprintf(fplog, "Warning: Weff = %g may be too small.\n",
                           pParam->BSIM3weff);
                WarnPrintf("Warning: Weff = %g may be too small.\n",
                           pParam->BSIM3weff);
            }

            if (pParam->BSIM3weffCV <= 1.0e-7)
            {
                ChkFprintf(fplog, "Warning: Weff for CV = %g may be too small.\n",
                           pParam->BSIM3weffCV);
                WarnPrintf("Warning: Weff for CV = %g may be too small.\n",
                           pParam->BSIM3weffCV);
            }

            /* Check threshold voltage parameters */
            if (pParam->BSIM3nlx < 0.0)
            {
                ChkFprintf(fplog, "Warning: Nlx = %g is negative.\n", pParam->BSIM3nlx);
                WarnPrintf("Warning: Nlx = %g is negative.\n", pParam->BSIM3nlx);
            }
            if (model->BSIM3tox < 1.0e-9)
            {
                ChkFprintf(fplog, "Warning: Tox = %g is less than 10A.\n",
                           model->BSIM3tox);
                WarnPrintf("Warning: Tox = %g is less than 10A.\n", model->BSIM3tox);
            }

            if (pParam->BSIM3npeak <= 1.0e15)
            {
                ChkFprintf(fplog, "Warning: Nch = %g may be too small.\n",
                           pParam->BSIM3npeak);
                WarnPrintf("Warning: Nch = %g may be too small.\n",
                           pParam->BSIM3npeak);
            }
            else if (pParam->BSIM3npeak >= 1.0e21)
            {
                ChkFprintf(fplog, "Warning: Nch = %g may be too large.\n",
                           pParam->BSIM3npeak);
                WarnPrintf("Warning: Nch = %g may be too large.\n",
                           pParam->BSIM3npeak);
            }

            if (pParam->BSIM3nsub <= 1.0e14)
            {
                ChkFprintf(fplog, "Warning: Nsub = %g may be too small.\n",
                           pParam->BSIM3nsub);
                WarnPrintf("Warning: Nsub = %g may be too small.\n",
                           pParam->BSIM3nsub);
            }
            else if (pParam->BSIM3nsub >= 1.0e21)
            {
                ChkFprintf(fplog, "Warning: Nsub = %g may be too large.\n",
                           pParam->BSIM3nsub);
                WarnPrintf("Warning: Nsub = %g may be too large.\n",
                           pParam->BSIM3nsub);
            }

            if ((pParam->BSIM3ngate > 0.0) &&
                    (pParam->BSIM3ngate <= 1.e18))
            {
                ChkFprintf(fplog, "Warning: Ngate = %g is less than 1.E18cm^-3.\n",
                           pParam->BSIM3ngate);
                WarnPrintf("Warning: Ngate = %g is less than 1.E18cm^-3.\n",
                           pParam->BSIM3ngate);
            }

            if (pParam->BSIM3dvt0 < 0.0)
            {
                ChkFprintf(fplog, "Warning: Dvt0 = %g is negative.\n",
                           pParam->BSIM3dvt0);
                WarnPrintf("Warning: Dvt0 = %g is negative.\n", pParam->BSIM3dvt0);
            }

            if (fabs(1.0e-6 / (pParam->BSIM3w0 + pParam->BSIM3weff)) > 10.0)
            {
                ChkFprintf(fplog, "Warning: (W0 + Weff) may be too small.\n");
                WarnPrintf("Warning: (W0 + Weff) may be too small.\n");
            }

            /* Check subthreshold parameters */
            if (pParam->BSIM3nfactor < 0.0)
            {
                ChkFprintf(fplog, "Warning: Nfactor = %g is negative.\n",
                           pParam->BSIM3nfactor);
                WarnPrintf("Warning: Nfactor = %g is negative.\n", pParam->BSIM3nfactor);
            }
            if (pParam->BSIM3cdsc < 0.0)
            {
                ChkFprintf(fplog, "Warning: Cdsc = %g is negative.\n",
                           pParam->BSIM3cdsc);
                WarnPrintf("Warning: Cdsc = %g is negative.\n", pParam->BSIM3cdsc);
            }
            if (pParam->BSIM3cdscd < 0.0)
            {
                ChkFprintf(fplog, "Warning: Cdscd = %g is negative.\n",
                           pParam->BSIM3cdscd);
                WarnPrintf("Warning: Cdscd = %g is negative.\n", pParam->BSIM3cdscd);
            }
            /* Check DIBL parameters */
            if (pParam->BSIM3eta0 < 0.0)
            {
                ChkFprintf(fplog, "Warning: Eta0 = %g is negative.\n",
                           pParam->BSIM3eta0);
                WarnPrintf("Warning: Eta0 = %g is negative.\n", pParam->BSIM3eta0);
            }

            /* Check Abulk parameters */
            if (fabs(1.0e-6 / (pParam->BSIM3b1 + pParam->BSIM3weff)) > 10.0)
            {
                ChkFprintf(fplog, "Warning: (B1 + Weff) may be too small.\n");
                WarnPrintf("Warning: (B1 + Weff) may be too small.\n");
            }


            /* Check Saturation parameters */
            if (pParam->BSIM3a2 < 0.01)
            {
                ChkFprintf(fplog, "Warning: A2 = %g is too small. Set to 0.01.\n", pParam->BSIM3a2);
                WarnPrintf("Warning: A2 = %g is too small. Set to 0.01.\n",
                           pParam->BSIM3a2);
                pParam->BSIM3a2 = 0.01;
            }
            else if (pParam->BSIM3a2 > 1.0)
            {
                ChkFprintf(fplog, "Warning: A2 = %g is larger than 1. A2 is set to 1 and A1 is set to 0.\n",
                           pParam->BSIM3a2);
                WarnPrintf("Warning: A2 = %g is larger than 1. A2 is set to 1 and A1 is set to 0.\n",
                           pParam->BSIM3a2);
                pParam->BSIM3a2 = 1.0;
                pParam->BSIM3a1 = 0.0;

            }

            if (pParam->BSIM3rdsw < 0.0)
            {
                ChkFprintf(fplog, "Warning: Rdsw = %g is negative. Set to zero.\n",
                           pParam->BSIM3rdsw);
                WarnPrintf("Warning: Rdsw = %g is negative. Set to zero.\n",
                           pParam->BSIM3rdsw);
                pParam->BSIM3rdsw = 0.0;
                pParam->BSIM3rds0 = 0.0;
            }
            else if ((pParam->BSIM3rds0 > 0.0) && (pParam->BSIM3rds0 < 0.001))
            {
                ChkFprintf(fplog, "Warning: Rds at current temperature = %g is less than 0.001 ohm. Set to zero.\n",
                           pParam->BSIM3rds0);
                WarnPrintf("Warning: Rds at current temperature = %g is less than 0.001 ohm. Set to zero.\n",
                           pParam->BSIM3rds0);
                pParam->BSIM3rds0 = 0.0;
            }
            if (pParam->BSIM3vsattemp < 1.0e3)
            {
                ChkFprintf(fplog, "Warning: Vsat at current temperature = %g may be too small.\n", pParam->BSIM3vsattemp);
                WarnPrintf("Warning: Vsat at current temperature = %g may be too small.\n", pParam->BSIM3vsattemp);
            }

            if (pParam->BSIM3pdibl1 < 0.0)
            {
                ChkFprintf(fplog, "Warning: Pdibl1 = %g is negative.\n",
                           pParam->BSIM3pdibl1);
                WarnPrintf("Warning: Pdibl1 = %g is negative.\n", pParam->BSIM3pdibl1);
            }
            if (pParam->BSIM3pdibl2 < 0.0)
            {
                ChkFprintf(fplog, "Warning: Pdibl2 = %g is negative.\n",
                           pParam->BSIM3pdibl2);
                WarnPrintf("Warning: Pdibl2 = %g is negative.\n", pParam->BSIM3pdibl2);
            }
            /* Check overlap capacitance parameters */
            if (model->BSIM3cgdo < 0.0)
            {
                ChkFprintf(fplog, "Warning: cgdo = %g is negative. Set to zero.\n", model->BSIM3cgdo);
                WarnPrintf("Warning: cgdo = %g is negative. Set to zero.\n", model->BSIM3cgdo);
                model->BSIM3cgdo = 0.0;
            }
            if (model->BSIM3cgso < 0.0)
            {
                ChkFprintf(fplog, "Warning: cgso = %g is negative. Set to zero.\n", model->BSIM3cgso);
                WarnPrintf("Warning: cgso = %g is negative. Set to zero.\n", model->BSIM3cgso);
                model->BSIM3cgso = 0.0;
            }
            if (model->BSIM3cgbo < 0.0)
            {
                ChkFprintf(fplog, "Warning: cgbo = %g is negative. Set to zero.\n", model->BSIM3cgbo);
                WarnPrintf("Warning: cgbo = %g is negative. Set to zero.\n", model->BSIM3cgbo);
                model->BSIM3cgbo = 0.0;
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

