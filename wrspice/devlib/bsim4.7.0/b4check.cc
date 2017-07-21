
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
 $Id: b4check.cc,v 1.3 2011/12/18 01:15:37 stevew Exp $
 *========================================================================*/

/**** BSIM4.7.0 Released by Darsen Lu 04/08/2011 ****/

/**********
 * Copyright 2006 Regents of the University of California. All rights reserved.
 * File: b4check.c of BSIM4.7.0.
 * Author: 2000 Weidong Liu
 * Authors: 2001- Xuemei Xi, Mohan Dunga, Ali Niknejad, Chenming Hu.
 * Authors: 2006- Mohan Dunga, Ali Niknejad, Chenming Hu
 * Authors: 2007- Mohan Dunga, Wenwei Yang, Ali Niknejad, Chenming Hu
 * Project Director: Prof. Chenming Hu.
 * Modified by Xuemei Xi, 04/06/2001.
 * Modified by Xuemei Xi, 10/05/2001.
 * Modified by Xuemei Xi, 11/15/2002.
 * Modified by Xuemei Xi, 05/09/2003.
 * Modified by Xuemei Xi, 03/04/2004.
 * Modified by Xuemei Xi, 07/29/2005.
 * Modified by Mohan Dunga, 12/13/2006
 * Modified by Mohan Dunga, Wenwei Yang, 05/18/2007.
 * Modified by  Wenwei Yang, 07/31/2008 .
 * Modified by Tanvir Morshed, Darsen Lu 03/27/2011
 **********/

#include <stdio.h>
#include "b4defs.h"


#define BSIM4modName GENmodName
#define CKTtemp CKTcurTask->TSKtemp

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
BSIM4dev::checkModel(sBSIM4model *model, sBSIM4instance *here, sCKT *ckt)
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
        ChkFprintf(fplog, "Developed by Xuemei (Jane) Xi, Mohan Dunga, Prof. Ali Niknejad and Prof. Chenming Hu in 2003.\n");
        ChkFprintf(fplog, "\n");
        ChkFprintf(fplog, "++++++++++ BSIM4 PARAMETER CHECKING BELOW ++++++++++\n");

