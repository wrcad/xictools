
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

/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: Min-Chie Jeng.
Modified by Weidong Liu (1997-1998).
* Revision 3.2 1998/6/16  18:00:00  Weidong 
* BSIM3v3.2 release
**********/

#include "stdio.h"
#include "b3defs.h"


#define B3modName GENmodName

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
B3dev::checkModel(sB3model *model, sB3instance *here, sCKT*)
{
struct bsim3SizeDependParam *pParam;
int Fatal_Flag = 0;
FILE *fplog;

bool name_shown = false;

fplog = 0;
if (true)
//    if ((fplog = fopen("b3v3check.log", "w")) != NULL)
    {   pParam = here->pParam;
        ChkFprintf(fplog, "BSIM3V3 Parameter Check\n");
        ChkFprintf(fplog, "Model = %s\n", (const char*)model->B3modName);
        ChkFprintf(fplog, "W = %g, L = %g\n", here->B3w, here->B3l);
            

            if (pParam->B3nlx < -pParam->B3leff)
            {   ChkFprintf(fplog, "Fatal: Nlx = %g is less than -Leff.\n",
                        pParam->B3nlx);
                FatalPrintf("Fatal: Nlx = %g is less than -Leff.\n",
                        pParam->B3nlx);
                Fatal_Flag = 1;
            }

        if (model->B3tox <= 0.0)
        {   ChkFprintf(fplog, "Fatal: Tox = %g is not positive.\n",
                    model->B3tox);
            FatalPrintf("Fatal: Tox = %g is not positive.\n", model->B3tox);
            Fatal_Flag = 1;
        }

        if (model->B3toxm <= 0.0)
        {   ChkFprintf(fplog, "Fatal: Toxm = %g is not positive.\n",
                    model->B3toxm);
            FatalPrintf("Fatal: Toxm = %g is not positive.\n", model->B3toxm);
            Fatal_Flag = 1;
        }

        if (pParam->B3npeak <= 0.0)
        {   ChkFprintf(fplog, "Fatal: Nch = %g is not positive.\n",
                    pParam->B3npeak);
            FatalPrintf("Fatal: Nch = %g is not positive.\n",
                   pParam->B3npeak);
            Fatal_Flag = 1;
        }
        if (pParam->B3nsub <= 0.0)
        {   ChkFprintf(fplog, "Fatal: Nsub = %g is not positive.\n",
                    pParam->B3nsub);
            FatalPrintf("Fatal: Nsub = %g is not positive.\n",
                   pParam->B3nsub);
            Fatal_Flag = 1;
        }
        if (pParam->B3ngate < 0.0)
        {   ChkFprintf(fplog, "Fatal: Ngate = %g is not positive.\n",
                    pParam->B3ngate);
            FatalPrintf("Fatal: Ngate = %g Ngate is not positive.\n",
                   pParam->B3ngate);
            Fatal_Flag = 1;
        }
        if (pParam->B3ngate > 1.e25)
        {   ChkFprintf(fplog, "Fatal: Ngate = %g is too high.\n",
                    pParam->B3ngate);
            FatalPrintf("Fatal: Ngate = %g Ngate is too high\n",
                   pParam->B3ngate);
            Fatal_Flag = 1;
        }
        if (pParam->B3xj <= 0.0)
        {   ChkFprintf(fplog, "Fatal: Xj = %g is not positive.\n",
                    pParam->B3xj);
            FatalPrintf("Fatal: Xj = %g is not positive.\n", pParam->B3xj);
            Fatal_Flag = 1;
        }

        if (pParam->B3dvt1 < 0.0)
        {   ChkFprintf(fplog, "Fatal: Dvt1 = %g is negative.\n",
                    pParam->B3dvt1);   
            FatalPrintf("Fatal: Dvt1 = %g is negative.\n", pParam->B3dvt1);   
            Fatal_Flag = 1;
        }
            
        if (pParam->B3dvt1w < 0.0)
        {   ChkFprintf(fplog, "Fatal: Dvt1w = %g is negative.\n",
                    pParam->B3dvt1w);
            FatalPrintf("Fatal: Dvt1w = %g is negative.\n", pParam->B3dvt1w);
            Fatal_Flag = 1;
        }
            
        if (pParam->B3w0 == -pParam->B3weff)
        {   ChkFprintf(fplog, "Fatal: (W0 + Weff) = 0 causing divided-by-zero.\n");
            FatalPrintf("Fatal: (W0 + Weff) = 0 causing divided-by-zero.\n");
            Fatal_Flag = 1;
        }   

        if (pParam->B3dsub < 0.0)
        {   ChkFprintf(fplog, "Fatal: Dsub = %g is negative.\n", pParam->B3dsub);
            FatalPrintf("Fatal: Dsub = %g is negative.\n", pParam->B3dsub);
            Fatal_Flag = 1;
        }
        if (pParam->B3b1 == -pParam->B3weff)
        {   ChkFprintf(fplog, "Fatal: (B1 + Weff) = 0 causing divided-by-zero.\n");
            FatalPrintf("Fatal: (B1 + Weff) = 0 causing divided-by-zero.\n");
            Fatal_Flag = 1;
        }  
        if (pParam->B3u0temp <= 0.0)
        {   ChkFprintf(fplog, "Fatal: u0 at current temperature = %g is not positive.\n", pParam->B3u0temp);
            FatalPrintf("Fatal: u0 at current temperature = %g is not positive.\n",
                   pParam->B3u0temp);
            Fatal_Flag = 1;
        }
    
/* Check delta parameter */      
        if (pParam->B3delta < 0.0)
        {   ChkFprintf(fplog, "Fatal: Delta = %g is less than zero.\n",
                    pParam->B3delta);
            FatalPrintf("Fatal: Delta = %g is less than zero.\n", pParam->B3delta);
            Fatal_Flag = 1;
        }      

        if (pParam->B3vsattemp <= 0.0)
        {   ChkFprintf(fplog, "Fatal: Vsat at current temperature = %g is not positive.\n", pParam->B3vsattemp);
            FatalPrintf("Fatal: Vsat at current temperature = %g is not positive.\n",
                   pParam->B3vsattemp);
            Fatal_Flag = 1;
        }
/* Check Rout parameters */
        if (pParam->B3pclm <= 0.0)
        {   ChkFprintf(fplog, "Fatal: Pclm = %g is not positive.\n", pParam->B3pclm);
            FatalPrintf("Fatal: Pclm = %g is not positive.\n", pParam->B3pclm);
            Fatal_Flag = 1;
        }

        if (pParam->B3drout < 0.0)
        {   ChkFprintf(fplog, "Fatal: Drout = %g is negative.\n", pParam->B3drout);
            FatalPrintf("Fatal: Drout = %g is negative.\n", pParam->B3drout);
            Fatal_Flag = 1;
        }

        if (pParam->B3pscbe2 <= 0.0)
        {   ChkFprintf(fplog, "Warning: Pscbe2 = %g is not positive.\n",
                    pParam->B3pscbe2);
            WarnPrintf("Warning: Pscbe2 = %g is not positive.\n", pParam->B3pscbe2);
        }

      if (model->B3unitLengthSidewallJctCap > 0.0 || 
            model->B3unitLengthGateSidewallJctCap > 0.0)
      {
        if (here->B3drainPerimeter < pParam->B3weff)
        {   ChkFprintf(fplog, "Warning: Pd = %g is less than W.\n",
                    here->B3drainPerimeter);
// SRW
if (here->B3drainPerimeterGiven)
            WarnPrintf("Warning: Pd = %g is less than W.\n",
                    here->B3drainPerimeter);
        }
        if (here->B3sourcePerimeter < pParam->B3weff)
        {   ChkFprintf(fplog, "Warning: Ps = %g is less than W.\n",
                    here->B3sourcePerimeter);
// SRW
if (here->B3sourcePerimeterGiven)
            WarnPrintf("Warning: Ps = %g is less than W.\n",
                    here->B3sourcePerimeter);
        }
      }

        if (pParam->B3noff < 0.1)
        {   ChkFprintf(fplog, "Warning: Noff = %g is too small.\n",
                    pParam->B3noff);
            WarnPrintf("Warning: Noff = %g is too small.\n", pParam->B3noff);
        }
        if (pParam->B3noff > 4.0)
        {   ChkFprintf(fplog, "Warning: Noff = %g is too large.\n",
                    pParam->B3noff);
            WarnPrintf("Warning: Noff = %g is too large.\n", pParam->B3noff);
        }

        if (pParam->B3voffcv < -0.5)
        {   ChkFprintf(fplog, "Warning: Voffcv = %g is too small.\n",
                    pParam->B3voffcv);
            WarnPrintf("Warning: Voffcv = %g is too small.\n", pParam->B3voffcv);
        }
        if (pParam->B3voffcv > 0.5)
        {   ChkFprintf(fplog, "Warning: Voffcv = %g is too large.\n",
                    pParam->B3voffcv);
            WarnPrintf("Warning: Voffcv = %g is too large.\n", pParam->B3voffcv);
        }

        if (model->B3ijth < 0.0)
        {   ChkFprintf(fplog, "Fatal: Ijth = %g cannot be negative.\n",
                    model->B3ijth);
            FatalPrintf("Fatal: Ijth = %g cannot be negative.\n", model->B3ijth);
            Fatal_Flag = 1;
        }

/* Check capacitance parameters */
        if (pParam->B3clc < 0.0)
        {   ChkFprintf(fplog, "Fatal: Clc = %g is negative.\n", pParam->B3clc);
            FatalPrintf("Fatal: Clc = %g is negative.\n", pParam->B3clc);
            Fatal_Flag = 1;
        }      

        if (pParam->B3moin < 5.0)
        {   ChkFprintf(fplog, "Warning: Moin = %g is too small.\n",
                    pParam->B3moin);
            WarnPrintf("Warning: Moin = %g is too small.\n", pParam->B3moin);
        }
        if (pParam->B3moin > 25.0)
        {   ChkFprintf(fplog, "Warning: Moin = %g is too large.\n",
                    pParam->B3moin);
            WarnPrintf("Warning: Moin = %g is too large.\n", pParam->B3moin);
        }

        if (pParam->B3acde < 0.4)
        {   ChkFprintf(fplog, "Warning:  Acde = %g is too small.\n",
                    pParam->B3acde);
            WarnPrintf("Warning: Acde = %g is too small.\n", pParam->B3acde);
        }
        if (pParam->B3acde > 1.6)
        {   ChkFprintf(fplog, "Warning:  Acde = %g is too large.\n",
                    pParam->B3acde);
            WarnPrintf("Warning: Acde = %g is too large.\n", pParam->B3acde);
        }

      if (model->B3paramChk ==1)
      {
/* Check L and W parameters */ 
        if (pParam->B3leff <= 5.0e-8)
        {   ChkFprintf(fplog, "Warning: Leff = %g may be too small.\n",
                    pParam->B3leff);
            WarnPrintf("Warning: Leff = %g may be too small.\n",
                    pParam->B3leff);
        }    
        
        if (pParam->B3leffCV <= 5.0e-8)
        {   ChkFprintf(fplog, "Warning: Leff for CV = %g may be too small.\n",
                    pParam->B3leffCV);
            WarnPrintf("Warning: Leff for CV = %g may be too small.\n",
                   pParam->B3leffCV);
        }  
        
        if (pParam->B3weff <= 1.0e-7)
        {   ChkFprintf(fplog, "Warning: Weff = %g may be too small.\n",
                    pParam->B3weff);
            WarnPrintf("Warning: Weff = %g may be too small.\n",
                   pParam->B3weff);
        }             
        
        if (pParam->B3weffCV <= 1.0e-7)
        {   ChkFprintf(fplog, "Warning: Weff for CV = %g may be too small.\n",
                    pParam->B3weffCV);
            WarnPrintf("Warning: Weff for CV = %g may be too small.\n",
                   pParam->B3weffCV);
        }        
        
/* Check threshold voltage parameters */
        if (pParam->B3nlx < 0.0)
        {   ChkFprintf(fplog, "Warning: Nlx = %g is negative.\n", pParam->B3nlx);
            WarnPrintf("Warning: Nlx = %g is negative.\n", pParam->B3nlx);
        }
         if (model->B3tox < 1.0e-9)
        {   ChkFprintf(fplog, "Warning: Tox = %g is less than 10A.\n",
                    model->B3tox);
            WarnPrintf("Warning: Tox = %g is less than 10A.\n", model->B3tox);
        }

        if (pParam->B3npeak <= 1.0e15)
        {   ChkFprintf(fplog, "Warning: Nch = %g may be too small.\n",
                    pParam->B3npeak);
            WarnPrintf("Warning: Nch = %g may be too small.\n",
                   pParam->B3npeak);
        }
        else if (pParam->B3npeak >= 1.0e21)
        {   ChkFprintf(fplog, "Warning: Nch = %g may be too large.\n",
                    pParam->B3npeak);
            WarnPrintf("Warning: Nch = %g may be too large.\n",
                   pParam->B3npeak);
        }

         if (pParam->B3nsub <= 1.0e14)
        {   ChkFprintf(fplog, "Warning: Nsub = %g may be too small.\n",
                    pParam->B3nsub);
            WarnPrintf("Warning: Nsub = %g may be too small.\n",
                   pParam->B3nsub);
        }
        else if (pParam->B3nsub >= 1.0e21)
        {   ChkFprintf(fplog, "Warning: Nsub = %g may be too large.\n",
                    pParam->B3nsub);
            WarnPrintf("Warning: Nsub = %g may be too large.\n",
                   pParam->B3nsub);
        }

        if ((pParam->B3ngate > 0.0) &&
            (pParam->B3ngate <= 1.e18))
        {   ChkFprintf(fplog, "Warning: Ngate = %g is less than 1.E18cm^-3.\n",
                    pParam->B3ngate);
            WarnPrintf("Warning: Ngate = %g is less than 1.E18cm^-3.\n",
                   pParam->B3ngate);
        }
       
        if (pParam->B3dvt0 < 0.0)
        {   ChkFprintf(fplog, "Warning: Dvt0 = %g is negative.\n",
                    pParam->B3dvt0);   
            WarnPrintf("Warning: Dvt0 = %g is negative.\n", pParam->B3dvt0);   
        }
            
        if (fabs(1.0e-6 / (pParam->B3w0 + pParam->B3weff)) > 10.0)
        {   ChkFprintf(fplog, "Warning: (W0 + Weff) may be too small.\n");
            WarnPrintf("Warning: (W0 + Weff) may be too small.\n");
        }

/* Check subthreshold parameters */
        if (pParam->B3nfactor < 0.0)
        {   ChkFprintf(fplog, "Warning: Nfactor = %g is negative.\n",
                    pParam->B3nfactor);
            WarnPrintf("Warning: Nfactor = %g is negative.\n", pParam->B3nfactor);
        }
        if (pParam->B3cdsc < 0.0)
        {   ChkFprintf(fplog, "Warning: Cdsc = %g is negative.\n",
                    pParam->B3cdsc);
            WarnPrintf("Warning: Cdsc = %g is negative.\n", pParam->B3cdsc);
        }
        if (pParam->B3cdscd < 0.0)
        {   ChkFprintf(fplog, "Warning: Cdscd = %g is negative.\n",
                    pParam->B3cdscd);
            WarnPrintf("Warning: Cdscd = %g is negative.\n", pParam->B3cdscd);
        }
/* Check DIBL parameters */
        if (pParam->B3eta0 < 0.0)
        {   ChkFprintf(fplog, "Warning: Eta0 = %g is negative.\n",
                    pParam->B3eta0); 
            WarnPrintf("Warning: Eta0 = %g is negative.\n", pParam->B3eta0); 
        }
              
/* Check Abulk parameters */        
        if (fabs(1.0e-6 / (pParam->B3b1 + pParam->B3weff)) > 10.0)
        {   ChkFprintf(fplog, "Warning: (B1 + Weff) may be too small.\n");
            WarnPrintf("Warning: (B1 + Weff) may be too small.\n");
        }    
    

/* Check Saturation parameters */
        if (pParam->B3a2 < 0.01)
        {   ChkFprintf(fplog, "Warning: A2 = %g is too small. Set to 0.01.\n", pParam->B3a2);
            WarnPrintf("Warning: A2 = %g is too small. Set to 0.01.\n",
                   pParam->B3a2);
            pParam->B3a2 = 0.01;
        }
        else if (pParam->B3a2 > 1.0)
        {   ChkFprintf(fplog, "Warning: A2 = %g is larger than 1. A2 is set to 1 and A1 is set to 0.\n",
                    pParam->B3a2);
            WarnPrintf("Warning: A2 = %g is larger than 1. A2 is set to 1 and A1 is set to 0.\n",
                   pParam->B3a2);
            pParam->B3a2 = 1.0;
            pParam->B3a1 = 0.0;

        }

        if (pParam->B3rdsw < 0.0)
        {   ChkFprintf(fplog, "Warning: Rdsw = %g is negative. Set to zero.\n",
                    pParam->B3rdsw);
            WarnPrintf("Warning: Rdsw = %g is negative. Set to zero.\n",
                   pParam->B3rdsw);
            pParam->B3rdsw = 0.0;
            pParam->B3rds0 = 0.0;
        }
        else if ((pParam->B3rds0 > 0.0) && (pParam->B3rds0 < 0.001))
        {   ChkFprintf(fplog, "Warning: Rds at current temperature = %g is less than 0.001 ohm. Set to zero.\n",
                    pParam->B3rds0);
            WarnPrintf("Warning: Rds at current temperature = %g is less than 0.001 ohm. Set to zero.\n",
                   pParam->B3rds0);
            pParam->B3rds0 = 0.0;
        }
        if (pParam->B3vsattemp < 1.0e3)
        {   ChkFprintf(fplog, "Warning: Vsat at current temperature = %g may be too small.\n", pParam->B3vsattemp);
            WarnPrintf("Warning: Vsat at current temperature = %g may be too small.\n", pParam->B3vsattemp);
        }

        if (pParam->B3pdibl1 < 0.0)
        {   ChkFprintf(fplog, "Warning: Pdibl1 = %g is negative.\n",
                    pParam->B3pdibl1);
            WarnPrintf("Warning: Pdibl1 = %g is negative.\n", pParam->B3pdibl1);
        }
        if (pParam->B3pdibl2 < 0.0)
        {   ChkFprintf(fplog, "Warning: Pdibl2 = %g is negative.\n",
                    pParam->B3pdibl2);
            WarnPrintf("Warning: Pdibl2 = %g is negative.\n", pParam->B3pdibl2);
        }
/* Check overlap capacitance parameters */
        if (model->B3cgdo < 0.0)
        {   ChkFprintf(fplog, "Warning: cgdo = %g is negative. Set to zero.\n", model->B3cgdo);
            WarnPrintf("Warning: cgdo = %g is negative. Set to zero.\n", model->B3cgdo);
            model->B3cgdo = 0.0;
        }      
        if (model->B3cgso < 0.0)
        {   ChkFprintf(fplog, "Warning: cgso = %g is negative. Set to zero.\n", model->B3cgso);
            WarnPrintf("Warning: cgso = %g is negative. Set to zero.\n", model->B3cgso);
            model->B3cgso = 0.0;
        }      
        if (model->B3cgbo < 0.0)
        {   ChkFprintf(fplog, "Warning: cgbo = %g is negative. Set to zero.\n", model->B3cgbo);
            WarnPrintf("Warning: cgbo = %g is negative. Set to zero.\n", model->B3cgbo);
            model->B3cgbo = 0.0;
        }

     }/* loop for the parameter check for warning messages */      
// SRW
if (fplog)
        fclose(fplog);
    }
    else
    {   fprintf(stderr, "Warning: Can't open log file. Parameter checking skipped.\n");
    }

    return(Fatal_Flag);
}

