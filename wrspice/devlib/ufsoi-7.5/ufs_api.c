
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

/* ****************************************************************************
 Copyright 1997: UNIVERSITY OF FLORIDA;  ALL RIGHTS RESERVED.
 Authors: Min-Chie Jeng and UF SOI Group 
          (Code evolved from UFSOI FD and NFD model routines in SOISPICE-4.41.)
 File: ufs_api.c
**************************************************************************** */

#include "ufs_api.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sys/types.h"
#include "sys/time.h"


int 
ufsInitModel(model, pEnv)
struct ufsAPI_ModelData *model;
struct ufsAPI_EnvData *pEnv;
{
double T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13;
double Ef, Efg, Efgb, Nldd, Tnom, TempRatio, SqrtTempRatio, Eg, Arg, Vtmnom;
double Coxf, Coxb, Cb, Tb, Vtmref, Efs, Phib;                                    /* 4.5 */
#define ESI SILICON_PERMITTIVITY

/* Begin `ufsoiInitModel' */

   Tnom = model->Tnom;
   TempRatio = Tnom / 300.15;
   SqrtTempRatio = sqrt(TempRatio);
   Eg = 1.16 - 7.02e-4 * Tnom * Tnom / (Tnom + 1108.0);
   Vtmnom = KoverQ * Tnom;
   Vtmref = KoverQ * 300.15;
   Arg = 1.1150877 / (2.0 * Vtmref) - Eg / (2.0 * Vtmnom);
   model->Xnin = 1.33e16 * TempRatio * SqrtTempRatio * exp(Arg);

   Tb = model->Tb;
   Coxf = model->Coxf = VACUUM_PERMITTIVITY*model->Kd / model->Toxf;             /* 7.0Y */
   Cb = model->Cb = SILICON_PERMITTIVITY / Tb;
   Coxb = model->Coxb = OXIDE_PERMITTIVITY / model->Toxb;

   model->Rf = 1.0 + ELECTRON_CHARGE * model->Nsf / Coxf;
   model->Rb = 1.0 + ELECTRON_CHARGE * model->Nsb / Coxb;
   if (model->NfdMod == 0)
   {   model->Xalpha = model->Rf + model->Rb * Cb * Coxb
                     / (Coxf * (Cb + model->Rb * Coxb));
       model->Xbeta = 1.0 + Cb / (Cb + model->Rb * Coxb);
       model->Lc = Tb * sqrt(Cb * model->Xbeta / (2.0
                 * Coxf * model->Xalpha));
       model->Phib = 2.0 * Vtmnom * log(model->Nbody / model->Xnin);             /* 4.5r */
   }
   else 
   {   model->Xalpha = 1.0 + Cb / Coxf;
       model->Xbeta = 1.0 + Cb / (Cb + model->Rb * Coxb);
       model->Lc = Tb * sqrt(Cb / (2.0 * Coxf * model->Xalpha));
       model->Phib =  2.0 * Vtmnom * log(model->Nbl / model->Xnin);              /* 4.5r */
       model->Phibb = 2.0 * Vtmnom * log(model->Nbh / model->Xnin);
   }

   model->Xalphab = 1.0 + Cb / (Coxb * (1.0 + Cb / Coxf));
   model->Lcb = Tb * sqrt(Cb / (2.0 * Coxb * model->Xalphab * (1.0 + Cb / (Cb
	      + model->Rf * Coxf))));
   model->Factor1 = Cb / Coxf * Tb * Tb * model->Xbeta;

   model->C = 1.0 / (Tb * Tb * (2.0 * Cb / Coxb + model->Rb));
   model->A = (model->Rb + Coxf / Cb * model->Rf
             * model->Rb + Coxf / Coxb * model->Rf)
	     * model->C;
   model->Aa = sqrt(2.0 * model->A);
   model->B = (Coxf / Cb * model->Rb + Coxf / Coxb) * model->C;

   model->Ccb = 1.0 / (Tb * Tb * (2.0 * Cb / Coxf + model->Rf));
   model->Ab = (model->Rf + Coxb / Cb * model->Rf
              * model->Rb + Coxb / Coxf * model->Rb)
	      * model->Ccb;
   model->Aab = sqrt(2.0 * model->Ab);
   model->Bb = (Coxb / Cb * model->Rf + Coxb / Coxf) * model->Ccb;

   Efs = model->Type * 0.5 * model->Phib;
   Efg = model->Type * model->Tpg * 0.5 * Eg;                                    /* 4.5 */
   if (model->WkfGiven)
   {   model->Lrsce = 0.0;                                                       /* 4.5r */
   }                                                                             /* 4.5r */
   else                                                                          /* 4.5r */
   {   model->Wkf = -Efg - Efs;                                                  /* 4.5 */
   }
   if (model->VfbfGiven)
   {   model->Nqff = (model->Wkf - model->Vfbf) * model->Coxf / ELECTRON_CHARGE; /* 4.5 */
       model->Nqfsw = 0.0;                                                       /* 4.5r */
       model->Lrsce = 0.0;                                                       /* 4.5r */
   }
   else
   {   model->Vfbf = model->Wkf - model->Nqff * ELECTRON_CHARGE / model->Coxf;
   }

   /*    if (model->NfdMod == 1) Efs = model->Type * 0.5 * model->Phibb;  */
   if (model->NfdMod > 0) Efs = model->Type * 0.5 * model->Phibb;                /* 6.0bulk */
   Efgb = model->Type * model->Tps * Vtmnom * log(model->Nsub / model->Xnin);
   model->Const5 = Efs + Efgb;
   if (!model->WkbGiven)
   {    model->Wkb = -model->Const5;
   }
   if (model->VfbbGiven)
   {   model->Nqfb = (model->Wkb - model->Vfbb) * model->Coxb / ELECTRON_CHARGE; /* 4.5 */
   }
   else
   {   model->Vfbb = model->Wkb - model->Nqfb * ELECTRON_CHARGE / model->Coxb;
   }
/*
   model->Vthf = model->Type * model->Vfbf + (1.0 + Cb / Coxf) * model->Phib
	       - 0.5 * model->Qb / Coxf;
   model->Vthb = model->Type * model->Vfbb - (Cb / Coxb) * model->Phib
	       - 0.5 * model->Qb / Coxb;
*/
   T1 =  Tnom / 300.0;
   model->Const1 = 0.57 + 0.43 * T1 * T1;                                        /* 4.5 */
   model->Const2 = 0.625 + 0.375 * T1;                                           /* 4.5 */
   T0 = 175.0 / Tnom; 
   T1 = exp(T0);
   model->Const3 = (T1 - 1.0 / T1) / (T1 + 1.0 / T1);

   model->Dum1 = sqrt(2.0 * ELECTRON_CHARGE * ESI * model->Nsub) / Coxb;

   if (model->Lldd > 0)
       Nldd = model->Nldd;
   else
       Nldd = model->Nds;
   T0 = Tnom / 300.15;                                                           /* 4.5r */
   T1 = 0.88 / pow(T0, 0.146);                                                   /* 4.5r */
   T2 = pow(T0, 2.4);                                                            /* 4.5r */
   T3 = pow(T0, 0.57);                                                           /* 4.5r */
   T4 = pow(Tnom, 2.33);                                                         /* 4.5r */
   T5 = Nldd / 1.26e23 / T2;                                                     /* 4.5r */
   T6 = Nldd / 2.35e23 / T2;                                                     /* 4.5r */
   if (model->Type > 0)                                                          /* 4.5r */
   {   model->Mumin = 5.4e-3 / T3 + 1.36e4 / T4 / (1.0 + pow(T6,T1));            /* 4.5r */
       model->Mumaj = 8.8e-3 / T3 + 7.40e4 / T4 / (1.0 + pow(T5,T1));            /* 4.5r */
       if(model->NfdMod > 0 )                                                    /* 6.0bulk */
       {T7 = model->Nbh / 1.26e23 / T2;                                          /* 4.5r */
	model->Mubh = 8.8e-3 / T3 + 7.40e4 / T4 / (1.0 + pow(T7,T1));            /* 4.5r */
       }                                                                         /* 4.5r */
   }                                                                             /* 4.5r */
   else                                                                          /* 4.5r */
   {   model->Mumin = 8.8e-3 / T3 + 7.40e4 / T4 / (1.0 + pow(T5,T1));            /* 4.5r */
       model->Mumaj = 5.4e-3 / T3 + 1.36e4 / T4 / (1.0 + pow(T6,T1));            /* 4.5r */
       if(model->NfdMod > 0)                                                     /* 6.0bulk */
       {T7 = model->Nbh / 2.35e23 / T2;                                          /* 4.5r */
	model->Mubh = 5.4e-3 / T3 + 1.36e4 / T4 / (1.0 + pow(T7,T1));            /* 4.5r */
       }                                                                         /* 4.5r */
   }                                                                             /* 4.5r */
   if (model->Thalo > 0.0)
   { model->Nbheff = model->Nhalo;
     model->Teff = model->Thalo;                                                 /* 4.5 */
   }
   else
   { model->Nbheff = model->Nbh;
     model->Teff = model->Tf - model->Tb;                                        /* 4.5 */
   }
   return 0;
}

int 
ufsInitInst(pModel, pInst, pEnv)
struct ufsAPI_ModelData  *pModel;
struct ufsAPI_InstData  *pInst;
struct ufsAPI_EnvData *pEnv;
{
double T1, Rldd, Xarg, Xarg1, Vtmnom, Tnom, Eg, Efs, Efg, Ef, T0, T8, T9, T6;    /* 4.5r */
double T7, T10, T2, T3, T4, T5, Ft0;                                             /* 4.5r */

    pInst->Leff = pInst->Length - pModel->Dl;
    pInst->Weff = pInst->Width - pModel->Dw;
    if(pInst->SourceJunctionPerimeter <= pInst->Weff)                            /* 4.5 */
       pInst->SourceJunctionPerimeter = pInst->Weff;                             /* 4.5 */
    if(pInst->DrainJunctionPerimeter <= pInst->Weff)                             /* 4.5 */
       pInst->DrainJunctionPerimeter = pInst->Weff;                              /* 4.5 */
    /* MCJ Error message */
    T1 = pModel->Aa * pInst->Leff;
    pInst->Expf = exp(T1);
    T1 = pModel->Aab * pInst->Leff;
    pInst->Expb = exp(T1);

    if (pModel->NfdMod > 0)                                                      /* 6.0bulk */
      {if (pModel->Lrsce == 0.0)                                                 /* 4.5r */
	 {pInst->Nblavg = pModel->Nbl;                                           /* 4.5r */
	 }                                                                       /* 4.5r */
       else                                                                      /* 4.5r */
	 {T1 = pModel->Nbh;                                                      /* 4.5r */
          if(pModel->Nhalo != 0.0) T1 = pModel->Nhalo;                           /* 4.5r */
          Xarg = pInst->Leff / pModel->Lrsce;                                    /* 4.5r */
          Xarg1 = Xarg * Xarg;                                                   /* 4.5r */
          if(Xarg1 > 80.0) Xarg1 = 80.0;                                         /* 4.5r */
          pInst->Nblavg = pModel->Nbl + 2.0 * (T1 - pModel->Nbl) /               /* 4.5r */
	                  Xarg * (1.0 - exp(-Xarg1));                            /* 4.5r */
	 }                                                                       /* 4.5r */
      }                                                                          /* 4.5r */
   else                                                                          /* 4.5r */
     {pInst->Nblavg = pModel->Nbody;                                             /* 4.5r */
     }                                                                           /* 4.5r */
   Tnom = pModel->Tnom;                                                          /* 4.5r */
   Eg = 1.16 - 7.02e-4 * Tnom * Tnom / (Tnom + 1108.0);                          /* 4.5r */
   Vtmnom = KoverQ * Tnom;                                                       /* 4.5r */
   pInst->Vbi = 0.5 * Eg + Vtmnom * log(pInst->Nblavg / pModel->Xnin);           /* 4.5r */
/*5.0   if (pInst->Vbi > Eg)                                                        4.5r */
/*5.0       pInst->Vbi = Eg;                                                        4.5r */
   pInst->Qb = -ELECTRON_CHARGE * pModel->Tb * pInst->Nblavg;                    /* 4.5r */
   pInst->Phib = 2.0 * Vtmnom * log(pInst->Nblavg / pModel->Xnin);               /* 4.5r */
   Efs = pModel->Type * 0.5 * pInst->Phib;                                       /* 4.5r */
   Efg = pModel->Type * pModel->Tpg * 0.5 * Eg;                                  /* 4.5r */
   pInst->Const4 = Efs + Efg;                                                    /* 4.5r */
   if (pModel->Lrsce == 0.0)                                                     /* 4.5r */
   {   pInst->Wkf = pModel->Wkf;                                                 /* 4.5r */
   }                                                                             /* 4.5r */
   else                                                                          /* 4.5r */
   {   pInst->Wkf = -Efg - Efs;                                                  /* 4.5r */
   }                                                                             /* 4.5r */
   pInst->Vfbf = pInst->Wkf - ELECTRON_CHARGE / pModel->Coxf *                   /* 4.5r */
               (pModel->Nqff + 2.0 * pModel->Nqfsw * pModel->Tb / pInst->Weff);  /* 4.5r */
   T0 = Tnom / 300.15;                                                           /* 4.5r */
   T1 = 0.88 / pow(T0, 0.146);                                                   /* 4.5r */
   T2 = pow(T0, 2.4);                                                            /* 4.5r */
   T3 = pow(T0, 0.57);                                                           /* 4.5r */
   T4 = pow(Tnom, 2.33);                                                         /* 4.5r */
   if (pModel->Type > 0)                                                         /* 4.5r */
   {   if (pModel->NfdMod == 0)                                                  /* 4.5r */
       { T5 = pModel->Nbody / 1.26e23 / T2;                                      /* 4.5r */
	 pInst->Mubody = 8.8e-3 / T3 + 7.40e4 / T4 / (1.0 + pow(T5,T1));         /* 4.5r */
         pInst->Uo = pModel->Uo;                                                 /* 4.5r */
	 pInst->Ft1 = pInst->Mubody;                                             /* 4.5r */
       }                                                                         /* 4.5r */
       else                                                                      /* 4.5r */
       { T5 = pModel->Nbl / 1.26e23 / T2;                                        /* 4.5r */
	 T6 = pInst->Nblavg/1.26e23 / T2;                                        /* 4.5r */
	 pInst->Mubody = 8.8e-3 / T3 + 7.40e4 / T4 / (1.0 + pow(T6,T1));         /* 4.5r */
	        Ft0    = 8.8e-3 / T3 + 7.40e4 / T4 / (1.0 + pow(T5,T1));         /* 4.5r */
	 pInst->Ft1 = pInst->Mubody;                                             /* 4.5r */
         pInst->Uo = pModel->Uo * pInst->Ft1 / Ft0;                              /* 4.5r */
       }                                                                         /* 4.5r */
   }                                                                             /* 4.5r */
   else                                                                          /* 4.5r */
   {   if (pModel->NfdMod == 0)                                                  /* 4.5r */
       { T5 = pModel->Nbody / 2.35e23 / T2;                                      /* 4.5r */
	 pInst->Mubody = 5.4e-3 / T3 + 1.36e4 / T4 / (1.0 + pow(T5,T1));         /* 4.5r */
         pInst->Uo = pModel->Uo;                                                 /* 4.5r */
	 pInst->Ft1 = pInst->Mubody;                                             /* 4.5r */
       }                                                                         /* 4.5r */
       else                                                                      /* 4.5r */
       { T5 = pModel->Nbl / 2.35e23 / T2;                                        /* 4.5r */
	 T6 = pInst->Nblavg/2.35e23 / T2;                                        /* 4.5r */
	 pInst->Mubody = 5.4e-3 / T3 + 1.36e4 / T4 / (1.0 + pow(T6,T1));         /* 4.5r */
	        Ft0    = 5.4e-3 / T3 + 1.36e4 / T4 / (1.0 + pow(T5,T1));         /* 4.5r */
	 pInst->Ft1 = pInst->Mubody;                                             /* 4.5r */
         pInst->Uo = pModel->Uo * pInst->Ft1 / Ft0;                              /* 4.5r */
       }                                                                         /* 4.5r */
   }                                                                             /* 4.5r */
    pInst->Vthf = pModel->Type * pInst->Vfbf + (1.0 + pModel->Cb / pModel->Coxf) /* 4.51 */
                * pInst->Phib - 0.5 * pInst->Qb / pModel->Coxf;                  /* 4.51 */
    pInst->Vthb = pModel->Type * pModel->Vfbb - (pModel->Cb / pModel->Coxb)      /* 4.51 */
                * pInst->Phib - 0.5 * pInst->Qb / pModel->Coxb;                  /* 4.51 */
    if (pModel->NfdMod == 0)
    {   if (pModel->Lldd > 0)
        {   Rldd = 1.0 / (pModel->Tb * pInst->Weff * ELECTRON_CHARGE
                 * pModel->Nldd * pModel->Mumaj);                                /* 4.5 */
        }
        else
        {   Rldd = 1.0 / (pModel->Tb * pInst->Weff * ELECTRON_CHARGE
                 * pModel->Nds * pModel->Mumaj);
        }
    }
    else
    {   if (pModel->Lldd > 0)
        {   Rldd = 1.0 / (pModel->Mumaj * pInst->Weff * ELECTRON_CHARGE          /* 4.5 */
                 * pModel->Nldd * (pModel->Tf - pModel->Thalo));                 /* 4.5 */
        }
        else
        {   Rldd = 1.0 / (pModel->Mumaj * pInst->Weff * ELECTRON_CHARGE          /* 4.5 */
                 * pModel->Nds * (pModel->Tf - pModel->Thalo));
        }
    }
    if (pModel->RdGiven)
    {   T1 = pModel->Rd / pInst->Weff + pModel->Lldd * Rldd;
    }
    else
    {   T1 = pModel->Rhosd * pInst->Nrd + pModel->Lldd * Rldd;
    }
    if (T1 > 0.0)
    {   pInst->DrainConductance = 1.0 / T1;
    }
    else
    {   pInst->DrainConductance = 0.0;
    }
    if (pModel->RsGiven)
    {   T1 = pModel->Rs / pInst->Weff + pModel->Lldd * Rldd;                     /* 4.5 */
    }
    else
    {   T1 = pModel->Rhosd * pInst->Nrs + pModel->Lldd * Rldd;                   /* 4.5 */
    }
    if (T1 > 0.0)
    {   pInst->SourceConductance = 1.0 / T1;
    }
    else
    {   pInst->SourceConductance = 0.0;
    }
    if (pModel->RbodyGiven)
    {   T1 = pModel->Rbody;                                                      /* 6.0 */
    }
    else
    {   T1 = pModel->Rhob * (pInst->Nrb + 0.345 * pInst->Weff / pInst->Leff);    /* 4.5 */
    }
    if (T1 > 0.0)
    {   pInst->BodyConductance = 1.0 / T1;
    }
    else
    {   pInst->BodyConductance = 0.0;
    }
    return 0;
}

int 
ufsTempEffect(pModel, pInst, pEnv, T, SH)                                        /* 4.5 */
struct ufsAPI_ModelData  *pModel;
struct ufsAPI_InstData  *pInst;
struct ufsAPI_EnvData *pEnv;
double T;
int SH;                                                                          /* 4.5 */
{
double T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10;
double Tnom, Vtm, RatioRef, SqrtRatioRef, Ratio, SqrtRatio, Arg, PstOld;
double Eg, Ef, Efg, Efgb, Nldd, Leff, Weff, WL, Coxf, Coxb, Vtmref;
double Igt, Ir0, Ir1, Igtb;                                                      /* 4.5 */
struct ufsTDModelData *pTempModel;

    if (fabs(pInst->Temperature - T) < 1.0e-4)
        return 0;                                                                /* 6.0 */
    Leff = pInst->Leff;
    Weff = pInst->Weff;
    WL = Weff * Leff;
    Coxf = pModel->Coxf;
    Coxb = pModel->Coxb;
    pTempModel = pInst->pTempModel;
    T0 = pModel->Tmax + pEnv->Temperature;
    if (T < pEnv->Temperature - 10.0)
        T = pEnv->Temperature - 10.0;
    else if (T > T0)
        T = T0;
    pInst->Temperature = T;
    Tnom = pModel->Tnom;
    Vtm = KoverQ * T;
    Eg = 1.16 - 7.02e-4 * T * T / (T + 1108.0);
    RatioRef = T / 300.15;
    SqrtRatioRef = sqrt(RatioRef);

    pTempModel->Vtm = KoverQ * T;
    pTempModel->Eg = 1.16 - 7.02e-4 * T * T / (T + 1108.0);                      /* 4.41 */
    Vtmref = KoverQ * 300.15;
    pTempModel->Qr = sqrt(2.0 * ELECTRON_CHARGE * ESI * pModel->Nbody * Vtm);
    Arg = 1.1150877 / (2.0 * Vtmref) - Eg / (2.0 * Vtm);
    pTempModel->Xnin = 1.33e16 * RatioRef * SqrtRatioRef * exp(Arg);
    pTempModel->Phib = 2.0 * Vtm * log(pInst->Nblavg / pTempModel->Xnin);        /* 4.5r */
    if (pModel->NfdMod > 0)                                                      /* 6.0bulk */
    { pTempModel->Phibb = 2.0 * Vtm * log(pModel->Nbh / pTempModel->Xnin);
    }

/*    T0 = 10.0 * pModel->Xalpha * Coxf * Vtm / pTempModel->Qr;                     4.5qm */
/*    PstOld = pTempModel->Phib + 2.0 * Vtm * log(T0);                              4.5qm */
/*    T1 = T0 * sqrt(PstOld / Vtm + exp((PstOld - pTempModel->Phib) / Vtm));        4.5qm */
/*    pTempModel->Pst = pTempModel->Phib + Vtm * log(T1);                           4.5qm */
/*    T2 = pTempModel->Pst / Vtm                                                    4.5qm */
/*       + exp((pTempModel->Pst - pTempModel->Phib) / Vtm);                         4.5qm */
/*    pTempModel->Qst = -pTempModel->Qr * sqrt(T2) - pInst->Qb;                     4.5qm */

    pTempModel->Vbi = 0.5 * Eg + Vtm * log(pInst->Nblavg / pTempModel->Xnin);    /* 4.5r */
/*5.0    if (pTempModel->Vbi > Eg) */
/*5.0        pTempModel->Vbi = Eg; */

    if (pModel->NfdMod > 0)                                                      /* 6.0bulk */
    {	pTempModel->Vbih = 0.5 * Eg + Vtm * log(pModel->Nbheff / pTempModel->Xnin);
/*5.0        if (pTempModel->Vbih > Eg) */
/*5.0            pTempModel->Vbih = Eg; */
    }
    T1 = pTempModel->Xnin / pModel->Xnin;
    pTempModel->Jro = pModel->Jro * pow(T1, 3.0 - pModel->M);

    Ef = pModel->Type * Vtm * log(pInst->Nblavg / pTempModel->Xnin);             /* 4.5r */
    Efg = pModel->Type * pModel->Tpg * 0.5 * Eg;                                 /* 4.5 */
    pTempModel->Wkf = pModel->Type * pInst->Wkf * (Ef + Efg) / pInst->Const4;    /* 4.5r */

    if (pModel->NfdMod > 0)                                                      /* 6.0bulk */
    {   Ef = pModel->Type * Vtm * log(pModel->Nbh / pTempModel->Xnin);
    }
    Efgb = pModel->Type * pModel->Tps * Vtm * log(pModel->Nsub
	 / pTempModel->Xnin);
    pTempModel->Wkb = pModel->Type * pModel->Wkb * (Ef + Efgb) / pModel->Const5;

    T0 = ELECTRON_CHARGE*(pModel->Nqff + 2.0*pModel->Nqfsw * pModel->Tb / Weff); /* 4.5r */
    pTempModel->Vfbf = pModel->Type * pInst->Vfbf                                /* 4.5r */
      * (pModel->Type * pTempModel->Wkf - T0 / Coxf) / (pInst->Wkf - T0 / Coxf); /* 4.5r */
    pTempModel->Vfbb = pModel->Type * pModel->Vfbb
      * (pModel->Type * pTempModel->Wkb - ELECTRON_CHARGE * pModel->Nqfb / Coxb) /* 4.5 */
      / (pModel->Wkb - ELECTRON_CHARGE * pModel->Nqfb / Coxb);                   /* 4.5 */

    T0 = T / pModel->Tnom;
    T1 = 1.16e3 * (1.0 / T - 1.0 / pModel->Tnom);
    pTempModel->Tauo = pModel->Tauo / sqrt(T0) * exp(T1);
    pTempModel->Taug = 2.0 * pTempModel->Tauo / (1.0 + pInst->Nblavg / 5.0e22);  /* 4.5r */
    if (pModel->NfdMod > 0 )                                                     /* 6.0bulk */
    {	pTempModel->Taugh = 2.0 * pTempModel->Tauo
			  / (1.0 + pModel->Nbheff / 5.0e22);
    }
    T1 = 0.88 / pow(RatioRef, 0.146);
    T2 = pow(RatioRef, 2.4);
    T3 = pow(RatioRef, 0.57);
    T4 = pow(T, 2.33);
    T5 = pInst->Nblavg / 2.35e23 / T2;                                           /* 4.5r */
    T6 = pInst->Nblavg / 1.26e23 / T2;                                           /* 4.5r */
    if (pModel->Lldd > 0)
        Nldd = pModel->Nldd;
    else
        Nldd = pModel->Nds;
    T7 = Nldd / 2.35e23 / T2;                                                    /* 4.5r */
    T8 = Nldd / 1.26e23 / T2;                                                    /* 4.5r */
    if (pModel->Type > 0)                                                        /* 4.5r */
    { pTempModel->Mumin  = 5.4e-3 / T3 + 1.36e4 / T4 / (1.0 + pow(T7,T1));       /* 4.5r */
      pTempModel->Mumaj  = 8.8e-3 / T3 + 7.40e4 / T4 / (1.0 + pow(T8,T1));       /* 4.5r */
      pTempModel->Mubody = 8.8e-3 / T3 + 7.40e4 / T4 / (1.0 + pow(T6,T1));       /* 4.5r */
      pTempModel->Uo = pInst->Uo * pTempModel->Mubody /  pInst->Ft1;             /* 4.5r */
      if(pModel->NfdMod > 0)                                                     /* 6.0bulk */
      { T9 = pModel->Nbh / 1.26e23 / T2;                                         /* 4.5r */
	pTempModel->Mubh = 8.8e-3 / T3 + 7.40e4 / T4 / (1.0 + pow(T9,T1));       /* 4.5r */
        pTempModel->Beta0 = 0.5 * pTempModel->Uo / Coxf * Weff / Leff;           /* 4.5r */
      }                                                                          /* 4.5r */
      else                                                                       /* 4.5r */
      { pTempModel->Beta0 = 0.5 * pTempModel->Uo * Coxf * Weff / Leff;           /* 4.5r */
      }                                                                          /* 4.5r */
    }                                                                            /* 4.5r */
    else                                                                         /* 4.5r */
    { pTempModel->Mumin  = 8.8e-3 / T3 + 7.40e4 / T4 / (1.0 + pow(T8,T1));       /* 4.5r */
      pTempModel->Mumaj  = 5.4e-3 / T3 + 1.36e4 / T4 / (1.0 + pow(T7,T1));       /* 4.5r */
      pTempModel->Mubody = 5.4e-3 / T3 + 1.36e4 / T4 / (1.0 + pow(T5,T1));       /* 4.5r */
      pTempModel->Uo = pInst->Uo * pTempModel->Mubody /  pInst->Ft1;             /* 4.5r */
      if(pModel->NfdMod > 0)                                                     /* 6.0bulk */
      { T9 = pModel->Nbh / 2.35e23 / T2;                                         /* 4.5r */
	pTempModel->Mubh = 5.4e-3 / T3 + 1.36e4 / T4 / (1.0 + pow(T9,T1));       /* 4.5r */
        pTempModel->Beta0 = 0.5 * pTempModel->Uo / Coxf * Weff / Leff;           /* 4.5r */
      }                                                                          /* 4.5r */
      else                                                                       /* 4.5r */
      { pTempModel->Beta0 = 0.5 * pTempModel->Uo * Coxf * Weff / Leff;           /* 4.5r */
      }                                                                          /* 4.5r */
    }                                                                            /* 4.5r */
    T0 = 175.0 / T; 
    T1 = exp(T0);
    T2 = (T1 - 1.0 / T1) / (T1 + 1.0 / T1);
    T3 = sqrt(T2 / pModel->Const3);
    pTempModel->Vsat = pModel->Vsat * T3;
/*    pTempModel->Fsat = 0.5 * pTempModel->Uo / pTempModel->Vsat / Leff; */      /* 5.0 */

    T1 = T / 300.0;                                                              /* 4.5 */
    pTempModel->Alpha = pModel->Alpha * (0.57 + 0.43 * T1 * T1)                  /* 4.5 */
                      / pModel->Const1;                                          /* 4.5 */
    pTempModel->Beta = pModel->Beta * (0.625 + 0.375 * T1)                       /* 4.5 */
                     / pModel->Const2 * T3;                                      /* 4.5 */

    if (pModel->NfdMod == 0)
    {   pTempModel->Rldd = 1.0 / (pModel->Tb * Weff * ELECTRON_CHARGE
		         * Nldd * pTempModel->Mumaj);
    }
    else
    {   pTempModel->Rldd = 1.0 / (pTempModel->Mumaj * Weff * ELECTRON_CHARGE
		         * Nldd * (pModel->Tf - pModel->Thalo));
    }
    /* Don't update conductances if calculating temperature derivitives (SH=0).     4.5 */
    /* GdsTempFac used when loading Gd and GS; GdsTempFac2 used when calculating    4.5 */
    /* the power in the model routines,                                             4.5 */
    pTempModel->GdsTempFac2 = pTempModel->Mumaj / pModel->Mumaj;                 /* 4.5 */
    if(SH == 1)                                                                  /* 4.5r */
    { pTempModel->GdsTempFac = pTempModel->GdsTempFac2;                          /* 4.5 */
      if (pModel->NfdMod == 0)                                                   /* 4.5 */
      {   pTempModel->GbTempFac = pTempModel->Mubody / pInst->Mubody;            /* 4.5r */
      }
      else
      {   pTempModel->GbTempFac = pTempModel->Mubh / pModel->Mubh;
      }
    }                                                                            /* 4.5r */
    pTempModel->Seff = pModel->Seff * T / pModel->Tnom * pTempModel->Mumin
		     / pModel->Mumin;
    pTempModel->D0 = sqrt(2.0 * ESI * pTempModel->Vbi / (ELECTRON_CHARGE
                   * pInst->Nblavg));                                            /* 4.5r */
    /* pTempModel->Lemin = sqrt(ESI * Vtm / (ELECTRON_CHARGE * pInst->Nblavg));     4.5r */
    if (pModel->NfdMod > 0)                                                      /* 6.0bulk */
    {   pTempModel->D0h = sqrt(2.0 * ESI * pTempModel->Vbih / (ELECTRON_CHARGE
                        * pModel->Nbheff));
        pTempModel->Leminh = sqrt(ESI * Vtm / (ELECTRON_CHARGE * pModel->Nbh));
    }
    T0 = 0.12 / 0.026 - 0.12 / Vtm;
    pTempModel->Ndseff = 1.0e24 * exp(T0);
    pTempModel->Efs = pModel->Tps * Vtm * log(pModel->Nsub
                    / pTempModel->Xnin);
    pTempModel->Efd = Vtm * log(pModel->Nds / pTempModel->Xnin);

    /* Vexplx Gexplx and Ioffsetx defined in terms of Weff, not PSJ or PDJ.
       Cannot use PSJ/PDJ because we do not know which mode the device will
       be in at this point. This should only cause a discontinuity in
       Ir and Igt near Imax (because PSJ, PDJ >= Weff);
       e.g. if PSJ > Weff, Ir0 > Imax for Vbs slightly less than Vexpl2.            4.5 */
    if(pTempModel->Tauo == 0.0)                                                  /* 4.51 */
    { pTempModel->Gexpl1 = 0.0;                                                  /* 4.51 */
      pTempModel->Ioffset1 = 0.0;                                                /* 4.51 */
      pTempModel->Gexpl4 = 0.0;                                                  /* 4.51 */
      pTempModel->Ioffset4 = 0.0;                                                /* 4.51 */
    }                                                                            /* 4.51 */
    else                                                                         /* 4.51 */
    { Igt = ELECTRON_CHARGE * pTempModel->Xnin * Weff
          * pModel->Tb * pTempModel->D0 / pTempModel->Taug;
      Igtb = ELECTRON_CHARGE * pTempModel->Xnin * Weff * Leff                    /* 4.5 */
          * pModel->Tb / pTempModel->Taug;                                       /* 4.5 */
      if (Igt > 0.0)
      {   pTempModel->Vexpl1 = Vtm * log(1.0 + pModel->Imax / Igt);
      }
      else
      {   pTempModel->Vexpl1 = 80.0 * Vtm;
      }
      pTempModel->Gexpl1 = (pModel->Imax + Igt) / Vtm;
      pTempModel->Ioffset1 = pModel->Imax - pTempModel->Gexpl1
			   * pTempModel->Vexpl1;
      if (Igtb > 0.0)                                                            /* 4.5 */
      {   pTempModel->Vexpl4 = Vtm * log(1.0 + pModel->Imax / Igtb);             /* 4.5 */
      }                                                                          /* 4.5 */
      else                                                                       /* 4.5 */
      {   pTempModel->Vexpl4 = 80.0 * Vtm;                                       /* 4.5 */
      }                                                                          /* 4.5 */
      pTempModel->Gexpl4 = (pModel->Imax + Igtb) / Vtm;                          /* 4.5 */
      pTempModel->Ioffset4 = pModel->Imax - pTempModel->Gexpl4;                  /* 4.5 */
    }                                                                            /* 4.51 */
    Ir0 = pTempModel->Jro * Weff;
    Ir1 = ELECTRON_CHARGE * pTempModel->Xnin * pTempModel->Xnin
        * pTempModel->Seff * Weff * pModel->Tb / pTempModel->Ndseff;
    if (Ir1 > 0.0)
    {   pTempModel->Vexpl2 = Vtm * log(1.0 + pModel->Imax / Ir1);
    }
    else
    {   pTempModel->Vexpl2 = 80.0 * Vtm;
    }
    pTempModel->Gexpl2 = (pModel->Imax + Ir1) / Vtm;
    pTempModel->Ioffset2 = pModel->Imax - pTempModel->Gexpl2
			 * pTempModel->Vexpl2;
    if (Ir0 > 0.0)
    {   pTempModel->Vexpl3 = pModel->M * Vtm * log(1.0 + pModel->Imax / Ir0);
    }
    else
    {   pTempModel->Vexpl3 = 80.0 * pModel->M * Vtm;
    }
    pTempModel->Gexpl3 = (pModel->Imax + Ir0) / (pModel->M * Vtm);
    pTempModel->Ioffset3 = pModel->Imax - pTempModel->Gexpl3
			 * pTempModel->Vexpl3;
    return 0;
}

int 
fdEvalMod(Vds, Vgfs, Vbs, Vgbs, pOpInfo, pInst, pModel, pEnv, DynamicNeeded,     /* 5.0 */
          ACNeeded)                                                              /* 5.0 */
double Vds, Vgfs, Vbs, Vgbs;
struct ufsAPI_InstData  *pInst;
struct ufsAPI_ModelData *pModel;
struct ufsAPI_OPData *pOpInfo;
struct ufsAPI_EnvData *pEnv;
int DynamicNeeded, ACNeeded;                                                     /* 5.0 */
{
register struct ufsTDModelData *pTempModel;
struct ufsDebugData *pDebug;
double Xnin, Xnin2, Xdumv, Arg, Sqrt_s, Sqrt_d, RootEtb, Etb;
double Vtm, Vthso, Vths, Vthw, Vfbf, Vfbb, Qbeff, Qst, Leff, Weff, D;
double Psblong, Psb, PsbOld, Pst, Vbi, Part, Cap1, Ldd, Lds, Vdss;
double T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14;
double Vdsat, Vbs_bjt, Argbjts, Vbd_bjt, Argbjtd, Delvtf;
double Vthwf, Vthwb, Sinhf, Coshf, Sinhb, Coshb, Sinh1, Cosh1, Sinh3;
double Yfm, Yfmold, Phib1, F1, F2, Vthwfold, Xk, Sinh2, Cosh2, Cosh3;
double Gbwk, Xkb, Dxm, Xm, Ybm, Ybmold, Vthwbold, Vgstart, Vgfso;
double Vgfbf, Vgwbb, Psfmin, Psbmin, Psfbm, Psbfm, Coxf, Coxb, Cb;
double Vlddd, Vgbwk, Dumchg, Yfb, Xminf, Am, Bm, Cm, Xkm, Vxmf, Xminb;
double Aam, Vxmb, Qmobf, Qmobb, Uef, Ueb, Efs, Efd, Ebs, Ebd, Xftl, Xbtl;
double Lf, Lb, Iwkf, Iwkb, Iwk, Xemwk, Xem, Xk11, Xk22, Vgbwk11, Vgbwk22;
double Ich, Vo1, Xbfact, Mobred, Mobref, Xbmu, Lcfac, Lefac, Ist, Arg3;
double Ist1, Ist2, Mobref1, Mobref2, Mobred1, Mobred2, Xbmu1, Xbmu2;
double DeltaVth, Gst, Gwk, R2, R3, Vref, Xs, Psf_bjt, Dellwk1, Dellwk2;          /* 4.5 */
double Vbseff, Vbdeff, Ds, Dd, Xlbjt, Xleff, Feff, Psb_bjt, Xaf, Exp_af;
double Xbf, Exp_bf, Ph_f, P0f, Tf1, Tf2, Xintegf, Xintegr, Ph_r, Tr1, Tr2;
double Ibjtf, Ibjtr, Ibjt, Itot, Eslope, Igts0, Igtd0, Irs0, Ird0, Dell_f;
double Irs1, Ird1, Rgtd, Rgts, Igt_g, Igt_r, Igt, Ir_g, Ir_r, Ir, Igi;
double Xecf, Xemf, Betap, Te_ch, Mult_ch, Mult_dep, Mult_qnr, Mult_drn;
double Mult, Mult_wk, Te_ldd, Ldep, Tempdell, Xstart, Xend, Te_dep, Dellwk;      /* 4.5 */
double Lcf, Power, CoxWL, Qd, Qn, Qgf, Qby, Xingvsf, Psiwk, Xvbys, Psieff;
double Xco0, Xco1, Xco2, Vgfsacc, Vgfseff, Qgfwk1, Qgfwk2;
double Vgfvt, Qnqsh, Qdqsh, Covf, Vdsx, Arg2, Dnmgf, Args;
double Qdeta, Xp, Argu, Argz, Xfactd, Tzm1, Qcfle, Qns, Zfact, Qgfst1;
double Qgfst2, Qdst1, Qdst2, Qnst1, Qnst2, Cst, Cwk, Vp, Qp;
double X3a, X3b, X3c, Xarg, t, Qnqff, Qnqfb, Qddep, Qsdep, Qterm, Qterms;
double Qtermd, Qnd, Qps, Qpd, Vgfst, Xemst, Iwk1, Iwk2, Qds, Qgb, Ad, As;
double Vldd[40], Vd[40];
double Vdsx1, Vdsx2, Lefac1, Lefac2, Vdseff, Xchk;				 /* 4.41 */
double Egbgn, Wkfgd, Vfbgd, Vgdx, Esd, Vgdt;                                     /* 4.41 */
double Igidl, Rmass, h, Eg, Qacc, Qinv;                                          /* 4.5 */
double Pdj, Psj, E_db, hnt, htoc, Itun, Ueffwk, Ueffst, Qs, Qnwk1;               /* 4.5 */
double Qnwk2, Lewk, Igtb0, Ueffwk1, Ueffst1, Xemst1, Xemwk1, Qsst1;              /* 4.5 */
double Vgfstp, Alphap, Psigf1, Alphap1, Psigf2, Alphap2, Psigf, Vo1p;            /* 4.5pd */
double Qmobff, Qmobfb, Qmobbf, Qmobbb, Exf, Exb, Mult_st1, Mult_st2;             /* 4.5 */
double Xchk2, Xchk3, Xchk4, Xchk5, Xchk10, Xx, Vdseff1, Lewk1, Qsst2;	         /* 5.0 */
double Ess, DelEg, PhibQM, XninQM, Vths1, Pst1, Psb1, Qbeff1, Qst1;              /* 4.5qm */
double Pst_bjt, Vths_bjt, D_bjt, Vths0, PstOld, Es1, Es2, DelEg1, DelEg2;        /* 4.5qm */
double DumchgQM1, DumchgQM2, Vthsx, Vthsxo, Vthsx1, Vthsx2, Pst2, Qst2;          /* 4.5qm */
double ExfLim, QmT1, QmT2, QmT3, QmT4, QmT5, Coeffz, VthsOld, Pst0, DVgfs;       /* 4.5F */
double T, Exo, Ueffo, Sich, Lc, Sis1, Sis2, Siw1, Siw2, Ich0, Le;                /* 5.0 */
int I, J, K, IB, Region, ICONT, JFlg;                                            /* 4.5F */
double Vo, Vsat, Vth, Vds_dif, Vds_eff, Uo, Toxf, Theta, Vtfa, Vgba;             /* 5.0vo */
double Xalpha, Ldf, Ld, Xec, Xex, Tau_w, Phib, Qb, Qcbycox;
double Vdsdemo, Xsinh, Xcosh, Vdemo, Xueff, Vsat_eff, Fsat, Vbdx; /* Vbdo; */    /* 6.0 */ 
double Vbsx, Vgst, Vgsx, Igisl;                                                  /* 6.0 */

/* Begin `fdEvalMod'. */

    pTempModel = pInst->pTempModel;
    Vtm = pTempModel->Vtm;
    Eg = pTempModel->Eg;                                                         /* 4.41 */
    Pst = pTempModel->Pst;
    Vbi = pTempModel->Vbi;
    Xnin = pTempModel->Xnin;
    Xnin2 = Xnin * Xnin;
    Leff = pInst->Leff;
    Weff = pInst->Weff;
    Coxf = pModel->Coxf;
    Coxb = pModel->Coxb;
    Cb = pModel->Cb;
    Vfbf = pTempModel->Vfbf;
    Vfbb = pTempModel->Vfbb;
    Ldd = pModel->Lldd;
    if (pInst->Mode > 0)
    {	As = pInst->SourceArea;
	Ad = pInst->DrainArea;
        Pdj = pInst->DrainJunctionPerimeter;                                     /* 4.5 */
        Psj = pInst->SourceJunctionPerimeter;                                    /* 4.5 */
    }
    else
    {	Ad = pInst->SourceArea;
	As = pInst->DrainArea;
        Pdj = pInst->SourceJunctionPerimeter;                                    /* 4.5 */
        Psj = pInst->DrainJunctionPerimeter;                                     /* 4.5 */
    }

    Sinhf = 0.5 * (pInst->Expf - 1.0 / pInst->Expf);
    Coshf = 0.5 * (pInst->Expf + 1.0 / pInst->Expf);
    Sinhb = 0.5 * (pInst->Expb - 1.0 / pInst->Expb);
    Coshb = 0.5 * (pInst->Expb + 1.0 / pInst->Expb);
    Cap1 = 1.0 / (pModel->Rb + Cb / Coxb);
    Part = pModel->Nldd / (pModel->Nldd + pModel->Nbody);                        /* 4.5qm */
    Lcfac = pModel->Lc / Leff;
    Xdumv = ELECTRON_CHARGE * pModel->Nbody / (2.0 * ESI);

/*  Start Overshoot Model  */                                                    /* 5.0vo */
    
/* SET VSAT INITIAL VALUE  */
     Vo = pModel->Vo;
     Toxf = pModel->Toxf;
     Vsat = pTempModel->Vsat;
     Uo = pTempModel->Uo;
     Theta = pModel->Theta;
     Lc = pModel->Lc;
     Xalpha = pModel->Xalpha;     

/* CALCULATE THRESHOLD VOLTAGE (for velocity overshoot) */
    Vtfa = Vfbf + (1.0 + Cb/Coxf) * pTempModel->Phib - 0.5 * pInst->Qb / Coxf;   /* 6.0 */
    Vgba = Vfbb - Cb/Coxb * pTempModel->Phib - 0.5 * pInst->Qb / Coxb;           /* 6.0 */
    Vth = Vtfa - Cb*Coxb * (Vgbs -Vgba)/(Coxf*(Cb+Coxb));                        /* 6.0 */
 /*  Vth = Vfbf + Xalpha * pTempModel->Phib - 0.5 * pInst->Qb / Coxf;  */

/* CALCULATE EFFECTIVE MOBILITY */      
     Xex = (Vgfs - Vth + ESI * pModel->Tb 
         * Vds / (Coxf * Leff * Leff)) * Coxf / (2.0 * ESI);
     Xueff = Uo / (1.0 + Theta * Xex);
     Tau_w = 3.0*Vtm*Xueff / (2.0*Vsat*Vsat);  

/* CALCULATE Vds_eff  */
     Qcbycox = (Vgfs - Vth) / Xalpha;   
     Vdsdemo = 1.0 + Qcbycox * 0.5 * Xueff / (Vsat * Leff);
     Vds_eff = Qcbycox / Vdsdemo / Xalpha;
    /*  if (Vds_eff<=0) Vds_eff=0;  */
     Vds_eff = log(1.0 + exp(10.0 * Vds_eff)) / 10.0;

/* CALCULATE Vsat_eff  */
     Xec = 2.0 * Vsat / Xueff;
     Vds_dif = Vds - Vds_eff;
     Vds_dif = log(1.0 + exp(10.0 * Vds_dif)) / 10.0;
     Ldf = Vds_dif / (Xec * Lc);
     Ld = Lc * log(Ldf + sqrt(1.0 + Ldf * Ldf));
     Xsinh = Vo * sinh((Vo * Ld) / Lc) / Lc;
     Xcosh = cosh((Vo * Ld) / Lc);
     Vdemo = Xsinh / Xcosh;
     Vsat_eff = Vsat * (1.0 + 2.0 * Vsat * Tau_w * Vdemo / 3.0);
     /* printf ("Vsat_eff=%.3e\n", Vsat_eff);  */

/*   REPLACE Vsat with Vsat_eff */
     pTempModel->Fsat = 0.5 * pTempModel->Uo/Vsat_eff/Leff;
       
/* VSAT IS VSAT_EFF EVERYWHERE FROM NOW ON */

    Arg = Vbs / Vtm;
    if (Arg > 80.0)
    {   Vbs_bjt = Vbs;
    }
    else if (Arg < -80.0)
    {   Vbs_bjt = 0.0;
    }
    else
    {   Vbs_bjt = Vtm * log(1.0 + exp(Arg));
    }
    Sqrt_s = exp(0.5 * Vbs_bjt / Vtm);
    Argbjts = Sqrt_s * Sqrt_s;

    T0 = (Vbi - Vbs) / Vtm;
    if (T0 > 80.0)
    {   Vbseff = Vbs;
    }
    else if (T0 < -80.0)
    {   Vbseff = Vbi;
    }
    else
    {   Vbseff = Vbi - Vtm * log(1.0 + exp(T0));
    }
    Xchk = 1.0 - Vbseff / Vbi;						         /* 4.41 */
    if (Xchk <0) Xchk = 0;						         /* 4.41 */
    Ds = pTempModel->D0 * sqrt(Xchk);					         /* 4.41 */

    if (Vgfs > 0.6)	            					         /* 4.5qm */
    {   Vldd[0] = 0.0;
        Vldd[1] = 0.05 * Vds;
    }
    else
    {   Vldd[0] = 0.7 * Vds;
        Vldd[1] = 0.5 * Vds;
    }
    for (IB = 0; IB < 30; IB++)
    {    if ((Ldd <= 0.0) || (Vds < 1.0e-6) || 
             (pModel->Nldd >= 1.0e25)) Vldd[IB] = 0.0;                           /* 4.5 */
         Vdss = Vds - Vldd[IB];

         Arg = (Vbs - Vdss) / Vtm;
         if (Arg > 80.0)
	 {   Vbd_bjt = Vbs - Vdss;
	 }
         else if (Arg < -80.0)
	 {   Vbd_bjt = 0.0;
	 }
	 else
	 {   Vbd_bjt = Vtm * log(1.0 + exp(Arg));
	 }
         Sqrt_d = exp(0.5 * Vbd_bjt / Vtm);
         Argbjtd = Sqrt_d * Sqrt_d;

	 T0 = (Vbi - Vbs + Vdss) / Vtm;
	 if (T0 > 80.0)
	 {   Vbdeff = Vbs - Vdss;
	 }
	 else if (T0 < -80.0)
	 {   Vbdeff = Vbi;
	 }
	 else
	 {   Vbdeff = Vbi - Vtm * log(1.0 + exp(T0));
	 }
	 Xchk = 1.0 - Vbdeff / Vbi;						 /* 4.41 */
	 if (Xchk <0) Xchk = 0;						         /* 4.41 */
	 Dd = pTempModel->D0 * sqrt(Xchk);					 /* 4.5qm */

         ExfLim = 2.0 * Vtm / pModel->Tb;                                        /* 4.5F */
         Delvtf = -pModel->Factor1 / (Leff * Leff) * Vdss;                       /* 4.5F */
         QmT1 = (pModel->Xalpha - pModel->Rf) / pModel->Rb;                      /* 4.5F */
         QmT2 = 0.5 * ELECTRON_CHARGE * pModel->Nbody / ESI * Part;              /* 4.5F */
         QmT3 = pModel->Qm * 5.33e-9 / 9.0;                                      /* 4.5F */
         QmT4 = ESI / (4.0 * Vtm * ELECTRON_CHARGE);                             /* 4.5F */
         QmT5 = 1.0 / 3.0;                                                       /* 4.5F */
         Coeffz = 10.0 * pModel->Xalpha * Coxf * Vtm / pTempModel->Qr;           /* 4.5F */
         if(pModel->Qm == 0.0)                                                   /* 4.5F */
         { JFlg = 1;                                                             /* 4.5F */
         }                                                                       /* 4.5F */
         else                                                                    /* 4.5F */
         { JFlg = 3;                                                             /* 4.5F */
         }                                                                       /* 4.5F */
         VthsOld = Vgfs;                                                         /* 4.5F */
         Pst0 = pTempModel->Phib + 2.0 * Vtm * log(Coeffz);                      /* 4.5F */
         for (J = 0; J < JFlg; J++)                                              /* 4.5F */
         { Pst = Pst0;                                                           /* 4.5F */
	   for (K = 0; K < 3; K++)                                               /* 4.5F */
	   { PstOld = Pst;                                                       /* 4.5F */
	     if (pModel->Qm == 0.0)                                              /* 4.5F */
             { PhibQM = pTempModel->Phib;                                        /* 4.5F */
             }		   		   		   		         /* 4.5F */
             else		   		   		   		 /* 4.5F */
             { Ess = Coxf / ESI * (VthsOld - Vfbf - PstOld * pModel->Rf);        /* 4.5F */
	       Arg = (Ess - ExfLim) / ExfLim;                                    /* 4.5F */
	       if(Arg < -80.0)                                                   /* 4.5F */
	       { Ess = ExfLim;                                                   /* 4.5F */
	       }                                                                 /* 4.5F */
               else if(Arg < 80.0)                                               /* 4.5F */
               { Ess = ExfLim + ExfLim * log(1.0 + exp(Arg));                    /* 4.5F */
	       }                                                                 /* 4.5F */
	       T1 = QmT4 * Ess * Ess;                                            /* 4.5F */
	       DelEg = QmT3 * pow(T1,QmT5);                                      /* 4.5F */
               XninQM = Xnin * exp(-DelEg / 2.0 / Vtm);                          /* 4.5F */
               PhibQM = 2.0 * Vtm * log(pModel->Nbody / XninQM);                 /* 4.5F */
	     }      		   		   		   		 /* 4.5F */
             T1 = Coeffz * sqrt(PstOld / Vtm + exp((PstOld - PhibQM) / Vtm));    /* 4.5F */
             Pst = PhibQM + Vtm * log(T1);                                       /* 4.5F */
	   }		              		   		                 /* 4.5F */
           Psblong = (Vgbs - Vfbb + (0.5 * pInst->Qb + Cb * Pst) / Coxb) * Cap1; /* 4.5F */
           if (Psblong < 0.0)Psblong = 0.0;                                      /* 4.5F */
           else if (Psblong > Pst)Psblong = Pst;                                 /* 4.5F */
           Psb = Psblong;                                                        /* 4.5F */
           for (I = 0; I < 3; I++)                                               /* 4.5F */
           { Psb = MIN(Psb, Vbi);                                                /* 4.5F */
	     PsbOld = Psb;                                                       /* 4.5F */
	     RootEtb = sqrt(QmT2 * (Vbi - Psb));                                 /* 4.5F */
	     T2 = 0.4 * (Vbi * Part + (1.0 - Part) * Psb)                        /* 4.5F */
	        - 0.2 * Psb - 0.2 * (Vgbs - Vfbb);                               /* 4.5F */
	     Etb = RootEtb + Coxb / ESI * T2;                                    /* 4.5F */
	     D = (Vbi - Psb) * Part / Etb;                                       /* 4.5F */
	     if (D < 0.0) D = 0.0;                                               /* 4.5F */
	     else if (D > 0.7 * Leff) D = 0.7 * Leff;                            /* 4.5F */
	     Psb = Psblong - 0.5 * D / Leff * pInst->Qb / Coxb * Cap1;           /* 4.5F */
	     Psb = MIN(Psb, Pst);                                                /* 4.5F */
	   }                                                                     /* 4.5F */
           Qbeff = pInst->Qb * (1.0 - D / Leff);                                 /* 4.5F */
           T2 = Pst / Vtm + exp((Pst - PhibQM) / Vtm);                           /* 4.5F */
           Qst = -pTempModel->Qr * sqrt(T2) - pInst->Qb;                         /* 4.5F */
           Vths = Vfbf - QmT1 * (Vgbs - Vfbb) + Pst * pModel->Xalpha + Delvtf    /* 4.5F */
                - (0.5 * Qbeff * pModel->Xbeta + Qst) / Coxf;                    /* 4.5F */
           if (J == 0)                                                           /* 4.5F */
           { Vths1 = Vths;                                                       /* 4.5F */
             Pst1 = Pst;                                                         /* 4.5F */
             Psb1 = Psb;                                                         /* 4.5F */
             Qbeff1 = Qbeff;                                                     /* 4.5F */
             Qst1 = Qst;                                                         /* 4.5F */
             Pst_bjt  = Pst;                                                     /* 4.5F */
             Vths_bjt = Vths;                                                    /* 4.5F */
             D_bjt = D;                                                          /* 4.5F */
             VthsOld = Vfbf - QmT1 * (Vgbs - Vfbb)                               /* 4.5F */
	             + Pst0 * pModel->Xalpha + Delvtf + 0.4;                     /* 4.5F */
           }                                                                     /* 4.5F */
           else                                                                  /* 4.5F */
	   { VthsOld = Vths;                                                     /* 4.5F */
	   }                                                                     /* 4.5F */
 	 }                                                                       /* 4.5F */
	 Vthw = Vths;

	 Lefac = 1.0;
	 if (Vgfs < Vths)
	 {   Phib1 = pTempModel->Phib - 5.0 * Vtm;
	     Yfmold = 0.5 * Leff;
/*           for (I = 0; I < 20; I++)                                        */
             for (I = 0; I < 5; I++)                                            /* 6.0 */
	     {    Arg = pModel->Aa * Yfmold;
		  T0 = exp(Arg);
	          Sinh1 = 0.5 * (T0 - 1.0 / T0);
		  F1 = Sinh1 / Sinhf;
		  T0 = pInst->Expf / T0;
	          Sinh2 = 0.5 * (T0 - 1.0 / T0);
		  F2 = (Sinh1 + Sinh2) / Sinhf;
		  T1 = Xdumv - pModel->A * (Phib1 - Vbi * F2
		     - Vdss * F1) / (F2 - 1.0);
		  Vthwfold = Vfbf + (pModel->C * (Vfbb - Vgbs) + T1)
		           / pModel->B;
		  /* Call Fring(pInst, pModel, Vgbs, &Vgbwk, &Xkb); */
		  Vgbwk = Vgbs;
		  Xkb = (pModel->Ccb * (Vfbf - Vthwfold) - pModel->Bb
		      * (Vgbwk - Vfbb) + Xdumv) / pModel->Ab;
		  if (pModel->Gamma != 0.0)
		  {   T0 = pModel->Toxb / Leff;
		      Dxm = pModel->Kappa * T0 * T0 * Vdss;
		      for (J = 0; J < 2; J++)
		      {    Ebs = -pModel->Aab * ((Xkb + Vbi)                     /* 4.5F */
			       * (1.0 - Coshb) + Vdss) / Sinhb;
	                   Arg = (Ebs - ExfLim) / ExfLim;                        /* 4.5F */
	                   if(Arg < -80.0)                                       /* 4.5F */
	                   { Ebs = ExfLim;                                       /* 4.5F */
	                   }                                                     /* 4.5F */
                           else if(Arg < 80.0)                                   /* 4.5F */
                           { Ebs = ExfLim + ExfLim * log(1.0 + exp(Arg));        /* 4.5F */
	                   }                                                     /* 4.5F */
		           Xm = T0 * pModel->Toxb * Ebs * pModel->Gamma;
			   Vgbwk = Vgbs + Xm + Dxm;
		           Xkb = (pModel->Ccb * (Vfbf - Vthwfold)
			       - pModel->Bb * (Vgbwk - Vfbb) + Xdumv)
			       / pModel->Ab;
		      }
		  }
		  Vthwf = Vfbf + (pModel->C * (Vfbb - Vgbwk) + T1)
		        / pModel->B;
	          Xk = (pModel->C * (Vfbb - Vgbwk) - pModel->B
		     * (Vthwf - Vfbf) + Xdumv) / pModel->A;
		  T2 = Xk + Vbi;
		  T3 = T2 * (pInst->Expf - 1.0) - Vdss;
		  T4 = T2 * (1.0 - 1.0 / pInst->Expf) + Vdss;
	          Yfm = log(T3 / T4) / (2.0 * pModel->Aa);
/*		  if (fabs(Yfm - Yfmold) <= 0.05 * Leff)
		      break;                                 */
	          Yfmold = Yfm;
	     }
	     if (pModel->Debug && (I >= 20))
	     {   fprintf(stderr, "No convergence in calculating Vthwf.\n");
	     }

	     Ybmold = 0.5 * Leff;
/*           for (I = 0; I < 20; I++)                                         */
             for (I = 0; I < 5; I++)                                            /* 6.0 */
	     {    Arg = pModel->Aab * Ybmold;
		  T0 = exp(Arg);
	          Sinh1 = 0.5 * (T0 - 1.0 / T0);
		  F1 = Sinh1 / Sinhb;
		  T0 = pInst->Expb / T0;
	          Sinh2 = 0.5 * (T0 - 1.0 / T0);
		  F2 = (Sinh1 + Sinh2) / Sinhb;
		  T1 = -Xdumv + pModel->Ab * (Phib1 - Vbi * F2
		     - Vdss * F1) / (F2 - 1.0);
		  Vthwbold = Vfbf - (pModel->Bb * (Vgbs - Vfbb) + T1)
		           / pModel->Ccb;
		  /* Call Fring(pInst, pModel, Vgbs, &Vgbwk, &Xkb); */
		  Vgbwk = Vgbs;
		  Xkb = (pModel->Ccb * (Vfbf - Vthwbold) - pModel->Bb
		      * (Vgbwk - Vfbb) + Xdumv) / pModel->Ab;
		  if (pModel->Gamma != 0.0)
		  {   T0 = pModel->Toxb / Leff;
		      Dxm = pModel->Kappa * T0 * T0 * Vdss;
		      for (J = 0; J < 2; J++)
		      {    Ebs = -pModel->Aab * ((Xkb + Vbi)                     /* 4.5F */
			       * (1.0 - Coshb) + Vdss) / Sinhb;
	                   Arg = (Ebs - ExfLim) / ExfLim;                        /* 4.5F */
	                   if(Arg < -80.0)                                       /* 4.5F */
	                   { Ebs = ExfLim;                                       /* 4.5F */
	                   }                                                     /* 4.5F */
                           else if(Arg < 80.0)                                   /* 4.5F */
                           { Ebs = ExfLim + ExfLim * log(1.0 + exp(Arg));        /* 4.5F */
	                   }                                                     /* 4.5F */
		           Xm = T0 * pModel->Toxb * Ebs * pModel->Gamma;
			   Vgbwk = Vgbs + Xm + Dxm;
		           Xkb = (pModel->Ccb * (Vfbf - Vthwbold)
			       - pModel->Bb * (Vgbwk - Vfbb) + Xdumv)
			       / pModel->Ab;
		      }
		  }
		  Vthwb = Vfbf - (pModel->Bb * (Vgbwk - Vfbb) + T1)
		        / pModel->Ccb;
	          Xkb = (pModel->Ccb * (Vfbf - Vthwb) - pModel->Bb
		      * (Vgbwk - Vfbb) + Xdumv) / pModel->Ab;
		  T2 = Xkb + Vbi;
		  T3 = T2 * (pInst->Expb - 1.0) - Vdss;
		  T4 = T2 * (1.0 - 1.0 / pInst->Expb) + Vdss;
	          Ybm = log(T3 / T4) / (2.0 * pModel->Aab);
/*		  if (fabs(Ybm - Ybmold) <= 0.05 * Leff)
		      break;                                */
	          Ybmold = Ybm;
	     }
	     if (pModel->Debug && (I >= 20))
	     {   fprintf(stderr, "No convergence in calculating Vthwb.\n");
	     }
	     Xchk = (Vthwf - Vthwb) / Vtm;                                       /* 4.5y */
 	     if(Xchk >= 80.0)                                                    /* 4.5y */
	     {   Vthw = Vthwb;							 /* 4.5y */
	     }                                                                   /* 4.5y */
	     else if(Xchk <= -80.0)                                              /* 4.5y */
	     {   Vthw = Vthwf;							 /* 4.5y */
	     }                                                                   /* 4.5y */
	     else                                                                /* 4.5y */
	     {   Vthw = Vthwf - Vtm * log(1.0 + exp(Xchk));                      /* 4.5y */
	     }									 /* 4.5y */
	     if (Vgfs < Vthw)
	     {   Region = 3;
	         ICONT = 1;
		 Vgstart = Vgfs;
	     }
	     else
	     {   Region = 2;
	         ICONT = 2;
		 Vgstart = Vthw;
	     }
	 }
	 else
	 {   Region = 1;
	     ICONT = 1;
	     Vgstart = Vgfs;
	 }

	 DVgfs = 1.0e-4;                                                         /* 4.5F */
	 Vgfso = Vgfs;
	 if (Region > 1)
	 {   for (I = 0; I < ICONT; I++)
	     {    Vgfs = Vgstart - I * DVgfs;		                 	 /* 4.5F */
		  Vgfbf = Vgfs - Vfbf;
		  /* Call Fring(pInst, pModel, Vgbs, &Vgbwk, &Xkb); */
		  Vgbwk = Vgbs;
		  Xkb = (pModel->Ccb * (Vfbf - Vgfs) - pModel->Bb
		      * (Vgbwk - Vfbb) + Xdumv) / pModel->Ab;
		  if (pModel->Gamma != 0.0)
		  {   T0 = pModel->Toxb / Leff;
		      Dxm = pModel->Kappa * T0 * T0 * Vdss;
		      for (J = 0; J < 2; J++)
		      {    Ebs = -pModel->Aab * ((Xkb + Vbi)                     /* 4.5F */
			       * (1.0 - Coshb) + Vdss) / Sinhb;
	                   Arg = (Ebs - ExfLim) / ExfLim;                        /* 4.5F */
	                   if(Arg < -80.0)                                       /* 4.5F */
	                   { Ebs = ExfLim;                                       /* 4.5F */
	                   }                                                     /* 4.5F */
                           else if(Arg < 80.0)                                   /* 4.5F */
                           { Ebs = ExfLim + ExfLim * log(1.0 + exp(Arg));        /* 4.5F */
	                   }                                                     /* 4.5F */
		           Xm = T0 * pModel->Toxb * Ebs * pModel->Gamma;
			   Vgbwk = Vgbs + Xm + Dxm;
		           Xkb = (pModel->Ccb * (Vfbf - Vgfs)
			       - pModel->Bb * (Vgbwk - Vfbb) + Xdumv)
			       / pModel->Ab;
		      }
		  }
		  Vgwbb = Vgbwk - Vfbb;
		  T2 = Xkb + Vbi;
		  T3 = T2 * (pInst->Expb - 1.0) - Vdss;
		  T4 = T2 * (1.0 - 1.0 / pInst->Expb) + Vdss;
	          Ybm = log(T3 / T4) / (2.0 * pModel->Aab);

	          Arg = pModel->Aab * Ybm;
		  T0 = exp(Arg);
	          Sinh1 = 0.5 * (T0 - 1.0 / T0);
		  T0 = pInst->Expb / T0;
	          Sinh2 = 0.5 * (T0 - 1.0 / T0);
		  Psbmin = (T2 * (Sinh1 + Sinh2) + Sinh1 * Vdss)
		         / Sinhb - Xkb;

	          Xk = (-pModel->C * Vgwbb - pModel->B * Vgfbf
		     + Xdumv) / pModel->A;
		  T2 = Xk + Vbi;
		  T3 = T2 * (pInst->Expf - 1.0) - Vdss;
		  T4 = T2 * (1.0 - 1.0 / pInst->Expf) + Vdss;
	          Yfm = log(T3 / T4) / (2.0 * pModel->Aa);

	          Arg = pModel->Aa * Yfm;
		  T0 = exp(Arg);
	          Sinh1 = 0.5 * (T0 - 1.0 / T0);
		  T0 = pInst->Expf / T0;
	          Sinh2 = 0.5 * (T0 - 1.0 / T0);
		  Psfmin = (T2 * (Sinh1 + Sinh2) + Sinh1 * Vdss)
		         / Sinhf - Xk;

		  T2 = Xkb + Vbi;
	          Arg = pModel->Aab * Yfm;
		  T0 = exp(Arg);
	          Sinh1 = 0.5 * (T0 - 1.0 / T0);
		  T0 = pInst->Expb / T0;
	          Sinh2 = 0.5 * (T0 - 1.0 / T0);
		  Psbfm = (T2 * (Sinh1 + Sinh2) + Sinh1 * Vdss)
		        / Sinhb - Xkb;

		  T2 = Xk + Vbi;
	          Arg = pModel->Aa * Ybm;
		  T0 = exp(Arg);
	          Sinh1 = 0.5 * (T0 - 1.0 / T0);
		  T0 = pInst->Expf / T0;
	          Sinh2 = 0.5 * (T0 - 1.0 / T0);
		  Psfbm = (T2 * (Sinh1 + Sinh2) + Sinh1 * Vdss)
		        / Sinhf - Xk;
		  Dumchg = ELECTRON_CHARGE * Xnin2 * Vtm / pModel->Nbody;
	          Yfb = Yfm - Ybm;
		  Xminf = Coxf * (Psfmin - Vgfbf) / (2.0 * ESI
		        * (pModel->A * Psfmin - pModel->B * Vgfbf
			- pModel->C * Vgwbb));
		  T10 = 10.0;                                                    /* 4.5y */
		  T11 = log(1.0 + exp(T10));				     	 /* 4.5y */
		  Xchk = T10 * (1.0 - Xminf / pModel->Tb);		       	 /* 4.5y */
		  if(Xchk <= -80.0)     				       	 /* 4.5y */
		  {Xminf = pModel->Tb;	       				       	 /* 4.5y */
		  }							       	 /* 4.5y */
		  else if(Xchk <= 80.0) 				       	 /* 4.5y */
		  {Xminf = pModel->Tb * (1.0 - log(1.0 + exp(Xchk))/T11);     	 /* 4.5y */
		  }							       	 /* 4.5y */
		  Xx = pModel->Tb - Xminf;                                       /* 4.5y */
		  Xchk = T10 * (1.0 - Xx / pModel->Tb);                          /* 4.5y */
		  if(Xchk <= -80.0)     					 /* 4.5y */
		  {Xx = pModel->Tb;	       					 /* 4.5y */
		  }								 /* 4.5y */
		  else          		     				 /* 4.5y */
		  {Xx = pModel->Tb * (1.0 - log(1.0 + exp(Xchk))/T11);      	 /* 4.5y */
		  }								 /* 4.5y */
	          Xminf = pModel->Tb - Xx;                                       /* 4.5y */

	          Am = 1.0 / ((1.0 + Coxf * Xminf / ESI)
		     / pModel->A - Xminf * Xminf);
		  if (Am < 0) Am = 0;					         /* 4.41 */
	          Aam = sqrt(2.0 * Am);
		  Bm = Am * (pModel->B * Xminf * Xminf - Coxf
		     * Xminf / ESI) + pModel->B;
	          Cm = pModel->C * (1.0 + Am * Xminf * Xminf);
	          Xkm = (-Cm * Vgwbb - Bm * Vgfbf + Xdumv) / Am;

	          Arg = Aam * Yfm;
		  T0 = exp(Arg);
	          Sinh1 = 0.5 * (T0 - 1.0 / T0);

	          Arg = Aam * Leff;
		  T1 = exp(Arg);
	          Sinh3 = 0.5 * (T1 - 1.0 / T1);
		  T2 = T1 / T0;
	          Sinh2 = 0.5 * (T2 - 1.0 / T2);
		  T3 = Xkm + Vbi;
		  Vxmf = (T3 * (Sinh1 + Sinh2) + Sinh1 * Vdss) / Sinh3
		       - Xkm;

	          Xchk2 = Psfmin / Vtm;                                          /* 4.5y */
	          if(Xchk2 <= -80.0) {Xchk2 = -80.0;}                            /* 4.5y */
	          Xchk3 = Psbfm / Vtm;                                           /* 4.5y */
	          if(Xchk3 <= -80.0) {Xchk3 = -80.0;}				 /* 4.5y */
		  if(pModel->Qm > 0.0)   				         /* 4.5F */
		  { Es1 = Coxf / ESI * (Vgfbf - Psfmin * pModel->Rf);            /* 4.5F */
	            Arg = (Es1 - ExfLim) / ExfLim;                               /* 4.5F */
	            if(Arg < -80.0)                                              /* 4.5F */
	            { Es1 = ExfLim;                                              /* 4.5F */
	            }                                                            /* 4.5F */
                    else if(Arg < 80.0)                                          /* 4.5F */
                    { Es1 = ExfLim + ExfLim * log(1.0 + exp(Arg));               /* 4.5F */
	            }                                                            /* 4.5F */
                    Es2 = Coxb / ESI * (Vgwbb - Psbfm * pModel->Rb);             /* 4.5F */
	            Arg = (Es2 - ExfLim) / ExfLim;                               /* 4.5F */
	            if(Arg < -80.0)                                              /* 4.5F */
	            { Es2 = ExfLim;                                              /* 4.5F */
	            }                                                            /* 4.5F */
                    else if(Arg < 80.0)                                          /* 4.5F */
                    { Es2 = ExfLim + ExfLim * log(1.0 + exp(Arg));               /* 4.5F */
	            }                                                            /* 4.5F */
                    T1 = QmT4 * Es1 * Es1;                                       /* 4.5F */
		    DelEg1 = QmT3 * pow(T1,QmT5);                                /* 4.5F */
                    T1 = QmT4 * Es2 * Es2;                                       /* 4.5F */
		    DelEg2 = QmT3 * pow(T1,QmT5);                                /* 4.5F */
		    DumchgQM1 = Dumchg * exp(- DelEg1 / Vtm);	                 /* 4.5F */
		    DumchgQM2 = Dumchg * exp(- DelEg2 / Vtm);		         /* 4.5F */
		  }                                                              /* 4.5F */
		  else                                                           /* 4.5F */
		  { DumchgQM1 = Dumchg;                                          /* 4.5F */
		    DumchgQM2 = Dumchg;                                          /* 4.5F */
		  }                                                              /* 4.5F */
		  Xchk10 = Vxmf / Vtm;                                           /* 5.0 */
	          if(Xchk10 <= -80.0){Xchk10 = -80.0;}                           /* 5.0 */
	          Xchk4 = Vxmf - Psfmin;                                         /* 5.0 */
	          if(fabs(Xchk4) <= 1.0e-50) {Xchk4 = 1.0e-50;}                  /* 5.0 */
	          Xchk5 = Vxmf - Psbfm;                                          /* 5.0 */
	          if(fabs(Xchk5) <= 1.0e-50) {Xchk5 = 1.0e-50;}                  /* 5.0 */
		  Qmobff = Xminf / Xchk4 * DumchgQM1 		                 /* 5.0 */
                         * (exp(Xchk10) - exp(Xchk2));                           /* 5.0 */
		  Qmobbf = (pModel->Tb - Xminf) / Xchk5 * DumchgQM2              /* 5.0 */
		         * (exp(Xchk10) - exp(Xchk3));                           /* 5.0 */
		  Xminb = Coxf * (Psfbm - Vgfbf) / (2.0 * ESI
		        * (pModel->A * Psfbm - pModel->B * Vgfbf
			- pModel->C * Vgwbb));

		  Xchk = T10 * (1.0 - Xminb / pModel->Tb);			 /* 4.5y */
		  if(Xchk <= -80.0)     					 /* 4.5y */
		  {Xminb = pModel->Tb;						 /* 4.5y */
		  }								 /* 4.5y */
		  else if(Xchk <= 80.0)				    		 /* 4.5y */
		  {Xminb = pModel->Tb * (1.0 - log(1.0 + exp(Xchk))/T11);      	 /* 4.5y */
		  }								 /* 4.5y */
		  Xx = pModel->Tb - Xminb;                                       /* 4.5y */
		  Xchk = T10 * (1.0 - Xx / pModel->Tb);				 /* 4.5y */
		  if(Xchk <= -80.0)						 /* 4.5y */
		  {Xx = pModel->Tb;						 /* 4.5y */
		  }								 /* 4.5y */
		  else								 /* 4.5y */
		  {Xx = pModel->Tb * (1.0 - log(1.0 + exp(Xchk))/T11);		 /* 4.5y */
		  }								 /* 4.5y */
	          Xminb = pModel->Tb - Xx;                                       /* 4.5y */

	          Am = 1.0 / ((1.0 + Coxf * Xminb / ESI)
		     / pModel->A - Xminb * Xminb);
		  if (Am < 0) Am = 0;						 /* 4.41 */
	          Aam = sqrt(2.0 * Am);
		  Bm = Am * (pModel->B * Xminb * Xminb - Coxf
		     * Xminb / ESI) + pModel->B;
	          Cm = pModel->C * (1.0 + Am * Xminb * Xminb);
	          Xkm = (-Cm * Vgwbb - Bm * Vgfbf + Xdumv) / Am;

	          Arg = Aam * Ybm;
		  T0 = exp(Arg);
	          Sinh1 = 0.5 * (T0 - 1.0 / T0);

	          Arg = Aam * Leff;
		  T1 = exp(Arg);
	          Sinh3 = 0.5 * (T1 - 1.0 / T1);
		  T2 = T1 / T0;
	          Sinh2 = 0.5 * (T2 - 1.0 / T2);

		  T3 = Xkm + Vbi;
		  Vxmb = (T3 * (Sinh1 + Sinh2) + Sinh1 * Vdss) / Sinh3 - Xkm;

	          Xchk2 = Psfbm / Vtm;                                           /* 4.5y */
	          if(Xchk2 <= -80.0) {Xchk2 = -80.0;}                            /* 4.5y */
	          Xchk3 = Psbmin / Vtm;                                          /* 4.5y */
	          if(Xchk3 <= -80.0) {Xchk3 = -80.0;}                            /* 4.5y */
		  if(pModel->Qm > 0.0)   				         /* 4.5F */
		  { Es1 = Coxf / ESI * (Vgfbf - Psfbm * pModel->Rf);             /* 4.5F */
	            Arg = (Es1 - ExfLim) / ExfLim;                               /* 4.5F */
	            if(Arg < -80.0)                                              /* 4.5F */
	            { Es1 = ExfLim;                                              /* 4.5F */
	            }                                                            /* 4.5F */
                    else if(Arg < 80.0)                                          /* 4.5F */
                    { Es1 = ExfLim + ExfLim * log(1.0 + exp(Arg));               /* 4.5F */
	            }                                                            /* 4.5F */
                    Es2 = Coxb / ESI * (Vgwbb - Psbmin * pModel->Rb);            /* 4.5F */
	            Arg = (Es2 - ExfLim) / ExfLim;                               /* 4.5F */
	            if(Arg < -80.0)                                              /* 4.5F */
	            { Es2 = ExfLim;                                              /* 4.5F */
	            }                                                            /* 4.5F */
                    else if(Arg < 80.0)                                          /* 4.5F */
                    { Es2 = ExfLim + ExfLim * log(1.0 + exp(Arg));               /* 4.5F */
	            }                                                            /* 4.5F */
                    T1 = QmT4 * Es1 * Es1;                                       /* 4.5F */
		    DelEg1 = QmT3 * pow(T1,QmT5);                                /* 4.5F */
                    T1 = QmT4 * Es2 * Es2;                                       /* 4.5F */
		    DelEg2 = QmT3 * pow(T1,QmT5);                                /* 4.5F */
		    DumchgQM1 = Dumchg * exp(- DelEg1 / Vtm);	                 /* 4.5F */
		    DumchgQM2 = Dumchg * exp(- DelEg2 / Vtm);		         /* 4.5F */
		  }                                                              /* 4.5F */
		  else                                                           /* 4.5F */
		  { DumchgQM1 = Dumchg;                                          /* 4.5F */
		    DumchgQM2 = Dumchg;                                          /* 4.5F */
		  }                                                              /* 4.5F */
		  Xchk10 = Vxmb / Vtm;                                           /* 5.0 */
	          if(Xchk10 <= -80.0){Xchk10 = -80.0;}                           /* 5.0 */
	          Xchk4 = Vxmb - Psfbm;                                          /* 5.0 */
	          if(fabs(Xchk4) <= 1.0e-50) {Xchk4 = 1.0e-50;}                  /* 5.0 */
	          Xchk5 = Vxmb - Psbmin;                                         /* 5.0 */
	          if(fabs(Xchk5) <= 1.0e-50) {Xchk5 = 1.0e-50;}                  /* 5.0 */
		  Qmobfb = Xminb / Xchk4 * DumchgQM1 	                         /* 5.0 */
                         * (exp(Xchk10) - exp(Xchk2));                           /* 5.0 */
		  Qmobbb = (pModel->Tb - Xminb) / Xchk5 * DumchgQM2 *            /* 5.0 */
		           (exp(Xchk10) - exp(Xchk3));                           /* 5.0 */
		  Qmobf = (Qmobff+Qmobfb)/2.0;                                   /* 4.5y */
		  Qmobb = (Qmobbf+Qmobbb)/2.0;                                   /* 4.5y */

		  /* JUNCTION FIELDS (= dVsf,b/dy|y=0,L)  */
		  T0 = Xk + Vbi;
		  Efs = -pModel->Aa * (T0 * (1.0 - Coshf) + Vdss)
		      / Sinhf;
		  if (Efs < 0.0)
		      Efs = -Efs;
		  Efd = -pModel->Aa * (T0 * (Coshf - 1.0) + Vdss
		      * Coshf) / Sinhf;
		  if (Efd < 0.0)
		      Efd = -Efd;
		  T0 = Xkb + Vbi;
		  Ebs = -pModel->Aab * (T0 * (1.0 - Coshb) + Vdss)
		      / Sinhb;
		  if (Ebs < 0.0)
		      Ebs = -Ebs;
		  Ebd = -pModel->Aab * (T0 * (Coshb - 1.0) + Vdss
		      * Coshb) / Sinhb;
		  if (Ebd < 0.0)
		      Ebd = -Ebd;

		 /* CALCULATIONS OF EFFECTIVE CHANNEL LENGTH  */
		  if (Psfmin < 0.0)
		      Psfmin = 0.0;
		  if (Psbmin < 0.0)
		      Psbmin = 0.0;
		  Xftl = 2.0 * ((Vbi - Psfmin) / Efs
		       + (Vbi + Vdss - Psfmin) / Efd);
		  Xbtl = 2.0 * ((Vbi - Psbmin) / Ebs
		       + (Vbi + Vdss - Psbmin) / Ebd);
		  Lf = Leff - Xftl;                                              /* 4.5 */
/*                  T1 = Lf / Leff;                                                 5.0 */
		  Lb = Leff - Xbtl;                                              /* 4.5 */
/*                  T2 = Lb / Leff;                                                 5.0 */
/*                  if (T1 < 1.0e-3) Lf = Leff * 1.0e-3;                            5.0 */
/*                  if (T2 < 1.0e-3) Lb = Leff * 1.0e-3;                            5.0 */
/*		    if (Lf < pTempModel->Lemin) Lf = pTempModel->Lemin;    */    /* 4.5 */
/*		    if (Lb < pTempModel->Lemin) Lb = pTempModel->Lemin;    */    /* 4.5 */
                  if (Lf < LEMIN) Lf = LEMIN;                                    /* 6.1 */
                  if (Lb < LEMIN) Lb = LEMIN;                                    /* 6.1 */

		  /* EFFECTIVE MOBILITY CALCULATIONS  */
		  /* LIMIT EXF and EXB TO 0 TO AVOID SIGN CHANGE AND NEGATIVE MOBILITIES */
		  Exf = Coxf * (Vgfs - Vfbf - Psfmin * pModel->Rf) / ESI;        /* 4.5 */
/*		  if(Exf < 0.0) Exf = 0.0;                                      */ /* 4.5 */
		  Arg = (Exf - ExfLim) / ExfLim;                                 /* 6.0 */
		  if(Arg < -80.0)                                                /* 6.0 */
	          { Exf = ExfLim;                                                /* 6.0 */
	          }                                                              /* 6.0 */
		  else if(Arg < 80.0)                                            /* 6.0 */
                  { Exf = ExfLim + ExfLim * log(1.0 + exp(Arg));                 /* 6.0 */
	          }                                                              /* 6.0 */
		  Uef = pTempModel->Uo / (1.0 + pModel->Theta * Exf);            /* 4.5 */
		  Exb = Coxb * (Vgbs - Vfbb - Psbmin * pModel->Rb) / ESI;        /* 4.5 */
/*		  if(Exb < 0.0) Exb = 0.0;                                      */ /* 4.5 */
		  Arg = (Exb - ExfLim) / ExfLim;                                 /* 6.0 */
		  if(Arg < -80.0)                                                /* 6.0 */
	          { Exb = ExfLim;                                                /* 6.0 */
	          }                                                              /* 6.0 */
		  else if(Arg < 80.0)                                            /* 6.0 */
                  { Exb = ExfLim + ExfLim * log(1.0 + exp(Arg));                 /* 6.0 */
	          }                                                              /* 6.0 */
		  Ueb = 0.8 * pTempModel->Uo / (1.0 + pModel->Theta * Exb);      /* 4.5 */

		  T11 = 1.0 - exp(-Vdss / Vtm);
		  Iwkf = Weff / Lf * Vtm * Uef * Qmobf;                          /* 5.0 */
		  Iwkb = Weff / Lb * Vtm * Ueb * Qmobb;                          /* 5.0 */
		  Iwk = (Iwkf + Iwkb) * T11;                                     /* 5.0 */
                  Ueffwk = (Uef * Iwkf + Ueb * Iwkb) / (Iwkf + Iwkb);            /* 5.0 */
		  Lewk = (Lf * Iwkf + Lb * Iwkb) / (Iwkf + Iwkb);                /* 5.0 */
		  /* MCJ */
		  Xemwk = 0.5 * Vdss * (1.0 / pModel->Lc + 1.0 / pModel->Lcb);
                  if(ACNeeded == 1)                                              /* 5.0 */
                  { Ich0 = Iwkf + Iwkb;                                          /* 5.0 */
                    Ich0 = 2.0 * Ich0 - Iwk;                                     /* 5.0 */
	            if (Ich0 < 1.0e-40) Ich0 = 1.0e-40;                          /* 5.0 */
                    Sich = 2.0 * ELECTRON_CHARGE * Ich0;                         /* 5.0 */
	            pOpInfo->Sich = Sich;                                        /* 5.0 */
                  }                                                              /* 5.0 */
		  if (I == 0)
		  {   Xk11 = Xk;
		      Vgbwk11 = Vgbwk;
		      Iwk1 = Iwk;
		      Qnwk1 = -(Qmobf + Qmobb) * Weff * Leff;                    /* 4.5 */
		      Ueffwk1 = Ueffwk;                                          /* 4.5 */
		      Xemwk1 = Xemwk;                                            /* 4.5 */
		      Lewk1 = Lewk;                                              /* 4.5 */
                      if(ACNeeded == 1) Siw1 = Sich;                             /* 5.0 */
		  }
		  else
		  {   Xk22 = Xk;
		      Vgbwk22 = Vgbwk;
		      Iwk2 = Iwk;
		      Qnwk2 = -(Qmobf + Qmobb) * Weff * Leff;                    /* 4.5 */
                      if(ACNeeded == 1) Siw2 = Sich;                             /* 5.0 */
		  }
	     }
	     if (ICONT == 1)
	     {   Ich = Iwk;
		 Qn = -(Qmobf + Qmobb) * Weff * Leff;                            /* 4.5 */
		 Qd = Qs = 0.0;                                                  /* 4.5 */
	         pOpInfo->Ueff = Ueffwk;                                         /* 4.5 */
		 pOpInfo->Le = Lewk;                                             /* 4.5 */
	         pOpInfo->Vdsat = 0.0;                                           /* 4.5 */
		 Xem = Xemwk;
	     }
	 }
	 if (Region < 3)
	 {   if (Region == 2)Vgstart = Vths;
             for (I = 0; I < ICONT; I++)
	     {    Vgfs = Vgstart + I * DVgfs;		                 	 /* 4.5F */
                  if (pModel->Qm == 0.0 || Region == 1)                          /* 4.5F */
	          { Vthsx = Vths1;                                               /* 4.5F */
                    Pst = Pst1;                                                  /* 4.5F */
                    Psb = Psb1;                                                  /* 4.5F */
                    Qbeff = Qbeff1;                                              /* 4.5F */
                    Qst = Qst1;                                                  /* 4.5F */
	          }                                                              /* 4.5F */
	          else                                                           /* 4.5F */
                  { Pst = Pst0;                                                  /* 4.5F */
	            for (K = 0; K < 3; K++)                                      /* 4.5F */
	            { PstOld = Pst;                                              /* 4.5F */
                      Ess = Coxf / ESI * (Vgfs - Vfbf - PstOld * pModel->Rf);    /* 4.5F */
	              Arg = (Ess - ExfLim) / ExfLim;                             /* 4.5F */
	              if(Arg < -80.0)                                            /* 4.5F */
	              { Ess = ExfLim;                                            /* 4.5F */
                      }                                                          /* 4.5F */
                      else if(Arg < 80.0)                                        /* 4.5F */
                      { Ess = ExfLim + ExfLim * log(1.0 + exp(Arg));             /* 4.5F */
                      }                                                          /* 4.5F */
                      T1 = QmT4 * Ess * Ess;                                     /* 4.5F */
                      DelEg = QmT3 * pow(T1,QmT5);                               /* 4.5F */
                      XninQM = Xnin * exp(-DelEg / 2.0 / Vtm);                   /* 4.5F */
                      PhibQM = 2.0 * Vtm * log(pModel->Nbody / XninQM);          /* 4.5F */
                      T1 = PstOld / Vtm + exp((PstOld - PhibQM) / Vtm);          /* 4.5F */
                      Pst = PhibQM + Vtm * log(Coeffz * sqrt(T1));               /* 4.5F */
	            }		              		      	                 /* 4.5F */
                    Psblong = (Vgbs - Vfbb + (0.5 * pInst->Qb + Cb * Pst) / Coxb)/* 4.5F */
		            * Cap1;                                              /* 4.5F */
                    if (Psblong < 0.0)Psblong = 0.0;                             /* 4.5F */
                    else if (Psblong > Pst)Psblong = Pst;                        /* 4.5F */
                    Psb = Psblong;                                               /* 4.5F */
                    for (J = 0; J < 3; J++)                                      /* 4.5F */
                    { Psb = MIN(Psb, Vbi);                                       /* 4.5F */
	              PsbOld = Psb;                                              /* 4.5F */
	              RootEtb = sqrt(QmT2 * (Vbi - Psb));                        /* 4.5F */
	              T2 = 0.4 * (Vbi * Part + (1.0 - Part) * Psb)               /* 4.5F */
	                 - 0.2 * Psb - 0.2 * (Vgbs - Vfbb);                      /* 4.5F */
	              Etb = RootEtb + Coxb / ESI * T2;                           /* 4.5F */
	              D = (Vbi - Psb) * Part / Etb;                              /* 4.5F */
	              if (D < 0.0) D = 0.0;                                      /* 4.5F */
	              else if (D > 0.7 * Leff) D = 0.7 * Leff;                   /* 4.5F */
	              Psb = Psblong - 0.5 * D / Leff * pInst->Qb / Coxb * Cap1;  /* 4.5F */
	              Psb = MIN(Psb, Pst);                                       /* 4.5F */
	            }                                                            /* 4.5F */
                    Qbeff = pInst->Qb * (1.0 - D / Leff);                        /* 4.5F */
                    T2 = Pst / Vtm + exp((Pst - PhibQM) / Vtm);                  /* 4.5F */
                    Qst = -pTempModel->Qr * sqrt(T2) - pInst->Qb;                /* 4.5F */
                    Vthsx = Vfbf - QmT1 * (Vgbs - Vfbb) + Pst * pModel->Xalpha   /* 4.5F */
                         - (0.5 * Qbeff * pModel->Xbeta + Qst) / Coxf + Delvtf;  /* 4.5F */
	          }
		  if(pModel->Ngate == 0.0 || pModel->Tpg == -1.0)                /* 4.5pd */
		  {  Psigf = 0.0;                                                /* 4.5pd */
		     Alphap = pModel->Xalpha;                                    /* 4.5pd */
		  }                                                              /* 4.5pd */
		  else                                                           /* 4.5pd */
		  {  T0 = ELECTRON_CHARGE * pModel->Ngate *                      /* 4.5pd */
		          SILICON_PERMITTIVITY;                                  /* 4.5pd */
		     T1 = Coxf * Coxf * (Vgfs - pTempModel->Wkf - Pst);          /* 4.5pd */
                     Psigf = (T0 + T1 - sqrt(T0 * (T0 + 2.0 * T1))) /            /* 4.5pd */
                          (Coxf * Coxf);                                         /* 4.5pd */
		     T1 = sqrt(T0 / (2.0 * Psigf)) / Coxf;                       /* 4.5pd */
		     Alphap = pModel->Xalpha - 1.0 / (1.0 + T1);                 /* 4.5pd */
		  }                                                              /* 4.5pd */
	          Vgfst = Vgfs - Vthsx;                                          /* 4.5qm */
		  Vgfstp = Vgfs - Vthsx - Psigf;                                 /* 4.5qm */

		  T10 = Vgfst - Qst / Coxf;
		  Vo1 = T10 / pModel->Xalpha;
		  Vo1p = (Vgfstp - Qst / Coxf) / Alphap;                         /* 4.5pd */

		  T1 = 0.5 * pModel->Theta * Coxf / (pModel->Tb * Cb);
		  Xbfact = pModel->Bfact * (2.0 * pModel->Rf - pModel->Xalpha)
			 * T1;
		  Mobred =  1.0 + T1 * (T10 - (Qbeff - 2.0
		         * Cb * (Pst - Psb)) / Coxf);  
		  Mobred = 1.0 / Mobred;
		  Xbmu = Mobred * Xbfact;
		  T1 = 1.0 + Vo1p * (Xbmu + Mobred * pTempModel->Fsat);          /* 4.5pd */
		  T2 = 0.25 - Xbmu * Vo1p / (T1 * T1);                           /* 4.5pd */
		  if (T2 < 0.0)
		      T2 = -T2;
		  Vdsat = Vo1p / (T1 * (0.5 + sqrt(T2)));                        /* 4.5pd */
		      if (Vdsat < 1.0e-15)                                       /* 4.5 */
		          Vdsat = 1.0e-15;                                       /* 4.5 */
		      T1 = 8.0 * (1.0 - Vdss / Vdsat);
		      Covf = 1.0 - log(1.0 + exp(T1)) / log(1.0 + exp(8.0));
		      Vdsat = Vdsat * Covf;
		      Mobref = Mobred / (1.0 - Xbmu * Vdsat);
		      Arg2 = (Vdss - Vdsat) * Mobref
		           * pTempModel->Fsat / Lcfac;
		      T3 = Arg2 + sqrt(1.0 + Arg2 * Arg2);
		      Lefac = 1.0 - Lcfac * log(T3);
		      if (Lefac < 1.0e-3)
		          Lefac = 1.0e-3;
		      Arg3 = 1.0 + Vo1p * (Xbmu + pTempModel->Fsat               /* 4.5pd */
		           * Mobred / Lefac);
		      T2 = 0.25 - Xbmu * Vo1p / (Arg3 * Arg3);                   /* 4.5pd */
		      if (T2 < 0.0)
		          T2 = -T2;
		      Vdsat = Vo1p / (Arg3 * (0.5 + sqrt(T2)));                  /* 4.5pd */
		  if (Vdsat < 1.0e-15)                                           /* 4.5 */
		      Vdsat = 1.0e-15;                                           /* 4.5 */
                  Vdseff = Vdsat;
		  T1 = 8.0 * (1.0 - Vdss / Vdseff);
		  Covf = 1.0 - log(1.0 + exp(T1)) / log(1.0 + exp(8.0));
		  Vdsx = Vdseff * Covf;
		  Mobref = Mobred / (1.0 - Xbmu * Vdsx);
		  Ueffst = Mobref * pTempModel->Uo;                              /* 4.5 */

		  T3 = Vo1p - Vdsx;                                              /* 4.5pd */
		  Ist = pTempModel->Beta0 * Mobref * Alphap                      /* 4.5pd */
		      * (Vo1p * Vo1p - T3 * T3) / (1.0                           /* 4.5pd */
		      + pTempModel->Fsat * Mobref * Vdsx / Lefac) / Lefac;
                  if(ACNeeded == 1)                                              /* 5.0 */
                  { T = Vtm / KoverQ;                                            /* 5.0 */
		    Le = Lefac * Leff;                                           /* 5.0 */
		    Exo = (Qst - Qbeff) / (2.0 * SILICON_PERMITTIVITY)           /* 5.0 */
                        + (Pst - Vbs) / pModel->Tb;                              /* 5.0 */
                    Ueffo = pTempModel->Uo / (1.0 + pModel->Theta * Exo);        /* 5.0 */
                    if(Ich == 0.0)                                               /* 5.0 */
                    { Sich = 4.0 * Weff * Boltz * T * Ueffo * Qst / Leff;        /* 5.0 */
	 	    }                                                            /* 5.0 */
		    else                                                         /* 5.0 */
		    { Lc = pModel->Lc;                                           /* 5.0 */
		      Sich = XIntSich(Weff, Leff, T, Vdsx, Le, Lc, Ist, Qst,     /* 5.0 */
                                      Ueffo);                                    /* 5.0 */
		    }                                                            /* 5.0 */
		    pOpInfo->Sich = Sich;                                        /* 5.0 */
		  }                                                              /* 5.0 */
		  if (I == 0)
		  {   Ist1 = Ist;
		      Mobref1 = Mobref;
		      Vdsx1 = Vdsx;						 /* 4.41 */
		      Vdseff1 = Vdseff;                                          /* 4.5 */
		      Mobred1 = Mobred;
		      Xbmu1 = Xbmu;
		      Lefac1 = Lefac;					         /* 4.41 */
		      Psigf1 = Psigf;                                            /* 4.5pd */
		      Alphap1 = Alphap;                                          /* 4.5pd */
                      Vthsx1 = Vthsx;                                            /* 4.5qm */
                      Pst1 = Pst;                                                /* 4.5qm */
                      Qst1 = Qst;                                                /* 4.5qm */
		      Ueffst1 = Ueffst;                                          /* 4.5 */
                      if(ACNeeded == 1) Sis1 = Sich;                             /* 5.0 */
		      if (Vdss > Vdseff)
		      {   Xemst = (Vdss - Vdseff) / pModel->Lc;
		      }
		      else
		      {   Xemst = 0.0;
		      }
		      Xemst1 = Xemst;                                            /* 4.5 */
		  }
		  else
		  {   Ist2 = Ist;
		      Mobref2 = Mobref;
		      Vdsx2 = Vdsx;						 /* 4.41 */
		      Mobred2 = Mobred;
		      Xbmu2 = Xbmu;
		      Lefac2 = Lefac;						 /* 4.41 */
		      Psigf2 = Psigf;                                            /* 4.5pd */
		      Alphap2 = Alphap;                                          /* 4.5pd */
                      Vthsx2 = Vthsx;                                            /* 4.5qm */
                      Pst2 = Pst;                                                /* 4.5qm */
                      Qst2 = Qst;                                                /* 4.5qm */
                      if(ACNeeded == 1) Sis2 = Sich;                             /* 5.0 */
		  }
	     }
	     if (ICONT == 1)
	     {   Ich = Ist;
	         Xem = Xemst;
		 pOpInfo->Ueff = Ueffst;                                         /* 4.5 */
		 pOpInfo->Vdsat = Vdseff;                                        /* 4.5 */
		 pOpInfo->Le = Lefac * Leff;                                     /* 4.5 */
	     }
	     else /* Region = 2 */
	     {   Vgfs = Vgfso;
	         DeltaVth = Vths - Vthw;
		 T0 = Vgfs - Vthw;
	         if (Vds == 0.0)
		 {   Ich = 0.0;   
		 }
		 else
		 {   
		     if(Ist1 <1.e-30) Ist1=1.e-30;				 /* 4.41 */
		     if(Ist2 <1.e-30) Ist2=1.e-30;				 /* 4.41 */
		     if(Iwk1 <1.e-30) Iwk1=1.e-30;				 /* 4.41 */
		     if(Iwk2 <1.e-30) Iwk2=1.e-30;				 /* 4.41 */
                     T1 = log(Ist1);
		     T2 = log(Ist2);
		     T3 = log(Iwk1);
		     T4 = log(Iwk2);
		     Gst = (T2 - T1) / DVgfs;                                    /* 4.5F */
		     Gwk = (T3 - T4) / DVgfs;                                    /* 4.5F */
		     R2 = (3.0 * (T1 - T3) / DeltaVth - Gst - 2.0
		        * Gwk) / DeltaVth;
		     R3 = (2.0 * (T3 - T1) / DeltaVth + Gst + Gwk)
		        / (DeltaVth * DeltaVth);
		     Ich = T3 + T0 * (Gwk + T0 * (R2 + T0 * R3));
		     Ich = exp(Ich);
	         }
		 Xem = (Xemst1 * T0 + Xemwk1 * (Vths - Vgfs)) / DeltaVth;
		 pOpInfo->Ueff = Ueffwk1 + (Ueffst1 - Ueffwk1) * T0 / DeltaVth;  /* 4.5 */
		 pOpInfo->Vdsat = Vdseff1 * T0 / DeltaVth;                       /* 4.5 */
		 pOpInfo->Le = Lewk1 + (Lefac1 * Leff - Lewk1) * T0 / DeltaVth;  /* 4.5 */
                 if(ACNeeded == 1)                                               /* 5.0 */
                 { T1 = log(Sis1);                                               /* 5.0 */
                   T2 = log(Sis2);                                               /* 5.0 */
                   Gst = (T2 - T1) / DVgfs;                                      /* 5.0 */
                   T3 = log(Siw1);                                               /* 5.0 */
                   T4 = log(Siw2);                                               /* 5.0 */
                   Gwk = (T3 - T4) / DVgfs;                                      /* 5.0 */
                   R2 = (3.0 * (T1 - T3) / DeltaVth - Gst - 2.0                  /* 5.0 */
                      * Gwk) / DeltaVth;                                         /* 5.0 */
                   R3 = (2.0 * (T3 - T1) / DeltaVth + Gst + Gwk )                /* 5.0 */
                      / (DeltaVth * DeltaVth);                                   /* 5.0 */
                   Sich = exp(T3 + T0 * (Gwk + T0 * (R2 + R3 * T0)));            /* 5.0 */
		   pOpInfo->Sich = Sich;                                         /* 5.0 */
                 }                                                               /* 5.0 */
	     }
	 }
	 else
	 {   Vdseff = 0.0;
	 }
	 /* Parasitic BJT current */
	 if (pModel->Bjt == 1)                                                   /* 4.5 */

/*	 {   Vref = 0.5 * (Vfbf + 0.6 + Vths_bjt);                  */             /* 4.5qm */
/*	     Xs = 2.3 * pModel->Xalpha * Vtm;                       */
/*	     Psf_bjt = Pst_bjt - (Pst_bjt - Vbs_bjt - Vtm)          */
/*	             / (1.0 + exp((Vgfs - Vref) / Xs));             */             /* 5.0 */

	 {   Xs = 2.3 * pModel->Xalpha * Vtm;                                    /* 6.0 */
             Vref = 0.5 * (Vfbf + 0.8 + Vths_bjt);                               /* 6.0 */
             T0 = fabs(Vthw - (Vfbf + 0.8)) + Vtm;                               /* 6.0 */
             T1 = (Vgfs - Vref)/T0;                                              /* 6.0 */
             Psf_bjt = ((1.0 / (1.0 + exp(-T1))) * (Pst_bjt) +                            
                      (1.0 / (1.0 + exp(T1))) * (Vbs_bjt));                      /* 6.0 */

	     T0 = Vfbb + (1.0 + Cb / Coxb) * pTempModel->Phib
		- pInst->Qb * (1.0 - D_bjt / Leff) / (2.0 * Coxb)                /* 4.5qm */
		- Cb / Coxb * Vbs_bjt;
	     T3 = 0.5 * (Vfbb + 0.6 + T0);
	     T1 = (Vgbs - T3) / Xs;
	     T12 = exp(T1);
	     Psb_bjt = pTempModel->Phib - (pTempModel->Phib - Vbs_bjt
	             - Vtm) / (1.0 + T12);                                       /* 4.5F */
/*	       Xlbjt = Leff - Ds - Dd;      */
             Xlbjt = Leff;                                                       /* 6.1 */

/*             if (Xlbjt / Leff < 1.0e-3) Xlbjt = Leff * 1.0e-3;                    5.0 */
/*             if (pModel->Toxb < 10.0 * pModel->Toxf) Xlbjt = Leff;                5.0 */

/*	     if (Xlbjt < pTempModel->Lemin) Xlbjt = pTempModel->Lemin;                   */

	     Xaf = (Psf_bjt - Vbs_bjt) / (Vtm * pModel->Tb);
	     T1 = Xaf * pModel->Tb;
	     if (T1 > 50.0) T1 = 50.0;
	     Exp_af = exp(T1);

	     Xbf = (Psf_bjt - Psb_bjt) / (pModel->Tb * Vtm);
	     /* To avoid overflow, set lower limit for Xbf, 
	        i.e., limit (Psf_bjt - Psb_bjt) to 1.0e-3*Vtm */
	     T3 = 1.0e-3 / pModel->Tb;                                           /* 5.0 */   
	     T4 = 10.0 * (Xbf / T3 - 1.0);                                       /* 5.0 */
	     if (T4 >= 80.0)                                                     /* 5.0 */
	     {  T5 = Xbf / T3;                                                   /* 5.0 */
	     }                                                                   /* 5.0 */
	     else if (T4 <= -80.0)                                               /* 5.0 */
	     {  T5 = 1.0;                                                        /* 5.0 */
	     }                                                                   /* 5.0 */
	     else                                                                /* 5.0 */
	     {  T5 = 1.0 + log(1.0 + exp(T4)) / log(1.0 + exp(10.0));            /* 5.0 */
	     }                                                                   /* 5.0 */
	     Xbf = T3 * T5;                                                      /* 5.0 */
	     T2 = Xbf * pModel->Tb;
	     if (T2 > 50.0) T2 = 50.0;
	     Exp_bf = exp(T2);

	     Ph_f = Xnin * Sqrt_s;
	     P0f = pModel->Nbody / Exp_af;
	     T10 = 2.0 * P0f / Ph_f;
	     Tf1 = 1.0 + T10;
	     Tf2 = 1.0 + T10 * Exp_bf;
	     Xchk = (1.0 - 1.0 / Tf2) / (1.0 - 1.0 / Tf1);			 /* 4.5 */
	     Xintegf = log(Xchk) / (Ph_f * Xbf);				 /* 4.5 */

	     Ph_r = Xnin * Sqrt_d;
	     T10 = 2.0 * P0f / Ph_r;
	     Tr1 = 1.0 + T10;
	     Tr2 = 1.0 + T10 * Exp_bf;
	     Xchk = (1.0 - 1.0 / Tr2) / (1.0 - 1.0 / Tr1);			 /* 4.5 */
	     Xintegr = log(Xchk) / (Ph_r * Xbf);				 /* 4.5 */

	     if (pModel->Fvbjt <= 0)
	     {   Xleff = Xlbjt;
	     }
	     else
	     {   Feff = pModel->Fvbjt / (1.0 + exp((Vref - Vgfs) / Xs));	 /* 5.0 */
	         Xleff = 1.0 / (Feff / pModel->Tb
		       + (1.0 - Feff) / Xlbjt);
	     }

	     T11 = Vtm * pTempModel->Mubody;
	     T0 = ELECTRON_CHARGE * Weff * 2.0 * T11 * Xnin2 / Xleff;
	     Ibjtf = T0 * Xintegf;
	     Ibjtr = T0 * Xintegr;
	     Ibjt = Ibjtf * Argbjts - Ibjtr * Argbjtd;
	 }
	 else
	 {   Ibjt = 0.0;
	 }
	 Itot = Ich + Ibjt;
	 Eslope = ELECTRON_CHARGE * pModel->Nldd / ESI;
	 T0 = Itot * pTempModel->Rldd;
	 if ((Ldd <= 0.0) || (Vds < 1.0e-6) || (Xem < T0) 
                          || (pModel->Nldd >= 1.0e25)) Vlddd = 0.0;              /* 4.5 */
         else if ((Xem - T0) < Eslope * Ldd)
	 {   T1 = (Xem - T0);
	     Vlddd = T1 * T1 / (2.0 * Eslope);
	 }
	 else
	 {   Vlddd = 0.5 * Ldd * (2.0 * Xem - Eslope * Ldd)
	           - T0 * Ldd;
	 }
	 Vd[IB] = Vldd[IB] - Vlddd;
	 if (fabs(Vd[IB]) < 1.0e-6)
	     break;
         if (IB > 0)
	 {   T1 = Vd[IB] - Vd[IB-1];
	     if (T1 == 0.0)
	     {   fprintf(stderr, "Error in Vldd calculation.\n");
	         Vlddd = Vldd[IB];
	         break;
	     }
	     Vldd[IB+1] = (Vd[IB] * Vldd[IB-1] - Vd[IB-1] * Vldd[IB])
	                / T1;
	     if (Vldd[IB+1] > Vds)
	         break;
	 }
    }
    if (pModel->Debug && (IB >= 30))
    {   fprintf(stderr, "No convergence in Vldd interation.\n");
    }

    /* Thermal generation and recombination currents */
    Irs0 = pTempModel->Jro * Psj;                                                /* 4.5 */
    Ird0 = pTempModel->Jro * Pdj;                                                /* 4.5 */
    Irs1 = ELECTRON_CHARGE * Xnin2 * pTempModel->Seff * Psj                      /* 4.5 */
	 * pModel->Tb / pTempModel->Ndseff;
    Ird1 = Irs1 / Psj * Pdj;                                                     /* 4.5 */
    T3 = Vbs - Vdss;
    T1 = Vbs;
    /* Vexplx Gexplx and Ioffsetx defined in terms of Weff, not PSJ or PDJ.
       Cannot us PSJ/PDJ because we did not know which mode the device would
       be in during pre-processing. This should cause a discontinuity in  
       Ir and Igt near Imax (because PSJ, PDJ >= Weff).
       (e.g. if PSJ > Weff, Ir0 > Imax  for Vbs slightly less than Vexpl2.)         4.5 */
    if (T3 >= pTempModel->Vexpl2)
    {   T10 = pTempModel->Ioffset2 + pTempModel->Gexpl2 * T3;
    }
    else
    {   T10 = Ird1 * (exp(T3 / Vtm) - 1.0);
    }
    if (T3 >= pTempModel->Vexpl3)
    {   Igt_r = pTempModel->Ioffset3 + pTempModel->Gexpl3 * T3 + T10;
    }
    else
    {   Igt_r = Ird0 * (exp(T3 / (pModel->M * Vtm)) - 1.0) + T10;
    }
    if (T1 >= pTempModel->Vexpl2)
    {   T10 = pTempModel->Ioffset2 + pTempModel->Gexpl2 * T1;
    }
    else
    {   T10 = Irs1 * (exp(T1 / Vtm) - 1.0);
    }
    if (T1 >= pTempModel->Vexpl3)
    {   Ir_r = pTempModel->Ioffset3 + pTempModel->Gexpl3 * T1 + T10;
    }
    else
    {   Ir_r = Irs0 * (exp(T1 / (pModel->M * Vtm)) - 1.0) + T10;
    }
    if (pTempModel->Tauo == 0.0)                                                 /* 4.5 */
    {   Igt = Igt_r;                                                             /* 4.5 */
        Ir = Ir_r;                                                               /* 4.5 */
    }                                                                            /* 4.5 */
    else                                                                         /* 4.5 */
    {   Igts0 = ELECTRON_CHARGE * Xnin * Psj                                     /* 4.5 */
          * pModel->Tb * pTempModel->D0 / pTempModel->Taug;
        Igtd0 = Igts0 / Psj * Pdj;                                               /* 4.5 */
        Igtb0 = ELECTRON_CHARGE * Xnin * Weff * Leff * pModel->Tb /              /* 4.5 */
                pTempModel->Taug;                                                /* 4.5 */
	Rgtd = Igtd0 / Ird0;
        Rgts = Igts0 / Irs0;
        if (T3 >= pTempModel->Vexpl1)
        {   Igt_g = pTempModel->Ioffset1 + pTempModel->Gexpl1 * T3;
	    T13 = Igt_g / Igtd0 + 1.0;
        }
        else
        {   T13 = exp(T3 / Vtm);
            Igt_g = Igtd0 * (T13 - 1.0);
        }
        if (T1 >= pTempModel->Vexpl1)
        {   Ir_g = pTempModel->Ioffset1 + pTempModel->Gexpl1 * T1;
  	    T11 = Ir_g / Igts0 + 1.0;
        }
        else
        {   T11 = exp(T1 / Vtm);
            Ir_g = Igts0 * (T11 - 1.0);
        }
        Igt = (Igt_g + Igt_r * Rgtd * T13) / (1.0 + Rgtd * T13);
        Ir = (Ir_g + Ir_r * Rgts * T11) / (1.0 + Rgts * T11);
        /* ACCOUNT FOR BODY GENERATION CURRENT (ONLY WHEN TAUO IS SPECIFIED)        4.5 */
        T3 = -Vdss;                                                              /* 4.5 */
        if (T3 >= pTempModel->Vexpl4)                                            /* 4.5 */
	{   Igt += pTempModel->Ioffset4 + pTempModel->Gexpl4 * T3;               /* 4.5 */
        }                                                                        /* 4.5 */
        else                                                                     /* 4.5 */
        {   T13 = exp(T3 / Vtm);                                                 /* 4.5 */
	    Igt += Igtb0 * (T13 - 1.0);                                          /* 4.5 */
        }                                                                        /* 4.5 */
    }                                                                            /* 4.5 */

	/* 4.41 : Add GIDL */
      if(pModel->Bgidl > 0.0) 
      { T0 = pModel->Nqff + 2.0 * pModel->Nqfsw * pModel->Tb / pInst->Weff;      /* 4.5r */
	T4 = exp(-50.0 * (Vbs - Vdss));                                          /* 6.0 */
/*	Vbdo = log(2.0) / 50.0;                 */                                 /* 6.0 */
/*	Vbdx = Vbdo - log(1.0 + T4) / 50.0;     */                                 /* 6.0 */
        Vbdx = -log(1.0 + T4) / 50.0;                                            /* 6.0 */
        Wkfgd = (1.0 - pModel->Tpg) * Eg / 2.0;                                  /* 6.0 */
        Vfbgd = Wkfgd - ELECTRON_CHARGE * pModel->Type * T0 / Coxf;              /* 6.0 */
        Vgdt = Vfbgd - Eg + Vbdx;                                                /* 6.0 */
        Vgdx = Vgdt - log(1.0 + exp(-0.4 * (Vgfs - Vdss - Vgdt))) / 0.4;         /* 6.0 */
/*      Esd = (Vgdx - Vfbgd + Eg - Vbdx) * Coxf / OXIDE_PERMITTIVITY / 3.0;    */
        Esd = (Vgdx - Vfbgd + Eg - Vbdx) * Coxf / ESI;                           /* 7.0Y */
        Rmass = 1.82e-31;
        h = 6.626e-34;
        T2 = -0.375 * Weff * pModel->Dl * ELECTRON_CHARGE * 
             ELECTRON_CHARGE * sqrt(Rmass * Eg * ELECTRON_CHARGE / 
             2.0) / h / h * Esd * exp(pModel->Bgidl / Esd);                      /* 6.0 */
	Igidl = -T2 * 8.0 * Vbdx / (3.0 * Eg * 3.1416);                          /* 6.0 */
      }
      else
      { Igidl = 0.0;
      }      
 
	/* 6.0 : Add GISL */    
      if(pModel->Bgidl >0.0)                                                     /* 6.0 */
      { T4 = exp(-50.0 * (Vbs));                                                 /* 6.0 */
/*	Vbsx = Vbdo - log(1.0 + T4) / 50.0;     */                                 /* 6.0 */
        Vbsx = -log(1.0 + T4) / 50.0;                                            /* 6.0 */
        Vgst= Vfbgd - Eg + Vbsx;                                                 /* 6.0 */
	T1 = exp(-0.4 * (Vgfs - Vgst));                                          /* 6.0 */
        Vgsx = Vgst - log(1.0 + T1) / 0.4;                                       /* 6.0 */
/*      Ess = (Vgsx - Vfbgd + Eg - Vbsx) * Coxf / OXIDE_PERMITTIVITY / 3.0;       */
        Ess = (Vgsx - Vfbgd + Eg - Vbsx) * Coxf / ESI;                           /* 7.0Y */
	T2 = -0.375 * Weff * pModel->Dl * ELECTRON_CHARGE * 
             ELECTRON_CHARGE * sqrt(Rmass * Eg * ELECTRON_CHARGE /
             2.0) / h / h * Ess * exp(pModel->Bgidl / Ess);                      /* 6.0 */
	Igisl = - T2 * 8.0 * Vbsx / (3.0 * Eg * 3.1416);                         /* 6.0 */
      }                                                                          /* 6.0 */
      else                                                                       /* 6.0 */
      { Igisl = 0.0;                                                             /* 6.0 */
      }                                                                          /* 6.0 */
      Ir -= Igisl;                                                               /* 6.0 */

	/* Impact-ionization current */
   if ((pTempModel->Alpha <= 0.0) || (pTempModel->Beta <= 0.0)
        || (Vds == 0.0))							 /* 4.41 */
    {   Igi = 0.0;
    }
    else
    {   if (Region < 3)
        { for (I = 0; I < ICONT; I++)                                            /* 4.5 */
	  { if (I == 0)                                                          /* 4.5 */
	    { Mobred = Mobred1;                                                  /* 4.5 */
	      Lefac = Lefac1;                                                    /* 4.5 */
	    }                                                                    /* 4.5 */
	    else                                                                 /* 4.5 */
	    { Mobred = Mobred2;                                                  /* 4.5 */
	      Lefac = Lefac2;                                                    /* 4.5 */
	    }                                                                    /* 4.5 */
	    Dell_f = Leff * (1.0 - Lefac);
	    T1 = (1.0 - Lefac) / Lcfac;
	    T2 = exp(T1);
	    Sinh1 = 0.5 * (T2 - 1.0 / T2);
	    Cosh1 = 0.5 * (T2 + 1.0 / T2);
	    if (Lefac > 1.0e-3)
	    {   Xecf = 1.0 / (pTempModel->Fsat * Leff * Mobred);
	    }
	    else
	    {   Xecf = Vdss / (pModel->Lc * Sinh1);
	    }
	    Xemf = Xecf * Cosh1;

	    /* Call IMPION IFLAG=0 */

	    Cst = 0.4 * ELECTRON_CHARGE / Boltz * 6.5e-8;
	    Betap = pTempModel->Beta * Cst;
	    Te_ch = ELECTRON_CHARGE * Xecf / (5.0 * Boltz) * T2
	          / (1.0 / pModel->Lc + 1.0 / 6.5e-8);
	    if (Te_ch < 1.0e-20) Te_ch = 1.0e-20;
	    T1 = Betap / Te_ch;
	    if (T1 > 80.0) T1 = 80.0;                                            /* 4.5 */
	    Mult_ch = pTempModel->Alpha * pModel->Lc / Betap * Te_ch * exp(-T1);
	    Mult_dep = 0.0;
	    Mult_qnr = 0.0;
	    T0 = Itot * pTempModel->Rldd;
	    if ((Ldd <= 0.0) || (Xemf < T0) || (pModel->Nldd >= 1.0e25))         /* 4.5 */
	    {   Te_ldd = Te_ch;
	    }
	    else
	    {   T10 = Eslope * Ldd;
	        if ((Xemf - T0) <= T10)
		{   Ldep = (Xemf - T0) / Eslope;
		}
		else
		{   Ldep = Ldd;
		}
		Tempdell = Te_ch;
		Xstart = 0.0;
		Xend = Ldep;
		Te_dep = 0.0;                                                    /* 6.0 */
                Te_ldd = 0.0;                                                    /* 7.0 */
		Mult_dep = Xnonlocm(Xstart, Xend, pTempModel->Alpha, Betap,
		                    Ldep, Tempdell, Ldd, Cst, Xemf,
				    Eslope, pTempModel->Rldd, Itot, Te_ldd,
				    Te_dep);
		Te_dep = Xnonloct(Xend, Xend, Ldep, Tempdell, Ldd, Cst, Xemf,
				  Eslope, pTempModel->Rldd, Itot, Te_ldd,
				  Te_dep);
		if (Ldep != Ldd)
		{   Xstart = Ldep;
		    Xend = Ldd;
		    Mult_qnr = Xnonlocm(Xstart, Xend, pTempModel->Alpha, Betap,
		                    Ldep, Tempdell, Ldd, Cst, Xemf,
				    Eslope, pTempModel->Rldd, Itot, Te_ldd,
				    Te_dep);
		    Te_ldd = Xnonloct(Xend, Xend, Ldep, Tempdell, Ldd, Cst,
				      Xemf, Eslope, pTempModel->Rldd, Itot,
				      Te_ldd, Te_dep);
		}
                else                                                             /* 4.5 */
                {   Te_ldd = Te_dep;                                             /* 4.5 */
                }                                                                /* 4.5 */
	    }
	    T1 = Betap / Te_ldd;
	    if (T1 > 80.0) T1 = 80.0;                                            /* 4.5 */
	    Mult_drn = pTempModel->Alpha * 6.5e-8 / Betap * Te_ldd * exp(-T1);
	    Mult = Mult_ch + Mult_dep + Mult_qnr + Mult_drn;
	    if (Mult <= 1.0e-50) Mult = 1.0e-50;                                 /* 4.5 */
	    if (Region == 1)                                                     /* 4.5 */
	    { Igi = Mult * Itot;                                                 /* 4.5 */
	    }                                                                    /* 4.5 */
	    else                                                                 /* 4.5 */
	    { if (I == 0)                                                        /* 4.5 */
	      { Mult_st1 = Mult;                                                 /* 4.5 */
	      }                                                                  /* 4.5 */
	      else                                                               /* 4.5 */
	      { Mult_st2 = Mult;                                                 /* 4.5 */
	      }                                                                  /* 4.5 */
	    }                                                                    /* 4.5 */
	  } 
	}
	if (Region > 1)
	{   T0 = Cb / (Coxf + Coxb);
	    Lcf = pModel->Tb * sqrt(T0);
	    Dell_f = 2.0 * Lcf * log(1.0 / Lcfac);
	    if (Dell_f < 0.5 * Leff)
	    {   Xecf = 2.0 * Vdss * Lcfac * Lcfac / Lcf;
		T0 = Dell_f / Lcf;
		T1 = exp(T0);
	    }
	    else
	    {   Dell_f = 0.5 * Leff;
		T0 = Dell_f / Lcf;
		T1 = exp(T0);
		Cosh1 = 0.5 * (T1 + 1.0 / T1);
	        Xecf = Vdss / (Lcf * (Cosh1 - 1.0));
	    }
	    Sinh1 = 0.5 * (T1 - 1.0 / T1);                                       /* 4.5 */
	    Xemf = Xecf * Sinh1;

	    /* Call IMPION IFLAG=0 */
	    Cst = 0.4 * ELECTRON_CHARGE / Boltz * 6.5e-8;
	    Betap = pTempModel->Beta * Cst;
	    T2 = Dell_f / pModel->Lc;
	    Te_ch = ELECTRON_CHARGE * Xecf / (5.0 * Boltz) * exp(T2)
	          / (1.0 / pModel->Lc + 1.0 / 6.5e-8);
	    if (Te_ch < 1.0e-20) Te_ch = 1.0e-20;
	    T1 = Betap / Te_ch;
	    if (T1 > 80.0) T1 = 80.0;                                            /* 4.5 */
	    Mult_ch = pTempModel->Alpha * pModel->Lc / Betap * Te_ch * exp(-T1);
	    Mult_dep = 0.0;
	    Mult_qnr = 0.0;
	    T0 = Itot * pTempModel->Rldd;
	    if ((Ldd <= 0.0) || (Xemf < T0) || (pModel->Nldd >= 1.0e25))         /* 4.5 */
	    {   Te_ldd = Te_ch;
	    }
	    else
	    {   T10 = Eslope * Ldd;
	        if ((Xemf - T0) <= T10)
		{   Ldep = (Xemf - T0) / Eslope;
		}
		else
		{   Ldep = Ldd;
		}
		Tempdell = Te_ch;
		Xstart = 0.0;
		Xend = Ldep;
		Te_dep = 0.0;                                                    /* 6.0 */
                Te_ldd = 0.0;                                                    /* 7.0 */
		Mult_dep = Xnonlocm(Xstart, Xend, pTempModel->Alpha, Betap,
		                    Ldep, Tempdell, Ldd, Cst, Xemf,
				    Eslope, pTempModel->Rldd, Itot, Te_ldd,
				    Te_dep);
		Te_dep = Xnonloct(Xend, Xend, Ldep, Tempdell, Ldd, Cst,
				      Xemf, Eslope, pTempModel->Rldd, Itot,
				      Te_ldd, Te_dep);
		if (Ldep != Ldd)
		{   Xstart = Ldep;
		    Xend = Ldd;
		    Mult_qnr = Xnonlocm(Xstart, Xend, pTempModel->Alpha, Betap,
		                    Ldep, Tempdell, Ldd, Cst, Xemf,
				    Eslope, pTempModel->Rldd, Itot, Te_ldd,
				    Te_dep);
		    Te_ldd = Xnonloct(Xend, Xend, Ldep, Tempdell, Ldd, Cst,
				      Xemf, Eslope, pTempModel->Rldd, Itot,
				      Te_ldd, Te_dep);
		}
                else                                                             /* 4.5 */
                {   Te_ldd = Te_dep;                                             /* 4.5 */
                }                                                                /* 4.5 */
	    }
	    T1 = Betap / Te_ldd;
	    if (T1 > 80.0) T1 = 80.0;                                            /* 4.5 */
	    Mult_drn = pTempModel->Alpha * 6.5e-8 / Betap * Te_ldd * exp(-T1);
	    Mult = Mult_ch + Mult_dep + Mult_qnr + Mult_drn;
	    if (Mult <= 1.0e-50) Mult = 1.0e-50;                                 /* 4.5 */
	    if(Region == 3)                                                      /* 4.5 */
	    { Igi = Mult * Itot;                                                 /* 4.5 */
	    }                                                                    /* 4.5 */
	    else                                                                 /* 4.5 */
	    { Mult_wk = Mult;                                                    /* 4.5 */
	    }                                                                    /* 4.5 */
	}                                                                        /* 4.5 */
	if (Region == 2)                                                         /* 4.5 */
	{   T1 = log(Mult_st1);                                                  /* 4.5 */
	    T2 = log(Mult_st2);                                                  /* 4.5 */
	    T11 = log(Mult_wk);                                                  /* 4.5 */
	    Gst = (T2 - T1) / DVgfs;                                             /* 4.5F */
	    T0 = Vgfs - Vthw;                                                    /* 4.5 */
	    R2 = (3.0 * (T1 - T11) / DeltaVth - Gst) / DeltaVth;                 /* 4.5 */
	    R3 = (2.0 * (T11 - T1) / DeltaVth + Gst) / (DeltaVth * DeltaVth);    /* 4.5 */
	    Mult = exp(T11 + T0 * T0 * (R2 + T0 * R3));                          /* 4.5 */
	    Igi = Mult * Itot;                                                   /* 4.5 */
	}                                                                        /* 4.5 */
    }
    /* 4.41 : Both GIDL and impact-ionization currents accounted for in Igi */
	Igi = Igi + Igidl;

    if (pModel->Selft > 0)
    {   Power = Itot * Vds;
	T0 = Itot * Itot;
	if (pInst->DrainConductance > 0.0)
	{   Power += T0 / (pInst->DrainConductance
	          * pTempModel->GdsTempFac2);                                    /* 4.5 */
	}
	if (pInst->SourceConductance > 0.0)
	{   Power += T0 / (pInst->SourceConductance
	          * pTempModel->GdsTempFac2);                                    /* 4.5 */
	}
    }
    else
    {   Power = 0.0;
    }

    pOpInfo->Ich = Ich * pInst->MFinger;
    pOpInfo->Ibjt = Ibjt * pInst->MFinger;
    pOpInfo->Igt = Igt * pInst->MFinger + pEnv->Gmin * (Vbs - Vds);
    pOpInfo->Ir = Ir * pInst->MFinger + pEnv->Gmin * Vbs;
    pOpInfo->Igi = Igi * pInst->MFinger;
    pOpInfo->Power = Power * pInst->MFinger;
    pOpInfo->DrainConductance = pInst->DrainConductance
                              * pTempModel->GdsTempFac;
    pOpInfo->SourceConductance = pInst->SourceConductance
                               * pTempModel->GdsTempFac;
    pOpInfo->BodyConductance = pInst->BodyConductance
                             * pTempModel->GbTempFac;
    pOpInfo->Vts = Vths;
    pOpInfo->Vtw = Vthw;

    if (pInst->pDebug)
    {   pDebug = pInst->pDebug;
    }
    if (DynamicNeeded)
    {   CoxWL = Coxf * Weff * Leff;
        Qby = pInst->Qb * Weff * Leff;                                           /* 4.5r */
        if (Region > 1)
        {   if (Region == 2) Vgstart = Vthw;
	    for (I = 0; I < ICONT; I++)
	    {    if (I == 0)
		 {   Xk = Xk11;
		 }
		 else
		 {   Xk = Xk22;
		 }
		 Vgfs = Vgstart - I * DVgfs;                                     /* 4.5F */
		 T0 = Xk + Vbi;
		 Xingvsf = (Coshf - 1.0) * (2.0 * T0 + Vdss) / Sinhf;
		 Psiwk = Xingvsf / (pModel->Aa * Leff) - Xk;
	         Qgf = CoxWL * (Vgfs - pTempModel->Wkf - Psiwk);
	         if (I == 0)
		 {   Qgfwk1 = Qgf;
		 }
		 else
		 {   Qgfwk2 = Qgf;
		 }
	    }
	}
	if (Region < 3)
        {   if (Region == 2)
	        Vgstart = Vths;
	    for (I = 0; I < ICONT; I++)
	    {    if (I == 0)
	         {   Mobref = Mobref1;
		     Vdsx = Vdsx1;
		     Ist = Ist1;
		     Mobred = Mobred1;
		     Xbmu = Xbmu1;
		     Lefac = Lefac1;
		     Psigf = Psigf1;					         /* 4.5pd */
		     Alphap = Alphap1;					         /* 4.5pd */
                     Vthsx = Vthsx1;                                             /* 4.5qm */
                     Pst = Pst1;                                                 /* 4.5qm */
                     Qst = Qst1;                                                 /* 4.5qm */
		 }
		 else
	         {   Mobref = Mobref2;
		     Vdsx = Vdsx2;
		     Ist = Ist2;
		     Mobred = Mobred2;
		     Xbmu = Xbmu2;
		     Lefac = Lefac2;
		     Psigf = Psigf2;					         /* 4.5pd */
		     Alphap = Alphap2;					         /* 4.5pd */
                     Vthsx = Vthsx2;                                             /* 4.5qm */
                     Pst = Pst2;                                                 /* 4.5qm */
                     Qst = Qst2;                                                 /* 4.5qm */
		 }
		 Vgfs = Vgstart + I * DVgfs;                                     /* 4.5F */
		 Vgfvt = Vgfs - Psigf - Vthsx - Qst / Coxf;	                 /* 4.5qm */
		 Dnmgf = 12.0 * (Vgfvt + Delvtf - 0.5 * Alphap * Vdsx);          /* 6.0 */
		 Args = 1.0 + pTempModel->Fsat * Mobref * Vdsx / Lefac;
		 Qgf = CoxWL * Lefac * (Vgfs - Psigf - pTempModel->Wkf - Pst     /* 4.5pd */
                   - 0.5 * Vdsx + Args * Vdsx * Vdsx * Alphap / Dnmgf);          /* 4.5pd */
		 if (Lefac < 1.0)
		 {   T1 = (1.0 - Lefac) / Lcfac;
		     T2 = exp(T1);
		     Cosh1 = 0.5 * (T2 + 1.0 / T2);
		     Xp = pModel->Lc * Lcfac * (Cosh1 - 1.0)
			/ (pTempModel->Fsat * Mobref);
		     Qgf += CoxWL * ((1.0 - Lefac) * (Vgfs - Psigf -             /* 4.5pd */
                            pTempModel->Wkf - Pst - Vdsx) - Xp / Leff);          /* 4.5pd */
		 }
		 T0 = pModel->Tb / Leff;
		 Qdeta = Weff * Leff * Cb * T0 * T0 * Vdss * (2.0 - Cap1);	 /* 6.0 */
/*		 Qdqsh = -0.5 * CoxWL * (pInst->Qb - Qbeff) / Coxf;                 4.5r */
		 Qdqsh = 0.0;                                                    /* 4.5 */
		 if (Vdsx > 1.0e-9)
		 {   Argu = Vgfvt / (Alphap * Vdsx);   		    		 /* 4.5pd */
		     if (pTempModel->Fsat > 0.0)
		     {   Argz = Argu - Ist * pTempModel->Fsat / (2.0
		              * pTempModel->Beta0 * Alphap * Vdsx); 	         /* 4.5pd */
		     }
		     else
		     {   Argz = Argu;
		     }
		     Tzm1 = 2.0 * Argz - 1.0;
		     Xfactd = (-Argz + 2.0 / 3.0) / Tzm1;
		     Zfact = (1.5 * Argz - 4.0 * Argz * Argz / 3.0
		           - 0.4) / (Tzm1 * Tzm1);
		     Qcfle = -Coxf * (Vgfvt - Alphap * Vdsx); 		         /* 4.5pd */
		     Qns = Qcfle * Weff * Leff * (1.0 - Lefac);
		     Qds = 0.5 * Weff * Qcfle * Leff * (1.0 - Lefac * Lefac);
		     Qn = -CoxWL*Lefac*Vgfvt * (Xfactd + Argu) / Argu + Qns;     /* 4.5 */
		     Qd = -CoxWL*Lefac*Vgfvt * (Zfact + 0.5 * Argu) / Argu + Qds;/* 4.5 */
		     Qs = Qn - Qd;
		     Qd += Qdqsh;
		     Qs += Qdqsh;
		 }
		 else
		 {   Qn = -CoxWL * Vgfvt;
		     Qd = 0.5 * Qn + Qdqsh;		                         /* 4.5 */
		     Qs = Qd;
		 }
		 Qd += Qdeta;	                        	                 /* 6.0 */
	         if (I == 0)
		 {   Qgfst1 = Qgf;
		     Qdst1 = Qd;
		     Qsst1 = Qs;
		     Qnst1 = Qn;
		 }
		 else
		 {   Qgfst2 = Qgf;
		     Qdst2 = Qd;
		     Qsst2 = Qs;
		     Qnst2 = Qn;
		 }
	    }
	    if (Region == 2)
	    {   /* Qgf */
		Vgfs = Vgfso;
	        Cst = (Qgfst2 - Qgfst1) / DVgfs;		         	 /* 4.5F */
	        Cwk = (Qgfwk1 - Qgfwk2) / DVgfs;		        	 /* 4.5F */
		Xchk = Cst - Cwk;						 /* 4.5 */
		if (fabs(Xchk) <= 1.0e-50)					 /* 4.5 */
		{ Qgf = (Vths - Vgfs) / (Vths - Vthw) * Qgfwk1			 /* 4.5 */
		      + (Vgfs - Vthw) / (Vths - Vthw) * Qgfst1;			 /* 4.5 */
		}								 /* 4.5 */
		else								 /* 4.5 */
		{ Vp = (Vths * Cst - Vthw * Cwk - Qgfst1 + Qgfwk1) / Xchk;	 /* 4.5 */
		  Qp = Qgfwk1 + (Vp - Vthw) * Cwk;
		  X3a = Vthw - 2.0 * Vp + Vths;
		  X3b = Vthw - Vp;
		  X3c = Vthw - Vgfs;
		  Xarg = X3b * X3b - X3a * X3c;
		  if (Xarg > 0.0)
		  {   Xarg = sqrt(Xarg);
		  }
		  else
		  {   Xarg = 0.0;
		  }
		  t = (X3b + Xarg) / X3a;
		  T1 = 1.0 - t;
		  Qgf = T1 * T1 * Qgfwk1 + 2.0 * t * T1 * Qp + t * t * Qgfst1;
		}								 /* 4.5 */

	        Cst = (Qnst2 - Qnst1) / DVgfs;                                   /* 4.5F */
	        Cwk = (Qnwk1 - Qnwk2) / DVgfs;                                   /* 4.5F */
		Xchk = Cst - Cwk;						 /* 4.5 */
		if (fabs(Xchk) <= 1.0e-50)					 /* 4.5 */
		{ Qn = (Vths - Vgfs) / (Vths - Vthw) * Qnwk1			 /* 4.5 */
		     + (Vgfs - Vthw) / (Vths - Vthw) * Qnst1;			 /* 4.5 */
		}								 /* 4.5 */
		else								 /* 4.5 */
		{ Vp = (Vths * Cst - Vthw * Cwk - Qnst1 + Qnwk1) / Xchk;	 /* 4.5 */
       		  Qp = Qnwk1 + (Vp - Vthw) * Cwk;                                /* 4.5 */
		  X3a = Vthw - 2.0 * Vp + Vths;
		  X3b = Vthw - Vp;
		  X3c = Vthw - Vgfs;
		  Xarg = X3b * X3b - X3a * X3c;
		  if (Xarg > 0.0)
		  {   Xarg = sqrt(Xarg);
		  }
		  else
		  {   Xarg = 0.0;
		  }
		  t = (X3b + Xarg) / X3a;
		  T1 = 1.0 - t;                                                  /* 4.5 */
		  Qn = T1 * T1 * Qnwk1 + 2.0 * t * T1 * Qp + t * t * Qnst1;      /* 4.5 */
		}								 /* 4.5 */

	        /* Cwk = Qdwk = 0.0; */                                          /* 4.5 */
	        Cst = (Qdst2 - Qdst1) / DVgfs;                                   /* 4.5F */
		Xchk = Cst;						         /* 4.5 */
		if (fabs(Xchk) <= 1.0e-50)					 /* 4.5 */
		{ Qd = (Vgfs - Vthw) / (Vths - Vthw) * Qdst1;			 /* 4.5 */
		}								 /* 4.5 */
		else								 /* 4.5 */
		{ Vp = (Vths * Cst - Qdst1) / Xchk;	                         /* 4.5 */
		  X3a = Vthw - 2.0 * Vp + Vths;
		  X3b = Vthw - Vp;
		  X3c = Vthw - Vgfs;
		  Xarg = X3b * X3b - X3a * X3c;
		  if (Xarg > 0.0)
		  {   Xarg = sqrt(Xarg);
		  }
		  else
		  {   Xarg = 0.0;
		  }
		  t = (X3b + Xarg) / X3a;
		  Qd = t * t * Qdst1;                                            /* 4.5 */
		}								 /* 4.5 */

	        /* Cwk = Qswk = 0.0; */                                          /* 4.5 */
	        Cst = (Qsst2 - Qsst1) / DVgfs;                                   /* 4.5F */
		Xchk = Cst;						         /* 4.5 */
		if (fabs(Xchk) <= 1.0e-50)					 /* 4.5 */
		{ Qs = (Vgfs - Vthw) / (Vths - Vthw) * Qsst1;			 /* 4.5 */
		}								 /* 4.5 */
		else								 /* 4.5 */
		{ Vp = (Vths * Cst - Qsst1) / Xchk;	                         /* 4.5 */
		  X3a = Vthw - 2.0 * Vp + Vths;
		  X3b = Vthw - Vp;
		  X3c = Vthw - Vgfs;
		  Xarg = X3b * X3b - X3a * X3c;
		  if (Xarg > 0.0)
		  {   Xarg = sqrt(Xarg);
		  }
		  else
		  {   Xarg = 0.0;
		  }
		  t = (X3b + Xarg) / X3a;
		  Qs = t * t * Qsst1;                                            /* 4.5 */
		}								 /* 4.5 */
	    }
	}
	T0 = 15.0 / CoxWL;                                                       /* 4.5F */
        Qacc = CoxWL * (Vgfs - pTempModel->Wkf - Vbs);                           /* 4.5F */
        Qinv = Qgf;                                                              /* 4.5F */
        T1 = T0 * (Qinv - Qacc);                                                 /* 4.5F */
        if (T1 >= 80.0)                                                          /* 4.5F */
        { Qgf = Qacc;                                                            /* 4.5F */
        }                                                                        /* 4.5F */
        else if (T1 <= -80.0)                                                    /* 4.5F */
        { Qgf = Qinv;                                                            /* 4.5F */
        }                                                                        /* 4.5F */
        else                                                                     /* 4.5F */
        { Qgf = Qinv - log(1.0 + exp(T1)) / T0;                                  /* 4.5F */
        }                                                                        /* 4.5F */
        Xco0 = (Coshf - 1.0) / (Sinhf * pModel->Aa * Leff);                      /* 4.5F */
        Xco1 = 1.0 - 2.0 * Xco0;                                                 /* 4.5F */
        Xco2 = (2.0 * Vbi + Vdss) * Xco0;                                        /* 4.5F */
        /* Call Fring(pInst, pModel, Vgbs, &Vgbwk, &Xkb);                           4.5F */
          Vgbwk = Vgbs;                                                          /* 4.5F */
          Xkb = (pModel->Ccb * (Vfbf - Vgfs) - pModel->Bb                        /* 4.5F */
              * (Vgbwk - Vfbb) + Xdumv) / pModel->Ab;                            /* 4.5F */
          if(pModel->Gamma != 0.0)                                               /* 4.5F */
          { T2 = pModel->Toxb / Leff;                                            /* 4.5F */
            Dxm = pModel->Kappa * T2 * T2 * Vdss;                                /* 4.5F */
            for(J = 0; J < 2; J++)                                               /* 4.5F */
            { Ebs = -pModel->Aab * ((Xkb + Vbi) * (1.0 - Coshb) + Vdss) / Sinhb; /* 4.5F */
	      Arg = (Ebs - ExfLim) / ExfLim;                                     /* 4.5F */
	      if(Arg < -80.0)                                                    /* 4.5F */
	      { Ebs = ExfLim;                                                    /* 4.5F */
	      }                                                                  /* 4.5F */
              else if(Arg < 80.0)                                                /* 4.5F */
              { Ebs = ExfLim + ExfLim * log(1.0 + exp(Arg));                     /* 4.5F */
	      }                                                                  /* 4.5F */
	      Xm = T2 * pModel->Toxb * Ebs * pModel->Gamma;                      /* 4.5F */
	      Vgbwk = Vgbs + Xm + Dxm;                                           /* 4.5F */
	      Xkb = (pModel->Ccb * (Vfbf - Vgfs)                                 /* 4.5F */
	          - pModel->Bb * (Vgbwk - Vfbb) + Xdumv) / pModel->Ab;           /* 4.5F */
	      }                                                                  /* 4.5F */
	  }                                                                      /* 4.5F */
	Vgfsacc = (-pModel->C * (Vgbwk - Vfbb) + Xdumv + pModel->A               /* 4.5F */
                * (Vbs - Xco2) / Xco1) / pModel->B + Vfbf;                       /* 4.5F */
	Qacc = Qby - CoxWL * (Vgfs - Vgfsacc);                                   /* 4.5F */
        Qinv = Qby;                                                              /* 4.5F */
        T1 = T0 * (Qinv - Qacc);                                                 /* 4.5F */
        if (T1 >= 80.0)                                                          /* 4.5F */
/*        { Qby = Qacc;                                                         */   /* 4.5F */
        { Qby = Qinv;                                                            /* 6.0 */
        }                                                                        /* 4.5F */
        else if (T1 <= -80.0)                                                    /* 4.5F */
/*        { Qby = Qinv;                                                         */   /* 4.5F */
        { Qby = Qacc;                                                            /* 6.0 */
        }                                                                        /* 4.5F */
        else                                                                     /* 4.5F */
/*        { Qby = Qinv - log(1.0 + exp(T1)) / T0;                               */   /* 4.5F */
        { Qby = Qacc + log(1.0 + exp(T1)) / T0;                                  /* 6.0 */
        }                                                                        /* 4.5F */

	T0 = pModel->Type * ELECTRON_CHARGE * Weff * Leff;
        Qnqff = T0 * (pModel->Nqff + 2.0 * pModel->Nqfsw * pModel->Tb / Weff);   /* 4.5r */
        Qnqfb = T0 * pModel->Nqfb;	
        Qgb = -(Qs + Qd + Qgf + Qby + Qnqff + Qnqfb);                            /* 4.5 */

	/* Substrate charge */
	T11 = pTempModel->Efs - pTempModel->Efd;
	if (pModel->Tps <= 0)
	{   T0 = Vds - Vgbs - T11;
            T1 = T0 - ELECTRON_CHARGE * pModel->Type * pModel->Nqfb / Coxb;      /* 4.5 */
	    if (T1 > 0.0)
	    {   T2 = 0.5 * (sqrt(pModel->Dum1 * pModel->Dum1
	           + 4.0 * T1) - pModel->Dum1);
	        T2 = T2 * T2;
	        Qddep = Coxb * (T0 - T2) * (Ad + Weff * Ldd);
	    }
	    else
	    {   Qddep = Coxb * T0 * (Ad + Weff * Ldd);
	    }

	    T0 = -Vgbs - T11;
            T1 = T0 - ELECTRON_CHARGE * pModel->Type * pModel->Nqfb / Coxb;      /* 4.5 */
	    if (T1 > 0.0)
	    {   T2 = 0.5 * (sqrt(pModel->Dum1 * pModel->Dum1
	           + 4.0 * T1) - pModel->Dum1);
	        T2 = T2 * T2;
	        Qsdep = Coxb * (T0 - T2) * (As + Weff * Ldd);                    /* 4.5 */
	    }
	    else
	    {   Qsdep = Coxb * T0 * (As + Weff * Ldd);                           /* 4.5 */
	    }
	    Qd += Qddep;
	    Qs += Qsdep;                                                         /* 4.5 */
	    Qgb -= (Qddep + Qsdep);
	}
	else
	{   T0 = Vds - Vgbs - T11;
            T1 = T0 - ELECTRON_CHARGE * pModel->Type * pModel->Nqfb / Coxb;      /* 4.5 */
	    if (T1 < 0.0)
	    {   T2 = 0.5 * (sqrt(pModel->Dum1 * pModel->Dum1
	           - 4.0 * T1) - pModel->Dum1);
	        T2 = - T2 * T2;                                                  /* 6.0 */
	        Qddep = Coxb * (T0 - T2) * (Ad + Weff * Ldd);
	    }
	    else
	    {   Qddep = Coxb * T0 * (Ad + Weff * Ldd);
	    }

	    T0 = -Vgbs - T11;
            T1 = T0 - ELECTRON_CHARGE * pModel->Type * pModel->Nqfb / Coxb;      /* 4.5 */
	    if (T1 < 0.0)
	    {   T2 = 0.5 * (sqrt(pModel->Dum1 * pModel->Dum1
	           - 4.0 * T1) - pModel->Dum1);
	        T2 = - T2 * T2;                                                  /* 6.0 */
	        Qsdep = Coxb * (T0 - T2) * (As + Weff * Ldd);                    /* 4.5 */
	    }
	    else
	    {   Qsdep = Coxb * T0 * (As + Weff * Ldd);                           /* 4.5 */
	    }
	    Qd += Qddep;
	    Qs += Qsdep;                                                         /* 4.5 */
	    Qgb -= (Qddep + Qsdep);
	}

	/* Diffusion capacitance */
	Qterms = ELECTRON_CHARGE * Xnin2 * Psj * (Argbjts - 1.0);                /* 4.5 */
	Qtermd = ELECTRON_CHARGE * Xnin2 * Pdj * (Argbjtd - 1.0);                /* 4.5 */
	T0 = pModel->Ldiff * pModel->Tb / pTempModel->Ndseff;
	Qns = Qterms * T0;
	Qnd = Qtermd * T0;
	if (pModel->Bjt == 0)                                                    /* 4.5 */
	{   Qpd = Qps = 0.0;
	}
	else
	{   T1 = 0.5 * ELECTRON_CHARGE * Xnin * Xlbjt;                           /* 4.5 */
            T0 = pModel->Nbody / Xnin * Sqrt_s / Exp_af;
	    Tf1 = Argbjts + T0;
	    Tf2 = Argbjts + T0 * Exp_bf;
	    Qps = T1 * Psj * Sqrt_s * log(Tf1 * Exp_bf / Tf2) / Xbf;             /* 4.5 */

	    T0 = pModel->Nbody / Xnin * Sqrt_d / Exp_af;
	    Tr1 = Argbjtd + T0;
	    Tr2 = Argbjtd + T0 * Exp_bf;
	    Qpd = T1 * Pdj * Sqrt_d * log(Tr1 * Exp_bf / Tr2) / Xbf;             /* 4.5 */
	}
	Qd -= (Qpd + Qnd);
	Qs -= (Qps + Qns);                                                       /* 4.5 */
	Qby += (Qpd + Qps + Qnd + Qns);

	pOpInfo->Qgf = Qgf * pInst->MFinger;
	pOpInfo->Qd = Qd * pInst->MFinger;
	pOpInfo->Qs = Qs * pInst->MFinger;                                       /* 4.5 */
	pOpInfo->Qb = Qby * pInst->MFinger;
	pOpInfo->Qgb = Qgb * pInst->MFinger;
	pOpInfo->Qn = Qn * pInst->MFinger;
    }
    return 0;                                                                    /* 6.0 */
}

int 
nfdEvalMod( Vds, Vgfs, Vbs, Vgbs, pOpInfo, pInst, pModel, pEnv, DynamicNeeded,
            ACNeeded)                                                            /* 7.0Y */
double Vds, Vgfs, Vbs, Vgbs;                                                     /* 4.5F */
struct ufsAPI_InstData  *pInst;
struct ufsAPI_ModelData *pModel;
struct ufsAPI_OPData *pOpInfo;
struct ufsAPI_EnvData *pEnv;
int DynamicNeeded, ACNeeded;                                                     /* 5.0 */
{
register struct ufsTDModelData *pTempModel;
struct ufsDebugData *pDebug;
double Xnin, Xnin2, Arg, Sqrt_s, Sqrt_d, Dell, Dshare, Qshare, Delqc;
double Vtm, Qc, Qc0, Qcs, Vths, Vthw, Vfbf, Vfbb, Qbeff, Leff, Weff;
double Vbi, Ldd, Lds, Vdss, Qc1, Qc2, Psis, Psiw, Coeffa, Mobfac1;
double T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14;
double Vdsat, Vbs_bjt, Argbjts, Vbd_bjt, Argbjtd, Qp_b;
double Exf00, Exf0, Exf, Sinh1, Cosh1, Dsh, Ddh, Qinv, Qacc, Qcle;               /* 4.5 */
double Gbwk, Mobrefwk, Vgstart, Vgfso, PsiwOld, PsisOld;                         /* 4.5F */
double Coxf, Coxb, Cb, Tshare, Vlddd, Dumchg, Veff, Xleff_b;
double Xem, WL, WLe, CoxWLe, Le, Lefac1, Lefac2, Qsst1, Qsst2;
double Ich, Xbfact, Mobred, Mobref, Xbmu, Lcfac, Lefac, Ist;
double Ist1, Ist2, Mobref1, Mobref2, Mobred1, Mobred2, Xbmu1, Xbmu2;
double DeltaVth, Gst, Gwk, R2, R3, Vref, Xs, Psf_bjt;
double Vbseffh, Vbseffl, Vbdeffh, Vbdeffl;                                       /* 4.5 */
double Ds, Dd, Xleff_t, Feff, Xaf, Exp_af, Qddb, Qdsb;
double Ph_f, P0f, Tf1, Tf2, Xintegf, Xintegr, Ph_r, Tr1, Tr2;
double Ibjtf, Ibjtr, Ibjt, Itot, Eslope, Igts0, Igtd0, Irs0, Ird0, Dell_f;
double Irs1, Ird1, Rgtd, Rgts, Igt_g, Igt_r, Igt, Ir_g, Ir_r, Ir, Igi, Igb;      /* 7.0Y */
double Xecf, Xemf, Betap, Te_ch, Mult_ch, Mult_dep, Mult_qnr, Mult_drn;
double Mult, Mult_st1, Mult_wk, Te_ldd, Ldep, Tempdell, Xstart, Xend, Te_dep;    /* 4.5 */
double Lcf, Power, CoxWL, Qd, Qn, Qgf, Qby, Xingvsf, Psiwk;
double Xco0, Xco1, Xco2, Vgfsacc, Vgfseff, Qgfwk;
double Qnqsh, Qdqsh, Covf, Vdsx, Arg2, Dnmgf, Args;
double Qdeta, Xp, Argu, Argz, Xfactd, Tzm1, Qcfle, Qns, Zfact, Qgfst1;
double Qgfst2, Qdst1, Qdst2, Qnst1, Qnst2, Cst, Cwk, Vp, Qp;
double X3a, X3b, X3c, Xarg, t, Qnqff, Qnqfb, Qddep, Qsdep, Qterm, Qterms;
double Qtermd, Qnd, Qps, Qpd, Iwkpr, Qds, Qgb, Ad, As;
double Xlbjt_t, Xlbjt_b, Qs, Ibjtf_b, Ibjtr_b, Ibjt_b;
double Vldd[40], Vd[40];
double Vdsx1, Vdsx2, Vdseff, Xchk;						 /* 4.41 */
double Egbgn, Wkfgd, Vfbgd, Vgdx, Esd, Vgdt;                                     /* 4.41 */
double Igidl, Rmass, h, Eg;                                                      /* 4.41 */
double Pdj, Psj, Qnwk, Ueff, Qc01, Qc02, Igtb0, Eo;                              /* 4.5 */
double T11pd, Alphap, Psigf1, Alphap1, Psigf2, Alphap2, Psigf;                   /* 4.5pd */
double toc, Btrev, E_dbm, E_dbo, E_sbm, E_sbo, Ituns, Itund;                     /* 4.5 */
double Mult_st2, Qnwk1, Qnwk2, Psiwk1, Psiwk2, Mobrefwk1, Qgfwk1, Qgfwk2;        /* 4.5F */
double VthsOld, Ess, DelEg, PhibQM, XniFac, XniFac2, Coeffz, Psis_str, Psis_bjt; /* 4.5F */
double DelEgQM, DelEgEx, QinvEx, NinvEx, NvlyEx;                                 /* 7.0Y */
double Vths_bjt, Esw, Psis1, Psis2, QmT00, QmT0, QmT1, QmT2, QmT3, QmT4, Exf02;  /* 4.5F */
double Coeffz0, ExfLim, QmT5, DVgfs, Iwk1, Iwk2;                                 /* 4.5F */
double Exf_Vthw, Coeffb, Coeffb2, Coeffb3, QmT2_Vths, Exf02_Vths;                /* 7.0Y */
double Dvtol, dVbd_bjtds, dPsigfgfs, Dicefac, CbbyCof;                           /* 5.0 */
double dVbdeffhds, dVdsxds, dLefacds, Gm1, Go1, Giigfs, Giids;                   /* 5.0 */
double dPsigfgfs1, dVdsxds1, dLefacds1, Go2, Gichds, Gow1, Gmw1, Gow2;           /* 5.0 */
double dGst, dGwk, Gom, Gichgfs, Gibjtds, Gidds, Gigtds;                         /* 5.0 */
double Gitundds, Gigidlgfs, Gigidlds, Gigigfs, Gigids;                           /* 5.0 */
double Gigbds, Gigbgfs, Vdsx_gb, Vgfsx_gb, Igb_ds, Igb_gfs;                      /* 7.0Y */
double dMult_chds, dMult_drnds, dMultds, dMult_wk, dMult_st1, Gidgfs;            /* 5.0 */
double CobWL, Cgfgfswk1, Cgfdswk1, Cgfgfs, Cgfds, Cdds, Csds, Cddsst;            /* 5.0 */
double Cdgfs, Csgfs, Cdgbs, Cbgfs, Cgbds, Csgbs, Cgbgbs, Cbds, Cbgbs;            /* 5.0 */
double T, Exo, Ueffo, Sich, Lc, Sis1, Sis2, Siw1, Siw2, Ich0;                    /* 5.0 */
double Ibjt_t, dSqrt_dds, dArgbjtdds, dVbdefflds;                                /* 5.0 */
double dDdhds, dDdds, dDdhdsi, dDddsi, dQpdds, Qpd_t, Qpd_b,dQpd_bds;            /* 5.0 */
int I, J, K, IB, Region, ICONT, Irfldd;                                          /* 4.5F */
double Vo, Vsat, Vth, Vds_dif, Vds_eff, Uo, Toxf, Theta;                         /* 5.0vo */ 
int Num_div;                                                                     /* 7.0Y */
double TempRatio, SqrtTempRatio, Nc, Nv, Psiw0;                                  /* 7.0Y */
double dDist_y, Jv_sum, Dist_y, Pot_y, Psi_y, VarP1, VarP2, VarP3, Psigf_y;      /* 7.0Y */
double Psi_ox, Barr_Vbe, Ediff, Ediff_q, Ediff_y, Arg_vbe, Efc;                  /* 7.0Y */
double Meff, Meffv, Meffc, Atun, Btun, Fox, Ktun1, Ktun2, Jtun1, Jtun2, Jtun;    /* 7.0Y */
double Igb1, Igb2, Dell_t, Psi_oxwk, Barr, BarrLim;                              /* 7.0Y */
double Kcbe, Kcbe1, Arg_cbe, Psibb, Psi_oxwkb, Sfb, Eox, Eoxb, J1, J1Sum;        /* 7.0Y */
double E_div, Excs, Excg, Exfg, Exfs, Fds, Fdg, Fd, Pt, Pt1, Pt2, Jcbe;          /* 7.0Y */
double Xalpha, Ldf, Ld, Xec, Xex, Tau_w, Phib, Qb, Qcbycox;
double Vdsdemo, Xsinh, Xcosh, Vdemo, Xueff, Vsat_eff, Fsat, Vbdx, DVbdds; /* Vbdo, */ /* 6.0 */
double Vbsx, Vgst, Vgsx, Igisl;                                                  /* 6.0 */
double Dum2, DelEgGe, Esige, Vbih, D0h, D0, XninGe, XninGe2, JroGe;              /* 7.5W */

/* Begin `nfdEvalMod'. */

    pTempModel = pInst->pTempModel;
    Vtm = pTempModel->Vtm;
    Eg = pTempModel->Eg;					                 /* 4.41 */
    TempRatio = Vtm / KoverQ / 300.15;                                           /* 7.0Y */
    SqrtTempRatio = sqrt(TempRatio);                                             /* 7.0Y */
    Nc = EFFECTIVE_DOS_CB*pow(SqrtTempRatio, 3);                                 /* 7.0Y */ 
    Nv = EFFECTIVE_DOS_VB*pow(SqrtTempRatio, 3);                                 /* 7.0Y */ 
    Vbi = pTempModel->Vbi;
    Xnin = pTempModel->Xnin;
    Xnin2 = Xnin * Xnin;
    Leff = pInst->Leff;
    Weff = pInst->Weff;
    Coxf = pModel->Coxf;
    Coxb = pModel->Coxb;
    Cb = pModel->Cb;
    Vfbf = pTempModel->Vfbf;
    Vfbb = pTempModel->Vfbb;
    Ldd = pModel->Lldd;
    Dicefac = ESI * pModel->Tb / (Leff * Leff);                                  /* 4.5d */
    CbbyCof = Cb / Coxf;                                                         /* 4.5d */
    if (pModel->Gex > 0)                                                         /* 7.5W */
    {   DelEgGe = 0.4 * pModel->Gex;                                             /* 7.5W */
        Esige = (4.6 * pModel->Gex + 11.7) * VACUUM_PERMITTIVITY;                /* 7.5W */
        Vbi = Vbi - DelEgGe;                                                     /* 7.5W */
        Vbih = pTempModel->Vbih - DelEgGe;                                       /* 7.5W */
        D0 = pTempModel->D0 * sqrt(Esige / ESI) * sqrt(Vbi / pTempModel->Vbi);   /* 7.5W */ 
        D0h = pTempModel->D0h * sqrt(Esige / ESI)                                /* 7.5W */
            * sqrt(Vbih / pTempModel->Vbih);                                     /* 7.5W */
        XninGe = Xnin * exp(DelEgGe / 2.0 / Vtm);                                /* 7.5W */
        XninGe2 = XninGe * XninGe;                                               /* 7.5W */ 
    }
    else                                                                         /* 7.5W */
    {   DelEgGe = 0.0;                                                           /* 7.5W */
        Esige = 11.7 * VACUUM_PERMITTIVITY;                                      /* 7.5W */
        Vbi = pTempModel->Vbi;                                                   /* 7.5W */ 
        Vbih = pTempModel->Vbih;                                                 /* 7.5W */
        D0 = pTempModel->D0;                                                     /* 7.5W */
        D0h = pTempModel->D0h;                                                   /* 7.5W */
        XninGe = Xnin;                                                           /* 7.5W */
        XninGe2 = Xnin2;                                                         /* 7.5W */
    }
    if (pInst->Mode > 0)
    {	As = pInst->SourceArea;
	Ad = pInst->DrainArea;
        Pdj = pInst->DrainJunctionPerimeter;   		                         /* 4.5 */
        Psj = pInst->SourceJunctionPerimeter;		                         /* 4.5 */
    }
    else
    {	Ad = pInst->SourceArea;
	As = pInst->DrainArea;
        Pdj = pInst->SourceJunctionPerimeter;		                         /* 4.5 */
        Psj = pInst->DrainJunctionPerimeter;		                         /* 4.5 */
    }
    Lcfac = pModel->Lc / Leff;

    Arg = Vbs / Vtm;
    if (Arg > 80.0)
    {   Vbs_bjt = Vbs;
    }
    else if (Arg < -80.0)
    {   Vbs_bjt = 0.0;
    }
    else
    {   Vbs_bjt = Vtm * log(1.0 + exp(Arg));                                     /* 5.0 */
    }
    Sqrt_s = exp(0.5 * Vbs_bjt / Vtm);
    Argbjts = Sqrt_s * Sqrt_s;

    T0 = (Vbih - Vbs) / Vtm;                                                     /* 7.5W */
    if (T0 > 80.0)
    {   Vbseffh = Vbs;                                                           /* 4.5 */
    }
/* Avoid dividing by zero in Itun model */                                       /* 7.5 */
/*    else if (T0 < -80.0)              */                                       /* 7.5 */ 
/*    {   Vbseffh = Vbih;               */                                       /* 7.5 */
/*    }                                 */                                       /* 7.5 */
    else
    {   Vbseffh = Vbih - Vtm * log(1.0 + exp(T0));                               /* 7.5W */
    }
    Xchk = 1.0 - Vbseffh / Vbih;			                         /* 7.5W */
/*    if (Xchk < 0) Xchk = 0;    */          /* 7.5 */				 /* 4.41 */
    Dsh = D0h * sqrt(Xchk);				                         /* 7.5W */
    T0 = (Vbi - Vbs) / Vtm;                                                      /* 7.5W */
    if (T0 > 80.0)                                                               /* 4.5 */
    {   Vbseffl = Vbs;                                                           /* 4.5 */
    }                                                                            /* 4.5 */
    else if (T0 < -80.0)                                                         /* 4.5 */
    {   Vbseffl = Vbi;                                                           /* 4.5 */
    }                                                                            /* 4.5 */
    else                                                                         /* 4.5 */
    {   Vbseffl = Vbi - Vtm * log(1.0 + exp(T0));                                /* 4.5 */
    }                                                                            /* 4.5 */
    Xchk = 1.0 - Vbseffl / Vbi;          			                 /* 4.5 */
    if (Xchk < 0) Xchk = 0;						         /* 4.41 */
    Ds = D0 * sqrt(Xchk);					                 /* 7.5W */

    Dshare = Ds;					                         /* 4.5 */
    if (Dshare > 0.5 * Leff) Dshare = 0.5 * Leff;
    Tshare = pModel->Tf - pModel->Thalo;
    if (Tshare > pModel->Tb) Tshare = pModel->Tb;
    Qshare = Dshare * Tshare / (pModel->Tb * Leff);
    Qbeff = pInst->Qb * (1.0 - Qshare);                                          /* 4.5r */
/*  Define a tolerance to avoid DIVIDE BY ZERO for analytical derivatives           4.5d */
    Dvtol = 1.0e-12;                                                             /* 4.5d */

/*  Start Overshoot Model  */                                                    /* 5.0vo */
    
/* SET VSAT INITIAL VALUE  */
     Vo = pModel->Vo; 
     Toxf = pModel->Toxf; 
     Vsat = pTempModel->Vsat;
     Uo = pTempModel->Uo;
     Theta = pModel->Theta;
     Lc = pModel->Lc;
     Xalpha = pModel->Xalpha;   

/* CALCULATE THRESHOLD VOLTAGE (for velocity overshoot) */
    Vth = Vfbf + Xalpha * pTempModel->Phib
	       - 0.5 * pInst->Qb / Coxf - (Xalpha - 1)*Vbs;                          
               
/* CALCULATE EFFECTIVE MOBILITY */      
     Xex = (Vgfs - Vth + ESI * pModel->Tb 
         * Vds / (Coxf * Leff * Leff)) * Coxf / (2.0 * ESI);
     Xueff = Uo / (1.0 + Theta * Xex);
     Tau_w=3.0*Vtm*Xueff/(2.0*Vsat*Vsat);  

/* CALCULATE Vds_eff  */
     Qcbycox = (Vgfs - Vth) / (Xalpha);   
     Vdsdemo = 1.0 + Qcbycox * 0.5 * Xueff / (Vsat * Leff);
     Vds_eff = Qcbycox / Vdsdemo / Xalpha;
    /*  if (Vds_eff<=0) Vds_eff=0;  */
     Vds_eff = log(1.0 + exp(10.0 * Vds_eff)) / 10.0;

/* CALCULATE Vsat_eff  */
     Xec = 2.0 * Vsat / Xueff;
     Vds_dif = Vds - Vds_eff;
     Vds_dif = log(1.0 + exp(10.0 * Vds_dif)) / 10.0;
     Ldf = Vds_dif / (Xec * Lc);
     Ld = Lc * log(Ldf + sqrt(1.0 + Ldf * Ldf));
     Xsinh = Vo * sinh((Vo * Ld) / Lc) / Lc;  
     Xcosh = cosh((Vo * Ld) / Lc);   
     Vdemo = Xsinh / Xcosh;
     Vsat_eff = Vsat * (1.0 + 2.0 * Vsat * Tau_w * Vdemo / 3.0);   
     /* printf ("Vsat_eff=%.3e\n", Vsat_eff); */   

/* SET VSAT EQUAL TO VSAT_eff  */ 
     pTempModel->Fsat = 0.5 * pTempModel->Uo/Vsat_eff/Leff; 
       
/* Vsat IS Vsat_eff EVERYWHERE FROM NOW ON*/

    /* Beginning of Ldd loop moved here for QM */
    Vldd[0] = 0.0;
    Vldd[1] = Vds;
    Irfldd = 0;
    for (IB = 0; IB < 30; IB++)
    {    Vdss = Vds - Vldd[IB];
         Arg = (Vbs - Vdss) / Vtm;
         if (Arg > 80.0)
	 {   Vbd_bjt = Vbs - Vdss;
             dVbd_bjtds = -1.0;                                                  /* 4.5d */
	 }
         else if (Arg < -80.0)
	 {   Vbd_bjt = 0.0;
             dVbd_bjtds = 0.0;                                                   /* 4.5d */
	 }
	 else
	 {   T1 = exp(Arg);                                                      /* 4.5d */
             Vbd_bjt = Vtm * log(1.0 + T1);                                      /* 4.5d */
             dVbd_bjtds = -T1 / (1.0 + T1);                                      /* 4.5d */
	 }
         Sqrt_d = exp(0.5 * Vbd_bjt / Vtm);
	 dSqrt_dds = 0.5 * Sqrt_d * dVbd_bjtds / Vtm;                            /* 5.0 */
         Argbjtd = Sqrt_d * Sqrt_d;
	 dArgbjtdds = 2.0 * Sqrt_d * dSqrt_dds;                                  /* 5.0 */

	 T0 = (Vbih - Vbs + Vdss) / Vtm;                                         /* 7.5W */
	 if (T0 > 80.0)
	 {   Vbdeffh = Vbs - Vdss;                                               /* 4.5 */
	     dVbdeffhds = -1.0;                                                  /* 4.5d */
	 }
/* Avoid dividing by zero in Itun model  */                                      /* 7.5 */
/*         else if (T0 < -80.0)          */                                      /* 7.5 */
/*         {   Vbdeffh = Vbih;           */                                      /* 7.5 */
/*             dVbdeffhds = 0.0;         */         /* 7.5 */                    /* 4.5d */
/*         }                             */                                      /* 7.5 */
	 else
	 {   T1 = exp(T0);                                                       /* 4.5d */
	     Vbdeffh = Vbih - Vtm * log(1.0 + T1);                               /* 7.5W */
	     dVbdeffhds = -T1 / (1.0 + T1);                                      /* 4.5d */
	 }

	 Xchk = 1.0 - Vbdeffh / Vbih;			                         /* 7.5W */
/*         if (Xchk <= 0)                */         /* 7.5 */                    /* 5.0 */
/*         {   Ddh = dDdhds = 0.0;       */         /* 7.5 */                    /* 5.0 */
/*         }                             */         /* 7.5 */                    /* 5.0 */
/*         else                          */         /* 7.5 */                    /* 5.0 */
	 Ddh = D0h * sqrt(Xchk);		                     		 /* 7.5W */
         dDdhds = -0.5 * Ddh * dVbdeffhds / (Vbih - Vbdeffh);                    /* 7.5W */

	 T0 = (Vbi - Vbs + Vdss) / Vtm;                                          /* 7.5W */
	 if (T0 > 80.0)                                                          /* 4.5 */
	 {   Vbdeffl = Vbs - Vdss;                                               /* 4.5 */
	     dVbdefflds = -1.0;                                                  /* 5.0 */
	 }                                                                       /* 4.5 */
	 else if (T0 < -80.0)                                                    /* 4.5 */
	 {   Vbdeffl = Vbi;                                                      /* 4.5 */
	     dVbdefflds = 0.0;                                                   /* 5.0 */
	 }                                                                       /* 4.5 */
	 else                                                                    /* 4.5 */
	 {   T1 = exp(T0);                                                       /* 5.0 */
	     Vbdeffl = Vbi - Vtm * log(1.0 + T1);                                /* 5.0 */
	     dVbdefflds = -T1 / (1.0 + T1);                                      /* 5.0 */
	 }                                                                       /* 4.5 */
	 Xchk = 1.0 - Vbdeffl / Vbi;            			         /* 4.5 */
	 if (Xchk <= 0)                                                          /* 5.0 */
	 {   Dd = dDdds = 0.0;                                                   /* 5.0 */
	 }                                                                       /* 5.0 */
	 else                                                                    /* 5.0 */
	 {   Dd = D0 * sqrt(Xchk);		       		                 /* 7.5W */
             dDdds = -0.5 * Dd * dVbdefflds / (Vbi - Vbdeffl);                   /* 5.0 */
	 }                                                                       /* 5.0 */

/* Noniterative Vthw */                                                          /* 7.0Y */
         Coeffa = pInst->Nblavg * Coxf * pModel->Xalpha / (ELECTRON_CHARGE       /* 7.0Y */
                * Xnin2 * 100.0 * pModel->Sfact);                                /* 7.0Y */
         Delqc = Dicefac * Vdss;                                                 /* 4.5d */
         Exf_Vthw = -pInst->Qb / ESI;                                            /* 7.0Y */
         Psiw = Vtm * log(Coeffa * Exf_Vthw);                                    /* 7.0Y */
         QmT00 = ELECTRON_CHARGE * Xnin2 * Vtm / (ESI * pInst->Nblavg);          /* 7.0Y */
         QmT3 = pModel->Qm * 5.33e-9 / 9.0;                                      /* 7.0Y */
         QmT4 = ESI / (4.0 * Vtm * ELECTRON_CHARGE);                             /* 7.0Y */
         QmT5 = 1.0 / 3.0;                                                       /* 7.0Y */
         T6 = QmT4 * Exf_Vthw * Exf_Vthw;                                        /* 7.0Y */
         DelEgQM = QmT3 * (9.0 / 13.0) * pow(T6,QmT5);                           /* 7.0Y */
         Psiw = Psiw + DelEgQM - DelEgGe;                                        /* 7.5W */
	 Arg = (Vbs - Psiw) / Vtm;                                               /* 4.5F */
	 if (Arg > 80.0)                                                         /* 4.5F */
	 {  Psiw = Vbs;                                                          /* 4.5F */
	 }                                                                       /* 4.5F */
	 else if (Arg > -80.0)                                                   /* 4.5F */
	 {  Psiw = Psiw + Vtm * log(1.0 + exp(Arg));                             /* 4.5F */
	 }                                                                       /* 4.5F */
	 Vthw = Vfbf + pModel->Xalpha * Psiw - (0.5 * pInst->Qb                  /* 4.5F */
	      + Cb * Vbs + Delqc) / Coxf;                                        /* 4.5F */

/* Noniterative Vths */                                                          /* 7.0Y */
         Coeffb = 5.0 * Coxf * pModel->Xalpha;                                   /* 7.0Y */
         Coeffb2 = Coeffb * Coeffb;                                              /* 7.0Y */
         Coeffb3 = 2.0 * Vtm * pInst->Nblavg * Coeffb2  / (ELECTRON_CHARGE       /* 7.0Y */
                   * Xnin2 * ESI );                                              /* 7.0Y */
         Psis = Vtm * log(Coeffb3);                                              /* 7.0Y */
         QmT2_Vths = exp(Psis / Vtm);                                            /* 7.0Y */
         Exf02_Vths = 2.0 * QmT00 * QmT2_Vths;                                   /* 7.0Y */
         Exf0 = sqrt(Exf02_Vths);                                                /* 7.0Y */
         Qcs=ESI*Exf0;                                                           /* 7.0Y */
         if (pModel->Qm == 0.0)                                                  /* 7.0Y */
         {  DelEg = -DelEgGe;                                                    /* 7.5W */
         }                                                                       /* 7.0Y */
         else                                                                    /* 7.0Y */
         {  T1 = QmT4 * Exf0 * Exf0;                                             /* 7.0Y */
/*   Account for exchange energy [Stern, 1973] in Vths   */                      /* 7.0Y */
            DelEgQM = QmT3 * pow(T1,QmT5);                                       /* 7.0Y */
            NinvEx = Qcs / ELECTRON_CHARGE;                                      /* 7.0Y */
            NvlyEx = 1.0 + pModel->Type + 0.5 * (1.0 - pModel->Type);            /* 7.0Y */
            DelEgEx = 1.93912e-10 * sqrt(NinvEx) / sqrt(NvlyEx);                 /* 7.0Y */
            DelEg = DelEgQM - DelEgEx - DelEgGe;                                 /* 7.5W */
         }                                                                       /* 7.0Y */
         Psis = Psis + DelEg;                                                    /* 7.0Y */
         Arg = (Vbs - Psis) / Vtm;                                               /* 7.0Y */
         if (Arg > 80.0)                                                         /* 7.0Y */
         {  Psis = Vbs;                                                          /* 7.0Y */
         }                                                                       /* 7.0Y */
         else if (Arg > -80.0)                                                   /* 7.0Y */
         {  Psis = Psis + Vtm * log(1.0 + exp(Arg));                             /* 7.0Y */
         }                                                                       /* 7.0Y */
         Vths = Vfbf + pModel->Xalpha * Psis - (0.5 * Qbeff - Qcs                /* 7.0Y */
                + Cb * Vbs + Delqc) / Coxf;                                      /* 7.0Y */
         Vths_bjt = Vths;                                                        /* 7.0Y */

/* Calculation of Psis for Vgfs > Vths */
         Coeffz0 = 5.0 * pInst->Nblavg * Coxf * pModel->Xalpha /                 /* 7.0Y */
                   (ELECTRON_CHARGE * Xnin2);                                    /* 7.0Y */
         ExfLim = 2.0 * Vtm / pModel->Tb;                                        /* 4.5F */
	 for (K = 0; K < 3; K++)                                                 /* 7.0Y */
         {  PsisOld = Psis;                                                      /* 7.0Y */
	    if (pModel->Qm == 0.0)                                               /* 7.0Y */
            {  DelEg= -DelEgGe;                                                  /* 7.5W */
               XniFac = exp(-DelEg / 2.0 / Vtm);                                 /* 7.0Y */
               XniFac2 = XniFac * XniFac;                                        /* 7.0Y */
               PhibQM = 2.0 * Vtm * log(pInst->Nblavg / Xnin / XniFac);          /* 7.0Y */
	    }		   		   		         		 /* 7.0Y */
	    else		   		   		   		 /* 7.0Y */
	    {  Ess = (Vgfs - pTempModel->Vfbf - PsisOld) * Coxf / ESI;           /* 7.0Y */
	       Arg = (Ess - ExfLim) / ExfLim;                                    /* 7.0Y */
	       if (Arg < -80.0)                                                  /* 7.0Y */
	       {  Ess = ExfLim;                                                  /* 7.0Y */
	       }                                                                 /* 7.0Y */
	       else if (Arg < 80.0)                                              /* 7.0Y */
	       {  Ess = ExfLim + ExfLim * log(1.0 + exp(Arg));                   /* 7.0Y */
	       }                                                                 /* 7.0Y */
	       T1 = QmT4 * Ess * Ess;                                            /* 7.0Y */
/*   Account for exchange energy [Stern, 1973] in Vths   */                      /* 7.0Y */
               DelEgQM = QmT3 * pow(T1,QmT5);                                    /* 7.0Y */
               QinvEx = ESI * Ess;                                               /* 7.0Y */
               NinvEx = QinvEx / ELECTRON_CHARGE;                                /* 7.0Y */
               NvlyEx = 1.0 + pModel->Type + 0.5 * (1.0 - pModel->Type);         /* 7.0Y */
               DelEgEx = 1.93912e-10 * sqrt(NinvEx) / sqrt(NvlyEx);              /* 7.0Y */
               DelEg = DelEgQM - DelEgEx - DelEgGe;                              /* 7.5W */
               XniFac = exp(-DelEg / 2.0 / Vtm);                                 /* 7.0Y */
	       XniFac2 = XniFac * XniFac;                                        /* 7.0Y */
               PhibQM = 2.0 * Vtm * log(pInst->Nblavg / Xnin / XniFac);          /* 7.0Y */
	    }		   		   		         		 /* 7.0Y */
 	    QmT0 = QmT00 * XniFac2;                                              /* 7.0Y */
	    QmT1 = exp(PhibQM / Vtm);                                            /* 7.0Y */
	    QmT2 = exp(PsisOld / Vtm);                                           /* 7.0Y */
            Coeffz = Coeffz0 / XniFac2;                                          /* 7.0Y */
            Exf00 = (PhibQM - Vbs - 0.5 * Qbeff / Cb) / pModel->Tb;              /* 7.0Y */
	    Exf02 = Exf00 * Exf00 + 2.0 * QmT0 * (QmT2 - QmT1); 	         /* 7.0Y */
	    Exf0 = sqrt(Exf02);				            		 /* 7.0Y */
	    Psis = PsisOld - (PsisOld - Vtm * log(Coeffz * Exf0))	         /* 7.0Y */
	                   / (1.0 - QmT0 * QmT2 / Exf02);	                 /* 7.0Y */
         }
         Arg = (Vbs - Psis) / Vtm;                                               /* 7.0Y */
	 if (Arg > 80.0)                                                         /* 7.0Y */
	 {  Psis = Vbs;                                                          /* 7.0Y */
	 }                                                                       /* 7.0Y */
	 else if (Arg > -80.0)                                                   /* 7.0Y */
	 {  Psis = Psis + Vtm * log(1.0 + exp(Arg));                             /* 7.0Y */
	 }                                                                       /* 7.0Y */
         Psis_str = Psis;                                                        /* 7.0Y */
         Psis_bjt = Psis;                                                        /* 7.0Y */
         Lcfac = pModel->Lc / Leff;
         Lefac = 1.0;
         Le = Leff;
	 DeltaVth = Delqc / Coxf;                                                /* 4.5F */
/*         DVgfs = 1.0e-4;               */                                          /* 4.5F */
         DVgfs = 1.0e-3;                                                         /* 7.0Y */
	 if (Vgfs >= Vths)
	 {   Region = 1;
	     ICONT = 1;
	     Vgstart = Vgfs;
	 }
	 else if (Vgfs <= Vthw)
	 {   Region = 3;
	     ICONT = 1;
	     Vgstart = Vgfs;
	 }
	 else
	 {   Region = 2;
	     ICONT = 2;
	     Vgstart = Vths;
	 }

	 Vgfso = Vgfs;
	 if (Region < 3)
	 {   Mobfac1 = pModel->Theta * Coxf / (2.0 * ESI);
	     Xbfact = pModel->Bfact * Mobfac1 * (2.0 - pModel->Xalpha);
	     for (I = 0; I < ICONT; I++)
	     {    Vgfs = Vgstart + I * DVgfs;                                    /* 4.5F */
                  if (Region == 1)                                               /* 4.5F */
	          { Psis = Psis_str;                                             /* 4.5F */
	          }                                                              /* 4.5F */
	          else                                                           /* 4.5F */
	          { for (K = 0; K < 3; K++)                                      /* 4.5F */
                    { PsisOld = Psis;                                            /* 4.5F */
                      if (pModel->Qm == 0.0)                                     /* 7.0Y */
                      {  DelEg= -DelEgGe;                                        /* 7.5W */
                         XniFac = exp(-DelEg / 2.0 / Vtm);                       /* 7.0Y */
                         XniFac2 = XniFac * XniFac;                              /* 7.0Y */
                         PhibQM = 2.0 * Vtm * log(pInst->Nblavg / Xnin / XniFac);   /* 7.0Y */
                      }                                                          /* 7.0Y */
                      else                                                       /* 7.0Y */
	              {  Ess = (Vgfs - pTempModel->Vfbf - PsisOld) * Coxf / ESI; /* 7.0Y */
	                 Arg = (Ess - ExfLim) / ExfLim;                          /* 7.0Y */
	                 if (Arg < -80.0)                                        /* 7.0Y */
	                 {  Ess = ExfLim;                                        /* 7.0Y */
	                 }                                                       /* 7.0Y */
	                 else if (Arg < 80.0)                                    /* 7.0Y */
	                 {  Ess = ExfLim + ExfLim * log(1.0 + exp(Arg));         /* 7.0Y */
	                 }                                                       /* 7.0Y */
	                 T1 = QmT4 * Ess * Ess;                                  /* 7.0Y */
                         DelEgQM = QmT3 * pow(T1,QmT5);                          /* 7.0Y */
                         QinvEx = ESI * Ess;                                     /* 7.0Y */
                         NinvEx = QinvEx / ELECTRON_CHARGE;                      /* 7.0Y */
                         NvlyEx = 1.0 + pModel->Type + 0.5 * (1.0 - pModel->Type);  /* 7.0Y */
                         DelEgEx = 1.93912e-10 * sqrt(NinvEx) / sqrt(NvlyEx);    /* 7.0Y */
                         DelEg = DelEgQM - DelEgEx - DelEgGe;                    /* 7.5W */
                         XniFac = exp(-DelEg / 2.0 / Vtm);                       /* 7.0Y */
                         XniFac2 = XniFac * XniFac;                              /* 7.0Y */
                         PhibQM = 2.0 * Vtm * log(pInst->Nblavg / Xnin / XniFac);   /* 7.0Y */
                      }
                      QmT0 = QmT00 * XniFac2;                                    /* 4.5F */
                      QmT1 = exp(PhibQM / Vtm);                                  /* 4.5F */
                      QmT2 = exp(PsisOld / Vtm);                                 /* 4.5F */
                      Coeffz = Coeffz0 / XniFac2;                                /* 4.5F */
                      Exf00 = (PhibQM - Vbs - 0.5 * Qbeff / Cb) / pModel->Tb;    /* 4.5F */
                      Exf02 = Exf00 * Exf00 + 2.0 * QmT0 * (QmT2 - QmT1);        /* 4.5F */
                      Exf0 = sqrt(Exf02);				       	 /* 4.5F */
                      Psis = PsisOld - (PsisOld - Vtm * log(Coeffz * Exf0))	 /* 4.5F */
                                       / (1.0 - QmT0 * QmT2 / Exf02);	         /* 4.5F */
      	            }                                                            /* 4.5F */
                    Arg = (Vbs - Psis) / Vtm;                                    /* 4.5F */
                    if (Arg > 80.0)                                              /* 4.5F */
                    { Psis = Vbs;                                                /* 4.5F */
                    }                                                            /* 4.5F */
                    else if (Arg > -80.0)                                        /* 4.5F */
                    { Psis = Psis + Vtm * log(1.0 + exp(Arg));                   /* 4.5F */
                    }
                  }			                                         /* 4.5F */
	          if(pModel->Ngate == 0.0 || pModel->Tpg == -1.0)                /* 4.5pd */
	          {  Psigf = 0.0;                                                /* 4.5pd */
                     dPsigfgfs = 0.0;                                            /* 4.5d */
	             Alphap = pModel->Xalpha;                                    /* 4.5pd */
	          }                                                              /* 4.5pd */
	          else                                                           /* 4.5pd */
	          {  T0 = ELECTRON_CHARGE * pModel->Ngate * ESI;                 /* 4.5pd */
                     T1 = T0 / (Coxf * Coxf);                                    /* 4.5d */
	             T2 = Vgfs - pTempModel->Wkf - Psis;                         /* 4.5d */
                     T3 = sqrt(T1 * (T1 + 2.0 * T2));                            /* 4.5d */
                     Psigf = T1 + T2 - T3;                                       /* 4.5d */
                     dPsigfgfs = 1.0 - (T1 / T3);                                /* 4.5d */
		     T4 = sqrt(T0 / (2.0 * Psigf)) / Coxf;                       /* 4.5pd */
		     Alphap = pModel->Xalpha - 1.0 / (1.0 + T4);                 /* 4.5pd */
		  }                                                              /* 4.5pd */
	          Qc0 = Coxf * (Vgfs - Psigf - Vfbf - pModel->Xalpha             /* 4.5pd */
                        * Psis) + 0.5 * Qbeff + Cb * Vbs;                        /* 4.5pd */
	          Qc = Qc0 + Delqc;
		  /* T11 is QCbyCOF in SOISPICE                                     4.5pd */
	          T11 = Qc / (Coxf * pModel->Xalpha);
		  T11pd = Qc / Coxf / Alphap;                                    /* 4.5pd */
	          T0 = pModel->Tb / Leff;
	          Mobred = 1.0 + Mobfac1 / Coxf * (Qc - Qbeff + 2.0 * Cb
		         * (Psis - Vbs) + Cb * T0 * T0 * Vdss);                  /* 4.5qm */
	          Mobred = 1.0 / Mobred;
	          Xbmu = Xbfact * Mobred; 
	          T1 = 1.0 + T11pd * (Xbmu + pTempModel->Fsat * Mobred);         /* 4.5pd */
	          Vdsat = T11pd / (T1 * (0.5 + sqrt(fabs(0.25 - Xbmu * T11pd     /* 4.5pd */
			/ (T1 * T1)))));					 /* 4.41 */
/* MERGE TRIODE AND SATURATION REGIONS :
   ALL VDSeff's (in Sat.) AND VDSS's (in Triode) ARE REPLACED BY VDSX;
   First smooth VDSsat :                                                            4.41 */
		      T0 = log(1.0 + exp(8.0));                                  /* 4.5d */
	              T2 = 0.0;                                                  /* 4.5d */
		      if (Vdsat < 1.0e-15)                                       /* 4.5 */
		          Vdsat = 1.0e-15;                                       /* 4.5 */
		      Covf = 1.0 - log(1.0 + exp(8.0 * (1.0 - Vdss / Vdsat)))    /* 4.5d */
			   / T0;                                                 /* 4.5d */
		      Vdsat = Vdsat * Covf;
		      Mobref = Mobred / (1.0 - Xbmu * Vdsat);
		      T2 = (Vdss - Vdsat) * Mobref * pTempModel->Fsat
		         / Lcfac;
		      Lefac = 1.0 - Lcfac * log(T2 + sqrt(1.0 + T2 * T2));
		      if (Lefac < 1.0e-3)
		          Lefac = 1.0e-3;
		      Le = Lefac * Leff;
/* Iterate once :                                                                   4.41 */
		      T3 = 1.0 + T11pd * (Xbmu + pTempModel->Fsat * Mobred       /* 4.5pd */
		         / Lefac);
	              Vdsat = T11pd / (T3 * (0.5 + sqrt(fabs(0.25                /* 4.5pd */
			    - Xbmu * T11pd / (T3 * T3)))));		         /* 4.5pd */
		 if (Vdsat < 1.0e-15)                                            /* 4.5 */
		     Vdsat = 1.0e-15;                                            /* 4.5 */
		 Vdseff = Vdsat;
/* Now smooth VDSX :                                                                4.41 */
		 T1= exp(8.0 * (1.0 - Vdss / Vdseff));                           /* 4.5d */
		 Covf = 1.0 - log(1.0 + T1) / T0;                                /* 4.5d */
		 Vdsx = Vdseff * Covf;
		 Mobref = Mobred / (1.0 - Xbmu * Vdsx);
/* CHANNEL CURRENT (NOW FROM UNIFIED ANALYSIS) :                                    4.41 */
		 T7 = Alphap * Vdsx * Coxf;                                      /* 4.5pd */
		 T3 = 2.0 * Qc - T7;                                             /* 4.5d */
		 T4 = T7 * T3;                                                   /* 4.5d */
		 T5 = 1.0 + pTempModel->Fsat * Mobref * Vdsx / Lefac;            /* 4.5d */
		 Ich = pTempModel->Beta0 * Mobref * T4 / (Alphap * T5)           /* 4.5d */
		     / Lefac;	 
		 Gichgfs = Ich * 2.0 * Coxf * (1.0 - dPsigfgfs) / T3;            /* 4.5d */
		 dVdsxds = 8.0 * T1 / (T0 * (1.0 + T1));                         /* 4.5d */
		 T6 = sqrt(1.0 + T2 * T2);                                       /* 4.5d */
		 dLefacds = (1.0 + T2 / T6) * Mobref * pTempModel->Fsat          /* 4.5d */
		          * (dVdsxds - 1.0) /(T2 + T6);                          /* 4.5d */
		 Gichds = pTempModel->Beta0 * Mobref * Coxf * ((T3 * dVdsxds     /* 4.5d */
		        + (2.0 * Dicefac - Alphap * Coxf * dVdsxds) * Vdsx)      /* 4.5d */
			/ (T5 * Lefac) - (dLefacds + Mobref * pTempModel->Fsat   /* 4.5d */
			* dVdsxds) * Vdsx * T3 / pow((T5 * Lefac),2.0));         /* 4.5d */
                 Ueff = Mobref * pTempModel->Uo;                                 /* 4.5 */
                 if(ACNeeded == 1)                                               /* 5.0 */
                 { T = Vtm / KoverQ;                                             /* 5.0 */
		   Exo = (Qc0 - Qbeff) / (2.0 * SILICON_PERMITTIVITY)            /* 5.0 */
                       + (Psis - Vbs) / pModel->Tb;                              /* 5.0 */
                   Ueffo = pTempModel->Uo / (1.0 + pModel->Theta * Exo);         /* 5.0 */
                   if(Ich == 0.0)                                                /* 5.0 */
                   { Sich = 4.0 * Weff * Boltz * T * Ueffo * Qc0 / Leff;         /* 5.0 */
		   }                                                             /* 5.0 */
		   else                                                          /* 5.0 */
		   { Lc = pModel->Lc;                                            /* 5.0 */
		     Sich = XIntSich(Weff, Leff, T, Vdsx, Le, Lc, Ich, Qc0,      /* 5.0 */
                                     Ueffo);                                     /* 5.0 */
		   }                                                             /* 5.0 */
		   pOpInfo->Sich = Sich;                                         /* 5.0 */
		 }                                                               /* 5.0 */
		 if (I == 0)
		 {   Ist1 = Ich;
		     Gm1 = Gichgfs;                                              /* 4.5d */
		     Go1 = Gichds;                                               /* 4.5d */
		     Qc01 = Qc0;                                                 /* 4.5 */
		     Qc1 = Qc;
		     Mobred1 = Mobred;
		     Mobref1 = Mobref;
		     Vdsx1 = Vdsx;						 /* 4.41 */
		     Xbmu1 = Xbmu;
		     Lefac1 = Lefac;						 /* 4.41 */
		     Psigf1 = Psigf;                                             /* 4.5pd */
		     Alphap1 = Alphap;                                           /* 4.5pd */
		     Psis1 = Psis;                                               /* 4.5qm */
		     dPsigfgfs1 = dPsigfgfs;                                     /* 4.5d */
		     dVdsxds1 = dVdsxds;                                         /* 4.5d */
		     dLefacds1 = dLefacds;                                       /* 4.5d */
                     if(ACNeeded == 1) Sis1 = Sich;                              /* 5.0 */
		 }
		 else
		 {   Ist2 = Ich;		     
		     Go2 = Gichds;                                               /* 4.5d */
		     Qc02 = Qc0;                                                 /* 4.5 */
		     Qc2 = Qc;
		     Mobred2 = Mobred;
		     Mobref2 = Mobref;
		     Vdsx2 = Vdsx;						 /* 4.41 */
		     Xbmu2 = Xbmu;
		     Lefac2 = Lefac;						 /* 4.41 */
		     Psigf2 = Psigf;                                             /* 4.5pd */
		     Alphap2 = Alphap;                                           /* 4.5pd */
		     Psis2 = Psis;                                               /* 4.5qm */
                     if(ACNeeded == 1) Sis2 = Sich;                              /* 5.0 */
		 }
	     }
	     if (Region == 2)
	     {   Vgstart = Vthw;                                                 /* 4.5F */
	     }
	 }
	 if (Region > 1)
	 { Vdseff = 0.0;
	   Dell = pModel->Lc * log(0.5 / (Lcfac * Lcfac));
	   Le = (Leff - 2.0 * Dell);
/*           if (Le < pTempModel->Lemin) Le = pTempModel->Lemin;      */
           if (Le < LEMIN) Le = LEMIN;                                           /* 6.1 */
           Lefac = Le / Leff;
	   T0 = Dicefac / (Coxf * pModel->Xalpha);                               /* 4.5d */
	   for (I = 0; I < ICONT; I++)                                           /* 4.5F */
	   { Vgfs = Vgstart - I * DVgfs;                                         /* 4.5F */
	     Psiw0 = (Vgfs - Vfbf + 0.5 * pInst->Qb / Coxf + CbbyCof * Vbs)      /* 4.5d */
		   / pModel->Xalpha;
	     Psiwk = Psiw0 + DeltaVth / pModel->Xalpha;
	     Exf = (Psiw0 - Vbs - 0.5 * pInst->Qb / Cb - Delqc                   /* 4.5F */
		 / (Cb * pModel->Xalpha)) / pModel->Tb;
	     Arg = (Exf - ExfLim) / ExfLim;                                      /* 4.5F */
	     if (Arg < -80.0)                                                    /* 4.5F */
	     {  Exf = ExfLim;                                                    /* 4.5F */
	     }                                                                   /* 4.5F */
             else if (Arg < 80.0)                                                /* 4.5F */
             {  Exf = ExfLim + ExfLim * log(1.0 + exp(Arg));                     /* 4.5F */
             }                                                                   /* 4.5F */
	     Mobref = 1.0 / (1.0 + pModel->Theta * Exf);                         /* 4.5 */
             Ueff = Mobref * pTempModel->Uo;                                     /* 4.5 */
	     T1 = 1.0 - exp(-Vdss / Vtm);                                        /* 4.5d */
	     T2 = 2.0 * pTempModel->Beta0 * Mobref * ELECTRON_CHARGE * Xnin2     /* 4.5d */
	        * Vtm * Vtm * Coxf / (Lefac * pInst->Nblavg);                    /* 4.5d */
	     T3 = exp(Psiwk / Vtm);                                              /* 4.5d */
	     Ich = T2 * T3 * T1 / Exf;                                           /* 4.5d */
             if (pModel->Qm > 0.0 || DelEgGe > 0.0)                              /* 7.5W */
	     { Esw = (Vgfs - pTempModel->Vfbf - Psiwk) * Coxf / ESI;             /* 4.5F */
	       Arg = (Esw - ExfLim) / ExfLim;                                    /* 4.5F */
	       if (Arg < -80.0)                                                  /* 4.5F */
	       {  Esw = ExfLim;                                                  /* 4.5F */
		  T5 = 0.0;                                                      /* 4.5d */
	       }                                                                 /* 4.5F */
               else if (Arg < 80.0)                                              /* 4.5F */
               {  T4 =  1.0 + exp(Arg);                                          /* 4.5d */
		  Esw = ExfLim + ExfLim * log(T4);                               /* 4.5d */
		  T5 = -T0 * Coxf / (ESI * T4);                                  /* 4.5d */
               }                                                                 /* 4.5F */
	       else                                                              /* 4.5d */
	       {                                                                 /* 4.5d */
		  T5 = -T0 * Coxf / ESI;                                         /* 4.5d */
	       }                                                                 /* 4.5d */
               T6 = QmT4 * Esw * Esw;                                            /* 4.5F */
	       DelEgQM = QmT3 * (9.0 / 13.0) * pow(T6,QmT5);                     /* 7.0Y */
               DelEg = DelEgQM - DelEgGe;                                        /* 7.5W */
               XniFac2 = exp(-DelEg / Vtm);                                      /* 4.5F */
	       Ich = Ich*XniFac2;                                                /* 4.5F */
	       Gichds = XniFac2 * (T2 * T3 * (T1 * T0 * (1.0 / Vtm               /* 4.5d */
		      + 1.0 / (Exf * pModel->Tb * CbbyCof))                      /* 4.5d */
                      + (1.0 - T1) / Vtm) / Exf)                                 /* 4.5d */
		      + Ich * 2.0 * DelEg * T5 / (3.0 * Vtm * Esw);              /* 4.5d */
	     }                                                                   /* 4.5d */
	     else                                                                /* 4.5d */
	     { Gichds = T2 * T3 * (T1 * T0 * (1.0 / Vtm                          /* 4.5d */
		      + 1.0 / (Exf * pModel->Tb * CbbyCof))                      /* 4.5d */
		      + (1.0 - T1) / Vtm) / Exf;                                 /* 4.5d */
	     }                                                                   /* 4.5F */
	     Gichgfs = Ich / (pModel->Xalpha * Vtm);                             /* 4.5d */
             if(ACNeeded == 1)                                                   /* 5.0 */
             { Ich0 = T2 * T3 / Exf;                                             /* 5.0 */
	       if (pModel->Qm > 0.0) Ich0 = Ich0 * XniFac2;                      /* 5.0 */
               Ich0 = 2.0 * Ich0 - Ich;                                          /* 5.0 */
	       if (Ich0 < 1.0e-40) Ich0 = 1.0e-40;                               /* 5.0 */
               Sich = 2.0 * ELECTRON_CHARGE * Ich0;                              /* 5.0 */
	       pOpInfo->Sich = Sich;                                             /* 5.0 */
             }                                                                   /* 5.0 */
	     if (I == 0)                                                         /* 4.5F */
	     { Iwk1 = Ich;                                                       /* 4.5F */
	       Gow1 = Gichds;                                                    /* 4.5d */
	       Gmw1 = Gichgfs;                                                   /* 4.5d */
	       Qnwk1 = -Ich * Le * Le / Mobref / pTempModel->Uo / Vtm / 2.0;     /* 4.5F */
	       Psiwk1 = Psiwk;                                                   /* 4.5F */
	       Mobrefwk1 = Mobref;                                               /* 4.5F */
               if(ACNeeded == 1) Siw1 = Sich;                                    /* 5.0 */
	     }                                                                   /* 4.5F */
	     else                                                                /* 4.5F */
	     { Iwk2 = Ich;                                                       /* 4.5F */
	       Gow2 = Gichds;                                                    /* 4.5d */
	       Qnwk2 = -Ich * Le * Le / Mobref / pTempModel->Uo / Vtm / 2.0;     /* 4.5F */
	       Psiwk2 = Psiwk;                                                   /* 4.5F */
               if(ACNeeded == 1) Siw2 = Sich;                                    /* 5.0 */
	     }                                                                   /* 4.5F */
	   }                                                                     /* 4.5F */
	   if (Region == 2)
	   {   Vgfs = Vgfso;
	       if(Go1 <1.e-30) Go1=1.e-30;		            		 /* 4.5d */
	       if(Go2 <1.e-30) Go2=1.e-30;				         /* 4.5d */
	       if(Gow1 <1.e-30) Gow1=1.e-30;				         /* 4.5d */
	       if(Gow2 <1.e-30) Gow2=1.e-30;				         /* 4.5d */
	       T1 = log(Go1);						         /* 4.5d */
	       T2 = log(Go2);                                                    /* 4.5d */
	       T3 = log(Gow1);                                                   /* 4.5d */
	       T4 = log(Gow2);                                                   /* 4.5d */
	       dGst = (T2 - T1) / DVgfs;                                         /* 4.5d */
	       dGwk = (T3 - T4) / DVgfs;                                         /* 4.5d */
	       T0 = Vgfs - Vthw;                                                 /* 4.5d */
	       T10 = Vths - Vthw;                                                /* 4.5d */
	       R2 = (3.0 * (T1 - T3) / T10 - (dGst + 2.0 * dGwk)) / T10;         /* 4.5d */
	       R3 = (2.0 * (T3 - T1) / T10 + (dGst + dGwk)) / (T10 * T10);       /* 4.5d */
	       Gom = T3 + T0 * (dGwk + T0 * (R2 + T0 * R3));                     /* 4.5d */
	       Gichds = exp(Gom);                                                /* 4.5d */
	       if (Vdss <= 0.0)
	       {   Ich = 0.0;
		   Gichgfs = 0.0;                                                /* 4.5d */
	       }
	       else
	       {   if(Ist1 <1.e-30) Ist1=1.e-30;				 /* 4.41 */
	           if(Ist2 <1.e-30) Ist2=1.e-30;				 /* 4.41 */
	           if(Iwk1 <1.e-30) Iwk1=1.e-30;				 /* 4.5F */
	           if(Iwk2 <1.e-30) Iwk2=1.e-30;				 /* 4.5F */
	           T1 = log(Ist1);
	           T2 = log(Ist2);
	           T3 = log(Iwk1);                                               /* 4.5F */
	           T4 = log(Iwk2);                                               /* 4.5F */
	           Gst = (T2 - T1) / DVgfs;                                      /* 4.5F */
	           Gwk = (T3 - T4) / DVgfs;                                      /* 4.5F */
		   T11 = 3.0 * (T1 - T3) / T10;                                  /* 4.5d */
		   T12 = 2.0 * (T3 - T1) / T10;                                  /* 4.5d */
	           R2 = (T11 - (Gst + 2.0 * Gwk)) / T10;                         /* 4.5d */
	           R3 = (T12 + (Gst + Gwk)) / (T10 * T10);                       /* 4.5d */
		   T5 = Gm1 / Ist1;                                              /* 4.5d */
		   T6 = Gmw1 / Iwk1;                                             /* 4.5d */
		   T7 = (T11 - (T5 + 2.0 * T6)) / T10;                           /* 4.5d */
		   T8 = (T12 + (T5 + T6)) / (T10 * T10);                         /* 4.5d */
	           Ich = T3 + T0 * (Gwk + T0 * (R2 + T0 * R3));                  /* 4.5F */
	           Ich = exp(Ich);
		   Gichgfs = Ich * (T6 + T0 * (2.0 * T7 + 3.0 * T8 * T0));       /* 4.5d */
	       }
	       Vdseff = Vdsx1 * T0 / T10;	         			 /* 4.41 */
	       Lefac = Lefac + (Lefac1 - Lefac) * T0 / T10;
	       Le = Leff * Lefac;
               Ueff = (Mobrefwk1 + (Mobref1 - Mobrefwk1) * T0 / T10)             /* 4.5F */
                    * pTempModel->Uo;                                            /* 4.5 */
               if(ACNeeded == 1)                                                 /* 5.0 */
               { T1 = log(Sis1);                                                 /* 5.0 */
                 T2 = log(Sis2);                                                 /* 5.0 */
                 Gst = (T2 - T1) / DVgfs;                                        /* 5.0 */
                 T3 = log(Siw1);                                                 /* 5.0 */
                 T4 = log(Siw2);                                                 /* 5.0 */
                 Gwk = (T3 - T4) / DVgfs;                                        /* 5.0 */
                 R2 = (3.0 * (T1 - T3) / T10 - Gst - 2.0 * Gwk) / T10;           /* 5.0 */
                 R3 = (2.0 * (T3 - T1) / T10 + Gst + Gwk ) / (T10 * T10);        /* 5.0 */
                 Sich = exp(T3 + T0 * (Gwk + T0 * (R2 + R3 * T0)));              /* 5.0 */
		 pOpInfo->Sich = Sich;                                           /* 5.0 */
               }                                                                 /* 5.0 */
	   }
	 }
	 if (pModel->Bjt == 1)                                                   /* 6.0 */

/*	 {   Vref = 0.5 * (Vfbf + 0.6 + Vths_bjt);            */                   /* 4.5qm */
/*	     Xs = 2.3 * pModel->Xalpha * Vtm;                 */
/*	     Psf_bjt = Psis_bjt - (Psis_bjt - Vbs_bjt - Vtm)  */
/*	             / (1.0 + exp((Vgfs - Vref) / Xs));       */                   /* 5.0 */

	 {   Xs = 2.3 * pModel->Xalpha * Vtm;                                    /* 6.0 */
             Vref = 0.5 * (Vfbf + 0.8 + Vths_bjt);                               /* 6.0 */
             T0 = fabs(Vthw - (Vfbf + 0.8)) + Vtm;                               /* 6.0 */
             T1 = (Vgfs - Vref) / T0;                                            /* 6.0 */
             Psf_bjt = ((1.0 / (1.0 + exp(-T1))) * (Psis_bjt) +                            
                     (1.0 / (1.0 + exp(T1))) * (Vbs_bjt));                       /* 6.0 */

/*             Xlbjt_t = Leff - Ds - Dd;              */
/*	       if (Xlbjt_t < pTempModel->Lemin)                                     5.0 */
/*	       {   Xlbjt_t = pTempModel->Lemin;                                     5.0 */
/*	           dDdds = 0.0;                                                     5.0 */
/*	       }                                                                    5.0 */
             Xlbjt_t = Leff;                                                     /* 6.1 */
	     Xlbjt_b = Leff + 2.0 * pModel->Lldd - Dsh - Ddh;                    /* 4.5r */
	     if (Xlbjt_b < pTempModel->Leminh)                                   /* 5.0 */
	     {   Xlbjt_b = pTempModel->Leminh;                                   /* 5.0 */
		 dDdhds = 0.0;                                                   /* 5.0 */
	     }                                                                   /* 5.0 */
	     Xaf = (Psf_bjt - Vbs_bjt) / (Vtm * pModel->Tb);
	     T1 = Xaf * pModel->Tb;
	     if (T1 > 50.0) T1 = 50.0;
	     Exp_af = exp(T1);
	     Ph_f = XninGe * Sqrt_s;                                             /* 7.5W */
	     P0f = pInst->Nblavg / Exp_af;                                       /* 4.5r */
	     Tf1 = 1.0 + 2.0 * P0f / Ph_f;
	     Tf2 = 1.0 + 2.0 * pInst->Nblavg / Ph_f;                             /* 4.5r */
	     Xchk = (1.0 - 1.0 / Tf2) / (1.0 - 1.0 / Tf1);			 /* 4.5 */
	     Xintegf = log(Xchk) / (Ph_f * Xaf);				 /* 4.5 */
	     Ph_r = XninGe * Sqrt_d;                                             /* 7.5W */
	     Tr1 = 1.0 + 2.0 * P0f / Ph_r;
	     Tr2 = 1.0 + 2.0 * pInst->Nblavg / Ph_r;;                            /* 4.5r */
	     Xchk = (1.0 - 1.0 / Tr2) / (1.0 - 1.0 / Tr1);			 /* 4.5 */
	     Xintegr = log(Xchk) / (Ph_r * Xaf);				 /* 4.5 */

	     if (pModel->Fvbjt <= 0)
	     {   Xleff_t = Xlbjt_t;
	         Xleff_b = Xlbjt_b;
		 dDddsi = dDdds;                                                 /* 5.0 */
		 dDdhdsi = dDdhds;                                               /* 5.0 */
	     }
	     else
	     {   Feff = pModel->Fvbjt / (1.0 + exp((Vref - Vgfs) / Xs));         /* 5.0 */
	         Xleff_t = 1.0 / (Feff / pModel->Tb
	                + (1.0 - Feff) / Xlbjt_t);
	         Xleff_b = 1.0 / (Feff / pModel->Tf
	                + (1.0 - Feff) / Xlbjt_b);
		 dDddsi = Xleff_t * Xleff_t * (1.0 - Feff) * dDdds               /* 5.0 */
		        / (Xlbjt_t * Xlbjt_t);                                   /* 5.0 */
		 dDdhdsi = Xleff_b * Xleff_b * (1.0 - Feff) * dDdhds             /* 5.0 */
		         / (Xlbjt_b * Xlbjt_b);                                  /* 5.0 */
	     }
	     T12 = Vtm * pTempModel->Mubody;
	     T0 = ELECTRON_CHARGE * Weff * 2.0 * T12 * XninGe2 / Xleff_t;        /* 7.5W */
	     Ibjtf = T0 * Xintegf;
	     Ibjtr = T0 * Xintegr;
	     Ibjt_t = Ibjtf * Argbjts - Ibjtr * Argbjtd;                         /* 5.0 */

	     Qp_b = pModel->Nbh * Xleff_b;
	     if (pModel->Thalo > 0.0)
	     {   Qp_b = Qp_b + 2.0 * pModel->Nhalo * Ldd;                        /* 4.5 */
	     }
	     T12 = Vtm * pTempModel->Mubh;
	     T0 = ELECTRON_CHARGE * Weff * pModel->Teff * XninGe2 * T12 / Qp_b;  /* 7.5W */
	     T1 = 2.0 * T0 / XninGe * pModel->Nbh;                               /* 7.5W */
	     Ibjtf_b = (T0 * T1 * Argbjts) / (T0 * Sqrt_s + T1);
	     T4 = (T0 * T1) / (T0 * Sqrt_d + T1);                                /* 4.5d */
	     Ibjtr_b = T4 * Argbjtd;                                             /* 4.5d */
	     Ibjt_b = Ibjtf_b - Ibjtr_b;
	     Ibjt = Ibjt_t + Ibjt_b;                                             /* 5.0 */
	     Gibjtds = -(Ibjtr + T4) * Argbjtd * dVbd_bjtds / Vtm                /* 5.0 */
	             + Ibjt_b * pModel->Nbh * dDdhdsi / Qp_b                     /* 5.0 */
		     + Ibjt_t * dDddsi / Xleff_t;                                /* 5.0 */
	 }
	 else
	 {   Ibjt = 0.0;
	     Gibjtds = 0.0;                                                      /* 4.5d */
	 }
	 Gidgfs = Gichgfs;                                                       /* 4.5d */
	 Gidds = Gibjtds + Gichds;                                               /* 4.5d */
	 if (Vdss >= Vdseff)
	 {   Xem = (Vdss - Vdseff) / pModel->Lc;
	 }
	 else
	 {   Xem = 0.0;
	 }
	 Itot = Ich + Ibjt;
	 Eslope = ELECTRON_CHARGE * pModel->Nldd / ESI;
	 T0 = Itot * pTempModel->Rldd;
	 if ((Ldd <= 0.0)||(Vds < 1.0e-6)||(Xem < T0)||(pModel->Nldd >= 1.0e25)) /* 4.5 */
	 {   Vlddd = 0.0;
	 }
         else if ((Xem - T0) < Eslope * Ldd)
	 {   T1 = (Xem - T0);
	     Vlddd = T1 * T1 / (2.0 * Eslope);
	 }
	 else
	 {   Vlddd = 0.5 * Ldd * (2.0 * Xem - Eslope * Ldd)
	           - T0 * Ldd;
	 }
	 Vd[IB] = Vldd[IB] - Vlddd;
	 if (fabs(Vd[IB]) < 1.0e-6)
	     break;
         if (IB > 0) /* MCJ */
	 {   T1 = Vd[IB] - Vd[IB-1];
	     if (IB > 1)
	     {   if ((Vd[IB] * Vd[IB-1] < 0.0) && (Irfldd == 0))
		 {   Irfldd = 1;
		 }
	     }
	     if ((Irfldd != 0) && (IB > 1))
	     {   if (Vd[IB] * Vd[IB-1] > 0.0)
		 {   Vldd[IB-1] = Vldd[IB-2];
		     Vd[IB-1] = Vd[IB-2];
		     T1 = Vd[IB] - Vd[IB-1];
		 }
	     }
	     Vldd[IB+1] = (Vd[IB] * Vldd[IB-1] - Vd[IB-1] * Vldd[IB]) / T1;
	     if (Vldd[IB+1] > Vds)
	         break;
	 }
    }
    if (pModel->Debug && (IB >= 30))
    {   fprintf(stderr, "No convergence in Vldd interation.\n");
    }

    /* Thermal generation and recombination currents */
    JroGe = pTempModel->Jro * pow(XninGe / Xnin, 3.0 - pModel->M);               /* 7.5W */
    Irs0 = JroGe * Psj;                                                          /* 7.5W */
    Ird0 = JroGe * Pdj;                                                          /* 7.5W */
    Irs1 = ELECTRON_CHARGE * XninGe2 * pTempModel->Seff * Psj                    /* 7.5W */
	 * pModel->Tf / pTempModel->Ndseff;
    Ird1 = Irs1 / Psj * Pdj;                                                     /* 4.5 */
    T3 = (Vbs - Vdss);
    T1 = Vbs;
    /* Vexplx Gexplx and Ioffsetx defined in terms of Weff, not PSJ or PDJ.
       Cannot use PSJ/PDJ because we did not know which mode the device would
       be in during pre-processing. This should cause a discontinuity in  
       Ir and Igt near Imax (because PSJ, PDJ >= Weff).
       (e.g. if PSJ > Weff, Ir0 > Imax  for Vbs slightly less than Vexpl2.)         4.5 */
    if (T3 >= pTempModel->Vexpl2)
    {   T10 = pTempModel->Ioffset2 + pTempModel->Gexpl2 * T3;
	T6 = -pTempModel->Gexpl2;                                                /* 4.5d */
    }
    else
    {   T2 = exp(T3 / Vtm);                                                      /* 4.5d */
        T10 = Ird1 * (T2 - 1.0);                                                 /* 4.5d */
	T6 = - Ird1 * T2 / Vtm;                                                  /* 4.5d */
    }
    if (T3 >= pTempModel->Vexpl3)
    {   Igt_r = pTempModel->Ioffset3 + pTempModel->Gexpl3 * T3 + T10;
	Gigtds = -pTempModel->Gexpl3 + T6;                                       /* 4.5d */
    }
    else
    {   T0 = exp(T3 / (pModel->M * Vtm));                                        /* 4.5d */
        Igt_r = Ird0 * (T0 - 1.0) + T10;                                         /* 4.5d */
	Gigtds = - Ird0 * T0 / (pModel->M * Vtm) + T6;                           /* 4.5d */
    }
    if (T1 >= pTempModel->Vexpl2)
    {   T10 = pTempModel->Ioffset2 + pTempModel->Gexpl2 * T1;
	T6 = pTempModel->Gexpl2;                                                 /* 4.5d */
    }
    else
    {   T5 = exp(T1 / Vtm);                                                      /* 4.5d */
        T10 = Irs1 * (T5 - 1.0);                                                 /* 4.5d */
	T6 = Irs1 * T5 / Vtm;                                                    /* 4.5d */
    }
    if (T1 >= pTempModel->Vexpl3)
    {   Ir_r = pTempModel->Ioffset3 + pTempModel->Gexpl3 * T1 + T10;
    }
    else
    {   Ir_r = Irs0 * (exp(T1 / (pModel->M * Vtm)) - 1.0) + T10;                 /* 5.0 */
    }
    if (pTempModel->Tauo == 0.0)                                                 /* 4.5 */
    {   Igt = Igt_r;                                                             /* 4.5 */
        Ir = Ir_r;                                                               /* 4.5 */
    }                                                                            /* 4.5 */
    else                                                                         /* 4.5 */
    {   Igts0 = ELECTRON_CHARGE * XninGe * Psj * (pModel->Tb * D0 /              /* 7.5W */
	  pTempModel->Taug + pModel->Teff * D0h / pTempModel->Taugh);            /* 7.5W */
        Igtd0 = Igts0 / Psj * Pdj;                                               /* 4.5 */
        Igtb0 = ELECTRON_CHARGE * XninGe * Weff * Leff * pModel->Tb /            /* 7.5W */
                pTempModel->Taug;                                                /* 4.5 */
        Rgtd = Igtd0 / Ird0;
        Rgts = Igts0 / Irs0;
        if (T3 >= pTempModel->Vexpl1)
        {   Igt_g = pTempModel->Ioffset1 + pTempModel->Gexpl1 * T3;
	    T13 = Igt_g / Igtd0 + 1.0;
	    T6 = - pTempModel->Gexpl1;                                           /* 4.5d */
        }
         else
        {   T13 = T2;                                                            /* 4.5d */
            Igt_g = Igtd0 * (T13 - 1.0);
	    T6 = - Igtd0 * T13 / Vtm;                                            /* 4.5d */
        }
        if (T1 >= pTempModel->Vexpl1)
        {   Ir_g = pTempModel->Ioffset1 + pTempModel->Gexpl1 * T1;
	    T11 = Ir_g / Igts0 + 1.0;
        }
        else
        {   T11 = T5;                                                            /* 4.5d */
            Ir_g = Igts0 * (T11 - 1.0);
        }
	T1 = 1.0 + Rgtd * T13;                                                   /* 4.5d */
	Igt = (Igt_g + Igt_r * Rgtd * T13) / T1;                                 /* 5.0 */
	Gigtds = (T6 + Rgtd * ((Igt_r - Igt) * T6 / Igtd0 + T13 * Gigtds)) / T1; /* 4.5d */
	Ir = (Ir_g + Ir_r * Rgts * T11) / (1.0 + Rgts * T11);                    /* 5.0 */
        /* ACCOUNT FOR BODY GENERATION CURRENT (ONLY WHEN TAUO IS SPECIFIED)        4.5 */
        T3 = -Vdss;                                                              /* 4.5 */
        if (T3 >= pTempModel->Vexpl4)                                            /* 4.5 */
	{   Igt += pTempModel->Ioffset4 + pTempModel->Gexpl4 * T3;               /* 4.5 */
	    Gigtds -= pTempModel->Gexpl4;                                        /* 4.5d */
        }                                                                        /* 4.5 */
        else                                                                     /* 4.5 */
        {   T13 = exp(T3 / Vtm);                                                 /* 4.5 */
	    Igt += Igtb0 * (T13 - 1.0);                                          /* 4.5 */
	    Gigtds -= Igtb0 * T13 / Vtm;                                         /* 4.5d */
        }                                                                        /* 4.5 */
    }                                                                            /* 4.5 */

    /*     REVERSE BIAS (TRAP-ASSISTED) TUNNELING CURRENT                           4.5 */
    if(pModel->Ntr > 0.0)
    { toc = 1.0e-12;
      if((Ldd > 0.0) && (pModel->Thalo == 0.0))
	{Egbgn = Eg - DelEgGe - 0.0225 * sqrt(pModel->Nldd / 1.0e24);}           /* 7.5W */
      else
	{Egbgn = Eg - DelEgGe - 0.0225 * sqrt(pModel->Nds  / 1.0e24);}           /* 7.5W */
      Btrev = 1.27e9 * pow((Egbgn / (Eg - DelEgGe)),1.5);                        /* 7.5W */
      Eo = -2.0 * Vbih/ D0h;                                                     /* 7.5W */
      E_dbm = -2.0 * (Vbih - Vbdeffh)/Ddh;                                       /* 7.5W */
      E_sbm = -2.0 * (Vbih - Vbseffh)/Dsh;                                       /* 7.5W */
      T0 = 5.0 * pModel->Teff * pModel->Ntr * Esige /                            /* 7.5W */
	 (pModel->Nbheff * toc * Btrev);	                                 /* 4.5d */
      T1 = Pdj * T0;	                                                         /* 4.5d */
      T2 = Eo * Eo * exp(Btrev / Eo);                                            /* 4.5d */
      T3 = T1 * T2;                                                              /* 4.5d */
      Itund = T1 * E_dbm * E_dbm * exp(Btrev / E_dbm) - T3;                      /* 4.5d */
      Gitundds = (Itund + T3) * (Btrev / E_dbm - 2.0) / (E_dbm * Ddh);           /* 4.5d */
      T1 = Psj * T0;	                                                         /* 4.5d */
      Ituns = T1 * E_sbm * E_sbm * exp(Btrev / E_sbm) - T1 * T2;                 /* 5.0 */
    }
    else
    { Ituns = 0.0;
      Itund = 0.0;
      Gitundds = 0.0;                                                            /* 4.5d */
    }
    Igt -= Itund;
    Ir -= Ituns;
    Gigtds -= Gitundds;                                                          /* 4.5d */

	/* 4.41 : Add GIDL */
      if(pModel->Bgidl >0.0)
      { T0 =pModel->Type*(pModel->Nqff + 2.0*pModel->Nqfsw * pModel->Tb / Weff); /* 4.5r */
	T4 = exp(-50.0 * (Vbs - Vdss));                                          /* 6.0 */
/*	Vbdo = log(2.0) / 50.0;                   */                               /* 6.0 */
/*	Vbdx = Vbdo - log(1.0 + T4) / 50.0;       */                               /* 6.0 */
        Vbdx = -log(1.0 + T4) / 50.0;                                            /* 6.0 */
        Wkfgd = (1.0 - pModel->Tpg) * (Eg - DelEgGe) / 2.0;                      /* 7.5W */
        Vfbgd = Wkfgd - ELECTRON_CHARGE * T0 / Coxf;                             /* 6.0 */
        Vgdt= Vfbgd - (Eg - DelEgGe) + Vbdx;                                      /* 7.5W */
	T1 = exp(-0.4 * (Vgfs - Vdss - Vgdt));                                   /* 6.0 */
        Vgdx = Vgdt - log(1.0 + T1) / 0.4;                                       /* 6.0 */
/*      Esd = (Vgdx - Vfbgd + (Eg - DelEgGe) - Vbdx) * Coxf / OXIDE_PERMITTIVITY / 3.0;    */
        Esd = (Vgdx - Vfbgd + (Eg - DelEgGe) - Vbdx) * Coxf / Esige;             /* 7.5W */
        Rmass = 1.82e-31;
        h = 6.626e-34;
	T2 = -0.375 * Weff * pModel->Dl * ELECTRON_CHARGE * 
             ELECTRON_CHARGE * sqrt(Rmass * (Eg - DelEgGe)  * ELECTRON_CHARGE /
             2.0) / h / h * Esd * exp(pModel->Bgidl / Esd);                      /* 7.5W */
	Igidl = - T2 * 8.0 * Vbdx / (3.0 * (Eg - DelEgGe) * 3.1416);             /* 7.5W */
	Gigidlgfs = Igidl * (1.0 - pModel->Bgidl / Esd) * T1
                  / ((Vgdx - Vfbgd + (Eg - DelEgGe) - Vbdx) * (1.0 + T1));       /* 7.5W */
	DVbdds = T4 / (1.0 + T4);                                                /* 6.0 */
	Gigidlds = - Igidl * DVbdds / Vbdx
                 + (Igidl / Esd + Igidl * pModel->Bgidl / (Esd * Esd))
         	 * (DVbdds - T1 * (1.0 - DVbdds) / (1.0 + T1) - DVbdds)
		 / (3.0 * Toxf);                                                 /* 6.0 */
      }
      else
      { Igidl = 0.0;
	Gigidlgfs = 0.0;                                                         /* 4.5d */
	Gigidlds = 0.0;                                                          /* 4.5d */
      }

	/* 6.0 : Add GISL */    
      if(pModel->Bgidl >0.0)                                                     /* 6.0 */
      { T4 = exp(-50.0 * (Vbs));                                                 /* 6.0 */
/*	Vbsx = Vbdo - log(1.0 + T4) / 50.0;       */                               /* 6.0 */
        Vbsx = -log(1.0 + T4) / 50.0;                                            /* 6.0 */
        Vgst= Vfbgd - (Eg - DelEgGe) + Vbsx;                                     /* 7.5W */
	T1 = exp(-0.4 * (Vgfs - Vgst));                                          /* 6.0 */
        Vgsx = Vgst - log(1.0 + T1) / 0.4;                                       /* 6.0 */
/*      Ess = (Vgsx - Vfbgd + (Eg - DelEgGe) - Vbsx) * Coxf / OXIDE_PERMITTIVITY / 3.0;       */
        Ess = (Vgsx - Vfbgd + (Eg - DelEgGe) - Vbsx) * Coxf / Esige;             /* 7.5W */
	T2 = -0.375 * Weff * pModel->Dl * ELECTRON_CHARGE * 
             ELECTRON_CHARGE * sqrt(Rmass * (Eg - DelEgGe) * ELECTRON_CHARGE /
             2.0) / h / h * Ess * exp(pModel->Bgidl / Ess);                      /* 7.5W */
	Igisl = - T2 * 8.0 * Vbsx / (3.0 * (Eg - DelEgGe) * 3.1416);             /* 7.5W */
      }                                                                          /* 6.0 */
      else                                                                       /* 6.0 */
      { Igisl = 0.0;                                                             /* 6.0 */
      }                                                                          /* 6.0 */
      Ir -= Igisl;                                                               /* 6.0 */

        /* Gate-body tunneling current */                                        /* 7.0Y */
   if (pModel->Mox == 0.0)
   {
       Igb = 0.0;
       Gigbgfs = 0.0;
       Gigbds = 0.0;
   }
   else 
   { 
       if (pModel->Type == PMOS)
       {
           Efc = Vtm * ELECTRON_CHARGE * (log(pModel->Ngate / Nv) +
                 (pModel->Ngate / Nv) / pow((64.0 + 3.6 * pModel->Ngate / Nv), 0.25));
       }
       else
       {
           Efc = Vtm * ELECTRON_CHARGE * (log(pModel->Ngate / Nc)+
                 (pModel->Ngate / Nc) / pow((64.0 + 3.6 * pModel->Ngate / Nc), 0.25));
       }
       Meffv = 0.65 * ELECTRON_REST_MASS;
       Meffc = 0.19 * ELECTRON_REST_MASS;
       Barr_Vbe = BARRIER_CBE + ELECTRON_CHARGE * (Eg - DelEgGe);                /* 7.5W */
       Btun = 4.0 * sqrt(2.0 * pModel->Mox * ELECTRON_REST_MASS)
              / (3.0 * ELECTRON_CHARGE * REDUCED_PLANCK);
       if (pModel->Svbe == 0.0)
       {
           Igb1 = 0.0;
       }
       else
       { 
           /* Valence Band Electron Tunneling */
           Num_div = 10;
           dDist_y = Leff / Num_div * (1.0 - 1.0e-6);
           Jv_sum = 0.0;
           for (I = 0; I <= Num_div; I++)
           {
               Dist_y = dDist_y * I;
               /* Surface potential variation along y-direction */
               Pot_y = Vdsat - sqrt(pow(Vdsat, 2) - 2.0 * Dist_y / Leff * Vdsat * Vdsx
                      + Dist_y / Leff * pow(Vdsx, 2));
               Psi_y = Psis + Pot_y;
               /* Polysilicon surface potential due to poly depletion */
               VarP1 = pow(Coxf, 2) * (Vgfs - Vfbf - Psi_y);
               VarP2 = ELECTRON_CHARGE * pModel->Ngate * SILICON_PERMITTIVITY;
               VarP3 = VarP2 + 2.0 * VarP1;
               Psigf_y = (VarP2 + VarP1 - sqrt(VarP2 * fabs(VarP3))) / pow(Coxf, 2);
               /* Oxide potential along channel */
               Psi_ox = Vgfs - Vfbf - Psi_y - Psigf_y;
               Ediff = Vgfs - Psi_y - Psigf_y + pTempModel->Phib / 2.0 
                       - (Eg - DelEgGe) / 2.0 + Efc / ELECTRON_CHARGE;          /* 7.5W */
               Arg_vbe = Ediff / Vtm;
               if (Arg_vbe > 240.0)
               {   Ediff_q = Ediff;
               }
               else if (Arg_vbe < -30.0)
               {   Ediff_q = 0.0;
               }
               else
               {   Ediff_q = log(1 + exp(Ediff * pModel->Svbe)) / pModel->Svbe;
               }
               Ediff_y = Ediff_q * ELECTRON_CHARGE;
               Meff = Meffv;
               Atun = 6.911e81 * Meff;
               Fox = Psi_ox / Toxf;
               Ktun1 = 1 / (sqrt(Barr_Vbe) - sqrt(Barr_Vbe - ELECTRON_CHARGE * Psi_ox));
               Ktun2 = 1 / (sqrt(Barr_Vbe + Ediff_y)
                        - sqrt(Barr_Vbe + Ediff_y - ELECTRON_CHARGE * Psi_ox));
               Jtun1 = Ktun1 * exp(-Btun / Fox * (pow(Barr_Vbe, 1.5)
                       - pow((Barr_Vbe - ELECTRON_CHARGE * Psi_ox), 1.5)));
               Jtun2 = Ktun2 * exp(-Btun / Fox * (pow((Barr_Vbe + Ediff_y), 1.5)
                       - pow((Barr_Vbe + Ediff_y - ELECTRON_CHARGE * Psi_ox), 1.5)));
               Jtun = Atun / (3.0 * Btun) * Fox * Ediff_y * (Jtun1 - Jtun2);
               /* Trapezoidal Integration */
               if ((I == 0) || (I == Num_div))
               {
                   Jv_sum = Jv_sum + Jtun;
               }
               else
               {
                   Jv_sum = Jv_sum + 2.0 * Jtun;
               }
           }
           Igb1 = Weff * dDist_y / 2.0 * Jv_sum;
       }
       if ((pModel->Scbe == 0.0) || (Region < 3))
       {
           Igb2 = 0.0;
       }
       else
       { 
          /* Conduction Band Electron Tunneling */
          if (pModel->Type == PMOS)
          {
              Barr = Barr_Vbe;
              Sfb = 15.0;
              Meff = Meffv;
          }
          else
          {
              Barr = BARRIER_CBE;
              Sfb = 10.0;
              Meff = Meffc;
          }
          Dell_t = Dell * 0.1;
          Le = Leff - 2.0 * Dell_t;
          if (Le < LEMIN) Le = LEMIN;
          WLe = Weff * Le;
          Psibb = log(1 + exp(10.0 * (Psiwk - Vbs))) / 10.0;
          Psi_oxwk = Vgfs - pTempModel->Wkf + Efc / ELECTRON_CHARGE - Vbs - Psibb;
          Atun = 6.911e81 * Meff;
          Num_div = 50;
          BarrLim = 0.3 * ELECTRON_CHARGE;
          E_div = BarrLim / Num_div;
          J1Sum = 0.0;
          Psi_oxwkb = log(1 + exp(Sfb * Psi_oxwk)) / Sfb;
          Eox = ELECTRON_CHARGE * Psi_oxwk;
          Eoxb = ELECTRON_CHARGE * Psi_oxwkb;
          for (I = 0; I <= Num_div; I++)
          {
              Excs = E_div * I;
              Excg = Excs + Eoxb;
              Exfg = Excg - Efc;
              Exfs = Exfg - ELECTRON_CHARGE * (Vgfs - Vbs);
              Fdg = 1 + exp(- Exfg / Vtm / ELECTRON_CHARGE);
              Fds = 1 + exp(- Exfs / Vtm / ELECTRON_CHARGE);
              Fd = log(Fds / Fdg);
              if (Psi_oxwk == 0.0)
              {
                  Pt1 = sqrt(Barr - Excg); 
                  Pt = exp(- 1.5 * ELECTRON_CHARGE * Btun * Toxf * Pt1);
              }
              else
              {
                  Pt1 = pow((Barr - Excg + Eox), 1.5);
                  Pt2 = pow((Barr - Excg), 1.5); 
                  Pt = exp(- Btun * Toxf * (Pt1 - Pt2) / Psi_oxwk);
              }
              J1 = Pt * Fd;
              if ((I == Num_div) || (I == 0))
              {
                  J1Sum = J1Sum + J1;
              }
              else
              {
                  J1Sum = J1Sum + 2.0 * J1;
              }
          }
          Jcbe = Atun * Vtm * ELECTRON_CHARGE * E_div / 2.0 * J1Sum;
          Kcbe1 = Psibb - 0.5 * pModel->Ffact * pTempModel->Phib;
          Arg_cbe = Kcbe1 / Vtm;
          if (Arg_cbe > 20.0)
          { 
              Kcbe = 0.0;
          }
          else if (Arg_cbe < -20.0)
          { 
              Kcbe = 1.0;
          }
          else
          {
              Kcbe = 1.0 / (exp(Kcbe1 / pModel->Scbe) + 1.0);
          }
          Igb2 =  WLe * Kcbe * Jcbe;
       }
       Igb = Igb1 + Igb2;

       /*  Calculation of Gigbds  */                                              /* 7.0Y */
       Vdsx_gb = Vdsx + 1.0e-4;
       if (pModel->Svbe == 0.0)
       {
           Igb1 = 0.0;
       }
       else
       { 
           /* Valence Band Electron Tunneling */
           Num_div = 10;
           dDist_y = Leff / Num_div * (1.0 - 1.0e-6);
           Jv_sum = 0.0;
           for (I = 0; I <= Num_div; I++)
           {
               Dist_y = dDist_y * I;
               /* Surface potential variation along y-direction */
               Pot_y = Vdsat - sqrt(pow(Vdsat, 2) - 2.0 * Dist_y / Leff * Vdsat * Vdsx_gb
                      + Dist_y / Leff * pow(Vdsx_gb, 2));
               Psi_y = Psis + Pot_y;
               /* Polysilicon surface potential due to poly depletion */
               VarP1 = pow(Coxf, 2) * (Vgfs - Vfbf - Psi_y);
               VarP2 = ELECTRON_CHARGE * pModel->Ngate * SILICON_PERMITTIVITY;
               VarP3 = VarP2 + 2.0 * VarP1;
               Psigf_y = (VarP2 + VarP1 - sqrt(VarP2 * fabs(VarP3))) / pow(Coxf, 2);
               /* Oxide potential along channel */
               Psi_ox = Vgfs - Vfbf - Psi_y - Psigf_y;
               Ediff = Vgfs - Psi_y - Psigf_y + pTempModel->Phib / 2.0
                       - (Eg - DelEgGe) / 2.0 + Efc / ELECTRON_CHARGE;           /* 7.5W */
               Arg_vbe = Ediff / Vtm;
               if (Arg_vbe > 240.0)
               {   Ediff_q = Ediff;
               }
               else if (Arg_vbe < -30.0)
               {   Ediff_q = 0.0;
               }
               else
               {   Ediff_q = log(1 + exp(Ediff * pModel->Svbe)) / pModel->Svbe;
               }
               Ediff_y = Ediff_q * ELECTRON_CHARGE;
               Meff = Meffv;
               Atun = 6.911e81 * Meff;
               Fox = Psi_ox / Toxf;
               Ktun1 = 1 / (sqrt(Barr_Vbe) - sqrt(Barr_Vbe - ELECTRON_CHARGE * Psi_ox));
               Ktun2 = 1 / (sqrt(Barr_Vbe + Ediff_y)
                        - sqrt(Barr_Vbe + Ediff_y - ELECTRON_CHARGE * Psi_ox));
               Jtun1 = Ktun1 * exp(-Btun / Fox * (pow(Barr_Vbe, 1.5)
                       - pow((Barr_Vbe - ELECTRON_CHARGE * Psi_ox), 1.5)));
               Jtun2 = Ktun2 * exp(-Btun / Fox * (pow((Barr_Vbe + Ediff_y), 1.5)
                       - pow((Barr_Vbe + Ediff_y - ELECTRON_CHARGE * Psi_ox), 1.5)));
               Jtun = Atun / (3.0 * Btun) * Fox * Ediff_y * (Jtun1 - Jtun2);
               /* Trapezoidal Integration */
               if ((I == 0) || (I == Num_div))
               {
                   Jv_sum = Jv_sum + Jtun;
               }
               else
               {
                   Jv_sum = Jv_sum + 2.0 * Jtun;
               }
           }
           Igb1 = Weff * dDist_y / 2.0 * Jv_sum;
       }
       if ((pModel->Scbe == 0.0) || (Region < 3))
       {
           Igb2 = 0.0;
       }
       else
       {
           /* Conduction Band Electron Tunneling */
           if (pModel->Type == PMOS)
           {
               Barr = Barr_Vbe;
               Sfb = 15.0;
               Meff = Meffv;
           }
           else
           {
               Barr = BARRIER_CBE;
               Sfb = 10.0;
               Meff = Meffc;
           }
           Dell_t = Dell * 0.1;
           Le = Leff - 2.0 * Dell_t;
           if (Le < LEMIN) Le = LEMIN;
           WLe = Weff * Le;
           Psibb = log(1 + exp(10.0 * (Psiwk - Vbs))) / 10.0;
           Psi_oxwk = Vgfs - pTempModel->Wkf + Efc / ELECTRON_CHARGE - Vbs - Psibb;
           Atun = 6.911e81 * Meff;
           Num_div = 50;
           BarrLim = 0.3 * ELECTRON_CHARGE;
           E_div = BarrLim / Num_div;
           J1Sum = 0.0;
           Psi_oxwkb = log(1 + exp(Sfb * Psi_oxwk)) / Sfb;
           Eox = ELECTRON_CHARGE * Psi_oxwk;
           Eoxb = ELECTRON_CHARGE * Psi_oxwkb;
           for (I = 0; I <= Num_div; I++)
           {
               Excs = E_div * I;
               Excg = Excs + Eoxb;
               Exfg = Excg - Efc;
               Exfs = Exfg - ELECTRON_CHARGE * (Vgfs - Vbs);
               Fdg = 1 + exp(- Exfg / Vtm / ELECTRON_CHARGE);
               Fds = 1 + exp(- Exfs / Vtm / ELECTRON_CHARGE);
               Fd = log(Fds / Fdg);
               if (Psi_oxwk == 0.0)
               {
                   Pt1 = sqrt(Barr - Excg);
                   Pt = exp(- 1.5 * ELECTRON_CHARGE * Btun * Toxf * Pt1);
               }
               else
               {
                   Pt1 = pow((Barr - Excg + Eox), 1.5);
                   Pt2 = pow((Barr - Excg), 1.5);
                   Pt = exp(- Btun * Toxf * (Pt1 - Pt2) / Psi_oxwk);
               }
               J1 = Pt * Fd;
               if ((I == Num_div) || (I == 0))
               {
                   J1Sum = J1Sum + J1;
               }
               else
               {
                   J1Sum = J1Sum + 2.0 * J1;
               }
           }
           Jcbe = Atun * Vtm * ELECTRON_CHARGE * E_div / 2.0 * J1Sum;
           Kcbe1 = Psibb - 0.5 * pModel->Ffact * pTempModel->Phib;
           Arg_cbe = Kcbe1 / Vtm;
           if (Arg_cbe > 20.0)
           {
               Kcbe = 0.0;
           }
           else if (Arg_cbe < -20.0)
           { 
               Kcbe = 1.0;
           }
           else
           {
               Kcbe = 1.0 / (exp(Kcbe1 / pModel->Scbe) + 1.0);
           }
           Igb2 =  WLe * Kcbe * Jcbe;
       }
       Igb_ds = Igb1 + Igb2;
       Gigbds=(Igb_ds-Igb)/1.0e-4;

       /*  Calculation of Gigbgfs  */                                             /* 7.0Y */
       Vgfsx_gb = Vgfs + 1.0e-4;
       if (pModel->Svbe == 0.0)
       {
           Igb1 = 0.0;
       }
       else
       { 
           /* Valence Band Electron Tunneling */
           Num_div = 10;
           dDist_y = Leff / Num_div * (1.0 - 1.0e-6);
           Jv_sum = 0.0;
           for (I = 0; I <= Num_div; I++)
           {
               Dist_y = dDist_y * I;
               /* Surface potential variation along y-direction */
               Pot_y = Vdsat - sqrt(pow(Vdsat, 2) - 2.0 * Dist_y / Leff * Vdsat * Vdsx
                      + Dist_y / Leff * pow(Vdsx, 2));
               Psi_y = Psis + Pot_y;
               /* Polysilicon surface potential due to poly depletion */
               VarP1 = pow(Coxf, 2) * (Vgfsx_gb - Vfbf - Psi_y);
               VarP2 = ELECTRON_CHARGE * pModel->Ngate * SILICON_PERMITTIVITY;
               VarP3 = VarP2 + 2.0 * VarP1;
               Psigf_y = (VarP2 + VarP1 - sqrt(VarP2 * fabs(VarP3))) / pow(Coxf, 2);
               /* Oxide potential along channel */
               Psi_ox = Vgfsx_gb - Vfbf - Psi_y - Psigf_y;
               Ediff = Vgfsx_gb - Psi_y - Psigf_y + pTempModel->Phib / 2.0
                       - (Eg - DelEgGe) / 2.0 + Efc / ELECTRON_CHARGE;            /* 7.5W */
               Arg_vbe = Ediff / Vtm;
               if (Arg_vbe > 240.0)
               {   Ediff_q = Ediff;
               }
               else if (Arg_vbe < -30.0)
               {   Ediff_q = 0.0;
               }
               else
               {   Ediff_q = log(1 + exp(Ediff * pModel->Svbe)) / pModel->Svbe;
               }
               Ediff_y = Ediff_q * ELECTRON_CHARGE;
               Meff = Meffv;
               Atun = 6.911e81 * Meff;
               Fox = Psi_ox / Toxf;
               Ktun1 = 1 / (sqrt(Barr_Vbe) - sqrt(Barr_Vbe - ELECTRON_CHARGE * Psi_ox));
               Ktun2 = 1 / (sqrt(Barr_Vbe + Ediff_y)
                        - sqrt(Barr_Vbe + Ediff_y - ELECTRON_CHARGE * Psi_ox));
               Jtun1 = Ktun1 * exp(-Btun / Fox * (pow(Barr_Vbe, 1.5)
                       - pow((Barr_Vbe - ELECTRON_CHARGE * Psi_ox), 1.5)));
               Jtun2 = Ktun2 * exp(-Btun / Fox * (pow((Barr_Vbe + Ediff_y), 1.5)
                       - pow((Barr_Vbe + Ediff_y - ELECTRON_CHARGE * Psi_ox), 1.5)));
               Jtun = Atun / (3.0 * Btun) * Fox * Ediff_y * (Jtun1 - Jtun2);
               /* Trapezoidal Integration */
               if ((I == 0) || (I == Num_div))
               {
                   Jv_sum = Jv_sum + Jtun;
               }
               else
               {
                   Jv_sum = Jv_sum + 2.0 * Jtun;
               }
           }
           Igb1 = Weff * dDist_y / 2.0 * Jv_sum;
       }
       if ((pModel->Scbe == 0.0) || (Region < 3))
       {
          Igb2 = 0.0;
       }
       else
       {
           /* Conduction Band Electron Tunneling */
           if (pModel->Type == PMOS)
           {
               Barr = Barr_Vbe;
               Sfb = 15.0;
               Meff = Meffv;
           }
           else
           {
               Barr = BARRIER_CBE;
               Sfb = 10.0;
               Meff = Meffc;
           }
           Dell_t = Dell * 0.1;
           Le = Leff - 2.0 * Dell_t;
           if (Le < LEMIN) Le = LEMIN;
           WLe = Weff * Le;
           Psibb = log(1 + exp(10.0 * (Psiwk - Vbs))) / 10.0;
           Psi_oxwk = Vgfsx_gb - pTempModel->Wkf + Efc / ELECTRON_CHARGE - Vbs - Psibb;
           Atun = 6.911e81 * Meff;
           Num_div = 50;
           BarrLim = 0.3 * ELECTRON_CHARGE;
           E_div = BarrLim / Num_div;
           J1Sum = 0.0;
           Psi_oxwkb = log(1 + exp(Sfb * Psi_oxwk)) / Sfb;
           Eox = ELECTRON_CHARGE * Psi_oxwk;
           Eoxb = ELECTRON_CHARGE * Psi_oxwkb;
           for (I = 0; I <= Num_div; I++)
           {
               Excs = E_div * I;
               Excg = Excs + Eoxb;
               Exfg = Excg - Efc;
               Exfs = Exfg - ELECTRON_CHARGE * (Vgfsx_gb - Vbs);
               Fdg = 1 + exp(- Exfg / Vtm / ELECTRON_CHARGE);
               Fds = 1 + exp(- Exfs / Vtm / ELECTRON_CHARGE);
               Fd = log(Fds / Fdg);
               if (Psi_oxwk == 0.0)
               {
                   Pt1 = sqrt(Barr - Excg);
                   Pt = exp(- 1.5 * ELECTRON_CHARGE * Btun * Toxf * Pt1);
               }
               else
               {
                   Pt1 = pow((Barr - Excg + Eox), 1.5);
                   Pt2 = pow((Barr - Excg), 1.5);
                   Pt = exp(- Btun * Toxf * (Pt1 - Pt2) / Psi_oxwk);
               }
               J1 = Pt * Fd;
               if ((I == Num_div) || (I == 0))
               {
                   J1Sum = J1Sum + J1;
               }
               else
               {
                   J1Sum = J1Sum + 2.0 * J1;
               }
           }
           Jcbe = Atun * Vtm * ELECTRON_CHARGE * E_div / 2.0 * J1Sum;
           Kcbe1 = Psibb - 0.5 * pModel->Ffact * pTempModel->Phib;
           Arg_cbe = Kcbe1 / Vtm;
           if (Arg_cbe > 20.0)
           {
               Kcbe = 0.0;
           }
           else if (Arg_cbe < -20.0)
           { 
               Kcbe = 1.0;
           }
           else
           {
               Kcbe = 1.0 / (exp(Kcbe1 / pModel->Scbe) + 1.0);
           }
           Igb2 =  WLe * Kcbe * Jcbe;
       }
       Igb_gfs = Igb1 + Igb2;
       Gigbgfs=(Igb_gfs-Igb)/1.0e-4;
   
       /*  Calculation of Gigbbs  */                                              /* 7.0Y */
   }

	/* Impact-ionization current */
    if ((pTempModel->Alpha <= 0.0) || (pTempModel->Beta <= 0.0)
        || (Vds == 0.0))							 /* 4.41 */
    {   Igi = 0.0;
	Gigigfs = 0.0;                                                           /* 4.5d */
	Gigids = 0.0;                                                            /* 4.5d */
	Mult = 0.0;                                                              /* 4.5d */
    }
    else
    {   if (Region > 1)
	{   Dell_f = Dell;
	    if (Dell_f < 0.5 * Leff)
	    {   Xecf = 4.0 * Vdss * Lcfac / Leff;
		T0 = Dell_f / pModel->Lc;
		T1 = exp(T0);
	    }
	    else
	    {   Dell_f = 0.5 * Leff;
		T0 = Dell_f / pModel->Lc;
		T1 = exp(T0);
		Cosh1 = 0.5 * (T1 + 1.0 / T1);
	        Xecf = Vdss / (pModel->Lc * (Cosh1 - 1.0));
	    }
	    Sinh1 = 0.5 * (T1 - 1.0 / T1);                                       /* 4.5 */
	    Xemf = Xecf * Sinh1;

	    /* Call IMPION IFLAG=0 */
	    Cst = 0.4 * ELECTRON_CHARGE / Boltz * 6.5e-8;
	    Betap = pTempModel->Beta * Cst;
	    T2 = Dell_f / pModel->Lc;
	    Te_ch = ELECTRON_CHARGE * Xecf / (5.0 * Boltz) * exp(T2)
	          / (1.0 / pModel->Lc + 1.0 / 6.5e-8);
	    if (Te_ch < 1.0e-20) Te_ch = 1.0e-20;
	    T1 = Betap / Te_ch;
	    if (T1 > 80.0) T1 = 80.0;                                            /* 4.5 */
	    Mult_ch = pTempModel->Alpha * pModel->Lc / Betap * Te_ch * exp(-T1);
	    dMult_chds = Mult_ch * (1.0 + T1) / Vdss;                            /* 4.5d */
	    Mult_dep = 0.0;
	    Mult_qnr = 0.0;
	    T0 = Itot * pTempModel->Rldd;
	    if ((Ldd <= 0.0) || (Xemf < T0) || (pModel->Nldd >= 1.0e25))         /* 4.5 */
	    {   Te_ldd = Te_ch;
	    }
	    else
	    {   T10 = Eslope * Ldd;
	        if ((Xemf - T0) <= T10)
		{   Ldep = (Xemf - T0) / Eslope;
		}
		else
		{   Ldep = Ldd;
		}
		Tempdell = Te_ch;
		Xstart = 0.0;
		Xend = Ldep;
		Te_dep = 0.0;                                                    /* 6.0 */
                Te_ldd = 0.0;                                                    /* 7.0 */
		Mult_dep = Xnonlocm(Xstart, Xend, pTempModel->Alpha, Betap,
		                    Ldep, Tempdell, Ldd, Cst, Xemf,
				    Eslope, pTempModel->Rldd, Itot, Te_ldd,
				    Te_dep);
		Te_dep = Xnonloct(Xend, Xend, Ldep, Tempdell, Ldd, Cst,
				      Xemf, Eslope, pTempModel->Rldd, Itot,
				      Te_ldd, Te_dep);
		if (Ldep != Ldd)
		{   Xstart = Ldep;
		    Xend = Ldd;
		    Mult_qnr = Xnonlocm(Xstart, Xend, pTempModel->Alpha, Betap,
		                    Ldep, Tempdell, Ldd, Cst, Xemf,
				    Eslope, pTempModel->Rldd, Itot, Te_ldd,
				    Te_dep);
		    Te_ldd = Xnonloct(Xend, Xend, Ldep, Tempdell, Ldd, Cst,
				      Xemf, Eslope, pTempModel->Rldd, Itot,
				      Te_ldd, Te_dep);
		}
                else                                                             /* 4.5 */
                {   Te_ldd = Te_dep;                                             /* 4.5 */
                }                                                                /* 4.5 */
	    }
	    T1 = Betap / Te_ldd;
	    if (T1 > 80.0) T1 = 80.0;                                            /* 4.5 */
	    Mult_drn = pTempModel->Alpha * 6.5e-8 / Betap * Te_ldd * exp(-T1);
	    dMult_drnds = 6.5e-8 * dMult_chds / pModel->Lc;                      /* 4.5d */
	    Mult = Mult_ch + Mult_dep + Mult_qnr + Mult_drn;
	    if (Mult <= 1.0e-50)                                                 /* 4.5d */
	    {  Mult = 1.0e-50;                                                   /* 4.5d */
	       dMultds = 0.0;                                                    /* 4.5d */
	    }                                                                    /* 4.5d */
	    else                                                                 /* 4.5d */
	    {  dMultds = dMult_chds + dMult_drnds;                               /* 4.5d */
	    }                                                                    /* 4.5d */
	    if(Region == 3)                                                      /* 4.5 */
	    { Igi = Mult * Itot;                                                 /* 4.5 */
	      Gigigfs = Mult * Gidgfs - Itot * dMultds;                          /* 4.5d */
	      Gigids = Mult * Gidds + Itot * dMultds;                            /* 4.5d */
	    }                                                                    /* 4.5 */
	    else                                                                 /* 4.5 */
	    { Mult_wk = Mult;                                                    /* 4.5 */
	      dMult_wk = dMultds;                                                /* 4.5d */
	    }                                                                    /* 4.5 */
	}
	if (Region < 3)
        { for (I = 0; I < ICONT; I++)                                            /* 4.5 */
	  { if (I == 0)                                                          /* 4.5 */
	    { Mobred = Mobred1;                                                  /* 4.5 */
	      Lefac = Lefac1;                                                    /* 4.5 */
	    }                                                                    /* 4.5 */
	    else                                                                 /* 4.5 */
	    { Mobred = Mobred2;                                                  /* 4.5 */
	      Lefac = Lefac2;                                                    /* 4.5 */
	    }                                                                    /* 4.5 */
            Dell_f = Leff * (1.0 - Lefac);                                       /* 4.5 */
	    T1 = (1.0 - Lefac) / Lcfac;                                          /* 4.5 */
	    T2 = exp(T1);
	    Sinh1 = 0.5 * (T2 - 1.0 / T2);
	    Cosh1 = 0.5 * (T2 + 1.0 / T2);
	    if (Lefac > 1.0e-3)                                                  /* 4.5 */
	    {   Xecf = 1.0 / (pTempModel->Fsat * Leff * Mobred);
	    }
	    else
	    {   Xecf = Vdss / (pModel->Lc * Sinh1);
	    }
	    Xemf = Xecf * Cosh1;

	    /* Call IMPION IFLAG=0 */
	    Cst = 0.4 * ELECTRON_CHARGE / Boltz * 6.5e-8;
	    Betap = pTempModel->Beta * Cst;
	    Te_ch = ELECTRON_CHARGE * Xecf / (5.0 * Boltz) * T2
	          / (1.0 / pModel->Lc + 1.0 / 6.5e-8);
	    if (Te_ch < 1.0e-20) Te_ch = 1.0e-20;
	    T1 = Betap / Te_ch;
	    if (T1 > 80.0) T1 = 80.0;                                            /* 4.5 */
	    Mult_ch = pTempModel->Alpha * pModel->Lc / Betap * Te_ch * exp(-T1);
	    T3 = Xemf / Xecf;                                                    /* 4.5d */
	    dMult_chds = Mult_ch * (1.0 + T1) * (1.0 + T3                        /* 4.5d */
		       / (sqrt(1.0 + T3 * T3))) / (T2 * pModel->Lc *Xecf);       /* 4.5d */
	    Mult_dep = 0.0;
	    Mult_qnr = 0.0;
	    T0 = Itot * pTempModel->Rldd;
	    if ((Ldd <= 0.0) || (Xemf < T0) || (pModel->Nldd >= 1.0e25))         /* 4.5 */
	    {   Te_ldd = Te_ch;
	    }
	    else
	    {   T10 = Eslope * Ldd;
	        if ((Xemf - T0) <= T10)
		{   Ldep = (Xemf - T0) / Eslope;
		}
		else
		{   Ldep = Ldd;
		}
		Tempdell = Te_ch;
		Xstart = 0.0;
		Xend = Ldep;
		Te_dep = 0.0;                                                    /* 6.0 */
                Te_ldd = 0.0;                                                    /* 7.0 */
		Mult_dep = Xnonlocm(Xstart, Xend, pTempModel->Alpha, Betap,
		                    Ldep, Tempdell, Ldd, Cst, Xemf,
				    Eslope, pTempModel->Rldd, Itot, Te_ldd,
				    Te_dep);
		Te_dep = Xnonloct(Xend, Xend, Ldep, Tempdell, Ldd, Cst, Xemf,
				  Eslope, pTempModel->Rldd, Itot, Te_ldd,
				  Te_dep);
		if (Ldep != Ldd)
		{   Xstart = Ldep;
		    Xend = Ldd;
		    Mult_qnr = Xnonlocm(Xstart, Xend, pTempModel->Alpha, Betap,
		                    Ldep, Tempdell, Ldd, Cst, Xemf,
				    Eslope, pTempModel->Rldd, Itot, Te_ldd,
				    Te_dep);
		    Te_ldd = Xnonloct(Xend, Xend, Ldep, Tempdell, Ldd, Cst,
				      Xemf, Eslope, pTempModel->Rldd, Itot,
				      Te_ldd, Te_dep);
		}
                else                                                             /* 4.5 */
                {   Te_ldd = Te_dep;                                             /* 4.5 */
                }                                                                /* 4.5 */
	    }
	    T1 = Betap / Te_ldd;
	    if (T1 > 80.0) T1 = 80.0;                                            /* 4.5 */
	    Mult_drn = pTempModel->Alpha * 6.5e-8 / Betap * Te_ldd * exp(-T1);
	    dMult_drnds = 6.5e-8 * dMult_chds / pModel->Lc;                      /* 4.5d */
	    Mult = Mult_ch + Mult_dep + Mult_qnr + Mult_drn;
	    if (Mult <= 1.0e-50)                                                 /* 4.5d */
	    {  Mult = 1.0e-50;                                                   /* 4.5d */
	       dMultds = 0.0;                                                    /* 4.5d */
	    }                                                                    /* 4.5d */
	    else                                                                 /* 4.5d */
	    {  dMultds = dMult_chds + dMult_drnds;                               /* 4.5d */
	    }                                                                    /* 4.5d */
	    if (Region == 1)                                                     /* 4.5 */
	    { Igi = Mult * Itot;                                                 /* 4.5 */
	      Gigigfs = Mult * Gidgfs - Itot * dMultds * (1.0 - dPsigfgfs);      /* 4.5d */
	      Gigids = Mult * Gidds + Itot * dMultds;                            /* 4.5d */
	    }                                                                    /* 4.5 */
	    else                                                                 /* 4.5 */
	    { if (I == 0)                                                        /* 4.5 */
	      { Mult_st1 = Mult;                                                 /* 4.5 */
		dMult_st1 = dMultds;                                             /* 4.5d */
	      }                                                                  /* 4.5 */
	      else                                                               /* 4.5 */
	      { Mult_st2 = Mult;                                                 /* 4.5 */
	      }                                                                  /* 4.5 */
	    }                                                                    /* 4.5 */
	  }                                                                      /* 4.5 */
	}
	if (Region == 2)                                                         /* 4.5 */
	{   T1 = log(Mult_st1);                                                  /* 4.5 */
	    T2 = log(Mult_st2);                                                  /* 4.5 */
	    T10 = Vths - Vthw;                                                   /* 4.5 */
	    T11 = log(Mult_wk);                                                  /* 4.5 */
	    Gst = (T2 - T1) / DVgfs;                                             /* 4.5F */
	    T0 = Vgfs - Vthw;                                                    /* 4.5 */
	    R2 = (3.0 * (T1 - T11) / T10 - Gst) / T10;                           /* 4.5 */
	    R3 = (2.0 * (T11 - T1) / T10 + Gst) / (T10 * T10);                   /* 4.5 */
	    Mult = exp(T11 + T0 * T0 * (R2 + T0 * R3));                          /* 4.5 */
	    Igi = Mult * Itot;                                                   /* 4.5 */
	    dMultds = ((Vths - Vgfs) * dMult_wk + T0 * dMult_st1) / T10;         /* 4.5d */
	    dPsigfgfs = T0 * dPsigfgfs1 / T10;                                   /* 4.5d */
	    Gigigfs = Mult * Gidgfs - Itot * dMultds * (1.0-dPsigfgfs);          /* 4.5d */
	    Gigids = Mult * Gidds + Itot * dMultds;                              /* 4.5d */
	}                                                                        /* 4.5 */
      }

	/* 4.41 : Both GIDL and impact-ionization currents accounted for in Igi */
	Igi = Igi + Igidl;
        Giigfs = Gigigfs + Gigidlgfs;                                            /* 4.5d */
        Giids = Gigids + Gigidlds;                                               /* 4.5d */

    if (pModel->Selft > 0)
    {   Power = Itot * Vds;
	T0 = Itot * Itot;
	if (pInst->DrainConductance > 0.0)
	{   Power += T0 / (pInst->DrainConductance
	          * pTempModel->GdsTempFac2);                                    /* 4.5 */
	}
	if (pInst->SourceConductance > 0.0)
	{   Power += T0 / (pInst->SourceConductance
	          * pTempModel->GdsTempFac2);                                    /* 4.5 */
	}
    }
    else
    {   Power = 0.0;
    }

    pOpInfo->Ich = Ich * pInst->MFinger;
    pOpInfo->Ibjt = Ibjt * pInst->MFinger;
    pOpInfo->Igt = Igt * pInst->MFinger + pEnv->Gmin * (Vbs - Vds);
    pOpInfo->Ir = Ir * pInst->MFinger + pEnv->Gmin * Vbs;
    pOpInfo->Igi = Igi * pInst->MFinger;
    pOpInfo->Igb = Igb * pInst->MFinger;                                      /* 7.0Y */
    pOpInfo->Power = Power * pInst->MFinger;
    pOpInfo->DrainConductance = pInst->DrainConductance
                              * pTempModel->GdsTempFac;
    pOpInfo->SourceConductance = pInst->SourceConductance
                               * pTempModel->GdsTempFac;
    pOpInfo->BodyConductance = pInst->BodyConductance
                             * pTempModel->GbTempFac;

    pOpInfo->Le = Le;
    pOpInfo->Vts = Vths;
    pOpInfo->Vtw = Vthw;
    pOpInfo->Vdsat = Vdseff;
    pOpInfo->Ueff = Ueff;

    pOpInfo->dId_dVgf = Gidgfs * pInst->MFinger;                                 /* 4.5d */
    pOpInfo->dId_dVd = Gidds * pInst->MFinger;                                   /* 4.5d */
    pOpInfo->dId_dVgb = 0.0;                                                     /* 4.5d */

    pOpInfo->dIgi_dVgf = Giigfs * pInst->MFinger;                                 
    pOpInfo->dIgi_dVd = Giids * pInst->MFinger;                                   
    pOpInfo->dIgi_dVgb = 0.0;                                                    /* 4.5d */

    pOpInfo->dIgb_dVgf = Gigbgfs * pInst->MFinger;                            /* 7.0Y */
    pOpInfo->dIgb_dVd = Gigbds * pInst->MFinger;                              /* 7.0Y */
    pOpInfo->dIgb_dVgb = 0.0;                                                  /* 7.0Y */

    pOpInfo->dIgt_dVgf = 0.0;                                                    /* 4.5d */
    pOpInfo->dIgt_dVd = Gigtds * pInst->MFinger;                                 /* 4.5d */
    pOpInfo->dIgt_dVgb = 0.0;                                                    /* 4.5d */


    if (pInst->pDebug)
    {   pDebug = pInst->pDebug;
    }
    if (DynamicNeeded)
    {   WL = Weff * Leff;
        CoxWL = Coxf * WL;
	CobWL = WL * Coxb;                                                       /* 4.5d */
        if (Region > 1)
        {   Dell = Dell * 0.1;
	    Le = Leff - 2.0 * Dell;
/*	      if (Le < pTempModel->Lemin) Le = pTempModel->Lemin;                   4.5 */
            if (Le < LEMIN) Le = LEMIN;                                          /* 6.1 */
	    WLe = Weff * Le;
	    CoxWLe = Coxf * WLe;
	    Qd = 0.0;                                                            /* 4.5 */
            Qs = Qd;                                                             /* 4.5 */
	    if(Region == 2)                                                      /* 4.5F */
	    { Qgfwk1 = CoxWLe * (Vthw - pTempModel->Wkf - Psiwk1);               /* 4.5F */
	      Qgfwk2 = CoxWLe * (Vthw - DVgfs - pTempModel->Wkf - Psiwk2);       /* 4.5F */
	      Cgfgfswk1 =  CoxWLe * (1.0 - 1.0 / pModel->Xalpha);                /* 4.5d */
 	      Cgfdswk1 = - WLe * Dicefac / pModel->Xalpha;                       /* 4.5d */
	    }                                                                    /* 4.5F */
	    else                                                                 /* 4.5F */
	    { Qgf = CoxWLe * (Vgfs - pTempModel->Wkf - Psiwk);                   /* 4.5F */
	      Cgfgfs = CoxWLe * (1.0 - 1.0 / pModel->Xalpha);                    /* 4.5d */
	      Cgfds = - WLe * Dicefac / pModel->Xalpha;                          /* 4.5d */
	      Cdgfs = 0.0;                                                       /* 4.5d */
	      Csgfs = 0.0;                                                       /* 4.5d */
	      Csds = 0.0;                                                        /* 4.5d */
	      Cdds = 0.0;                                                        /* 4.5d */
	    }                                                                    /* 4.5F */
	}
	if (Region < 3)
        {   if (Region == 2) Vgstart = Vths;
	    for (I = 0; I < ICONT; I++)
	    {    if (I == 0)
	         {   Mobref = Mobref1;
		     Vdsx = Vdsx1;						 /* 4.41 */
		     Ist = Ist1;
		     Mobred = Mobred1;
		     Xbmu = Xbmu1;
		     Qc = Qc1;
		     Qc0 = Qc01;                                                 /* 4.5 */
		     Lefac = Lefac1;						 /* 4.41 */
		     Psigf = Psigf1;					         /* 4.5pd */
		     Alphap = Alphap1;					         /* 4.5pd */
		     Psis = Psis1;					         /* 4.5qm */
		     dPsigfgfs = dPsigfgfs1;                                     /* 4.5d */
		     dVdsxds = dVdsxds1;                                         /* 4.5d */
		     dLefacds = dLefacds1;                                       /* 4.5d */
		 }
		 else
	         {   Mobref = Mobref2;
		     Vdsx = Vdsx2;						 /* 4.41 */
		     Ist = Ist2;
		     Mobred = Mobred2;
		     Xbmu = Xbmu2;
		     Qc = Qc2;
		     Qc0 = Qc02;                                                 /* 4.5 */
		     Lefac = Lefac2;						 /* 4.41 */
		     Psigf = Psigf2;					         /* 4.5pd */
		     Alphap = Alphap2;					         /* 4.5pd */
		     Psis = Psis2;					         /* 4.5qm */
		 }
	         Vgfs = Vgstart + I * DVgfs;			                 /* 4.5F */
		 Le = Leff * Lefac;
		 WLe = Weff * Le;
		 /* QGF defined independent of DICE */	                         /* 6.0 */
		 Dnmgf = 12.0 * (Qc0 / Coxf - 0.5 * Alphap * Vdsx);              /* 4.5d */
		 Args = 1.0 + pTempModel->Fsat * Mobref * Vdsx / Lefac;
		 T0 = Args * Vdsx * Vdsx * Alphap / Dnmgf;                       /* 4.5d */
		 T1 = Coxf * (Vgfs - Psigf - pTempModel->Wkf - Psis              /* 4.5d */
		    - 0.5 * Vdsx + T0);                                          /* 4.5d */
		 Qgf = WLe * T1;                                                 /* 4.5d */
		 T2 = (1.0 - Lefac) / Lcfac;                                     /* 4.5d */
		 T5 = exp(T2);                                                   /* 4.5d */
		 Cosh1 = 0.5 * (T5 + 1.0 / T5);                                  /* 4.5d */
		 T3 = pModel->Lc * Lcfac / (pTempModel->Fsat * Mobref);          /* 4.5d */
		 Xp = T3 * (Cosh1 - 1.0);                                        /* 4.5d */
		 T2 = Vgfs - Psigf - pTempModel->Wkf - Psis - Vdsx;              /* 4.5d */
		 Qgf += CoxWL * ((1.0 - Lefac) * T2 - Xp / Leff);
		 T4 = Coxf * Alphap * Vdsx;                                      /* 4.5d */
		 T12 = pModel->Tb / Leff;		 	 	 	 /* 6.0 */
		 Qdeta = WL * Cb * T12 * T12 * Vdss;				 /* 6.0 */
		 Qcfle = -Qc + T4;                                               /* 4.5d */
		 Qds = 0.5 * WL * Qcfle * (1.0 - Lefac * Lefac);
		 Qns = Qcfle * WL * (1.0 - Lefac);
/*         Remove charge-sharing model.                                             4.5 */
/*		 Qdqsh = -0.5 * WL * (pInst->Qb - Qbeff);                           4.5r */
/*4.5d		 Qdqsh = 0.0;                                                       4.5d */
/*4.5d		 if (Vdsx > 1.0e-15)                                                4.5d */
		 Argu = Qc / (Coxf * Alphap * (Vdsx + Dvtol));                   /* 4.5d */
		 if (pTempModel->Fsat > 0.0)
		 {   Argz = Argu - Ist * pTempModel->Fsat / (2.0
	                  * pTempModel->Beta0 * Alphap * (Vdsx + Dvtol)          /* 4.5pd */
	                  * Coxf * Coxf);
		 }
		 else
		 {   Argz = Argu;
		 }
		 Tzm1 = 2.0 * Argz - 1.0;
		 Xfactd = (-Argz + 2.0 / 3.0) / Tzm1;
		 Zfact = (1.5 * Argz - 4.0 * Argz * Argz / 3.0
	               - 0.4) / (Tzm1 * Tzm1);
		 Qn = -WLe * Qc * (Xfactd + Argu) / Argu + Qns;                  /* 4.5 */
		 Qd = -WLe * Qc * (Zfact + 0.5 * Argu) / Argu + Qds;             /* 4.5 */
		 Qs = Qn - Qd;                                                   /* 4.5 */
		 Qd += Qdeta;                                                    /* 6.0 */
/* 4.5d		 Qd += Qdqsh;                                                       4.5 */
/* 4.5d		 Qs += Qdqsh;                                                       4.5d */
/* 4.5d	       	 }                                                                  4.5d */
	         if (I == 0)
		 {   Qgfst1 = Qgf;
		     Qdst1 = Qd;
		     Qsst1 = Qs;
		     Qnst1 = Qn;                                                 /* 4.5 */
		     Cgfgfs = WLe * Coxf * (1.0 - dPsigfgfs);                    /* 4.5d */
/* note dell=Leff-Leff*Lefac=Leff(1.0-Lefac) */
		     Cgfds = Coxf * (WLe * (-0.5 + 2.0 * T0 / (Vdsx + Dvtol)
			      + 6.0 * Alphap * T0 / Dnmgf) - WL  
			      * (1.0 - Lefac)) * dVdsxds + (T1 - Coxf
			      * (T2 - 0.5 * T3 * (T5 - 1.0 / T5) / pModel->Lc))
		              * WL * dLefacds;                                   /* 4.5d */
		     T6 = Coxf * Alphap * dVdsxds - Dicefac;                     /* 4.5d */
		     T7 = Argu * (Dicefac / Qc - dVdsxds / (Vdsx + Dvtol));      /* 4.5d */
		     T8 = T7 + (Argz - Argu) * (Go1 / (Ist + 1.0e-30) - dVdsxds  /* 4.5d */
			/ (Vdsx + Dvtol));                                       /* 4.5d */
		     T9 = ((-8.0 / 3.0 * Argz + 1.5) / Tzm1 - 4.0 * Zfact)       /* 4.5d */
		        * T8 / Tzm1;                                             /* 4.5d */
		     T10 = (-1.0 - 2.0 * Xfactd) * T8 / Tzm1;                    /* 4.5d */
		     T11 = dLefacds / Lefac + Dicefac / Qc;                      /* 4.5d */
		     Cdds = -((Qd - Qds) * T7 + WLe * Qc * (T9 + 0.5 * T7))      /* 4.5d */
		            / Argu + (Qd - Qds) * T11 + Qds * T6 / Qcfle         /* 4.5d */
			    - WLe * Qcfle * dLefacds;                            /* 4.5d */
		     Cdds += WL * Cb * T12 * T12;	 	                 /* 6.0 */
		     Csds = (Qn - Qns) * (-T7 / Argu + T11 + (T10 + T7)          /* 4.5d */
		            / (Xfactd + Argu)) - Cdds + Qns * (T6 / Qcfle        /* 4.5d */
			    - dLefacds / (1.0 - Lefac + 1.0e-15 / Leff));        /* 4.5d */
		     Cdgfs = Coxf * ((Qd - Qds) / Qc - Qds / Qcfle)              /* 4.5d */
			     * (1.0 - dPsigfgfs);                                /* 4.5d */
		     Csgfs = Coxf * ((Qn - Qns) / Qc - Qns / Qcfle)              /* 4.5d */
			     * (1.0 - dPsigfgfs) - Cdgfs;                        /* 4.5d */
		 }
		 else
		 {   Qgfst2 = Qgf;
		     Qdst2 = Qd;
		     Qsst2 = Qs;
		     Qnst2 = Qn;                                                 /* 4.5 */
		 }
	    }
	    if (Region == 2)
	    {   /* Qgf */
		Vgfs = Vgfso;
	        Cst = (Qgfst2 - Qgfst1) / DVgfs;			         /* 4.5F */
		Cwk = (Qgfwk1 - Qgfwk2) / DVgfs;			         /* 4.5F */
		Xchk = Cst - Cwk;						 /* 4.5 */
		T0 = Vgfs - Vthw;						 /* 4.5d */
		T2 = Vths - Vgfs;						 /* 4.5d */
		T10 = Vths - Vthw;						 /* 4.5d */
		T3 = T0 / T10;                                  		 /* 4.5d */
		if (fabs(Xchk) <= 1.0e-50)					 /* 4.5 */
		{ Qgf = (T2 * Qgfwk1 + T0 * Qgfst1) / T10;                 	 /* 4.5d */
		}								 /* 4.5 */
		else								 /* 4.5 */
		{ Vp = (Vths * Cst - Vthw * Cwk - Qgfst1 + Qgfwk1) / Xchk;	 /* 4.5F */
		  Qp = Qgfwk1 + (Vp - Vthw) * Cwk;			         /* 4.5F */
		  X3a = Vthw - 2.0 * Vp + Vths;
		  X3b = Vthw - Vp;
		  X3c = -T0;
		  Xarg = X3b * X3b - X3a * X3c;
		  if (Xarg > 0.0)
		  {   Xarg = sqrt(Xarg);
		  }
		  else
		  {   Xarg = 0.0;
		  }
		  t = (X3b + Xarg) / X3a;
		  T1 = 1.0 - t;
		  Qgf = T1 * T1 * Qgfwk1 + 2.0 * t * T1 * Qp + t * t * Qgfst1;   /* 4.5F */
		}								 /* 4.5 */
		Cgfgfs = (T2 * Cgfgfswk1 + T0 * Cgfgfs) / T10;		         /* 4.5d */
		Cgfds = (T2 * Cgfdswk1 + T0 * Cgfds) / T10;		         /* 4.5d */
		/* Cwk = 0.0 Qdwk = 0.0 */                                       /* 4.5 */
	        Cst = (Qdst2 - Qdst1) / DVgfs;			                 /* 4.5F */
		Xchk = Cst;						         /* 4.5 */
		if (fabs(Xchk) <= 1.0e-50)					 /* 4.5 */
		{ Qd = T3 * Qdst1;					         /* 4.5d */
		}								 /* 4.5 */
		else								 /* 4.5 */
		{ Vp = (Vths * Cst - Qdst1) / Xchk;     	                 /* 4.5 */
		  X3a = Vthw - 2.0 * Vp + Vths;
		  X3b = Vthw - Vp;
		  X3c = -T0;							 /* 4.5d */
		  Xarg = X3b * X3b - X3a * X3c;
		  if (Xarg > 0.0)
		  {   Xarg = sqrt(Xarg);
		  }
		  else
		  {   Xarg = 0.0;
		  }
		  t = (X3b + Xarg) / X3a;
		  Qd = t * t * Qdst1;                   		         /* 4.5 */
		}								 /* 4.5 */
		Cdgfs = T3 * Cdgfs; 					         /* 4.5d */
		Cdds = T3 * Cdds; 					         /* 4.5d */
		/* Cwk = 0.0 Qswk = 0.0 */                                       /* 4.5 */
	        Cst = (Qsst2 - Qsst1) / DVgfs;   			         /* 4.5F */
		Xchk = Cst;						         /* 4.5 */
		if (fabs(Xchk) <= 1.0e-50)					 /* 4.5 */
		{ Qs = T3 * Qsst1;  					         /* 4.5d */
		}								 /* 4.5 */
		else								 /* 4.5 */
		{ Vp = (Vths * Cst - Qsst1) / Xchk;     	                 /* 4.5 */
		  X3a = Vthw - 2.0 * Vp + Vths;
		  X3b = Vthw - Vp;
		  X3c = -T0;		                                         /* 4.5d */
		  Xarg = X3b * X3b - X3a * X3c;
		  if (Xarg > 0.0)
		  {   Xarg = sqrt(Xarg);
		  }
		  else
		  {   Xarg = 0.0;
		  }
		  t = (X3b + Xarg) / X3a;
		  Qs = t * t * Qsst1;                   		         /* 4.5 */
		}								 /* 4.5 */
		Csgfs = T3 * Csgfs; 					         /* 4.5d */
		Csds = T3 * Csds; 					         /* 4.5d */
	        Cst = (Qnst2 - Qnst1) / DVgfs;   			         /* 4.5F */
	        Cwk = (Qnwk1 - Qnwk2) / DVgfs;                                   /* 4.5F */
		Xchk = Cst - Cwk;						 /* 4.5 */
		if (fabs(Xchk) <= 1.0e-50)					 /* 4.5 */
		{ Qn = (T2 * Qnwk1 + T0 * Qnst1) / T10;			         /* 4.5d */
		}								 /* 4.5 */
		else								 /* 4.5 */
		{ Vp = (Vths * Cst - Vthw * Cwk - Qnst1 + Qnwk1) / Xchk;         /* 4.5F */
	          Qp = Qnwk1 + (Vp - Vthw) * Cwk;                                /* 4.5F */
		  X3a = Vthw - 2.0 * Vp + Vths;                                  /* 4.5 */
		  X3b = Vthw - Vp;                                               /* 4.5 */
		  X3c = -T0;                                                     /* 4.5d */
		  Xarg = X3b * X3b - X3a * X3c;                                  /* 4.5 */
		  if (Xarg > 0.0)                                                /* 4.5 */
		  {   Xarg = sqrt(Xarg);                                         /* 4.5 */
		  }                                                              /* 4.5 */
		  else                                                           /* 4.5 */
		  {   Xarg = 0.0;                                                /* 4.5 */
		  }                                                              /* 4.5 */
		  t = (X3b + Xarg) / X3a;                                        /* 4.5 */
		  T1 = 1.0 - t;                                                  /* 4.5 */
		  Qn = T1 * T1 * Qnwk1 + 2.0 * t * T1 * Qp + t * t * Qnst1;      /* 4.5F */
		}								 /* 4.5 */
	    }
	}
        Qacc = CoxWL * (Vgfs - pTempModel->Wkf - Vbs);			         /* 4.5F */
	Qinv = Qgf;							         /* 4.5F */
	T0 = 15.0 / CoxWL;					                 /* 4.5F */
	T1 = T0 * (Qinv - Qacc);					         /* 4.5F */
	if (T1 > 80.0)					                         /* 4.5F */
	{   Qgf = Qacc;					                         /* 4.5F */
	    Cgfgfs = CoxWL;			                                 /* 4.5d */
	    Cgfds = 0.0;			                                 /* 4.5d */
	}								         /* 4.5F */
	else if (T1 < -80.0)					                 /* 4.5F */
	{   Qgf = Qinv;					                         /* 4.5F */
	}								         /* 4.5F */
	else								         /* 4.5F */
	{   T2 = exp(T1);			                                 /* 4.5d */
	    Qgf = Qinv - log(1.0 + T2) / T0;       	      		         /* 4.5d */
	    T3 = T2 / (1.0 + T2);		                                 /* 4.5d */
	    Cgfgfs = Cgfgfs - (Cgfgfs - CoxWL) * T3;                             /* 4.5d */
	    Cgfds = Cgfds * (1.0 - T3);	                                         /* 4.5d */
	}								         /* 4.5F */
	Qgb = CobWL * (Vgbs - pTempModel->Wkb - Vbs);                            /* 4.5d */

	T0 = pModel->Type * ELECTRON_CHARGE * WL;
        Qnqff = T0 * (pModel->Nqff + 2.0 * pModel->Nqfsw * pModel->Tb / Weff);   /* 4.5r */
	Qnqfb = T0 * pModel->Nqfb;                                               /* 4.5 */
	Qby = -(Qgf + Qgb + Qs + Qd + Qnqff + Qnqfb);                            /* 4.5 */
	Cbgfs = -(Cgfgfs + Csgfs + Cdgfs);                                       /* 4.5d */

	/* Substrate charge */
	T11 = pTempModel->Efs - pTempModel->Efd + DelEgGe;                       /* 7.5W */
	T0 = Vds - Vgbs - T11;                                                   /* 4.5d */
        T1 = T0 - ELECTRON_CHARGE * pModel->Type * pModel->Nqfb / Coxb;	         /* 4.5d */	
	T6 = - Vgbs - T11;                                                       /* 4.5d */
        T7 = T6 - ELECTRON_CHARGE*pModel->Type*pModel->Nqfb/Coxb;	         /* 4.5d */
        Dum2 = pModel->Dum1 * sqrt(Esige / ESI);                                 /* 7.5W */
	T8 = Dum2 * Dum2;                                                        /* 7.5W */
	T9 = Coxb * Ad;                                                          /* 4.5d */
	T10 = Coxb * As;                                                         /* 4.5d */
	if (pModel->Tps <= 0)
	{   if (T1 > 0.0)
	    {   T2 = sqrt(T8 + 4.0 * T1);                                        /* 4.5d */
	        T3 = pow((T2 - Dum2) * 0.5, 2.0);                                /* 7.5W */
	        Cgbds = T9 * ((T2 - Dum2) / T2 - 1.0);                           /* 7.5W */
	    }
	    else
	    {   T2 = Dum2;                                                       /* 7.5W */ 
		T3 = 0.0;                                                        /* 4.5d */
		Cgbds = -T9;                                                     /* 4.5d */
	    }
	    if (T7 > 0.0)                                                        /* 4.5d */
	    {   T4 = sqrt(T8 + 4.0 * T7);                                        /* 4.5d */
		T5 = pow((T4 - Dum2) * 0.5, 2.0);                                /* 7.5W */
	    }
	    else
	    {   T4 = Dum2;                                                       /* 7.5W */
		T5 = 0.0;                                                        /* 4.5d */
	    }
	}
	else
	{   if (T1 < 0.0)
	    {   T2 = sqrt(T8 - 4.0 * T1);                                        /* 4.5d */
	        T3 = -pow((T2 - Dum2) * 0.5, 2.0);                               /* 7.5W */
	        Cgbds = T9 * ((T2 - Dum2) / T2 - 1.0);                           /* 7.5W */
	    }
	    else
	    {   T2 = Dum2;                                                       /* 7.5W */
		T3 = 0.0;                                                        /* 4.5d */
		Cgbds = -T9;                                                     /* 4.5d */
	    }

	    if (T7 < 0.0)
	    {   T4 = sqrt(T8 - 4.0 * T7);                                        /* 4.5d */
		T5 = -pow((T4 - Dum2) * 0.5, 2.0);                               /* 7.5W */
	    }
	    else
	    {   T4 = Dum2;                                                       /* 7.5W */
		T5 = 0.0;                                                        /* 4.5d */
	    }
	}
	Qddep = T9 * (T0 - T3);                                                  /* 4.5d */
	Qsdep = T10 * (T6 - T5);                                                 /* 4.5d */
	Cdgbs = T9 * (-Dum2 / T2);                                               /* 7.5W */
	Csgbs = T10 * (-Dum2 / T4);                                              /* 7.5W */
	Qd += Qddep;
	Qs += Qsdep;
	Qgb -= (Qddep + Qsdep);
	Cgbgbs = (CobWL - Cdgbs - Csgbs);                                        /* 4.5d */
	Cbgbs = -CobWL;                                                          /* 4.5d */

	/* Diffusion capacitance */
	T10 = ELECTRON_CHARGE * XninGe2;                                         /* 7.5W */
	T11 = pModel->Ldiff * pModel->Tf / pTempModel->Ndseff;                   /* 5.0 */
	Qterms = T10 * Psj * (Argbjts - 1.0);                                    /* 5.0 */
	Qtermd = T10 * Pdj * (Argbjtd - 1.0);                                    /* 5.0 */
	Qns = Qterms * T11;                                                      /* 5.0 */
	Qnd = Qtermd * T11;                                                      /* 5.0 */
	if (pModel->Bjt == 0)                                                    /* 4.5 */
	{   Qpd = Qps = dQpdds = 0.0;                                            /* 5.0 */
	}
	else
	{   T3 = ELECTRON_CHARGE * XninGe * Xlbjt_t / Xaf;                       /* 7.5W */
	    T0 = pInst->Nblavg / XninGe * Sqrt_s / Exp_af;                       /* 7.5W */
	    Tf1 = Argbjts + T0;
	    Tf2 = Argbjts + T0 * Exp_af;
	    Qps = 0.5 * T3 * Psj * Sqrt_s * log(Tf1 * Exp_af / Tf2);             /* 4.5 */

	    T0 = pInst->Nblavg / XninGe * Sqrt_d / Exp_af;                       /* 7.5W */
	    Tr1 = Argbjtd + T0;
	    Tr2 = Argbjtd + T0 * Exp_af;
	    Qpd_t = 0.5 * T3 * Pdj * Sqrt_d * log(Tr1 * Exp_af / Tr2);           /* 5.0 */

	    T0 = Xlbjt_b * pModel->Teff;                                         /* 4.5 */
	    T1 = ELECTRON_CHARGE * Psj * pModel->Nbh;                            /* 4.5 */
	    T2 = 4.0 * Qterms * Psj * ELECTRON_CHARGE;			         /* 4.5 */
	    Xchk = T1 * T1 + T2;						 /* 4.41 */
	    if (Xchk < 0) Xchk = 0;						 /* 4.41 */
	    Qps += 0.25 * T0 * (-T1 + sqrt(Xchk));				 /* 4.41 */

	    T1 = ELECTRON_CHARGE * Pdj * pModel->Nbh;                            /* 4.5 */
	    T2 = 4.0 * Qtermd * Pdj * ELECTRON_CHARGE;			         /* 4.5 */
	    Xchk = T1 * T1 + T2;						 /* 4.41 */
	    if (Xchk <= 0)		   		      	   	      	 /* 5.0 */
	    {   Xchk = dQpd_bds = 0.0;   		      	   	      	 /* 5.0 */
	    }		   	   	   		      	   	      	 /* 5.0 */
            else	   	   	   		      	   	      	 /* 5.0 */
	    {   dQpd_bds = 0.5 * T0 * pow((Pdj*ELECTRON_CHARGE*XninGe),2.0)   	 /* 7.5W */
		         * dArgbjtdds / sqrt(Xchk);      	   	      	 /* 5.0 */
	    }   	   	   	   		      	   	      	 /* 5.0 */
	    Qpd_b = 0.25 * T0 * (-T1 + sqrt(Xchk));		      	      	 /* 5.0 */
	    Qpd = Qpd_t + Qpd_b;		 		       	 	 /* 5.0 */
	    dQpdds = Qpd_t * (dSqrt_dds / Sqrt_d - dDdds / Xlbjt_t)    	 	 /* 5.0 */
	           - Qpd_b * dDdhds / Xlbjt_b + dQpd_bds;	       	 	 /* 5.0 */
	}
	Qd -= (Qpd + Qnd);
	Qs -= (Qps + Qns);
	Qby += (Qpd + Qps + Qnd + Qns);	
	T1 = T10 * Pdj * T11 * Argbjtd * dVbd_bjtds / Vtm;	       	 	 /* 5.0 */

	/* Depletion capacitance */
	T0 = ELECTRON_CHARGE * pModel->Nbheff * pModel->Teff;                    /* 4.5 */
	Qddb = T0 * Ddh * Pdj;						         /* 4.5 */
	Qdsb = T0 * Dsh * Psj;						         /* 4.5 */
	Qd += Qddb;
	Qs += Qdsb;
	Qby -= (Qddb + Qdsb);
	T3 = -0.5 * Qddb * D0h * D0h * dVbdeffhds                                /* 7.5W */
	   / (Ddh * Ddh * Vbih);	 		                         /* 7.5W */
	Cdds += T3 - T1 - Cgbds - dQpdds;		       		         /* 5.0 */
	Cbds = -(Cgfds + Csds + Cdds + Cgbds); 		                         /* 4.5d */

	pOpInfo->Qgf = Qgf * pInst->MFinger;
	pOpInfo->Qd = Qd * pInst->MFinger;
	pOpInfo->Qs = Qs * pInst->MFinger;
	pOpInfo->Qb = Qby * pInst->MFinger;
	pOpInfo->Qgb = Qgb * pInst->MFinger;
	pOpInfo->Qn = Qn * pInst->MFinger;                                       /* 4.5 */

        pOpInfo->dQgf_dVgf = Cgfgfs * pInst->MFinger;                            /* 4.5d */
	pOpInfo->dQgf_dVd = Cgfds * pInst->MFinger;                              /* 4.5d */
	pOpInfo->dQgf_dVgb = 0.0;                                                /* 4.5d */

	pOpInfo->dQd_dVgf = Cdgfs * pInst->MFinger;                              /* 4.5d */
	pOpInfo->dQd_dVd = Cdds * pInst->MFinger;                                /* 4.5d */
	pOpInfo->dQd_dVgb = Cdgbs * pInst->MFinger;                              /* 4.5d */

	pOpInfo->dQb_dVgf = Cbgfs * pInst->MFinger;                              /* 4.5d */
	pOpInfo->dQb_dVd = Cbds * pInst->MFinger;                                /* 4.5d */
	pOpInfo->dQb_dVgb = Cbgbs * pInst->MFinger;                              /* 4.5d */

	pOpInfo->dQgb_dVgf = 0.0;                                                /* 4.5d */
	pOpInfo->dQgb_dVd = Cgbds * pInst->MFinger;                              /* 4.5d */
	pOpInfo->dQgb_dVgb = Cgbgbs * pInst->MFinger;                            /* 4.5d */

    }
    return 0;                                                                    /* 6.0 */
}

double
Xnonlocm(Xstart, Xend, Alpha, Betap, Ldep, Tempdell, Ldd, Cst, Xemf,
         Eslope, Rldd, Itot, Te_ldd, Te_dep)
double Xstart, Xend, Alpha, Betap, Ldep, Tempdell, Ldd, Cst, Xemf;
double Eslope, Rldd, Itot, Te_ldd, Te_dep;
{
double Hrom, T0, T1, T2, Sum, Xpos, Xtemp;
double R[31][31];
int L, La, Lb, Lc, M;

    Hrom = Xend - Xstart;
    T2 = Xnonloct(Xstart, Xend, Ldep, Tempdell, Ldd, Cst,
		      Xemf, Eslope, Rldd, Itot, Te_ldd, Te_dep);
    T0 = Betap / T2;
    if (T0 > 80.0) T0 = 80.0;                                                    /* 4.5 */
    T0 = exp(-T0);
    T2 = Xnonloct(Xend, Xend, Ldep, Tempdell, Ldd, Cst,
		      Xemf, Eslope, Rldd, Itot, Te_ldd, Te_dep);
    T1 = Betap / T2;
    if (T1 > 80.0) T1 = 80.0;                                                    /* 4.5 */
    T1 = exp(-T1);
    R[1][1] = 0.5 * Hrom * (T0 + T1);
    if (R[1][1] == 0.0)
    {   return 0.0; 
    }
    else
    {   L = 1;
	for (La = 2; La <= 7; La++)
	{    Hrom = 0.5 * Hrom;
	     L = L + L;
	     Sum = 0.0;
	     for (Lb = 1; Lb <= L - 1; Lb += 2)
	     {    Xpos = Xstart + Hrom * Lb;
                  Xtemp = Xnonloct(Xpos, Xend, Ldep, Tempdell, Ldd, Cst,
		      Xemf, Eslope, Rldd, Itot, Te_ldd, Te_dep);
		  T1 = Betap / Xtemp;
                  if (T1 > 80.0) T1 = 80.0;                                      /* 4.5 */
                  if (T1 < -10.0) T1 = -10.0;                                    /* 4.5d */
		  Sum = Sum + exp(-T1);
	     }
	     R[La][1] = 0.5 * R[La-1][1] + Hrom * Sum;
	     M = 1;
	     for (Lc = 2; Lc <= La; Lc++)
	     {    M = M * 4;
		  R[La][Lc] = R[La][Lc-1] + (R[La][Lc-1] - R[La-1][Lc-1])
			    / (M - 1.0);
	     }
	}
	T1 = Alpha * R[La-1][Lc-1];
	if (T1 < 0.0)
	    T1 = 0.0;
	return T1;
    }
}

double
Xnonloct(X, Xend, Ldep, Tempdell, Ldd, Cst, Xem, Eslope, Rldd, Itot, 
	 Te_ldd, Te_dep)
double X, Xend, Ldep, Tempdell, Ldd, Cst, Xem, Eslope, Rldd, Itot;
double Te_ldd, Te_dep;
{
double Y, Y1, T1, T2;

    if (Xend <= Ldep)
    {   Y = X / 6.5e-8;
        Y1 = exp(-Y);
        T1 = Tempdell * Y1;
	T2 = Cst * (Xem * (1.0 - Y1) - Eslope * 6.5e-8 * (Y - (1.0 - Y1)));    /* 4.5 */
	return (T1 + T2);
    }
    else
    {   Y = (X - Ldep) / 6.5e-8;
        T2 = exp(-Y);
        T1 = Te_dep * T2 + Cst * Itot * Rldd * (1.0 - T2);
        return T1;
    }
}

double                                                                         /* 5.0 */
XIntSich(Weff, Leff, T, Vdseff, Le, lc, Ich, Qc0, Ueffo)                       /* 5.0 */
double Weff, Leff, T, Vdseff, Le, lc, Ich, Qc0, Ueffo;                         /* 5.0 */
{                                                                              /* 5.0 */
double lambda, T1, T2, T3, T4, T5, T6, Eyo, Ey, Ec, dy, TcLe, dSich;           /* 5.0 */
double Tc, dSich_old, y, Sich;                                                 /* 5.0 */
int I, N;                                                                      /* 5.0 */
    lambda = 6.5e-8;                                                           /* 5.0 */
    T5 = (1.0 / lc + 1.0 / lambda);                                            /* 5.0 */
    T6 = (1.0 / lc - 1.0 / lambda);                                            /* 5.0 */
    Eyo = Ich / (Weff * Ueffo * Qc0);                                          /* 5.0 */
    Ec = 2.0 * Vdseff / Le - Eyo;                                              /* 5.0 */
    dy = 1.0e-9;                                                               /* 5.0 */
    N = Leff / dy + 0.1;                                                       /* 5.0 */
    for (I = 0; I <= N; I++)                                                   /* 5.0 */
    {   y = dy * I;                                                            /* 5.0 */
        if(y <= Le)                                                            /* 5.0 */
        { Ey = Eyo + (Ec - Eyo) / Le * y;                                      /* 5.0 */
          T1 = Le / lambda * Eyo / (Ec - Eyo);                                 /* 5.0 */
          T2 = exp(-y / lambda);                                               /* 5.0 */
          Tc = T + 2.0 * ELECTRON_CHARGE * (Ec - Eyo) * lambda * lambda /      /* 5.0 */
               (5.0 * Le * Boltz) * (y / lambda + (T2 - 1.0) * (1.0 - T1));    /* 5.0 */
          TcLe = Tc;                                                           /* 5.0 */
        }                                                                      /* 5.0 */
        else                                                                   /* 5.0 */
        { Ey = Ec * cosh((y - Le) / lc);                                       /* 5.0 */
          T2 = exp((y - Le) / lc);                                             /* 5.0 */
          T3 = 1.0 / T2;                                                       /* 5.0 */
          T4 = exp((y - Le) / lambda);                                         /* 5.0 */
          Tc = TcLe + ELECTRON_CHARGE * Ec / 5.0 / Boltz *                     /* 5.0 */
                      ((T2 - T4) / T5 + (T3 - T4) / T6);                       /* 5.0 */
        }                                                                      /* 5.0 */
        dSich = 4.0 * Weff * Boltz * Ueffo * Qc0 * Eyo /                       /* 5.0 */
                (Leff * Leff) * (Tc * dy / Ey);                                /* 5.0 */
        if(I == 1)                                                             /* 5.0 */
        {  Sich = 0.0;                                                         /* 5.0 */
	}                                                                      /* 5.0 */
	else                                                                   /* 5.0 */
        {  Sich = Sich + (dSich + dSich_old) / 2.0;                            /* 5.0 */
	}                                                                      /* 5.0 */
        dSich_old = dSich;                                                     /* 5.0 */
    }                                                                          /* 5.0 */
    Sich = fabs(Sich);                                                         /* 5.0 */
    return Sich;                                                               /* 5.0 */
}                                                                              /* 5.0 */

int 
ufsInitInstFlag( pInst )
struct ufsAPI_InstData  *pInst;
{   
    pInst->LengthGiven = 0;
    pInst->WidthGiven = 0;
    pInst->NrsGiven = 0;
    pInst->NrdGiven = 0;
    pInst->NrbGiven = 0;
    pInst->DrainAreaGiven = 0;
    pInst->SourceAreaGiven = 0;
    pInst->BodyAreaGiven = 0;
    pInst->MFingerGiven = 0;
    pInst->RthGiven = 0;
    pInst->CthGiven = 0;
    pInst->Temperature = 0.0;
    pInst->DrainJunctionPerimeterGiven = 0;                                      /* 4.5 */
    pInst->SourceJunctionPerimeterGiven = 0;                                     /* 4.5 */
    return 0;
}

ufsInitModelFlag( pModel )
struct ufsAPI_ModelData  *pModel;
{
time_t tloc, TimeDiff, CompileTime;
double DayPast;

    pModel->Tmax = 1000.0;
    pModel->Imax = 1.0;
    pModel->VfbfGiven = 0;
    pModel->VfbbGiven = 0;
    pModel->WkfGiven = 0;
    pModel->WkbGiven = 0;
    pModel->NqffGiven = 0;
    pModel->NqfbGiven = 0;
    pModel->NsfGiven = 0;
    pModel->NsbGiven = 0;
    pModel->ToxfGiven = 0;
    pModel->ToxbGiven = 0;
    pModel->NsubGiven = 0;
    pModel->NgateGiven = 0;
    pModel->TpgGiven = 0;
    pModel->TpsGiven = 0;
    pModel->NdsGiven = 0;
    pModel->TbGiven = 0;
    pModel->NbodyGiven = 0;
    pModel->LlddGiven = 0;
    pModel->NlddGiven = 0;
    pModel->UoGiven = 0;
    pModel->ThetaGiven = 0;
    pModel->BfactGiven = 0;
    pModel->VsatGiven = 0;
    pModel->AlphaGiven = 0;
    pModel->BetaGiven = 0;
    pModel->GammaGiven = 0;
    pModel->KappaGiven = 0;
    pModel->TauoGiven = 0;
    pModel->JroGiven = 0;
    pModel->MGiven = 0;
    pModel->LdiffGiven = 0;
    pModel->SeffGiven = 0;
    pModel->FvbjtGiven = 0;
    pModel->CgfdoGiven = 0;
    pModel->CgfsoGiven = 0;
    pModel->CgfboGiven = 0;
    pModel->RhosdGiven = 0;
    pModel->RhobGiven = 0;
    pModel->RdGiven = 0;
    pModel->RsGiven = 0;
    pModel->RbodyGiven = 0;
    pModel->DlGiven = 0;
    pModel->DwGiven = 0;
    pModel->FnkGiven = 0;
    pModel->FnaGiven = 0;
    pModel->TfGiven = 0;
    pModel->ThaloGiven = 0;
    pModel->NblGiven = 0;
    pModel->NbhGiven = 0;
    pModel->NhaloGiven = 0;
    pModel->TmaxGiven = 0;
    pModel->ImaxGiven = 0;
    pModel->BgidlGiven = 0;                                                      /* 4.41 */
    pModel->NtrGiven = 0;                                                        /* 4.5 */
    pModel->BjtGiven = 0;                                                        /* 4.5 */
    pModel->LrsceGiven = 0;                                                      /* 4.5r */
    pModel->NqfswGiven = 0;                                                      /* 4.5r */
    pModel->QmGiven = 0;                                                         /* 4.5qm */
    pModel->VoGiven = 0;                                                         /* 5.0vo */
    pModel->TypeGiven = 0;
    pModel->TnomGiven = 0;                                                       /* 4.51 */
    pModel->DebugGiven = 0;                                                      /* 4.51 */
    pModel->ParamCheckGiven = 0;                                                 /* 4.51 */
    pModel->SelftGiven = 0;                                                      /* 4.51 */
    pModel->NfdModGiven = 0;                                                     /* 4.51 */
    pModel->MoxGiven = 0;                                                        /* 7.0Y */
    pModel->SvbeGiven = 0;                                                       /* 7.0Y */
    pModel->ScbeGiven = 0;                                                       /* 7.0Y */
    pModel->KdGiven = 0;                                                         /* 7.0Y */
    pModel->GexGiven = 0;                                                        /* 7.5W */
    pModel->SfactGiven = 0;                                                      /* 7.0Y */
    pModel->FfactGiven = 0;                                                      /* 7.0Y */
    return 0;
}

double
ufsLimiting(pInst, OldVbs, Gbs, abstol, DynamicNeeded, Ibtot, vbs)
struct ufsAPI_InstData *pInst;
int DynamicNeeded;
double Ibtot, vbs, OldVbs, abstol, Gbs;
{
double T1, T2;

   T1 = OldVbs - 0.5 * Ibtot / Gbs;
   T2 = OldVbs - 0.5 * Ibtot / Gbs;

   if ((pInst->BulkContact == 0) && (DynamicNeeded == 0))
   {   
       if (fabs(Ibtot) < 0.1 * abstol)
       {   vbs = OldVbs;
       }
       else
       {   
	   if (Ibtot < 0.0)
	   {   if ((vbs > T1) || (vbs < OldVbs))
		   vbs = T1;
	   }
	   else
	   {   if ((vbs < T2) || (vbs > OldVbs))
		   vbs = T2;
	   }
       }
   }

   if (vbs > OldVbs + 0.3)
       vbs = OldVbs + 0.3;
   else if (vbs < OldVbs - 0.3)
       vbs = OldVbs - 0.3;

    return vbs;
}

int
ufsGetModelParam(pModel, Index, pValue)
struct ufsAPI_ModelData *pModel;
int Index;
double *pValue;
{
    switch(Index) 
    {   case UFS_MOD_SELFT:
            *pValue = (double) pModel->Selft; 
            return(0);
        case UFS_MOD_BODY:
            *pValue = (double) pModel->NfdMod; 
            return(0);
        case  UFS_MOD_VFBF:
            *pValue = pModel->Vfbf;
            return(0);
        case  UFS_MOD_VFBB:
            *pValue = pModel->Vfbb;
            return(0);
        case  UFS_MOD_WKF:
            *pValue = pModel->Wkf;
            return(0);
        case  UFS_MOD_WKB:
            *pValue = pModel->Wkb;
            return(0);
        case  UFS_MOD_NQFF :
            *pValue = pModel->Nqff / 1.0e4;
            return(0);
        case  UFS_MOD_NQFB:
            *pValue = pModel->Nqfb / 1.0e4;
            return(0);
        case  UFS_MOD_NSF:
            *pValue = pModel->Nsf / 1.0e4;
            return(0);
        case  UFS_MOD_NSB:
            *pValue = pModel->Nsb / 1.0e4;
            return(0);
        case  UFS_MOD_TOXF:
            *pValue = pModel->Toxf;
            return(0);
        case  UFS_MOD_TOXB:
            *pValue = pModel->Toxb;
            return(0);
        case  UFS_MOD_NSUB:
            *pValue = pModel->Nsub / 1.0e6;
            return(0);
        case  UFS_MOD_NGATE:
            *pValue = pModel->Ngate / 1.0e6;
            return(0);
        case  UFS_MOD_TPG:
            *pValue = (double) pModel->Tpg;
            return(0);
        case  UFS_MOD_TPS:
            *pValue = (double) pModel->Tps;
            return(0);
        case  UFS_MOD_NDS:
            *pValue = pModel->Nds / 1.0e6;
            return(0);
        case  UFS_MOD_TB:
            *pValue = pModel->Tb;
            return(0);
        case UFS_MOD_NBODY:
            *pValue = pModel->Nbody / 1.0e6;
            return(0);
        case UFS_MOD_LLDD:
            *pValue = pModel->Lldd;
            return(0);
        case UFS_MOD_NLDD:
            *pValue = pModel->Nldd / 1.0e6;
            return(0);
        case UFS_MOD_UO:
            *pValue = pModel->Uo * 1.0e4;
            return(0);
        case UFS_MOD_THETA:
            *pValue = pModel->Theta * 100.0;
            return(0);
        case UFS_MOD_BFACT:
            *pValue = pModel->Bfact;
            return(0);   
        case UFS_MOD_VSAT:
            *pValue = pModel->Vsat * 100.0;
            return(0);
        case  UFS_MOD_ALPHA:
            *pValue = pModel->Alpha / 100.0;
            return(0);
        case  UFS_MOD_BETA:
            *pValue = pModel->Beta / 100.0;
            return(0);
        case  UFS_MOD_GAMMA:
            *pValue = pModel->Gamma;
            return(0);
        case  UFS_MOD_KAPPA:
            *pValue = pModel->Kappa;
            return(0);
        case  UFS_MOD_TAUO:
            *pValue = pModel->Tauo;
            return(0);
        case  UFS_MOD_JRO:
            *pValue = pModel->Jro;
            return(0);
        case  UFS_MOD_M:
            *pValue = pModel->M;
            return(0);
        case  UFS_MOD_LDIFF:
            *pValue = pModel->Ldiff;
            return(0);
        case  UFS_MOD_SEFF:
            *pValue = pModel->Seff * 100.0;
            return(0);
        case  UFS_MOD_FVBJT:
            *pValue = pModel->Fvbjt;
            return(0);
        case  UFS_MOD_TNOM:
            *pValue = pModel->Tnom;
            return(0);
        case  UFS_MOD_CGFDO:
            *pValue = pModel->Cgfdo;
            return(0);
        case  UFS_MOD_CGFSO:
            *pValue = pModel->Cgfso;
            return(0);
        case  UFS_MOD_CGFBO:
            *pValue = pModel->Cgfbo;
            return(0);
        case  UFS_MOD_RHOSD:
            *pValue = pModel->Rhosd;
            return(0);
        case  UFS_MOD_RHOB:
            *pValue = pModel->Rhob;
            return(0);
        case  UFS_MOD_RD:
            *pValue = pModel->Rd;
            return(0);
        case  UFS_MOD_RS:
            *pValue = pModel->Rs;
            return(0);
        case  UFS_MOD_RB:
            *pValue = pModel->Rbody;
            return(0);
        case  UFS_MOD_DL:
            *pValue = pModel->Dl;
            return(0);
        case UFS_MOD_DW:
            *pValue = pModel->Dw; 
            return(0);
        case UFS_MOD_FNK:
            *pValue = pModel->Fnk; 
            return(0);
        case UFS_MOD_FNA:
            *pValue = pModel->Fna; 
            return(0);
        case UFS_MOD_TF:
            *pValue = pModel->Tf; 
            return(0);
        case UFS_MOD_THALO:
            *pValue = pModel->Thalo; 
            return(0);
        case UFS_MOD_NBL:
            *pValue = pModel->Nbl / 1.0e6; 
            return(0);
        case UFS_MOD_NBH:
            *pValue = pModel->Nbh / 1.0e6; 
            return(0);
        case UFS_MOD_NHALO:
            *pValue = pModel->Nhalo / 1.0e6; 
            return(0);
        case UFS_MOD_TMAX:
            *pValue = pModel->Tmax; 
            return(0);
        case UFS_MOD_IMAX:
            *pValue = pModel->Imax; 
            return(0);
        case UFS_MOD_BGIDL:                                                      /* 4.41 */
            *pValue = pModel->Bgidl;                                             /* 4.41 */
            return(0);                                                           /* 4.41 */
        case UFS_MOD_NTR:                                                        /* 4.5 */
            *pValue = pModel->Ntr / 1.0e6;                                       /* 4.5 */
            return(0);                                                           /* 4.5 */
        case UFS_MOD_BJT:                                                        /* 4.5 */
            *pValue = (double) pModel->Bjt;                                      /* 4.5 */
            return(0);                                                           /* 4.5 */
        case UFS_MOD_LRSCE:                                                      /* 4.5r */
            *pValue = pModel->Lrsce;                                             /* 4.5r */
            return(0);                                                           /* 4.5r */
        case UFS_MOD_NQFSW:                                                      /* 4.5r */
            *pValue = pModel->Nqfsw / 1.0e4;                                     /* 4.5r */
            return(0);                                                           /* 4.5r */
        case UFS_MOD_QM:                                                         /* 4.5qm */
            *pValue = pModel->Qm;                                                /* 4.5qm */
            return(0);                                                           /* 4.5qm */
        case UFS_MOD_VO:                                                         /* 5.0vo */
            *pValue = pModel->Vo;                                                /* 5.0vo */
            return(0);                                                           /* 5.0vo */
        case UFS_MOD_MOX:                                                        /* 7.0Y */
            *pValue = pModel->Mox;                                               /* 7.0Y */
            return(0);                                                           /* 7.0Y */
        case UFS_MOD_SVBE:                                                       /* 7.0Y */
            *pValue = pModel->Svbe;                                              /* 7.0Y */
            return(0);                                                           /* 7.0Y */
        case UFS_MOD_SCBE:                                                       /* 7.0Y */
            *pValue = pModel->Scbe;                                              /* 7.0Y */
            return(0);                                                           /* 7.0Y */
        case UFS_MOD_KD:                                                         /* 7.0Y */
            *pValue = pModel->Kd;                                                /* 7.0Y */
            return(0);                                                           /* 7.0Y */
        case UFS_MOD_GEX:                                                        /* 7.5W */
            *pValue = pModel->Gex;                                               /* 7.5W */
            return(0);                                                           /* 7.0Y */
        case UFS_MOD_SFACT:                                                      /* 7.0Y */
            *pValue = pModel->Sfact;                                             /* 7.0Y */
            return(0);                                                           /* 7.0Y */
        case UFS_MOD_FFACT:                                                      /* 7.0Y */
            *pValue = pModel->Ffact;                                             /* 7.0Y */
            return(0);                                                           /* 7.0Y */

    }
    return(1);
}

int
ufsSetModelParam(pModel, Index, Value)
struct ufsAPI_ModelData *pModel;
int Index;
double Value;
{
    switch(Index)
    {   case  UFS_MOD_SELFT:
            pModel->Selft = (int) Value;
            pModel->SelftGiven = YES;
            return(0);
        case  UFS_MOD_BODY:
            pModel->NfdMod = (int) Value;
            pModel->NfdModGiven = YES;
            return(0);
        case  UFS_MOD_VFBF:
            pModel->Vfbf = Value;
            pModel->VfbfGiven = YES;
            return(0);
        case  UFS_MOD_VFBB:
            pModel->Vfbb = Value;
            pModel->VfbbGiven = YES;
            return(0);
        case  UFS_MOD_WKF:
            pModel->Wkf = Value;
            pModel->WkfGiven = YES;
            return(0);
        case  UFS_MOD_WKB:
            pModel->Wkb = Value;
            pModel->WkbGiven = YES;
            return(0);
        case  UFS_MOD_NQFF:
            pModel->Nqff = Value * 1.0e4;
            pModel->NqffGiven = YES;
            return(0);
        case  UFS_MOD_NQFB:
            pModel->Nqfb = Value * 1.0e4;
            pModel->NqfbGiven = YES;
            return(0);
        case  UFS_MOD_NSF:
            pModel->Nsf = Value * 1.0e4;
            pModel->NsfGiven = YES;
            return(0);
        case  UFS_MOD_NSB:
            pModel->Nsb = Value * 1.0e4;
            pModel->NsbGiven = YES;
            return(0);
        case  UFS_MOD_TOXF:
            pModel->Toxf = Value;
            pModel->ToxfGiven = YES;
            return(0);
        case  UFS_MOD_TOXB:
            pModel->Toxb = Value;
            pModel->ToxbGiven = YES;
            return(0);
        case  UFS_MOD_NSUB:
            pModel->Nsub = Value * 1.0e6;
            pModel->NsubGiven = YES;
            return(0);
        case  UFS_MOD_NGATE:
            pModel->Ngate = Value * 1.0e6;
            pModel->NgateGiven = YES;
            return(0);
        case  UFS_MOD_TPG:
            pModel->Tpg = (int) Value;
            pModel->TpgGiven = YES;
            return(0);
        case  UFS_MOD_TPS:
            pModel->Tps = (int) Value;
            pModel->TpsGiven = YES;
            return(0);
        case  UFS_MOD_NDS:
            pModel->Nds = Value * 1.0e6;
            pModel->NdsGiven = YES;
            return(0);
        case  UFS_MOD_TB:
            pModel->Tb = Value;
            pModel->TbGiven = YES;
            return(0);
        case UFS_MOD_NBODY:
            pModel->Nbody = Value * 1.0e6;
            pModel->NbodyGiven = YES;
            return(0);
        case UFS_MOD_LLDD:
            pModel->Lldd= Value;
            pModel->LlddGiven = YES;
            return(0);
        case UFS_MOD_NLDD:
            pModel->Nldd = Value * 1.0e6;
            pModel->NlddGiven = YES;
            return(0);
        case UFS_MOD_UO:
            pModel->Uo = Value / 1.0e4;
            pModel->UoGiven = YES;
            return(0);
        case UFS_MOD_THETA:
            pModel->Theta = Value / 100.0;
            pModel->ThetaGiven = YES;
            return(0);
        case UFS_MOD_BFACT:
            pModel->Bfact = Value;
            pModel->BfactGiven = YES;
            return(0);    
        case UFS_MOD_VSAT:
            pModel->Vsat = Value / 100.0;
            pModel->VsatGiven = YES;
            return(0);
        case  UFS_MOD_ALPHA:
            pModel->Alpha = Value * 100.0;
            pModel->AlphaGiven = YES;
            return(0);
        case  UFS_MOD_BETA:
            pModel->Beta = Value * 100.0;
            pModel->BetaGiven = YES;
            return(0);
        case  UFS_MOD_GAMMA:
            pModel->Gamma = Value;
            pModel->GammaGiven = YES;
            return(0);
        case  UFS_MOD_KAPPA:
            pModel->Kappa = Value;
            pModel->KappaGiven = YES;
            return(0);
        case  UFS_MOD_TAUO:
            pModel->Tauo = Value;
            pModel->TauoGiven = YES;
            return(0);
        case  UFS_MOD_JRO:
            pModel->Jro = Value;
            pModel->JroGiven = YES;
            return(0);
        case  UFS_MOD_M:
            pModel->M = Value;
            pModel->MGiven = YES;
            return(0);
        case  UFS_MOD_LDIFF:
            pModel->Ldiff = Value;
            pModel->LdiffGiven = YES;
            return(0);
        case  UFS_MOD_SEFF:               
            pModel->Seff = Value / 100.0;
            pModel->SeffGiven = YES;
            return(0);
        case  UFS_MOD_FVBJT:             
            pModel->Fvbjt = Value;
            pModel->FvbjtGiven = YES;
            return(0);
        case  UFS_MOD_TNOM:             
            pModel->Tnom = Value;
            pModel->TnomGiven = YES;
            return(0);
        case  UFS_MOD_CGFDO:               
            pModel->Cgfdo = Value;
            pModel->CgfdoGiven = YES;
            return(0);
        case  UFS_MOD_CGFSO:
            pModel->Cgfso = Value;
            pModel->CgfsoGiven = YES;
            return(0);
        case  UFS_MOD_CGFBO:               
            pModel->Cgfbo = Value;
            pModel->CgfboGiven = YES;
            return(0);
        case  UFS_MOD_RHOSD:             
            pModel->Rhosd = Value;
            pModel->RhosdGiven = YES;
            return(0);
        case  UFS_MOD_RHOB:             
            pModel->Rhob = Value;
            pModel->RhobGiven = YES;
            return(0);
        case  UFS_MOD_RD:             
            pModel->Rd = Value;
            pModel->RdGiven = YES;
            return(0);
        case  UFS_MOD_RS:             
            pModel->Rs = Value;
            pModel->RsGiven = YES;
            return(0);
        case UFS_MOD_RB:
            pModel->Rbody = Value;
            pModel->RbodyGiven = YES;
            return(0);
        case UFS_MOD_DL:
            pModel->Dl = Value;
            pModel->DlGiven = YES;
            return(0);
        case UFS_MOD_DW:
            pModel->Dw = Value;
            pModel->DwGiven = YES;
            return(0);
        case UFS_MOD_FNK:
            pModel->Fnk = Value;
            pModel->FnkGiven = YES;
            return(0);
        case UFS_MOD_FNA:
            pModel->Fna = Value;
            pModel->FnaGiven = YES;
            return(0);
        case UFS_MOD_TF:
            pModel->Tf = Value;
            pModel->TfGiven = YES;
            return(0);
        case UFS_MOD_THALO:
            pModel->Thalo = Value;
            pModel->ThaloGiven = YES;
            return(0);
        case UFS_MOD_NBL:
            pModel->Nbl = Value * 1.0e6;
            pModel->NblGiven = YES;
            return(0);
        case UFS_MOD_NBH:
            pModel->Nbh = Value * 1.0e6;
            pModel->NbhGiven = YES;
            return(0);
        case UFS_MOD_NHALO:
            pModel->Nhalo = Value * 1.0e6;
            pModel->NhaloGiven = YES;
            return(0);
        case UFS_MOD_TMAX:
            pModel->Tmax = Value;
            pModel->TmaxGiven = YES;
            return(0);
        case UFS_MOD_IMAX:
            pModel->Imax = Value;
            pModel->ImaxGiven = YES;
            return(0);
        case UFS_MOD_BGIDL:                                                      /* 4.41 */
            pModel->Bgidl = Value;                                               /* 4.41 */
            pModel->BgidlGiven = YES;                                            /* 4.41 */
            return(0);                                                           /* 4.41 */
        case UFS_MOD_NTR:                                                        /* 4.5 */
            pModel->Ntr = Value * 1.0e6;                                         /* 4.5 */
            pModel->NtrGiven = YES;                                              /* 4.5 */
            return(0);                                                           /* 4.5 */
        case UFS_MOD_BJT:                                                        /* 4.5 */
            pModel->Bjt = (int) Value;                                           /* 4.5 */
            pModel->BjtGiven = YES;                                              /* 4.5 */
            return(0);                                                           /* 4.5 */
        case UFS_MOD_LRSCE:                                                      /* 4.5r */
            pModel->Lrsce = Value;                                               /* 4.5r */
            pModel->LrsceGiven = YES;                                            /* 4.5r */
            return(0);                                                           /* 4.5r */
        case UFS_MOD_NQFSW:                                                      /* 4.5r */
            pModel->Nqfsw = Value * 1.0e4;                                       /* 4.5r */
            pModel->NqfswGiven = YES;                                            /* 4.5r */
            return(0);                                                           /* 4.5r */
        case UFS_MOD_QM:                                                         /* 4.5qm */
            pModel->Qm = Value;                                                  /* 4.5qm */
            pModel->QmGiven = YES;                                               /* 4.5qm */
            return(0);                                                           /* 4.5qm */
        case UFS_MOD_VO:                                                         /* 5.0vo */
            pModel->Vo = Value;                                                  /* 5.0vo */
            pModel->VoGiven = YES;                                               /* 5.0vo */
            return(0);                                                           /* 5.0vo */
        case UFS_MOD_MOX:                                                        /* 7.0Y */
            pModel->Mox = Value;                                                 /* 7.0Y */
            pModel->MoxGiven = YES;                                              /* 7.0Y */
            return(0);                                                           /* 7.0Y */
        case UFS_MOD_SVBE:                                                       /* 7.0Y */
            pModel->Svbe = Value;                                                /* 7.0Y */
            pModel->SvbeGiven = YES;                                             /* 7.0Y */
            return(0);                                                           /* 7.0Y */
        case UFS_MOD_SCBE:                                                       /* 7.0Y */
            pModel->Scbe = Value;                                                /* 7.0Y */
            pModel->ScbeGiven = YES;                                             /* 7.0Y */
            return(0);                                                           /* 7.0Y */
        case UFS_MOD_KD:                                                         /* 7.0Y */
            pModel->Kd = Value;                                                  /* 7.0Y */
            pModel->KdGiven = YES;                                               /* 7.0Y */
            return(0);                                                           /* 7.0Y */
        case UFS_MOD_GEX:                                                        /* 7.5W */
            pModel->Gex = Value;                                                 /* 7.5W */
            pModel->GexGiven = YES;                                              /* 7.5W */
            return(0);                                                           /* 7.0Y */
        case UFS_MOD_SFACT:                                                      /* 7.0Y */
            pModel->Sfact = Value;                                               /* 7.0Y */
            pModel->SfactGiven = YES;                                            /* 7.0Y */
            return(0);                                                           /* 7.0Y */
        case UFS_MOD_FFACT:                                                      /* 7.0Y */
            pModel->Ffact = Value;                                               /* 7.0Y */
            pModel->FfactGiven = YES;                                            /* 7.0Y */
            return(0);                                                           /* 7.0Y */
        case  UFS_MOD_NMOS:
	    pModel->Type = 1;
            pModel->TypeGiven = YES;
            return(0);
        case  UFS_MOD_PMOS:
            pModel->Type = -1;
            pModel->TypeGiven = YES;
            return(0);
    }
    return(1);
}

int
ufsSetInstParam(pInst, Index, Value)
struct ufsAPI_InstData *pInst;
int Index;
double Value;
{
    switch(Index) 
    {   case UFS_W:
            pInst->Width = Value;
            pInst->WidthGiven = YES;
            return(0);
        case UFS_L:
            pInst->Length = Value;
            pInst->LengthGiven = YES;
            return(0);
        case UFS_M:
            pInst->MFinger = Value;
            pInst->MFingerGiven = YES;
            return(0);
        case UFS_AS:
            pInst->SourceArea = Value;
            pInst->SourceAreaGiven = YES;
            return(0);
        case UFS_AD:
            pInst->DrainArea = Value;
            pInst->DrainAreaGiven = YES;
            return(0);
        case UFS_AB:
            pInst->BodyArea = Value;
            pInst->BodyAreaGiven = YES;
            return(0);
        case UFS_NRS:
            pInst->Nrs = Value;
            pInst->NrsGiven = YES;
            return(0);
        case UFS_NRD:
            pInst->Nrd = Value;
            pInst->NrdGiven = YES;
            return(0);
        case UFS_NRB:
            pInst->Nrb = Value;
            pInst->NrbGiven = YES;
            return(0);
        case UFS_RTH:
            pInst->Rth = Value;
            pInst->RthGiven = YES;
            return(0);
        case UFS_CTH:
            pInst->Cth = Value;
            pInst->CthGiven = YES;
            return(0);
        case UFS_PSJ:                                                            /* 4.5 */
            pInst->SourceJunctionPerimeter = Value;                              /* 4.5 */
            pInst->SourceJunctionPerimeterGiven = YES;                           /* 4.5 */
            return(0);                                                           /* 4.5 */
        case UFS_PDJ:                                                            /* 4.5 */
            pInst->DrainJunctionPerimeter = Value;                               /* 4.5 */
            pInst->DrainJunctionPerimeterGiven = YES;                            /* 4.5 */
            return(0);                                                           /* 4.5 */
    }
    return(1);
}

int
ufsGetInstParam(pInst, Index, pValue)
struct ufsAPI_InstData *pInst;
int Index;
double *pValue;
{
    switch(Index) 
    {   case UFS_L:
            *pValue = pInst->Length;
            return(0);
        case UFS_W:
            *pValue = pInst->Width;
            return(0);
        case UFS_M:
            *pValue = pInst->MFinger;
            return(0);
        case UFS_AS:
            *pValue = pInst->SourceArea;
            return(0);
        case UFS_AD:
            *pValue = pInst->DrainArea;
            return(0);
        case UFS_AB:
            *pValue = pInst->BodyArea;
            return(0);
        case UFS_NRS:
            *pValue = pInst->Nrs;
            return(0);
        case UFS_NRD:
            *pValue = pInst->Nrd;
            return(0);
        case UFS_NRB:
            *pValue = pInst->Nrb;
            return(0);
        case UFS_RTH:
            *pValue = pInst->Rth;
            return(0);
        case UFS_CTH:
            *pValue = pInst->Cth;
            return(0);
        case UFS_TEMP:
            *pValue = pInst->Temperature - 273.15;
            return(0);
        case UFS_PSJ:                                                            /* 4.5 */
            *pValue = pInst->SourceJunctionPerimeter;                            /* 4.5 */
            return(0);                                                           /* 4.5 */
        case UFS_PDJ:                                                            /* 4.5 */
            *pValue = pInst->DrainJunctionPerimeter;                             /* 4.5 */
            return(0);                                                           /* 4.5 */
    }
    return(1);
}


int
ufsGetOpParam(pOpInfo, Index, pValue)
struct ufsAPI_OPData *pOpInfo;
int Index;
double *pValue;
{
    switch(Index)
    {
        case UFS_ICH:
            *pValue = pOpInfo->Ich;
            return(0);
        case UFS_IBJT:
            *pValue = pOpInfo->Ibjt;
            return(0);
        case UFS_IR:
            *pValue = pOpInfo->Ir;
            return(0);
        case UFS_IGT:
            *pValue = pOpInfo->Igt;
            return(0);
        case UFS_IGI:
            *pValue = pOpInfo->Igi;
            return(0);
        case UFS_IGB:                                                           /* 7.0Y */
            *pValue = pOpInfo->Igb;                                             /* 7.0Y */
            return(0);                                                          /* 7.0Y */
        case UFS_VBS:
            *pValue = pOpInfo->Vbs;
            return(0);
        case UFS_VBD:
            *pValue = pOpInfo->Vbd;
            return(0);
        case UFS_VGFS:
            *pValue = pOpInfo->Vgfs;
            return(0);
        case UFS_VGFD:
            *pValue = pOpInfo->Vgfd;
            return(0);
        case UFS_VGBS:
            *pValue = pOpInfo->Vgbs;
            return(0);
        case UFS_VDS:
            *pValue = pOpInfo->Vds;
            return(0);
        case UFS_RD:
            if (pOpInfo->DrainConductance > 0.0)
                *pValue = 1.0 / pOpInfo->DrainConductance;
	    else
                *pValue = 0.0;
            return(0);
        case UFS_RS:
            if (pOpInfo->SourceConductance > 0.0)
                *pValue = 1.0 / pOpInfo->SourceConductance;
	    else
                *pValue = 0.0;
            return(0);
        case UFS_RB:
            if (pOpInfo->BodyConductance > 0.0)
                *pValue = 1.0 / pOpInfo->BodyConductance;
	    else
                *pValue = 0.0;
            return(0);
        case UFS_GDGF:
            *pValue = pOpInfo->dId_dVgf;
            return(0);
        case UFS_GDD:
            *pValue = pOpInfo->dId_dVd;
            return(0);
        case UFS_GDGB:
            *pValue = pOpInfo->dId_dVgb;
            return(0);
        case UFS_GDB:
            *pValue = pOpInfo->dId_dVb;
            return(0);
        case UFS_GDS:
            *pValue = -(pOpInfo->dId_dVgf + pOpInfo->dId_dVd
		    + pOpInfo->dId_dVgb + pOpInfo->dId_dVb);
            return(0);
        case UFS_GRB:
            *pValue = pOpInfo->dIr_dVb;
            return(0);
        case UFS_GRS:
            *pValue = -pOpInfo->dIr_dVb;
            return(0);
        case UFS_GGTGF:
            *pValue = pOpInfo->dIgt_dVgf;
            return(0);
        case UFS_GGTD:
            *pValue = pOpInfo->dIgt_dVd;
            return(0);
        case UFS_GGTGB:
            *pValue = pOpInfo->dIgt_dVgb;
            return(0);
        case UFS_GGTB:
            *pValue = pOpInfo->dIgt_dVb;
            return(0);
        case UFS_GGTS:
            *pValue = -(pOpInfo->dIgt_dVgf + pOpInfo->dIgt_dVd
                    + pOpInfo->dIgt_dVgb + pOpInfo->dIgt_dVb);                   /* 6.0 */
            return(0);
        case UFS_GGIGF:
            *pValue = pOpInfo->dIgi_dVgf;
            return(0);
        case UFS_GGID:
            *pValue = pOpInfo->dIgi_dVd;
            return(0);
        case UFS_GGIGB:
            *pValue = pOpInfo->dIgi_dVgb;
            return(0);
        case UFS_GGIB:
            *pValue = pOpInfo->dIgi_dVb;
            return(0);
        case UFS_GGIS:
            *pValue = -(pOpInfo->dIgi_dVgf + pOpInfo->dIgi_dVd
		    + pOpInfo->dIgi_dVgb + pOpInfo->dIgi_dVb);                   /* 6.0 */
            return(0);
        case UFS_GGBGF:                                                          /* 7.0Y */
            *pValue = pOpInfo->dIgb_dVgf;                                        /* 7.0Y */
            return(0);                                                           /* 7.0Y */
        case UFS_GGBD:                                                           /* 7.0Y */
            *pValue = pOpInfo->dIgb_dVd;                                         /* 7.0Y */
            return(0);                                                           /* 7.0Y */
        case UFS_GGBGB:                                                          /* 7.0Y */
            *pValue = pOpInfo->dIgb_dVgb;                                        /* 7.0Y */
            return(0);                                                           /* 7.0Y */
        case UFS_GGBB:                                                           /* 7.0Y */
            *pValue = pOpInfo->dIgb_dVb;                                         /* 7.0Y */
            return(0);                                                           /* 7.0Y */
        case UFS_GGBS:                                                           /* 7.0Y */
            *pValue = -(pOpInfo->dIgb_dVgf + pOpInfo->dIgb_dVd
                    + pOpInfo->dIgb_dVgb + pOpInfo->dIgb_dVb);                   /* 7.0Y */
            return(0);                                                           /* 7.0Y */
        case UFS_QGF:
            *pValue = pOpInfo->Qgf;
            return(0);
        case UFS_QD:
            *pValue = pOpInfo->Qd;
            return(0);
        case UFS_QGB:
            *pValue = pOpInfo->Qgb;
            return(0);
        case UFS_QB:
            *pValue = pOpInfo->Qb;
            return(0);
        case UFS_QS:
            *pValue = -pOpInfo->Qs;
            return(0);
        case UFS_QN:                                                             /* 4.5 */
            *pValue = pOpInfo->Qn;                                               /* 4.5 */
            return(0);                                                           /* 4.5 */
        case UFS_UEFF:                                                           /* 4.5 */
            *pValue = pOpInfo->Ueff;                                             /* 4.5 */
            return(0);                                                           /* 4.5 */
        case UFS_VTW:
            *pValue = pOpInfo->Vtw;
            return(0);
        case UFS_VTS:
            *pValue = pOpInfo->Vts;
            return(0);
        case UFS_VDSAT:
            *pValue = pOpInfo->Vdsat;
            return(0);
        case UFS_CGFGF:
            *pValue = pOpInfo->dQgf_dVgf;
            return(0);
        case UFS_CGFD:
            *pValue = pOpInfo->dQgf_dVd;
            return(0);
        case UFS_CGFGB:
            *pValue = pOpInfo->dQgf_dVgb;
            return(0);
        case UFS_CGFB:
            *pValue = pOpInfo->dQgf_dVb;
            return(0);
        case UFS_CGFS:
            *pValue = -(pOpInfo->dQgf_dVgf + pOpInfo->dQgf_dVd
		    + pOpInfo->dQgf_dVgb + pOpInfo->dQgf_dVb);
            return(0);
        case UFS_CDGF:
            *pValue = pOpInfo->dQd_dVgf;
            return(0);
        case UFS_CDD:
            *pValue = pOpInfo->dQd_dVd;
            return(0);
        case UFS_CDGB:
            *pValue = pOpInfo->dQd_dVgb;
            return(0);
        case UFS_CDB:
            *pValue = pOpInfo->dQd_dVb;
            return(0);
        case UFS_CDS:
            *pValue = -(pOpInfo->dQd_dVgf + pOpInfo->dQd_dVd
		    + pOpInfo->dQd_dVgb + pOpInfo->dQd_dVb);
            return(0);
        case UFS_CGBGF:
            *pValue = pOpInfo->dQgb_dVgf;
            return(0);
        case UFS_CGBD:
            *pValue = pOpInfo->dQgb_dVd;
            return(0);
        case UFS_CGBGB:
            *pValue = pOpInfo->dQgb_dVgb;
            return(0);
        case UFS_CGBB:
            *pValue = pOpInfo->dQgb_dVb;
            return(0);
        case UFS_CGBS:
            *pValue = -(pOpInfo->dQgb_dVgf + pOpInfo->dQgb_dVd
		    + pOpInfo->dQgb_dVgb + pOpInfo->dQgb_dVb);
            return(0);
        case UFS_CBGF:
            *pValue = pOpInfo->dQb_dVgf;
            return(0);
        case UFS_CBD:
            *pValue = pOpInfo->dQb_dVd;
            return(0);
        case UFS_CBGB:
            *pValue = pOpInfo->dQb_dVgb;
            return(0);
        case UFS_CBB:
            *pValue = pOpInfo->dQb_dVb;
            return(0);
        case UFS_CBS:
            *pValue = -(pOpInfo->dQb_dVgf + pOpInfo->dQb_dVd
		    + pOpInfo->dQb_dVgb + pOpInfo->dQb_dVb);
            return(0);
        case UFS_CSGF:
            *pValue = -(pOpInfo->dQgf_dVgf + pOpInfo->dQd_dVgf
		    + pOpInfo->dQgb_dVgf + pOpInfo->dQb_dVgf);
            return(0);
        case UFS_CSD:
            *pValue = -(pOpInfo->dQgf_dVd + pOpInfo->dQd_dVd
		    + pOpInfo->dQgb_dVd + pOpInfo->dQb_dVd);
            return(0);
        case UFS_CSGB:
            *pValue = -(pOpInfo->dQgf_dVgb + pOpInfo->dQd_dVgb
		    + pOpInfo->dQgb_dVgb + pOpInfo->dQb_dVgb);
            return(0);
        case UFS_CSB:
            *pValue = -(pOpInfo->dQgf_dVb + pOpInfo->dQd_dVb
		    + pOpInfo->dQgb_dVb + pOpInfo->dQb_dVb);
            return(0);
        case UFS_CSS:
            *pValue = (pOpInfo->dQgf_dVgf + pOpInfo->dQd_dVgf
		    + pOpInfo->dQgb_dVgf + pOpInfo->dQb_dVgf
                    + pOpInfo->dQgf_dVd + pOpInfo->dQd_dVd
		    + pOpInfo->dQgb_dVd + pOpInfo->dQb_dVd
                    + pOpInfo->dQgf_dVgb + pOpInfo->dQd_dVgb
		    + pOpInfo->dQgb_dVgb + pOpInfo->dQb_dVgb
                    + pOpInfo->dQgf_dVb + pOpInfo->dQd_dVb
		    + pOpInfo->dQgb_dVb + pOpInfo->dQb_dVb);
            return(0);

        case UFS_LE:
            *pValue = pOpInfo->Le;
            return(0);
        case UFS_POWER:
            *pValue = pOpInfo->Power;
            return(0);
        case UFS_GDT:
            *pValue = pOpInfo->dId_dT;
            return(0);
        case UFS_GRT:
            *pValue = pOpInfo->dIr_dT;
            return(0);
        case UFS_GGTT:
            *pValue = pOpInfo->dIgt_dT;
            return(0);
        case UFS_GGIT:
            *pValue = pOpInfo->dIgi_dT;
            return(0);
        case UFS_GGBT:                                                     /* 7.0Y */
            *pValue = pOpInfo->dIgb_dT;                                    /* 7.0Y */
            return(0);                                                     /* 7.0Y */
        case UFS_CGFT:
            *pValue = pOpInfo->dQgf_dT;
            return(0);
        case UFS_CDT:
            *pValue = pOpInfo->dQd_dT;
            return(0);
        case UFS_CGBT:
            *pValue = pOpInfo->dQgb_dT;
            return(0);
        case UFS_CBT:
            *pValue = pOpInfo->dQb_dT;
            return(0);
        case UFS_CST:
            *pValue = -(pOpInfo->dQgf_dT + pOpInfo->dQd_dT
                    + pOpInfo->dQgb_dT + pOpInfo->dQb_dT);
            return(0);
        case UFS_GPT:
            *pValue = pOpInfo->dP_dT;
            return(0);
        case UFS_GPGF:
            *pValue = pOpInfo->dP_dVgf;
            return(0);
        case UFS_GPD:
            *pValue = pOpInfo->dP_dVd;
            return(0);
        case UFS_GPGB:
            *pValue = pOpInfo->dP_dVgb;
            return(0);
        case UFS_GPB:
            *pValue = pOpInfo->dP_dVb;
            return(0);
        case UFS_GPS:
            *pValue = -(pOpInfo->dP_dVgf + pOpInfo->dP_dVd
		    + pOpInfo->dP_dVgb + pOpInfo->dP_dVb);
            return(0);
    }
    return(1);
}

int
ufsDefaultModelParam(pModel, pEnv)
struct ufsAPI_ModelData *pModel;
struct ufsAPI_EnvData *pEnv;
{
    pModel->Debug = 0;
    if (!pModel->TypeGiven)
        pModel->Type = NMOS;     
    if (!pModel->SelftGiven) 
        pModel->Selft = 0;
    if (!pModel->NfdModGiven) 
        pModel->NfdMod = 0;
    if (!pModel->ToxfGiven)
        pModel->Toxf = 100.0e-10;
    if (!pModel->ToxbGiven)
        pModel->Toxb = 5.0e-7;
    if (pModel->NfdMod == 2)                                                     /* 6.0bulk */
        pModel->Toxb = 1.0e-10;                                                  /* 6.0bulk */
    if (!pModel->TbGiven)
        pModel->Tb = 1.0e-7;
    if (!pModel->NqffGiven)
        pModel->Nqff = 0.0;
    if (!pModel->NqfbGiven)
        pModel->Nqfb = 0.0;
    if (!pModel->NsfGiven)
        pModel->Nsf = 0.0;
    if (!pModel->NsbGiven)
        pModel->Nsb = 0.0;
    if (!pModel->NsubGiven)
    { if (pModel->NfdMod == 2)                                                   /* 6.0bulk */
      { pModel->Nsub = 1.0e23;                                                   /* 6.0bulk */
      }                                                                          /* 6.0bulk */
      else                                                                       /* 6.0bulk */
      { pModel->Nsub = 1.0e21;                                                   /* 6.0bulk */
      }                                                                          /* 6.0bulk */
    }                                                                            /* 6.0bulk */
    if (!pModel->NgateGiven)
	pModel->Ngate = 0.0;                                                     /* 4.5 */
    if (!pModel->TpgGiven)
	pModel->Tpg = 1;

    if (!pModel->TpsGiven)
    { if (pModel->Type == PMOS)                                                  /* 6.0bulk */
      { pModel->Tps = 1;                                                         /* 6.0bulk */
      }                                                                          /* 6.0bulk */
      else                                                                       /* 6.0bulk */
      { pModel->Tps = -1;                                                        /* 6.0bulk */
      }                                                                          /* 6.0bulk */
    }                                                                            /* 6.0bulk */
    if (pModel->NfdMod == 2)                                                     /* 6.0bulk */
        pModel->Tps = -1;                                                        /* 6.0bulk */
 
    if (!pModel->NdsGiven)
        pModel->Nds = 5.0e25;
    if (!pModel->NbodyGiven)
        pModel->Nbody = 5.0e22;
    if (!pModel->LlddGiven)
        pModel->Lldd = 0.0;  
    if (!pModel->NlddGiven)                                                      /* 4.5 */
        pModel->Nldd = 5.0e25;                                                   /* 4.5 */
    if (!pModel->UoGiven)                                                        /* 4.5 */
    { if (pModel->Type == PMOS)                                                  /* 4.5 */
      { pModel->Uo = 250.0 / 1.0e4;                                              /* 4.5 */
      }                                                                          /* 4.5 */
      else                                                                       /* 4.5 */
      { pModel->Uo = 700.0 / 1.0e4;                                              /* 4.5 */
      }                                                                          /* 4.5 */
    }                                                                            /* 4.5 */
    if (!pModel->ThetaGiven)
        pModel->Theta = 1.0e-6 / 100.0;
    if (!pModel->BfactGiven)
        pModel->Bfact = 0.3;
    if (!pModel->VsatGiven)
        pModel->Vsat = 7.0e6 / 100.0;                                            /* 5.0 */
    if (!pModel->AlphaGiven)
        pModel->Alpha = 0.0;
    if (!pModel->BetaGiven)
        pModel->Beta = 0.0;
    if (!pModel->GammaGiven)
        pModel->Gamma = 0.3;
    if (!pModel->KappaGiven)
        pModel->Kappa = 0.5;      
    if (!pModel->TauoGiven)
        pModel->Tauo = 0.0e0;                                                    /* 4.5 */
    if (!pModel->JroGiven)
        pModel->Jro = 1.0e-10;    
    if (!pModel->MGiven)
        pModel->M = 2.0;     
    if (!pModel->LdiffGiven)
        pModel->Ldiff = 1.0e-7;    
    if (!pModel->SeffGiven)
        pModel->Seff = 1.0e5 / 100.0;      
    if (!pModel->FvbjtGiven)
        pModel->Fvbjt = 0.0;
    if (!pModel->CgfsoGiven)
        pModel->Cgfso = 0.0;    
    if (!pModel->CgfdoGiven)
        pModel->Cgfdo = 0.0;    
    if (!pModel->CgfboGiven)
        pModel->Cgfbo = 0.0;   
    if (!pModel->RhosdGiven)
        pModel->Rhosd = 0.0;     
    if (!pModel->RhobGiven)
        pModel->Rhob = 0.0;     
    if (!pModel->RdGiven)
        pModel->Rd = 0.0;
    if (!pModel->RsGiven)
        pModel->Rs = 0.0;
    if (!pModel->RbodyGiven)
        pModel->Rbody = 0.0;
    if (!pModel->DlGiven)
        pModel->Dl = 0.0;
    if (!pModel->DwGiven)
        pModel->Dw = 0.0;
    if (!pModel->FnkGiven)
        pModel->Fnk = 0.0;
    if (!pModel->FnaGiven)
        pModel->Fna = 0.0;
    if (!pModel->TfGiven)
        pModel->Tf = 0.2e-6;
    if (!pModel->ThaloGiven)
        pModel->Thalo = 0.0;
    if (!pModel->NblGiven)
        pModel->Nbl = 5.0e16 * 1.0e6;
    if (!pModel->NbhGiven)
        pModel->Nbh = 5.0e17 * 1.0e6;
    if (!pModel->NhaloGiven)
        pModel->Nhalo = 0.0;
    if (!pModel->TmaxGiven)
        pModel->Tmax = 1000.0;
    if (!pModel->ImaxGiven)
        pModel->Imax = 1.0;
    if (!pModel->BgidlGiven)                                                     /* 4.41 */
        pModel->Bgidl = 0.0;                                                     /* 4.41 */
    if (!pModel->NtrGiven)                                                       /* 4.5 */
        pModel->Ntr = 0.0;                                                       /* 4.5 */
    if (!pModel->BjtGiven)                                                       /* 4.5 */
        pModel->Bjt = 1;                                                         /* 4.5 */
    if (!pModel->LrsceGiven)                                                     /* 4.5r */
        pModel->Lrsce = 0.0;                                                     /* 4.5r */
    if (!pModel->NqfswGiven)                                                     /* 4.5r */
        pModel->Nqfsw = 0.0;                                                     /* 4.5r */
    if (!pModel->QmGiven)                                                        /* 4.5qm */
        pModel->Qm = 0.0;                                                        /* 4.5qm */
    if (!pModel->VoGiven)                                                        /* 5.0vo */
        pModel->Vo = 0.0;                                                        /* 5.0vo */
    if (!pModel->MoxGiven)                                                       /* 7.0Y */
        pModel->Mox = 0.0;                                                       /* 7.0Y */
    if (!pModel->SvbeGiven)                                                      /* 7.0Y */
        pModel->Svbe = 13.5;                                                     /* 7.0Y */
    if (!pModel->ScbeGiven)                                                      /* 7.0Y */
    { if (pModel->Type == PMOS)                                                  /* 7.0Y */
      { pModel->Scbe = 0.045;                                                    /* 7.0Y */
      }                                                                          /* 7.0Y */
      else                                                                       /* 7.0Y */
      { pModel->Scbe = 0.04;                                                     /* 7.0Y */
      }                                                                          /* 7.0Y */
    }                                                                            /* 7.0Y */
    if (!pModel->KdGiven)                                                        /* 7.0Y */
        pModel->Kd = 3.9;                                                        /* 7.0Y */
    if (!pModel->GexGiven)                                                       /* 7.5W */
        pModel->Gex = 0.0;                                                       /* 7.5W */
    if (!pModel->SfactGiven)                                                     /* 7.0Y */
        pModel->Sfact = 50.0;                                                    /* 7.0Y */
    if (!pModel->FfactGiven)                                                     /* 7.0Y */
    { if (pModel->Type == PMOS)                                                  /* 7.0Y */
      { pModel->Ffact = 0.8;                                                     /* 7.0Y */
      }                                                                          /* 7.0Y */
      else                                                                       /* 7.0Y */
      { pModel->Ffact = 0.5;                                                     /* 7.0Y */
      }                                                                          /* 7.0Y */
    }                                                                            /* 7.0Y */
    if (!pModel->TnomGiven)  
	pModel->Tnom = pEnv->Tnom; 
    else
        pModel->Tnom = pModel->Tnom + 273.15;    
    return (0);
}

int
ufsDefaultInstParam(pModel, pInst, pEnv)
struct ufsAPI_ModelData *pModel;
struct ufsAPI_InstData *pInst;
struct ufsAPI_EnvData *pEnv;
{
    if (!pInst->WidthGiven)
        pInst->Width = 1.0e-6;
    if (!pInst->LengthGiven)
        pInst->Length = 1.0e-6;
    if (!pInst->DrainAreaGiven)
        pInst->DrainArea = 0.0;
    if (!pInst->SourceAreaGiven)
        pInst->SourceArea = 0.0;
    if (!pInst->BodyAreaGiven)
        pInst->BodyArea = 0.0;
    if (!pInst->NrdGiven)
        pInst->Nrd = 0.0;
    if (!pInst->NrsGiven)
        pInst->Nrs = 0.0;
    if (!pInst->NrbGiven)
        pInst->Nrb = 0.0;
    if (!pInst->MFingerGiven)
        pInst->MFinger = 1.0;
    if (!pInst->RthGiven)
        pInst->Rth = 1000.0;
    if (!pInst->CthGiven)
        pInst->Cth = 0.0;
    if (!pInst->DrainJunctionPerimeterGiven)                                     /* 4.5 */
        pInst->DrainJunctionPerimeter = 0.0;                                     /* 4.5 */
    if (!pInst->SourceJunctionPerimeterGiven)                                    /* 4.5 */
        pInst->SourceJunctionPerimeter = 0.0;                                    /* 4.5 */
    pInst->Temperature = 0.0; /* Must be initialized */
    return (0);
}

int 
fdEvalDeriv(Vds, Vgfs, Vbs, Vgbs, pOpInfo, pInst, pModel, pEnv, DynamicNeeded,   /* 5.0 */
            ACNeeded)                                                            /* 5.0 */
double Vds, Vgfs, Vbs, Vgbs;
struct ufsAPI_InstData  *pInst;
struct ufsAPI_ModelData *pModel;
struct ufsAPI_OPData *pOpInfo;
struct ufsAPI_EnvData *pEnv;
int DynamicNeeded, ACNeeded;                                                     /* 5.0 */
{
struct ufsAPI_OPData OpInfog, OpInfod, OpInfob, OpInfobg, OpInfot;
double Idrain, Idrain1, Vtemp;
int SH;

    fdEvalMod(Vds, Vgfs, Vbs, Vgbs, pOpInfo, pInst, pModel, pEnv,
	      DynamicNeeded, ACNeeded);                                          /* 5.0 */

    Vtemp = Vgfs + DeltaV;
    fdEvalMod(Vds, Vtemp, Vbs, Vgbs, &OpInfog, pInst, pModel, pEnv,
	      DynamicNeeded, ACNeeded);                                          /* 5.0 */

    Vtemp = Vds + DeltaV;
    fdEvalMod(Vtemp, Vgfs, Vbs, Vgbs, &OpInfod, pInst, pModel, pEnv,
	      DynamicNeeded, ACNeeded);                                          /* 5.0 */
	   
    Vtemp = Vbs + DeltaV1;
    fdEvalMod(Vds, Vgfs, Vtemp, Vgbs, &OpInfob, pInst, pModel, pEnv,
	      DynamicNeeded, ACNeeded);                                          /* 5.0 */
	   
    Vtemp = Vgbs + DeltaV;
    fdEvalMod(Vds, Vgfs, Vbs, Vtemp, &OpInfobg, pInst, pModel, pEnv,
	      DynamicNeeded, ACNeeded);                                          /* 5.0 */
	   
    Idrain = pOpInfo->Ich + pOpInfo->Ibjt;
    Idrain1 = OpInfog.Ich + OpInfog.Ibjt;
    pOpInfo->dId_dVgf = (Idrain1 - Idrain) / DeltaV;
    Idrain1 = OpInfod.Ich + OpInfod.Ibjt;
    pOpInfo->dId_dVd = (Idrain1 - Idrain) / DeltaV;
    Idrain1 = OpInfob.Ich + OpInfob.Ibjt;
    pOpInfo->dId_dVb = (Idrain1 - Idrain) / DeltaV1;
    Idrain1 = OpInfobg.Ich + OpInfobg.Ibjt;
    pOpInfo->dId_dVgb = (Idrain1 - Idrain) / DeltaV;

    pOpInfo->dIgi_dVgf = (OpInfog.Igi - pOpInfo->Igi) / DeltaV;
    pOpInfo->dIgi_dVd = (OpInfod.Igi - pOpInfo->Igi) / DeltaV;
    pOpInfo->dIgi_dVb = (OpInfob.Igi - pOpInfo->Igi) / DeltaV1;
    pOpInfo->dIgi_dVgb = (OpInfobg.Igi - pOpInfo->Igi) / DeltaV;

    pOpInfo->dIgt_dVgf = (OpInfog.Igt - pOpInfo->Igt) / DeltaV;
    pOpInfo->dIgt_dVd = (OpInfod.Igt - pOpInfo->Igt) / DeltaV;
    pOpInfo->dIgt_dVb = (OpInfob.Igt - pOpInfo->Igt) / DeltaV1;
    pOpInfo->dIgt_dVgb = (OpInfobg.Igt - pOpInfo->Igt) / DeltaV;

    pOpInfo->dIr_dVb = (OpInfob.Ir - pOpInfo->Ir) / DeltaV1;

    pOpInfo->dP_dVgf = (OpInfog.Power - pOpInfo->Power) / DeltaV;
    pOpInfo->dP_dVd = (OpInfod.Power - pOpInfo->Power) / DeltaV;
    pOpInfo->dP_dVb = (OpInfob.Power - pOpInfo->Power) / DeltaV1;
    pOpInfo->dP_dVgb = (OpInfobg.Power - pOpInfo->Power) / DeltaV;

    if (DynamicNeeded)
    {   pOpInfo->dQgf_dVgf = (OpInfog.Qgf - pOpInfo->Qgf) / DeltaV;
	pOpInfo->dQgf_dVd = (OpInfod.Qgf - pOpInfo->Qgf) / DeltaV;
	pOpInfo->dQgf_dVb = (OpInfob.Qgf - pOpInfo->Qgf) / DeltaV1;
	pOpInfo->dQgf_dVgb = (OpInfobg.Qgf - pOpInfo->Qgf) / DeltaV;

	pOpInfo->dQd_dVgf = (OpInfog.Qd - pOpInfo->Qd) / DeltaV;
	pOpInfo->dQd_dVd = (OpInfod.Qd - pOpInfo->Qd) / DeltaV;
	pOpInfo->dQd_dVb = (OpInfob.Qd - pOpInfo->Qd) / DeltaV1;
	pOpInfo->dQd_dVgb = (OpInfobg.Qd - pOpInfo->Qd) / DeltaV;

	pOpInfo->dQb_dVgf = (OpInfog.Qb - pOpInfo->Qb) / DeltaV;
	pOpInfo->dQb_dVd = (OpInfod.Qb - pOpInfo->Qb) / DeltaV;
	pOpInfo->dQb_dVb = (OpInfob.Qb - pOpInfo->Qb) / DeltaV1;
	pOpInfo->dQb_dVgb = (OpInfobg.Qb - pOpInfo->Qb) / DeltaV;

	pOpInfo->dQgb_dVgf = (OpInfog.Qgb - pOpInfo->Qgb) / DeltaV;
	pOpInfo->dQgb_dVd = (OpInfod.Qgb - pOpInfo->Qgb) / DeltaV;
	pOpInfo->dQgb_dVb = (OpInfob.Qgb - pOpInfo->Qgb) / DeltaV1;
	pOpInfo->dQgb_dVgb = (OpInfobg.Qgb - pOpInfo->Qgb) / DeltaV;
    }
    if (pModel->Selft > 1)
    {   Vtemp = pInst->Temperature + 1.0;
	/* Add flag SH to prevent GdsTempFac and GbTempFac from being updated       4.5 */
	SH = 0;                                                                  /* 4.5 */
	ufsTempEffect(pModel, pInst, pEnv, Vtemp, SH);                           /* 4.5 */
	fdEvalMod(Vds, Vgfs, Vbs, Vgbs, &OpInfot, pInst, pModel, pEnv,
		  DynamicNeeded, ACNeeded);                                      /* 5.0 */
       	pInst->Temperature = Vtemp - 1.0;
	Idrain1 = OpInfot.Ich + OpInfot.Ibjt;
	pOpInfo->dP_dT = (OpInfot.Power - pOpInfo->Power);
	pOpInfo->dId_dT = (Idrain1 - Idrain);
	pOpInfo->dIgi_dT = (OpInfot.Igi - pOpInfo->Igi);
	pOpInfo->dIgt_dT = (OpInfot.Igt - pOpInfo->Igt);
	pOpInfo->dIr_dT = (OpInfot.Ir - pOpInfo->Ir);
	if (DynamicNeeded)
	{   pOpInfo->dQgf_dT = (OpInfot.Qgf - pOpInfo->Qgf);
	    pOpInfo->dQd_dT = (OpInfot.Qd - pOpInfo->Qd);
	    pOpInfo->dQb_dT = (OpInfot.Qb - pOpInfo->Qb);
	    pOpInfo->dQgb_dT = (OpInfot.Qgb - pOpInfo->Qgb);
	} 
    }
/*
    printf("\n");
    printf("   Vgfs=%22.15e\n", Vgfs);
    printf("    Vds=%22.15e\n", Vds);
    printf("    Vbs=%22.15e\n", Vbs);
    printf("    Ich=%22.15e\n", pOpInfo->Ich);
    printf("   Ibjt=%22.15e\n", pOpInfo->Ibjt);
    printf("     Ir=%22.15e\n", pOpInfo->Ir);
    printf("    Igt=%22.15e\n", pOpInfo->Igt);
    printf("    Igi=%22.15e\n", pOpInfo->Igi);
    printf("    Qgf=%22.15e\n", pOpInfo->Qgf);
    printf("     Qd=%22.15e\n", pOpInfo->Qd);
    printf("     Qs=%22.15e\n", pOpInfo->Qs);
    printf("     Qb=%22.15e\n", pOpInfo->Qb);
    printf("    Qgb=%22.15e\n", pOpInfo->Qgb);
    printf("   gdgf=%22.15e\n", pOpInfo->dId_dVgf);
    printf("   gdd =%22.15e\n", pOpInfo->dId_dVd);
    printf("   gdb =%22.15e\n", pOpInfo->dId_dVb);
    printf("   gdgb=%22.15e\n", pOpInfo->dId_dVgb);
    printf("  giigf=%22.15e\n", pOpInfo->dIgi_dVgf- pOpInfo->dIgt_dVgf);
    printf("  giid =%22.15e\n", pOpInfo->dIgi_dVd - pOpInfo->dIgt_dVd);
    printf("  giib =%22.15e\n", pOpInfo->dIgi_dVb - pOpInfo->dIgt_dVb);
    printf("  giigb=%22.15e\n", pOpInfo->dIgi_dVgb- pOpInfo->dIgt_dVgb);
    printf("   girb=%22.15e\n", pOpInfo->dIr_dVb);
    printf("  cgfgf=%22.15e\n", pOpInfo->dQgf_dVgf);
    printf("  cgfd =%22.15e\n", pOpInfo->dQgf_dVd);
    printf("  cgfb =%22.15e\n", pOpInfo->dQgf_dVb);
    printf("  cgfgb=%22.15e\n", pOpInfo->dQgf_dVgb);
    printf("   cdgf=%22.15e\n", pOpInfo->dQd_dVgf);
    printf("   cdd =%22.15e\n", pOpInfo->dQd_dVd);
    printf("   cdb =%22.15e\n", pOpInfo->dQd_dVb);
    printf("   cdgb=%22.15e\n", pOpInfo->dQd_dVgb);
    printf("   cbgf=%22.15e\n", pOpInfo->dQb_dVgf);
    printf("   cbd =%22.15e\n", pOpInfo->dQb_dVd);
    printf("   cbb =%22.15e\n", pOpInfo->dQb_dVb);
    printf("   cbgb=%22.15e\n", pOpInfo->dQb_dVgb);
    printf("  cgbgf=%22.15e\n", pOpInfo->dQgb_dVgf);
    printf("  cgbd =%22.15e\n", pOpInfo->dQgb_dVd);
    printf("  cgbb =%22.15e\n", pOpInfo->dQgb_dVb);
    printf("  cgbgb=%22.15e\n", pOpInfo->dQgb_dVgb);
    printf("    DT =%22.15e\n", pInst->Temperature -  pEnv->Temperature);
    printf(" gpwrT =%22.15e\n", pOpInfo->dP_dT);
    printf("  gidT =%22.15e\n", pOpInfo->dId_dT);
    printf("  giiT =%22.15e\n", pOpInfo->dIgi_dT - pOpInfo->dIgt_dT);
    printf("  girT =%22.15e\n", pOpInfo->dIr_dT);
    printf("  cgfT =%22.15e\n", pOpInfo->dQgf_dT);
    printf("   cdT =%22.15e\n", pOpInfo->dQd_dT);
    printf("   cbT =%22.15e\n", pOpInfo->dQb_dT);
    printf("  cgbT =%22.15e\n", pOpInfo->dQgb_dT);
*/
    return (0);
}

int 
nfdEvalDeriv( Vds, Vgfs, Vbs, Vgbs, pOpInfo, pInst, pModel, pEnv,
	      DynamicNeeded, ACNeeded )                                          /* 4.5d */
double Vds, Vgfs, Vbs, Vgbs;
struct ufsAPI_InstData  *pInst;
struct ufsAPI_ModelData *pModel;
struct ufsAPI_OPData *pOpInfo;
struct ufsAPI_EnvData *pEnv;
int DynamicNeeded, ACNeeded;                                                     /* 4.5d */
{
register struct ufsTDModelData *pTempModel;                                      /* 4.5F */
struct ufsAPI_OPData OpInfog, OpInfod, OpInfob, OpInfobg, OpInfot;
double Idrain, Idrain1, Vtemp;
double q, esi;                                               /* 4.5F */
int SH;

    pTempModel = pInst->pTempModel;                                              /* 4.5F */
    q = ELECTRON_CHARGE;
    esi = SILICON_PERMITTIVITY;
    nfdEvalMod(Vds, Vgfs, Vbs, Vgbs, pOpInfo, pInst, pModel, pEnv,
	       DynamicNeeded, ACNeeded);                                         /* 7.0Y */
    if (!ACNeeded)                                                               /* 4.5d */
    {    Idrain = pOpInfo->Ich + pOpInfo->Ibjt;                                  /* 4.5d */
	 Vtemp = Vbs + DeltaV1;                                                  /* 5.0 */
         nfdEvalMod(Vds, Vgfs, Vtemp, Vgbs, &OpInfob, pInst, pModel, pEnv,       /* 5.0 */
	            DynamicNeeded, ACNeeded);                                    /* 7.0Y */
         Idrain1 = OpInfob.Ich + OpInfob.Ibjt;                                   /* 5.0 */
         pOpInfo->dId_dVb = (Idrain1 - Idrain) / DeltaV1;                        /* 5.0 */
         pOpInfo->dIgi_dVb = (OpInfob.Igi - pOpInfo->Igi) / DeltaV1;             /* 5.0 */
         pOpInfo->dIgt_dVb = (OpInfob.Igt - pOpInfo->Igt) / DeltaV1;             /* 5.0 */
         pOpInfo->dIr_dVb = (OpInfob.Ir - pOpInfo->Ir) / DeltaV1;                /* 5.0 */
         pOpInfo->dIgb_dVb = (OpInfob.Igb - pOpInfo->Igb) / DeltaV1;          /* 7.0Y */

	 pOpInfo->dP_dVgf = 0.0;                                                 /* 4.5d */
	 pOpInfo->dP_dVd = 0.0;                                                  /* 4.5d */
         pOpInfo->dP_dVb = (OpInfob.Power - pOpInfo->Power) / DeltaV1;           /* 5.0 */
	 pOpInfo->dP_dVgb = 0.0;                                                 /* 4.5d */
         if (DynamicNeeded)                                                      /* 5.0 */
	 {   pOpInfo->dQgf_dVb = (OpInfob.Qgf - pOpInfo->Qgf) / DeltaV1;         /* 5.0 */
	     pOpInfo->dQd_dVb = (OpInfob.Qd - pOpInfo->Qd) / DeltaV1;            /* 5.0 */
	     pOpInfo->dQb_dVb = (OpInfob.Qb - pOpInfo->Qb) / DeltaV1;            /* 5.0 */
	     pOpInfo->dQgb_dVb = (OpInfob.Qgb - pOpInfo->Qgb) / DeltaV1;         /* 5.0 */
	 }                                                                       /* 5.0 */
    }                                                                            /* 4.5d */
    else                                                                         /* 4.5d */
    {                                                                            /* 4.5d */
    Vtemp = Vgfs + DeltaV;
    nfdEvalMod(Vds, Vtemp, Vbs, Vgbs, &OpInfog, pInst, pModel, pEnv,
	       DynamicNeeded, ACNeeded);                                         /* 7.0Y */

    Vtemp = Vds + DeltaV;
    nfdEvalMod(Vtemp, Vgfs, Vbs, Vgbs, &OpInfod, pInst, pModel, pEnv,
	       DynamicNeeded, ACNeeded);                                         /* 7.0Y */
	   
    Vtemp = Vbs + DeltaV1;
    nfdEvalMod(Vds, Vgfs, Vtemp, Vgbs, &OpInfob, pInst, pModel, pEnv,
	       DynamicNeeded, ACNeeded);                                         /* 7.0Y */
	   
    Vtemp = Vgbs + DeltaV;
    nfdEvalMod(Vds, Vgfs, Vbs, Vtemp, &OpInfobg, pInst, pModel, pEnv,
	       DynamicNeeded, ACNeeded);                                         /* 7.0Y */

    Idrain = pOpInfo->Ich + pOpInfo->Ibjt;
    Idrain1 = OpInfog.Ich + OpInfog.Ibjt;
    pOpInfo->dId_dVgf = (Idrain1 - Idrain) / DeltaV;
    Idrain1 = OpInfod.Ich + OpInfod.Ibjt;
    pOpInfo->dId_dVd = (Idrain1 - Idrain) / DeltaV;
    Idrain1 = OpInfob.Ich + OpInfob.Ibjt;
    pOpInfo->dId_dVb = (Idrain1 - Idrain) / DeltaV1;
    Idrain1 = OpInfobg.Ich + OpInfobg.Ibjt;
    pOpInfo->dId_dVgb = (Idrain1 - Idrain) / DeltaV;

    pOpInfo->dIgi_dVgf = (OpInfog.Igi - pOpInfo->Igi) / DeltaV;
    pOpInfo->dIgi_dVd = (OpInfod.Igi - pOpInfo->Igi) / DeltaV;
    pOpInfo->dIgi_dVb = (OpInfob.Igi - pOpInfo->Igi) / DeltaV1;
    pOpInfo->dIgi_dVgb = (OpInfobg.Igi - pOpInfo->Igi) / DeltaV;

    pOpInfo->dIgt_dVgf = (OpInfog.Igt - pOpInfo->Igt) / DeltaV;
    pOpInfo->dIgt_dVd = (OpInfod.Igt - pOpInfo->Igt) / DeltaV;
    pOpInfo->dIgt_dVb = (OpInfob.Igt - pOpInfo->Igt) / DeltaV1;
    pOpInfo->dIgt_dVgb = (OpInfobg.Igt - pOpInfo->Igt) / DeltaV;

    pOpInfo->dIgb_dVgf = (OpInfog.Igb - pOpInfo->Igb) / DeltaV;                /* 7.0Y */
    pOpInfo->dIgb_dVd = (OpInfod.Igb - pOpInfo->Igb) / DeltaV;                 /* 7.0Y */
    pOpInfo->dIgb_dVb = (OpInfob.Igb - pOpInfo->Igb) / DeltaV1;                /* 7.0Y */
    pOpInfo->dIgb_dVgb = (OpInfobg.Igb - pOpInfo->Igb) / DeltaV;               /* 7.0Y */

    pOpInfo->dIr_dVb = (OpInfob.Ir - pOpInfo->Ir) / DeltaV1;
    pOpInfo->dP_dVgf = (OpInfog.Power - pOpInfo->Power) / DeltaV;
    pOpInfo->dP_dVd = (OpInfod.Power - pOpInfo->Power) / DeltaV;
    pOpInfo->dP_dVb = (OpInfob.Power - pOpInfo->Power) / DeltaV1;
    pOpInfo->dP_dVgb = (OpInfobg.Power - pOpInfo->Power) / DeltaV;

    if (DynamicNeeded)
    {   pOpInfo->dQgf_dVgf = (OpInfog.Qgf - pOpInfo->Qgf) / DeltaV;
	pOpInfo->dQgf_dVd = (OpInfod.Qgf - pOpInfo->Qgf) / DeltaV;
	pOpInfo->dQgf_dVb = (OpInfob.Qgf - pOpInfo->Qgf) / DeltaV1;
	pOpInfo->dQgf_dVgb = (OpInfobg.Qgf - pOpInfo->Qgf) / DeltaV;

	pOpInfo->dQd_dVgf = (OpInfog.Qd - pOpInfo->Qd) / DeltaV;
	pOpInfo->dQd_dVd = (OpInfod.Qd - pOpInfo->Qd) / DeltaV;
	pOpInfo->dQd_dVb = (OpInfob.Qd - pOpInfo->Qd) / DeltaV1;
	pOpInfo->dQd_dVgb = (OpInfobg.Qd - pOpInfo->Qd) / DeltaV;

	pOpInfo->dQb_dVgf = (OpInfog.Qb - pOpInfo->Qb) / DeltaV;
	pOpInfo->dQb_dVd = (OpInfod.Qb - pOpInfo->Qb) / DeltaV;
	pOpInfo->dQb_dVb = (OpInfob.Qb - pOpInfo->Qb) / DeltaV1;
	pOpInfo->dQb_dVgb = (OpInfobg.Qb - pOpInfo->Qb) / DeltaV;

	pOpInfo->dQgb_dVgf = (OpInfog.Qgb - pOpInfo->Qgb) / DeltaV;
	pOpInfo->dQgb_dVd = (OpInfod.Qgb - pOpInfo->Qgb) / DeltaV;
	pOpInfo->dQgb_dVb = (OpInfob.Qgb - pOpInfo->Qgb) / DeltaV1;
	pOpInfo->dQgb_dVgb = (OpInfobg.Qgb - pOpInfo->Qgb) / DeltaV;
    }
    }                                                                            /* 4.5d */

    if (pModel->Selft > 1)
    {   Vtemp = pInst->Temperature + 1.0;
	/* Add flag SH to prevent GdsTempFac and GbTempFac from being updated       4.5 */
	SH = 0;                                                                  /* 4.5 */
	ufsTempEffect(pModel, pInst, pEnv, Vtemp, SH);                           /* 4.5 */
	nfdEvalMod(Vds, Vgfs, Vbs, Vgbs, &OpInfot, pInst, pModel, pEnv,
		   DynamicNeeded, ACNeeded);                                     /* 7.0Y */
       	pInst->Temperature = Vtemp - 1.0;

	Idrain1 = OpInfot.Ich + OpInfot.Ibjt;
	pOpInfo->dP_dT = (OpInfot.Power - pOpInfo->Power);
	pOpInfo->dId_dT = (Idrain1 - Idrain);
	pOpInfo->dIgi_dT = (OpInfot.Igi - pOpInfo->Igi);
	pOpInfo->dIgt_dT = (OpInfot.Igt - pOpInfo->Igt);
        pOpInfo->dIgb_dT = (OpInfot.Igb - pOpInfo->Igb);                      /* 7.0Y */
	pOpInfo->dIr_dT = (OpInfot.Ir - pOpInfo->Ir);

	if (DynamicNeeded)
	{   pOpInfo->dQgf_dT = (OpInfot.Qgf - pOpInfo->Qgf);
	    pOpInfo->dQd_dT = (OpInfot.Qd - pOpInfo->Qd);
	    pOpInfo->dQb_dT = (OpInfot.Qb - pOpInfo->Qb);
	    pOpInfo->dQgb_dT = (OpInfot.Qgb - pOpInfo->Qgb);
	}
    }
/*
    printf("\n");
    printf("DynamicNeeded=%d\n", DynamicNeeded);
    printf("     ACNeeded=%d\n", ACNeeded);
    printf("   Vgfs=%22.15e\n", Vgfs);
    printf("    Vds=%22.15e\n", Vds);
    printf("    Vbs=%22.15e\n", Vbs);
    printf("    Ich=%22.15e\n", pOpInfo->Ich);
    printf("   Ibjt=%22.15e\n", pOpInfo->Ibjt);
    printf("     Ir=%22.15e\n", pOpInfo->Ir);
    printf("    Igt=%22.15e\n", pOpInfo->Igt);
    printf("    Igi=%22.15e\n", pOpInfo->Igi);
    printf("    Qgf=%22.15e\n", pOpInfo->Qgf);
    printf("    Qd =%22.15e\n", pOpInfo->Qd);
    printf("    Qs =%22.15e\n", pOpInfo->Qs);
    printf("    Qb =%22.15e\n", pOpInfo->Qb);
    printf("    Qgb=%22.15e\n", pOpInfo->Qgb);
    printf("   gdgf=%22.15e\n", pOpInfo->dId_dVgf);
    printf("   gdd =%22.15e\n", pOpInfo->dId_dVd);
    printf("   gdb =%22.15e\n", pOpInfo->dId_dVb);
    printf("  giigf=%22.15e\n", pOpInfo->dIgi_dVgf- pOpInfo->dIgt_dVgf);
    printf("  giid =%22.15e\n", pOpInfo->dIgi_dVd - pOpInfo->dIgt_dVd);
    printf("  giib =%22.15e\n", pOpInfo->dIgi_dVb - pOpInfo->dIgt_dVb);
    printf("  girb =%22.15e\n", pOpInfo->dIr_dVb);
    printf("  cgfgf=%22.15e\n", pOpInfo->dQgf_dVgf);
    printf("  cgfd =%22.15e\n", pOpInfo->dQgf_dVd);
    printf("  cgfb =%22.15e\n", pOpInfo->dQgf_dVb);
    printf("  cgfgb=%22.15e\n", pOpInfo->dQgf_dVgb);
    printf("   cdgf=%22.15e\n", pOpInfo->dQd_dVgf);
    printf("   cdd =%22.15e\n", pOpInfo->dQd_dVd);
    printf("   cdb =%22.15e\n", pOpInfo->dQd_dVb);
    printf("   cdgb=%22.15e\n", pOpInfo->dQd_dVgb);
    printf("   cbgf=%22.15e\n", pOpInfo->dQb_dVgf);
    printf("   cbd =%22.15e\n", pOpInfo->dQb_dVd);
    printf("   cbb =%22.15e\n", pOpInfo->dQb_dVb);
    printf("   cbgb=%22.15e\n", pOpInfo->dQb_dVgb);
    printf("  cgbgf=%22.15e\n", pOpInfo->dQgb_dVgf);
    printf("  cgbd =%22.15e\n", pOpInfo->dQgb_dVd);
    printf("  cgbb =%22.15e\n", pOpInfo->dQgb_dVb);
    printf("  cgbgb=%22.15e\n", pOpInfo->dQgb_dVgb);
    printf("    DT =%22.15e\n", pInst->Temperature -  pEnv->Temperature);
    printf(" gpwrT =%22.15e\n", pOpInfo->dP_dT);
    printf("  gidT =%22.15e\n", pOpInfo->dId_dT);
    printf("  giiT =%22.15e\n", pOpInfo->dIgi_dT - pOpInfo->dIgt_dT);
    printf("  girT =%22.15e\n", pOpInfo->dIr_dT);
    printf("  cgfT =%22.15e\n", pOpInfo->dQgf_dT);
    printf("   cdT =%22.15e\n", pOpInfo->dQd_dT);
    printf("   cbT =%22.15e\n", pOpInfo->dQb_dT);
    printf("  cgbT =%22.15e\n", pOpInfo->dQgb_dT);
*/
      return (0);
}