// SRW -- modified version checking
        if (DEV.checkVersion(B4VERSION, model->BSIM4version))
        {
            ChkFprintf(fplog, "Warning: This model is %s, given version is %s.\n", B4VERSION, model->BSIM4version);
            WarnPrintf("Warning: This model is %s, given version is %s.\n", B4VERSION, model->BSIM4version);
        }
        ChkFprintf(fplog, "Model = %s\n", (const char*)model->BSIM4modName);


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
        if (model->BSIM4eot <= 0.0)
        {
            ChkFprintf(fplog, "Fatal: EOT = %g is not positive.\n",
                       model->BSIM4eot);
            FatalPrintf("Fatal: EOT = %g is not positive.\n", model->BSIM4eot);
            Fatal_Flag = 1;
        }
        if (model->BSIM4epsrgate < 0.0)
        {
            ChkFprintf(fplog, "Fatal: Epsrgate = %g is not positive.\n",
                       model->BSIM4epsrgate);
            FatalPrintf("Fatal: Epsrgate = %g is not positive.\n", model->BSIM4epsrgate);
            Fatal_Flag = 1;
        }
        if (model->BSIM4epsrsub < 0.0)
        {
            ChkFprintf(fplog, "Fatal: Epsrsub = %g is not positive.\n",
                       model->BSIM4epsrsub);
            FatalPrintf("Fatal: Epsrsub = %g is not positive.\n", model->BSIM4epsrsub);
            Fatal_Flag = 1;
        }
        if (model->BSIM4easub < 0.0)
        {
            ChkFprintf(fplog, "Fatal: Easub = %g is not positive.\n",
                       model->BSIM4easub);
            FatalPrintf("Fatal: Easub = %g is not positive.\n", model->BSIM4easub);
            Fatal_Flag = 1;
        }
        if (model->BSIM4ni0sub <= 0.0)
        {
            ChkFprintf(fplog, "Fatal: Ni0sub = %g is not positive.\n",
                       model->BSIM4ni0sub);
            FatalPrintf("Fatal: Easub = %g is not positive.\n", model->BSIM4ni0sub);
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
        if (model->BSIM4lintnoi > pParam->BSIM4leff/2)
        {
            ChkFprintf(fplog, "Fatal: Lintnoi = %g is too large - Leff for noise is negative.\n",
                       model->BSIM4lintnoi);
            FatalPrintf("Fatal: Lintnoi = %g is too large - Leff for noise is negative.\n",
                        model->BSIM4lintnoi);
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
        if (pParam->BSIM4ndep <= 0.0)
        {
            ChkFprintf(fplog, "Fatal: Ndep = %g is not positive.\n",
                       pParam->BSIM4ndep);
            FatalPrintf("Fatal: Ndep = %g is not positive.\n",
                        pParam->BSIM4ndep);
            Fatal_Flag = 1;
        }
        if (pParam->BSIM4phi <= 0.0)
        {
            ChkFprintf(fplog, "Fatal: Phi = %g is not positive. Please check Phin and Ndep\n",
                       pParam->BSIM4phi);
            ChkFprintf(fplog, "       Phin = %g  Ndep = %g \n",
                       pParam->BSIM4phin, pParam->BSIM4ndep);
            FatalPrintf("Fatal: Phi = %g is not positive. Please check Phin and Ndep\n",
                        pParam->BSIM4phi);
            FatalPrintf("       Phin = %g  Ndep = %g \n",
                        pParam->BSIM4phin, pParam->BSIM4ndep);
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
        if (here->BSIM4u0temp <= 0.0)
        {
            ChkFprintf(fplog, "Fatal: u0 at current temperature = %g is not positive.\n", here->BSIM4u0temp);
            FatalPrintf("Fatal: u0 at current temperature = %g is not positive.\n",
                        here->BSIM4u0temp);
            Fatal_Flag = 1;
        }

        if (pParam->BSIM4delta < 0.0)
        {
            ChkFprintf(fplog, "Fatal: Delta = %g is less than zero.\n",
                       pParam->BSIM4delta);
            FatalPrintf("Fatal: Delta = %g is less than zero.\n", pParam->BSIM4delta);
            Fatal_Flag = 1;
        }

        if (here->BSIM4vsattemp <= 0.0)
        {
            ChkFprintf(fplog, "Fatal: Vsat at current temperature = %g is not positive.\n", here->BSIM4vsattemp);
            FatalPrintf("Fatal: Vsat at current temperature = %g is not positive.\n",
                        here->BSIM4vsattemp);
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

        if (here->BSIM4nf < 1.0)
        {
            ChkFprintf(fplog, "Fatal: Number of finger = %g is smaller than one.\n", here->BSIM4nf);
            FatalPrintf("Fatal: Number of finger = %g is smaller than one.\n", here->BSIM4nf);
            Fatal_Flag = 1;
        }

        if((here->BSIM4sa > 0.0) && (here->BSIM4sb > 0.0) &&
                ((here->BSIM4nf == 1.0) || ((here->BSIM4nf > 1.0) && (here->BSIM4sd > 0.0))) )
        {
            if (model->BSIM4saref <= 0.0)
            {
                ChkFprintf(fplog, "Fatal: SAref = %g is not positive.\n",model->BSIM4saref);
                FatalPrintf("Fatal: SAref = %g is not positive.\n",model->BSIM4saref);
                Fatal_Flag = 1;
            }
            if (model->BSIM4sbref <= 0.0)
            {
                ChkFprintf(fplog, "Fatal: SBref = %g is not positive.\n",model->BSIM4sbref);
                FatalPrintf("Fatal: SBref = %g is not positive.\n",model->BSIM4sbref);
                Fatal_Flag = 1;
            }
        }

        if ((here->BSIM4l + model->BSIM4xl) <= model->BSIM4xgl)
        {
            ChkFprintf(fplog, "Fatal: The parameter xgl must be smaller than Ldrawn+XL.\n");
            FatalPrintf("Fatal: The parameter xgl must be smaller than Ldrawn+XL.\n");
            Fatal_Flag = 1;
        }
        if (here->BSIM4ngcon < 1.0)
        {
            ChkFprintf(fplog, "Fatal: The parameter ngcon cannot be smaller than one.\n");
            FatalPrintf("Fatal: The parameter ngcon cannot be smaller than one.\n");
            Fatal_Flag = 1;
        }
        if ((here->BSIM4ngcon != 1.0) && (here->BSIM4ngcon != 2.0))
        {
            here->BSIM4ngcon = 1.0;
            ChkFprintf(fplog, "Warning: Ngcon must be equal to one or two; reset to 1.0.\n");
            WarnPrintf("Warning: Ngcon must be equal to one or two; reset to 1.0.\n");
        }

        if (model->BSIM4gbmin < 1.0e-20)
        {
            ChkFprintf(fplog, "Warning: Gbmin = %g is too small.\n",
                       model->BSIM4gbmin);
            WarnPrintf("Warning: Gbmin = %g is too small.\n", model->BSIM4gbmin);
        }

        /* Check saturation parameters */
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

        /* Check gate current parameters */
        if (model->BSIM4igbMod)
        {
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
        }
        if (model->BSIM4igcMod)
        {
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
        }

        /* Check capacitance parameters */
        if (pParam->BSIM4clc < 0.0)
        {
            ChkFprintf(fplog, "Fatal: Clc = %g is negative.\n", pParam->BSIM4clc);
            FatalPrintf("Fatal: Clc = %g is negative.\n", pParam->BSIM4clc);
            Fatal_Flag = 1;
        }

        /* Check overlap capacitance parameters */
        if (pParam->BSIM4ckappas < 0.02)
        {
            ChkFprintf(fplog, "Warning: ckappas = %g is too small. Set to 0.02\n",
                       pParam->BSIM4ckappas);
            WarnPrintf("Warning: ckappas = %g is too small.\n", pParam->BSIM4ckappas);
            pParam->BSIM4ckappas = 0.02;
        }
        if (pParam->BSIM4ckappad < 0.02)
        {
            ChkFprintf(fplog, "Warning: ckappad = %g is too small. Set to 0.02\n",
                       pParam->BSIM4ckappad);
            WarnPrintf("Warning: ckappad = %g is too small.\n", pParam->BSIM4ckappad);
            pParam->BSIM4ckappad = 0.02;
        }

        if (model->BSIM4vtss < 0.0)
        {
            ChkFprintf(fplog, "Fatal: Vtss = %g is negative.\n",
                       model->BSIM4vtss);
            FatalPrintf("Fatal: Vtss = %g is negative.\n",
                        model->BSIM4vtss);
            Fatal_Flag = 1;
        }
        if (model->BSIM4vtsd < 0.0)
        {
            ChkFprintf(fplog, "Fatal: Vtsd = %g is negative.\n",
                       model->BSIM4vtsd);
            FatalPrintf("Fatal: Vtsd = %g is negative.\n",
                        model->BSIM4vtsd);
            Fatal_Flag = 1;
        }
        if (model->BSIM4vtssws < 0.0)
        {
            ChkFprintf(fplog, "Fatal: Vtssws = %g is negative.\n",
                       model->BSIM4vtssws);
            FatalPrintf("Fatal: Vtssws = %g is negative.\n",
                        model->BSIM4vtssws);
            Fatal_Flag = 1;
        }
        if (model->BSIM4vtsswd < 0.0)
        {
            ChkFprintf(fplog, "Fatal: Vtsswd = %g is negative.\n",
                       model->BSIM4vtsswd);
            FatalPrintf("Fatal: Vtsswd = %g is negative.\n",
                        model->BSIM4vtsswd);
            Fatal_Flag = 1;
        }
        if (model->BSIM4vtsswgs < 0.0)
        {
            ChkFprintf(fplog, "Fatal: Vtsswgs = %g is negative.\n",
                       model->BSIM4vtsswgs);
            FatalPrintf("Fatal: Vtsswgs = %g is negative.\n",
                        model->BSIM4vtsswgs);
            Fatal_Flag = 1;
        }
        if (model->BSIM4vtsswgd < 0.0)
        {
            ChkFprintf(fplog, "Fatal: Vtsswgd = %g is negative.\n",
                       model->BSIM4vtsswgd);
            FatalPrintf("Fatal: Vtsswgd = %g is negative.\n",
                        model->BSIM4vtsswgd);
            Fatal_Flag = 1;
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

            if (fabs(1.0e-8 / (pParam->BSIM4w0 + pParam->BSIM4weff)) > 10.0)
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
            if (here->BSIM4eta0 < 0.0)
            {
                ChkFprintf(fplog, "Warning: Eta0 = %g is negative.\n",
                           here->BSIM4eta0);
                WarnPrintf("Warning: Eta0 = %g is negative.\n", here->BSIM4eta0);
            }

            /* Check Abulk parameters */
            if (fabs(1.0e-8 / (pParam->BSIM4b1 + pParam->BSIM4weff)) > 10.0)
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

            if (pParam->BSIM4pscbe2 <= 0.0)
            {
                ChkFprintf(fplog, "Warning: Pscbe2 = %g is not positive.\n",
                           pParam->BSIM4pscbe2);
                WarnPrintf("Warning: Pscbe2 = %g is not positive.\n", pParam->BSIM4pscbe2);
            }

            if (pParam->BSIM4vsattemp < 1.0e3)
            {
                ChkFprintf(fplog, "Warning: Vsat at current temperature = %g may be too small.\n", pParam->BSIM4vsattemp);
                WarnPrintf("Warning: Vsat at current temperature = %g may be too small.\n", pParam->BSIM4vsattemp);
            }

            if((model->BSIM4lambdaGiven) && (pParam->BSIM4lambda > 0.0) )
            {
                if (pParam->BSIM4lambda > 1.0e-9)
                {
                    ChkFprintf(fplog, "Warning: Lambda = %g may be too large.\n", pParam->BSIM4lambda);
                    WarnPrintf("Warning: Lambda = %g may be too large.\n", pParam->BSIM4lambda);
                }
            }

            if((model->BSIM4vtlGiven) && (pParam->BSIM4vtl > 0.0) )
            {
                if (pParam->BSIM4vtl < 6.0e4)
                {
                    ChkFprintf(fplog, "Warning: Thermal velocity vtl = %g may be too small.\n", pParam->BSIM4vtl);
                    WarnPrintf("Warning: Thermal velocity vtl = %g may be too small.\n", pParam->BSIM4vtl);
                }

                if (pParam->BSIM4xn < 3.0)
                {
                    ChkFprintf(fplog, "Warning: back scattering coeff xn = %g is too small.\n", pParam->BSIM4xn);
                    WarnPrintf("Warning: back scattering coeff xn = %g is too small. Reset to 3.0 \n", pParam->BSIM4xn);
                    pParam->BSIM4xn = 3.0;
                }

                if (model->BSIM4lc < 0.0)
                {
                    ChkFprintf(fplog, "Warning: back scattering coeff lc = %g is too small.\n", model->BSIM4lc);
                    WarnPrintf("Warning: back scattering coeff lc = %g is too small. Reset to 0.0\n", model->BSIM4lc);
                    pParam->BSIM4lc = 0.0;
                }
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

            /* Check stress effect parameters */
            if((here->BSIM4sa > 0.0) && (here->BSIM4sb > 0.0) &&
                    ((here->BSIM4nf == 1.0) || ((here->BSIM4nf > 1.0) && (here->BSIM4sd > 0.0))) )
            {
                if (model->BSIM4lodk2 <= 0.0)
                {
                    ChkFprintf(fplog, "Warning: LODK2 = %g is not positive.\n",model->BSIM4lodk2);
                    WarnPrintf("Warning: LODK2 = %g is not positive.\n",model->BSIM4lodk2);
                }
                if (model->BSIM4lodeta0 <= 0.0)
                {
                    ChkFprintf(fplog, "Warning: LODETA0 = %g is not positive.\n",model->BSIM4lodeta0);
                    WarnPrintf("Warning: LODETA0 = %g is not positive.\n",model->BSIM4lodeta0);
                }
            }

            /* Check gate resistance parameters */
            if (here->BSIM4rgateMod == 1)
            {
                if (model->BSIM4rshg <= 0.0)
                    WarnPrintf("Warning: rshg should be positive for rgateMod = 1.\n");
            }
            else if (here->BSIM4rgateMod == 2)
            {
                if (model->BSIM4rshg <= 0.0)
                    WarnPrintf("Warning: rshg <= 0.0 for rgateMod = 2.\n");
                else if (pParam->BSIM4xrcrg1 <= 0.0)
                    WarnPrintf("Warning: xrcrg1 <= 0.0 for rgateMod = 2.\n");
            }
            if (here->BSIM4rgateMod == 3)
            {
                if (model->BSIM4rshg <= 0.0)
                    WarnPrintf("Warning: rshg should be positive for rgateMod = 3.\n");
                else if (pParam->BSIM4xrcrg1 <= 0.0)
                    WarnPrintf("Warning: xrcrg1 should be positive for rgateMod = 3.\n");
            }

            /* Check capacitance parameters */
            if (pParam->BSIM4noff < 0.1)
            {
                ChkFprintf(fplog, "Warning: Noff = %g is too small.\n",
                           pParam->BSIM4noff);
                WarnPrintf("Warning: Noff = %g is too small.\n", pParam->BSIM4noff);
            }

            if (pParam->BSIM4voffcv < -0.5)
            {
                ChkFprintf(fplog, "Warning: Voffcv = %g is too small.\n",
                           pParam->BSIM4voffcv);
                WarnPrintf("Warning: Voffcv = %g is too small.\n", pParam->BSIM4voffcv);
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
                if (pParam->BSIM4acde < 0.1)
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
            if (model->BSIM4cgbo < 0.0)
            {
                ChkFprintf(fplog, "Warning: cgbo = %g is negative. Set to zero.\n", model->BSIM4cgbo);
                WarnPrintf("Warning: cgbo = %g is negative. Set to zero.\n", model->BSIM4cgbo);
                model->BSIM4cgbo = 0.0;
            }

            /* v4.7 */
            if (model->BSIM4tnoiMod == 1 || model->BSIM4tnoiMod == 2)
            {
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

                if (model->BSIM4rnoia < 0.0)
                {
                    ChkFprintf(fplog, "Warning: rnoia = %g is negative. Set to zero.\n", model->BSIM4rnoia);
                    WarnPrintf("Warning: rnoia = %g is negative. Set to zero.\n", model->BSIM4rnoia);
                    model->BSIM4rnoia = 0.0;
                }
                if (model->BSIM4rnoib < 0.0)
                {
                    ChkFprintf(fplog, "Warning: rnoib = %g is negative. Set to zero.\n", model->BSIM4rnoib);
                    WarnPrintf("Warning: rnoib = %g is negative. Set to zero.\n", model->BSIM4rnoib);
                    model->BSIM4rnoib = 0.0;
                }
            }

            /* v4.7 */
            if (model->BSIM4tnoiMod == 2)
            {
                if (model->BSIM4tnoic < 0.0)
                {
                    ChkFprintf(fplog, "Warning: tnoic = %g is negative. Set to zero.\n", model->BSIM4tnoic);
                    WarnPrintf("Warning: tnoic = %g is negative. Set to zero.\n", model->BSIM4tnoic);
                    model->BSIM4tnoic = 0.0;
                }
                if (model->BSIM4rnoic < 0.0)
                {
                    ChkFprintf(fplog, "Warning: rnoic = %g is negative. Set to zero.\n", model->BSIM4rnoic);
                    WarnPrintf("Warning: rnoic = %g is negative. Set to zero.\n", model->BSIM4rnoic);
                    model->BSIM4rnoic = 0.0;
                }
            }

            /* Limits of Njs and Njd modified in BSIM4.7 */
            if (model->BSIM4SjctEmissionCoeff < 0.1)
            {
                ChkFprintf(fplog, "Warning: Njs = %g is less than 0.1. Setting Njs to 0.1.\n", model->BSIM4SjctEmissionCoeff);
                WarnPrintf("Warning: Njs = %g is less than 0.1. Setting Njs to 0.1.\n", model->BSIM4SjctEmissionCoeff);
                model->BSIM4SjctEmissionCoeff = 0.1;
            }
            else if (model->BSIM4SjctEmissionCoeff < 0.7)
            {
                ChkFprintf(fplog, "Warning: Njs = %g is less than 0.7.\n", model->BSIM4SjctEmissionCoeff);
                WarnPrintf("Warning: Njs = %g is less than 0.7.\n", model->BSIM4SjctEmissionCoeff);
            }
            if (model->BSIM4DjctEmissionCoeff < 0.1)
            {
                ChkFprintf(fplog, "Warning: Njd = %g is less than 0.1. Setting Njd to 0.1.\n", model->BSIM4DjctEmissionCoeff);
                WarnPrintf("Warning: Njd = %g is less than 0.1. Setting Njd to 0.1.\n", model->BSIM4DjctEmissionCoeff);
                model->BSIM4DjctEmissionCoeff = 0.1;
            }
            else if (model->BSIM4DjctEmissionCoeff < 0.7)
            {
                ChkFprintf(fplog, "Warning: Njd = %g is less than 0.7.\n", model->BSIM4DjctEmissionCoeff);
                WarnPrintf("Warning: Njd = %g is less than 0.7.\n", model->BSIM4DjctEmissionCoeff);
            }

            if (model->BSIM4njtsstemp < 0.0)
            {
                ChkFprintf(fplog, "Warning: Njts = %g is negative at temperature = %g.\n",
                           model->BSIM4njtsstemp, ckt->CKTtemp);
                WarnPrintf("Warning: Njts = %g is negative at temperature = %g.\n",
                           model->BSIM4njtsstemp, ckt->CKTtemp);
            }
            if (model->BSIM4njtsswstemp < 0.0)
            {
                ChkFprintf(fplog, "Warning: Njtssw = %g is negative at temperature = %g.\n",
                           model->BSIM4njtsswstemp, ckt->CKTtemp);
                WarnPrintf("Warning: Njtssw = %g is negative at temperature = %g.\n",
                           model->BSIM4njtsswstemp, ckt->CKTtemp);
            }
            if (model->BSIM4njtsswgstemp < 0.0)
            {
                ChkFprintf(fplog, "Warning: Njtsswg = %g is negative at temperature = %g.\n",
                           model->BSIM4njtsswgstemp, ckt->CKTtemp);
                WarnPrintf("Warning: Njtsswg = %g is negative at temperature = %g.\n",
                           model->BSIM4njtsswgstemp, ckt->CKTtemp);
            }

            if (model->BSIM4njtsdGiven && model->BSIM4njtsdtemp < 0.0)
            {
                ChkFprintf(fplog, "Warning: Njtsd = %g is negative at temperature = %g.\n",
                           model->BSIM4njtsdtemp, ckt->CKTtemp);
                WarnPrintf("Warning: Njtsd = %g is negative at temperature = %g.\n",
                           model->BSIM4njtsdtemp, ckt->CKTtemp);
            }
            if (model->BSIM4njtsswdGiven && model->BSIM4njtsswdtemp < 0.0)
            {
                ChkFprintf(fplog, "Warning: Njtsswd = %g is negative at temperature = %g.\n",
                           model->BSIM4njtsswdtemp, ckt->CKTtemp);
                WarnPrintf("Warning: Njtsswd = %g is negative at temperature = %g.\n",
                           model->BSIM4njtsswdtemp, ckt->CKTtemp);
            }
            if (model->BSIM4njtsswgdGiven && model->BSIM4njtsswgdtemp < 0.0)
            {
                ChkFprintf(fplog, "Warning: Njtsswgd = %g is negative at temperature = %g.\n",
                           model->BSIM4njtsswgdtemp, ckt->CKTtemp);
                WarnPrintf("Warning: Njtsswgd = %g is negative at temperature = %g.\n",
                           model->BSIM4njtsswgdtemp, ckt->CKTtemp);
            }

            if (model->BSIM4ntnoi < 0.0)
            {
                ChkFprintf(fplog, "Warning: ntnoi = %g is negative. Set to zero.\n", model->BSIM4ntnoi);
                WarnPrintf("Warning: ntnoi = %g is negative. Set to zero.\n", model->BSIM4ntnoi);
                model->BSIM4ntnoi = 0.0;
            }

            /* diode model */
            if (model->BSIM4SbulkJctBotGradingCoeff >= 0.99)
            {
                ChkFprintf(fplog, "Warning: MJS = %g is too big. Set to 0.99.\n", model->BSIM4SbulkJctBotGradingCoeff);
                WarnPrintf("Warning: MJS = %g is too big. Set to 0.99.\n", model->BSIM4SbulkJctBotGradingCoeff);
                model->BSIM4SbulkJctBotGradingCoeff = 0.99;
            }
            if (model->BSIM4SbulkJctSideGradingCoeff >= 0.99)
            {
                ChkFprintf(fplog, "Warning: MJSWS = %g is too big. Set to 0.99.\n", model->BSIM4SbulkJctSideGradingCoeff);
                WarnPrintf("Warning: MJSWS = %g is too big. Set to 0.99.\n", model->BSIM4SbulkJctSideGradingCoeff);
                model->BSIM4SbulkJctSideGradingCoeff = 0.99;
            }
            if (model->BSIM4SbulkJctGateSideGradingCoeff >= 0.99)
            {
                ChkFprintf(fplog, "Warning: MJSWGS = %g is too big. Set to 0.99.\n", model->BSIM4SbulkJctGateSideGradingCoeff);
                WarnPrintf("Warning: MJSWGS = %g is too big. Set to 0.99.\n", model->BSIM4SbulkJctGateSideGradingCoeff);
                model->BSIM4SbulkJctGateSideGradingCoeff = 0.99;
            }

            if (model->BSIM4DbulkJctBotGradingCoeff >= 0.99)
            {
                ChkFprintf(fplog, "Warning: MJD = %g is too big. Set to 0.99.\n", model->BSIM4DbulkJctBotGradingCoeff);
                WarnPrintf("Warning: MJD = %g is too big. Set to 0.99.\n", model->BSIM4DbulkJctBotGradingCoeff);
                model->BSIM4DbulkJctBotGradingCoeff = 0.99;
            }
            if (model->BSIM4DbulkJctSideGradingCoeff >= 0.99)
            {
                ChkFprintf(fplog, "Warning: MJSWD = %g is too big. Set to 0.99.\n", model->BSIM4DbulkJctSideGradingCoeff);
                WarnPrintf("Warning: MJSWD = %g is too big. Set to 0.99.\n", model->BSIM4DbulkJctSideGradingCoeff);
                model->BSIM4DbulkJctSideGradingCoeff = 0.99;
            }
            if (model->BSIM4DbulkJctGateSideGradingCoeff >= 0.99)
            {
                ChkFprintf(fplog, "Warning: MJSWGD = %g is too big. Set to 0.99.\n", model->BSIM4DbulkJctGateSideGradingCoeff);
                WarnPrintf("Warning: MJSWGD = %g is too big. Set to 0.99.\n", model->BSIM4DbulkJctGateSideGradingCoeff);
                model->BSIM4DbulkJctGateSideGradingCoeff = 0.99;
            }
            if (model->BSIM4wpemod == 1)
            {
                if (model->BSIM4scref <= 0.0)
                {
                    ChkFprintf(fplog, "Warning: SCREF = %g is not positive. Set to 1e-6.\n", model->BSIM4scref);
                    WarnPrintf("Warning: SCREF = %g is not positive. Set to 1e-6.\n", model->BSIM4scref);
                    model->BSIM4scref = 1e-6;
                }
                /*Move these checks to temp.c for sceff calculation*/
                /*
                if (here->BSIM4sca < 0.0)
                {   ChkFprintf(fplog, "Warning: SCA = %g is negative. Set to 0.0.\n", here->BSIM4sca);
                    WarnPrintf("Warning: SCA = %g is negative. Set to 0.0.\n", here->BSIM4sca);
                    here->BSIM4sca = 0.0;
                }
                if (here->BSIM4scb < 0.0)
                {   ChkFprintf(fplog, "Warning: SCB = %g is negative. Set to 0.0.\n", here->BSIM4scb);
                    WarnPrintf("Warning: SCB = %g is negative. Set to 0.0.\n", here->BSIM4scb);
                    here->BSIM4scb = 0.0;
                }
                if (here->BSIM4scc < 0.0)
                {   ChkFprintf(fplog, "Warning: SCC = %g is negative. Set to 0.0.\n", here->BSIM4scc);
                    WarnPrintf("Warning: SCC = %g is negative. Set to 0.0.\n", here->BSIM4scc);
                    here->BSIM4scc = 0.0;
                }
                if (here->BSIM4sc < 0.0)
                {   ChkFprintf(fplog, "Warning: SC = %g is negative. Set to 0.0.\n", here->BSIM4sc);
                    WarnPrintf("Warning: SC = %g is negative. Set to 0.0.\n", here->BSIM4sc);
                    here->BSIM4sc = 0.0;
                }
                */
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

