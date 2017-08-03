
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
 * Copyright 2001 Regents of the University of California. All rights reserved.
 * File: b4check.c of BSIM4.2.1.
 * Author: 2000 Weidong Liu
 * Authors: Xuemei Xi, Kanyu M. Cao, Hui Wan, Mansun Chan, Chenming Hu.
 * Project Director: Prof. Chenming Hu.
 * Modified by Xuemei Xi, 04/06/2001.
 * Modified by Xuemei Xi, 10/05/2001.
 **********/

#include <stdio.h>
#include "b4defs.h"


#define BSIM4modName GENmodName

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
BSIM4dev::checkModel(sBSIM4model *model, sBSIM4instance *here, sCKT*)
{
    struct bsim4SizeDependParam *pParam;
    int Fatal_Flag = 0;
    FILE *fplog;

    bool name_shown = false;

    fplog = 0;
    if (true)
//    if ((fplog = fopen("bsim4.out", "w")) != NULL)
    {
        pParam = here->pParam;
        ChkFprintf(fplog, "BSIM4: Berkeley Short Channel IGFET Model-4\n");
        ChkFprintf(fplog, "Developed by Weidong Liu, Xuemei Xi , Xiaodong Jin, Kanyu M. Cao and Prof. Chenming Hu in 2001.\n");
        ChkFprintf(fplog, "\n");
        ChkFprintf(fplog, "++++++++++ BSIM4 PARAMETER CHECKING BELOW ++++++++++\n");

// SRW -- modified version checking
        if (DEV.checkVersion(B4VERSION, model->BSIM4version))
        {
            ChkFprintf(fplog, "Warning: This model is %s, given version is %s.\n", B4VERSION, model->BSIM4version);
            WarnPrintf("Warning: This model is %s, given version is %s.\n", B4VERSION, model->BSIM4version);
        }
        ChkFprintf(fplog, "Model = %s\n", (char*)model->BSIM4modName);


        if ((here->BSIM4rgateMod == 2) || (here->BSIM4rgateMod == 3))
        {
            if ((here->BSIM4trnqsMod == 1) || (here->BSIM4acnqsMod == 1))
            {
                ChkFprintf(fplog, "Warning: You've selected both Rg and charge deficit NQS; select one only.\n");
                WarnPrintf("Warning: You've selected both Rg and charge deficit NQS; select one only.\n");
            }
        }


        if (model->BSIM4toxe <= 0.0)
        {
            ChkFprintf(fplog, "Fatal: Toxe = %g is not positive.\n",
                       model->BSIM4toxe);
            FatalPrintf("Fatal: Toxe = %g is not positive.\n", model->BSIM4toxe);
            Fatal_Flag = 1;
        }
        if (model->BSIM4toxp <= 0.0)
        {
            ChkFprintf(fplog, "Fatal: Toxp = %g is not positive.\n",
                       model->BSIM4toxp);
            FatalPrintf("Fatal: Toxp = %g is not positive.\n", model->BSIM4toxp);
            Fatal_Flag = 1;
        }

        if (model->BSIM4toxm <= 0.0)
        {
            ChkFprintf(fplog, "Fatal: Toxm = %g is not positive.\n",
                       model->BSIM4toxm);
            FatalPrintf("Fatal: Toxm = %g is not positive.\n", model->BSIM4toxm);
            Fatal_Flag = 1;
        }

        if (model->BSIM4toxref <= 0.0)
        {
            ChkFprintf(fplog, "Fatal: Toxref = %g is not positive.\n",
                       model->BSIM4toxref);
            FatalPrintf("Fatal: Toxref = %g is not positive.\n", model->BSIM4toxref);
            Fatal_Flag = 1;
        }

        if (pParam->BSIM4lpe0 < -pParam->BSIM4leff)
        {
            ChkFprintf(fplog, "Fatal: Lpe0 = %g is less than -Leff.\n",
                       pParam->BSIM4lpe0);
            FatalPrintf("Fatal: Lpe0 = %g is less than -Leff.\n",
                        pParam->BSIM4lpe0);
            Fatal_Flag = 1;
        }
        if (pParam->BSIM4lpeb < -pParam->BSIM4leff)
        {
            ChkFprintf(fplog, "Fatal: Lpeb = %g is less than -Leff.\n",
                       pParam->BSIM4lpeb);
            FatalPrintf("Fatal: Lpeb = %g is less than -Leff.\n",
                        pParam->BSIM4lpeb);
            Fatal_Flag = 1;
        }

        if (pParam->BSIM4phin < -0.4)
        {
            ChkFprintf(fplog, "Fatal: Phin = %g is less than -0.4.\n",
                       pParam->BSIM4phin);
            FatalPrintf("Fatal: Phin = %g is less than -0.4.\n",
                        pParam->BSIM4phin);
            Fatal_Flag = 1;
        }
        if (pParam->BSIM4ndep <= 0.0)
        {
            ChkFprintf(fplog, "Fatal: Ndep = %g is not positive.\n",
                       pParam->BSIM4ndep);
            FatalPrintf("Fatal: Ndep = %g is not positive.\n",
                        pParam->BSIM4ndep);
            Fatal_Flag = 1;
        }
        if (pParam->BSIM4nsub <= 0.0)
        {
            ChkFprintf(fplog, "Fatal: Nsub = %g is not positive.\n",
                       pParam->BSIM4nsub);
            FatalPrintf("Fatal: Nsub = %g is not positive.\n",
                        pParam->BSIM4nsub);
            Fatal_Flag = 1;
        }
        if (pParam->BSIM4ngate < 0.0)
        {
            ChkFprintf(fplog, "Fatal: Ngate = %g is not positive.\n",
                       pParam->BSIM4ngate);
            FatalPrintf("Fatal: Ngate = %g Ngate is not positive.\n",
                        pParam->BSIM4ngate);
            Fatal_Flag = 1;
        }
        if (pParam->BSIM4ngate > 1.e25)
        {
            ChkFprintf(fplog, "Fatal: Ngate = %g is too high.\n",
                       pParam->BSIM4ngate);
            FatalPrintf("Fatal: Ngate = %g Ngate is too high\n",
                        pParam->BSIM4ngate);
            Fatal_Flag = 1;
        }
        if (pParam->BSIM4xj <= 0.0)
        {
            ChkFprintf(fplog, "Fatal: Xj = %g is not positive.\n",
                       pParam->BSIM4xj);
            FatalPrintf("Fatal: Xj = %g is not positive.\n", pParam->BSIM4xj);
            Fatal_Flag = 1;
        }

        if (pParam->BSIM4dvt1 < 0.0)
        {
            ChkFprintf(fplog, "Fatal: Dvt1 = %g is negative.\n",
                       pParam->BSIM4dvt1);
            FatalPrintf("Fatal: Dvt1 = %g is negative.\n", pParam->BSIM4dvt1);
            Fatal_Flag = 1;
        }

        if (pParam->BSIM4dvt1w < 0.0)
        {
            ChkFprintf(fplog, "Fatal: Dvt1w = %g is negative.\n",
                       pParam->BSIM4dvt1w);
            FatalPrintf("Fatal: Dvt1w = %g is negative.\n", pParam->BSIM4dvt1w);
            Fatal_Flag = 1;
        }

        if (pParam->BSIM4w0 == -pParam->BSIM4weff)
        {
            ChkFprintf(fplog, "Fatal: (W0 + Weff) = 0 causing divided-by-zero.\n");
            FatalPrintf("Fatal: (W0 + Weff) = 0 causing divided-by-zero.\n");
            Fatal_Flag = 1;
        }

        if (pParam->BSIM4dsub < 0.0)
        {
            ChkFprintf(fplog, "Fatal: Dsub = %g is negative.\n", pParam->BSIM4dsub);
            FatalPrintf("Fatal: Dsub = %g is negative.\n", pParam->BSIM4dsub);
            Fatal_Flag = 1;
        }
        if (pParam->BSIM4b1 == -pParam->BSIM4weff)
        {
            ChkFprintf(fplog, "Fatal: (B1 + Weff) = 0 causing divided-by-zero.\n");
            FatalPrintf("Fatal: (B1 + Weff) = 0 causing divided-by-zero.\n");
            Fatal_Flag = 1;
        }
        if (pParam->BSIM4u0temp <= 0.0)
        {
            ChkFprintf(fplog, "Fatal: u0 at current temperature = %g is not positive.\n", pParam->BSIM4u0temp);
            FatalPrintf("Fatal: u0 at current temperature = %g is not positive.\n",
                        pParam->BSIM4u0temp);
            Fatal_Flag = 1;
        }

        if (pParam->BSIM4delta < 0.0)
        {
            ChkFprintf(fplog, "Fatal: Delta = %g is less than zero.\n",
                       pParam->BSIM4delta);
            FatalPrintf("Fatal: Delta = %g is less than zero.\n", pParam->BSIM4delta);
            Fatal_Flag = 1;
        }

        if (pParam->BSIM4vsattemp <= 0.0)
        {
            ChkFprintf(fplog, "Fatal: Vsat at current temperature = %g is not positive.\n", pParam->BSIM4vsattemp);
            FatalPrintf("Fatal: Vsat at current temperature = %g is not positive.\n",
                        pParam->BSIM4vsattemp);
            Fatal_Flag = 1;
        }

        if (pParam->BSIM4pclm <= 0.0)
        {
            ChkFprintf(fplog, "Fatal: Pclm = %g is not positive.\n", pParam->BSIM4pclm);
            FatalPrintf("Fatal: Pclm = %g is not positive.\n", pParam->BSIM4pclm);
            Fatal_Flag = 1;
        }

        if (pParam->BSIM4drout < 0.0)
        {
            ChkFprintf(fplog, "Fatal: Drout = %g is negative.\n", pParam->BSIM4drout);
            FatalPrintf("Fatal: Drout = %g is negative.\n", pParam->BSIM4drout);
            Fatal_Flag = 1;
        }

        if (pParam->BSIM4pscbe2 <= 0.0)
        {
            ChkFprintf(fplog, "Warning: Pscbe2 = %g is not positive.\n",
                       pParam->BSIM4pscbe2);
            WarnPrintf("Warning: Pscbe2 = %g is not positive.\n", pParam->BSIM4pscbe2);
        }

        if (here->BSIM4nf < 1.0)
        {
            ChkFprintf(fplog, "Fatal: Number of finger = %g is smaller than one.\n", here->BSIM4nf);
            FatalPrintf("Fatal: Number of finger = %g is smaller than one.\n", here->BSIM4nf);
            Fatal_Flag = 1;
        }

        if ((here->BSIM4l + model->BSIM4xl) <= model->BSIM4xgl)
        {
            ChkFprintf(fplog, "Fatal: The parameter xgl must be smaller than Ldrawn+XL.\n");
            FatalPrintf("Fatal: The parameter xgl must be smaller than Ldrawn+XL.\n");
            Fatal_Flag = 1;
        }
        if (model->BSIM4ngcon < 1.0)
        {
            ChkFprintf(fplog, "Fatal: The parameter ngcon cannot be smaller than one.\n");
            FatalPrintf("Fatal: The parameter ngcon cannot be smaller than one.\n");
            Fatal_Flag = 1;
        }
        if ((model->BSIM4ngcon != 1.0) && (model->BSIM4ngcon != 2.0))
        {
            model->BSIM4ngcon = 1.0;
            ChkFprintf(fplog, "Warning: Ngcon must be equal to one or two; reset to 1.0.\n");
            WarnPrintf("Warning: Ngcon must be equal to one or two; reset to 1.0.\n");
        }

        if (model->BSIM4gbmin < 1.0e-20)
        {
            ChkFprintf(fplog, "Warning: Gbmin = %g is too small.\n",
                       model->BSIM4gbmin);
            WarnPrintf("Warning: Gbmin = %g is too small.\n", model->BSIM4gbmin);
        }

        if (pParam->BSIM4noff < 0.1)
        {
            ChkFprintf(fplog, "Warning: Noff = %g is too small.\n",
                       pParam->BSIM4noff);
            WarnPrintf("Warning: Noff = %g is too small.\n", pParam->BSIM4noff);
        }
        if (pParam->BSIM4noff > 4.0)
        {
            ChkFprintf(fplog, "Warning: Noff = %g is too large.\n",
                       pParam->BSIM4noff);
            WarnPrintf("Warning: Noff = %g is too large.\n", pParam->BSIM4noff);
        }

        if (pParam->BSIM4voffcv < -0.5)
        {
            ChkFprintf(fplog, "Warning: Voffcv = %g is too small.\n",
                       pParam->BSIM4voffcv);
            WarnPrintf("Warning: Voffcv = %g is too small.\n", pParam->BSIM4voffcv);
        }
        if (pParam->BSIM4voffcv > 0.5)
        {
            ChkFprintf(fplog, "Warning: Voffcv = %g is too large.\n",
                       pParam->BSIM4voffcv);
            WarnPrintf("Warning: Voffcv = %g is too large.\n", pParam->BSIM4voffcv);
        }

        /* Check capacitance parameters */
        if (pParam->BSIM4clc < 0.0)
        {
            ChkFprintf(fplog, "Fatal: Clc = %g is negative.\n", pParam->BSIM4clc);
            FatalPrintf("Fatal: Clc = %g is negative.\n", pParam->BSIM4clc);
            Fatal_Flag = 1;
        }

        if (pParam->BSIM4moin < 5.0)
        {
            ChkFprintf(fplog, "Warning: Moin = %g is too small.\n",
                       pParam->BSIM4moin);
            WarnPrintf("Warning: Moin = %g is too small.\n", pParam->BSIM4moin);
        }
        if (pParam->BSIM4moin > 25.0)
        {
            ChkFprintf(fplog, "Warning: Moin = %g is too large.\n",
                       pParam->BSIM4moin);
            WarnPrintf("Warning: Moin = %g is too large.\n", pParam->BSIM4moin);
        }
        if(model->BSIM4capMod ==2)
        {
            if (pParam->BSIM4acde < 0.4)
            {
                ChkFprintf(fplog, "Warning:  Acde = %g is too small.\n",
                           pParam->BSIM4acde);
                WarnPrintf("Warning: Acde = %g is too small.\n", pParam->BSIM4acde);
            }
            if (pParam->BSIM4acde > 1.6)
            {
                ChkFprintf(fplog, "Warning:  Acde = %g is too large.\n",
                           pParam->BSIM4acde);
                WarnPrintf("Warning: Acde = %g is too large.\n", pParam->BSIM4acde);
            }
        }

        if (model->BSIM4paramChk ==1)
        {
            /* Check L and W parameters */
            if (pParam->BSIM4leff <= 1.0e-9)
            {
                ChkFprintf(fplog, "Warning: Leff = %g <= 1.0e-9. Recommended Leff >= 1e-8 \n",
                           pParam->BSIM4leff);
                WarnPrintf("Warning: Leff = %g <= 1.0e-9. Recommended Leff >= 1e-8 \n",
                           pParam->BSIM4leff);
            }

            if (pParam->BSIM4leffCV <= 1.0e-9)
            {
                ChkFprintf(fplog, "Warning: Leff for CV = %g <= 1.0e-9. Recommended LeffCV >=1e-8 \n",
                           pParam->BSIM4leffCV);
                WarnPrintf("Warning: Leff for CV = %g <= 1.0e-9. Recommended LeffCV >=1e-8 \n",
                           pParam->BSIM4leffCV);
            }

            if (pParam->BSIM4weff <= 1.0e-9)
            {
                ChkFprintf(fplog, "Warning: Weff = %g <= 1.0e-9. Recommended Weff >=1e-7 \n",
                           pParam->BSIM4weff);
                WarnPrintf("Warning: Weff = %g <= 1.0e-9. Recommended Weff >=1e-7 \n",
                           pParam->BSIM4weff);
            }

            if (pParam->BSIM4weffCV <= 1.0e-9)
            {
                ChkFprintf(fplog, "Warning: Weff for CV = %g <= 1.0e-9. Recommended WeffCV >= 1e-7 \n",
                           pParam->BSIM4weffCV);
                WarnPrintf("Warning: Weff for CV = %g <= 1.0e-9. Recommended WeffCV >= 1e-7 \n",
                           pParam->BSIM4weffCV);
            }

            /* Check threshold voltage parameters */
            if (model->BSIM4toxe < 1.0e-10)
            {
                ChkFprintf(fplog, "Warning: Toxe = %g is less than 1A. Recommended Toxe >= 5A\n",
                           model->BSIM4toxe);
                WarnPrintf("Warning: Toxe = %g is less than 1A. Recommended Toxe >= 5A\n", model->BSIM4toxe);
            }
            if (model->BSIM4toxp < 1.0e-10)
            {
                ChkFprintf(fplog, "Warning: Toxp = %g is less than 1A. Recommended Toxp >= 5A\n",
                           model->BSIM4toxp);
                WarnPrintf("Warning: Toxp = %g is less than 1A. Recommended Toxp >= 5A\n", model->BSIM4toxp);
            }
            if (model->BSIM4toxm < 1.0e-10)
            {
                ChkFprintf(fplog, "Warning: Toxm = %g is less than 1A. Recommended Toxm >= 5A\n",
                           model->BSIM4toxm);
                WarnPrintf("Warning: Toxm = %g is less than 1A. Recommended Toxm >= 5A\n", model->BSIM4toxm);
            }

            if (pParam->BSIM4ndep <= 1.0e12)
            {
                ChkFprintf(fplog, "Warning: Ndep = %g may be too small.\n",
                           pParam->BSIM4ndep);
                WarnPrintf("Warning: Ndep = %g may be too small.\n",
                           pParam->BSIM4ndep);
            }
            else if (pParam->BSIM4ndep >= 1.0e21)
            {
                ChkFprintf(fplog, "Warning: Ndep = %g may be too large.\n",
                           pParam->BSIM4ndep);
                WarnPrintf("Warning: Ndep = %g may be too large.\n",
                           pParam->BSIM4ndep);
            }

            if (pParam->BSIM4nsub <= 1.0e14)
            {
                ChkFprintf(fplog, "Warning: Nsub = %g may be too small.\n",
                           pParam->BSIM4nsub);
                WarnPrintf("Warning: Nsub = %g may be too small.\n",
                           pParam->BSIM4nsub);
            }
            else if (pParam->BSIM4nsub >= 1.0e21)
            {
                ChkFprintf(fplog, "Warning: Nsub = %g may be too large.\n",
                           pParam->BSIM4nsub);
                WarnPrintf("Warning: Nsub = %g may be too large.\n",
                           pParam->BSIM4nsub);
            }

            if ((pParam->BSIM4ngate > 0.0) &&
                    (pParam->BSIM4ngate <= 1.e18))
            {
                ChkFprintf(fplog, "Warning: Ngate = %g is less than 1.E18cm^-3.\n",
                           pParam->BSIM4ngate);
                WarnPrintf("Warning: Ngate = %g is less than 1.E18cm^-3.\n",
                           pParam->BSIM4ngate);
            }

            if (pParam->BSIM4dvt0 < 0.0)
            {
                ChkFprintf(fplog, "Warning: Dvt0 = %g is negative.\n",
                           pParam->BSIM4dvt0);
                WarnPrintf("Warning: Dvt0 = %g is negative.\n", pParam->BSIM4dvt0);
            }

            if (fabs(1.0e-6 / (pParam->BSIM4w0 + pParam->BSIM4weff)) > 10.0)
            {
                ChkFprintf(fplog, "Warning: (W0 + Weff) may be too small.\n");
                WarnPrintf("Warning: (W0 + Weff) may be too small.\n");
            }

            /* Check subthreshold parameters */
            if (pParam->BSIM4nfactor < 0.0)
            {
                ChkFprintf(fplog, "Warning: Nfactor = %g is negative.\n",
                           pParam->BSIM4nfactor);
                WarnPrintf("Warning: Nfactor = %g is negative.\n", pParam->BSIM4nfactor);
            }
            if (pParam->BSIM4cdsc < 0.0)
            {
                ChkFprintf(fplog, "Warning: Cdsc = %g is negative.\n",
                           pParam->BSIM4cdsc);
                WarnPrintf("Warning: Cdsc = %g is negative.\n", pParam->BSIM4cdsc);
            }
            if (pParam->BSIM4cdscd < 0.0)
            {
                ChkFprintf(fplog, "Warning: Cdscd = %g is negative.\n",
                           pParam->BSIM4cdscd);
                WarnPrintf("Warning: Cdscd = %g is negative.\n", pParam->BSIM4cdscd);
            }
            /* Check DIBL parameters */
            if (pParam->BSIM4eta0 < 0.0)
            {
                ChkFprintf(fplog, "Warning: Eta0 = %g is negative.\n",
                           pParam->BSIM4eta0);
                WarnPrintf("Warning: Eta0 = %g is negative.\n", pParam->BSIM4eta0);
            }

            /* Check Abulk parameters */
            if (fabs(1.0e-6 / (pParam->BSIM4b1 + pParam->BSIM4weff)) > 10.0)
            {
                ChkFprintf(fplog, "Warning: (B1 + Weff) may be too small.\n");
                WarnPrintf("Warning: (B1 + Weff) may be too small.\n");
            }


            /* Check Saturation parameters */
            if (pParam->BSIM4a2 < 0.01)
            {
                ChkFprintf(fplog, "Warning: A2 = %g is too small. Set to 0.01.\n", pParam->BSIM4a2);
                WarnPrintf("Warning: A2 = %g is too small. Set to 0.01.\n",
                           pParam->BSIM4a2);
                pParam->BSIM4a2 = 0.01;
            }
            else if (pParam->BSIM4a2 > 1.0)
            {
                ChkFprintf(fplog, "Warning: A2 = %g is larger than 1. A2 is set to 1 and A1 is set to 0.\n",
                           pParam->BSIM4a2);
                WarnPrintf("Warning: A2 = %g is larger than 1. A2 is set to 1 and A1 is set to 0.\n",
                           pParam->BSIM4a2);
                pParam->BSIM4a2 = 1.0;
                pParam->BSIM4a1 = 0.0;

            }

            if (pParam->BSIM4prwg < 0.0)
            {
                ChkFprintf(fplog, "Warning: Prwg = %g is negative. Set to zero.\n",
                           pParam->BSIM4prwg);
                WarnPrintf("Warning: Prwg = %g is negative. Set to zero.\n",
                           pParam->BSIM4prwg);
                pParam->BSIM4prwg = 0.0;
            }
            if (pParam->BSIM4rdsw < 0.0)
            {
                ChkFprintf(fplog, "Warning: Rdsw = %g is negative. Set to zero.\n",
                           pParam->BSIM4rdsw);
                WarnPrintf("Warning: Rdsw = %g is negative. Set to zero.\n",
                           pParam->BSIM4rdsw);
                pParam->BSIM4rdsw = 0.0;
                pParam->BSIM4rds0 = 0.0;
            }
            if (pParam->BSIM4rds0 < 0.0)
            {
                ChkFprintf(fplog, "Warning: Rds at current temperature = %g is negative. Set to zero.\n",
                           pParam->BSIM4rds0);
                WarnPrintf("Warning: Rds at current temperature = %g is negative. Set to zero.\n",
                           pParam->BSIM4rds0);
                pParam->BSIM4rds0 = 0.0;
            }

            if (pParam->BSIM4rdswmin < 0.0)
            {
                ChkFprintf(fplog, "Warning: Rdswmin at current temperature = %g is negative. Set to zero.\n",
                           pParam->BSIM4rdswmin);
                WarnPrintf("Warning: Rdswmin at current temperature = %g is negative. Set to zero.\n",
                           pParam->BSIM4rdswmin);
                pParam->BSIM4rdswmin = 0.0;
            }

            if (pParam->BSIM4vsattemp < 1.0e3)
            {
                ChkFprintf(fplog, "Warning: Vsat at current temperature = %g may be too small.\n", pParam->BSIM4vsattemp);
                WarnPrintf("Warning: Vsat at current temperature = %g may be too small.\n", pParam->BSIM4vsattemp);
            }

            if (pParam->BSIM4fprout < 0.0)
            {
                ChkFprintf(fplog, "Fatal: fprout = %g is negative.\n",
                           pParam->BSIM4fprout);
                FatalPrintf("Fatal: fprout = %g is negative.\n", pParam->BSIM4fprout);
                Fatal_Flag = 1;
            }
            if (pParam->BSIM4pdits < 0.0)
            {
                ChkFprintf(fplog, "Fatal: pdits = %g is negative.\n",
                           pParam->BSIM4pdits);
                FatalPrintf("Fatal: pdits = %g is negative.\n", pParam->BSIM4pdits);
                Fatal_Flag = 1;
            }
            if (model->BSIM4pditsl < 0.0)
            {
                ChkFprintf(fplog, "Fatal: pditsl = %g is negative.\n",
                           model->BSIM4pditsl);
                FatalPrintf("Fatal: pditsl = %g is negative.\n", model->BSIM4pditsl);
                Fatal_Flag = 1;
            }

            if (pParam->BSIM4pdibl1 < 0.0)
            {
                ChkFprintf(fplog, "Warning: Pdibl1 = %g is negative.\n",
                           pParam->BSIM4pdibl1);
                WarnPrintf("Warning: Pdibl1 = %g is negative.\n", pParam->BSIM4pdibl1);
            }
            if (pParam->BSIM4pdibl2 < 0.0)
            {
                ChkFprintf(fplog, "Warning: Pdibl2 = %g is negative.\n",
                           pParam->BSIM4pdibl2);
                WarnPrintf("Warning: Pdibl2 = %g is negative.\n", pParam->BSIM4pdibl2);
            }

            if (pParam->BSIM4nigbinv <= 0.0)
            {
                ChkFprintf(fplog, "Fatal: nigbinv = %g is non-positive.\n",
                           pParam->BSIM4nigbinv);
                FatalPrintf("Fatal: nigbinv = %g is non-positive.\n", pParam->BSIM4nigbinv);
                Fatal_Flag = 1;
            }
            if (pParam->BSIM4nigbacc <= 0.0)
            {
                ChkFprintf(fplog, "Fatal: nigbacc = %g is non-positive.\n",
                           pParam->BSIM4nigbacc);
                FatalPrintf("Fatal: nigbacc = %g is non-positive.\n", pParam->BSIM4nigbacc);
                Fatal_Flag = 1;
            }
            if (pParam->BSIM4nigc <= 0.0)
            {
                ChkFprintf(fplog, "Fatal: nigc = %g is non-positive.\n",
                           pParam->BSIM4nigc);
                FatalPrintf("Fatal: nigc = %g is non-positive.\n", pParam->BSIM4nigc);
                Fatal_Flag = 1;
            }
            if (pParam->BSIM4poxedge <= 0.0)
            {
                ChkFprintf(fplog, "Fatal: poxedge = %g is non-positive.\n",
                           pParam->BSIM4poxedge);
                FatalPrintf("Fatal: poxedge = %g is non-positive.\n", pParam->BSIM4poxedge);
                Fatal_Flag = 1;
            }
            if (pParam->BSIM4pigcd <= 0.0)
            {
                ChkFprintf(fplog, "Fatal: pigcd = %g is non-positive.\n",
                           pParam->BSIM4pigcd);
                FatalPrintf("Fatal: pigcd = %g is non-positive.\n", pParam->BSIM4pigcd);
                Fatal_Flag = 1;
            }


            /* Check overlap capacitance parameters */
            if (model->BSIM4cgdo < 0.0)
            {
                ChkFprintf(fplog, "Warning: cgdo = %g is negative. Set to zero.\n", model->BSIM4cgdo);
                WarnPrintf("Warning: cgdo = %g is negative. Set to zero.\n", model->BSIM4cgdo);
                model->BSIM4cgdo = 0.0;
            }
            if (model->BSIM4cgso < 0.0)
            {
                ChkFprintf(fplog, "Warning: cgso = %g is negative. Set to zero.\n", model->BSIM4cgso);
                WarnPrintf("Warning: cgso = %g is negative. Set to zero.\n", model->BSIM4cgso);
                model->BSIM4cgso = 0.0;
            }

            if (model->BSIM4tnoia < 0.0)
            {
                ChkFprintf(fplog, "Warning: tnoia = %g is negative. Set to zero.\n", model->BSIM4tnoia);
                WarnPrintf("Warning: tnoia = %g is negative. Set to zero.\n", model->BSIM4tnoia);
                model->BSIM4tnoia = 0.0;
            }
            if (model->BSIM4tnoib < 0.0)
            {
                ChkFprintf(fplog, "Warning: tnoib = %g is negative. Set to zero.\n", model->BSIM4tnoib);
                WarnPrintf("Warning: tnoib = %g is negative. Set to zero.\n", model->BSIM4tnoib);
                model->BSIM4tnoib = 0.0;
            }

            if (model->BSIM4ntnoi < 0.0)
            {
                ChkFprintf(fplog, "Warning: ntnoi = %g is negative. Set to zero.\n", model->BSIM4ntnoi);
                WarnPrintf("Warning: ntnoi = %g is negative. Set to zero.\n", model->BSIM4ntnoi);
                model->BSIM4ntnoi = 0.0;
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

