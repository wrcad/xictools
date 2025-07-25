
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

/**** BSIM4.3.0 Released by Xuemei(Jane) Xi 05/09/2003 ****/

/**********
 * Copyright 2003 Regents of the University of California. All rights reserved.
 * File: b4geo.c of BSIM4.3.0.
 * Author: 2000 Weidong Liu
 * Authors: 2001- Xuemei Xi, Jin He, Kanyu Cao, Mohan Dunga, Mansun Chan, Ali Niknejad, Chenming Hu.
 * Project Director: Prof. Chenming Hu.
 **********/

#include "b4defs.h"


#define MAX SPMAX

// SRW - replace all printfs
#define printf(...) DVO.pushMsg(__VA_ARGS__)

namespace {
    /*
     * WDLiu:
     * This subrutine is a special module to process the geometry dependent
     * parasitics for BSIM4, which calculates Ps, Pd, As, Ad, and Rs and  Rd
     * for multi-fingers and varous GEO and RGEO options.
     */

    int BSIM4NumFingerDiff(double nf, int minSD, double *nuIntD, double *nuEndD,
                       double *nuIntS, double *nuEndS)
    {
        int NF;
        NF = (int)nf;
        if ((NF%2) != 0)
        {
            *nuEndD = *nuEndS = 1.0;
            *nuIntD = *nuIntS = 2.0 * MAX((nf - 1.0) / 2.0, 0.0);
        }
        else
        {
            if (minSD == 1) /* minimize # of source */
            {
                *nuEndD = 2.0;
                *nuIntD = 2.0 * MAX((nf / 2.0 - 1.0), 0.0);
                *nuEndS = 0.0;
                *nuIntS = nf;
            }
            else
            {
                *nuEndD = 0.0;
                *nuIntD = nf;
                *nuEndS = 2.0;
                *nuIntS = 2.0 * MAX((nf / 2.0 - 1.0), 0.0);
            }
        }
        return 0;
    }


    int BSIM4RdsEndIso(double Weffcj, double Rsh, double DMCG, double DMCI,
                   double /*DMDG*/, double nuEnd, int rgeo, int Type, double *Rend)
    {
        if (Type == 1)
        {
            switch(rgeo)
            {
            case 1:
            case 2:
            case 5:
                if (nuEnd == 0.0)
                    *Rend = 0.0;
                else
                    *Rend = Rsh * DMCG / (Weffcj * nuEnd);
                break;
            case 3:
            case 4:
            case 6:
                if ((DMCG + DMCI) == 0.0)
                    printf("(DMCG + DMCI) can not be equal to zero\n");
                if (nuEnd == 0.0)
                    *Rend = 0.0;
                else
                    *Rend = Rsh * Weffcj / (3.0 * nuEnd * (DMCG + DMCI));
                break;
            default:
                printf("Warning: Specified RGEO = %d not matched\n", rgeo);
            }
        }
        else
        {
            switch(rgeo)
            {
            case 1:
            case 3:
            case 7:
                if (nuEnd == 0.0)
                    *Rend = 0.0;
                else
                    *Rend = Rsh * DMCG / (Weffcj * nuEnd);
                break;
            case 2:
            case 4:
            case 8:
                if ((DMCG + DMCI) == 0.0)
                    printf("(DMCG + DMCI) can not be equal to zero\n");
                if (nuEnd == 0.0)
                    *Rend = 0.0;
                else
                    *Rend = Rsh * Weffcj / (3.0 * nuEnd * (DMCG + DMCI));
                break;
            default:
                printf("Warning: Specified RGEO = %d not matched\n", rgeo);
            }
        }
        return 0;
    }


    int BSIM4RdsEndSha(double Weffcj, double Rsh, double DMCG, double /*DMCI*/,
                   double /*DMDG*/, double nuEnd, int rgeo, int Type, double *Rend)
    {
        if (Type == 1)
        {
            switch(rgeo)
            {
            case 1:
            case 2:
            case 5:
                if (nuEnd == 0.0)
                    *Rend = 0.0;
                else
                    *Rend = Rsh * DMCG / (Weffcj * nuEnd);
                break;
            case 3:
            case 4:
            case 6:
                if (DMCG == 0.0)
                    printf("DMCG can not be equal to zero\n");
                if (nuEnd == 0.0)
                    *Rend = 0.0;
                else
                    *Rend = Rsh * Weffcj / (6.0 * nuEnd * DMCG);
                break;
            default:
                printf("Warning: Specified RGEO = %d not matched\n", rgeo);
            }
        }
        else
        {
            switch(rgeo)
            {
            case 1:
            case 3:
            case 7:
                if (nuEnd == 0.0)
                    *Rend = 0.0;
                else
                    *Rend = Rsh * DMCG / (Weffcj * nuEnd);
                break;
            case 2:
            case 4:
            case 8:
                if (DMCG == 0.0)
                    printf("DMCG can not be equal to zero\n");
                if (nuEnd == 0.0)
                    *Rend = 0.0;
                else
                    *Rend = Rsh * Weffcj / (6.0 * nuEnd * DMCG);
                break;
            default:
                printf("Warning: Specified RGEO = %d not matched\n", rgeo);
            }
        }
        return 0;
    }
}


int
BSIM4dev::PAeffGeo(double nf, int geo, int minSD, double Weffcj, double DMCG,
                   double DMCI, double DMDG, double *Ps, double *Pd, double *As, double *Ad)
{
    double T0, T1, T2;
    double ADiso, ADsha, ADmer, ASiso, ASsha, ASmer;
    double PDiso, PDsha, PDmer, PSiso, PSsha, PSmer;
    double nuIntD = 0.0, nuEndD = 0.0, nuIntS = 0.0, nuEndS = 0.0;

    if (geo < 9) /* For geo = 9 and 10, the numbers of S/D diffusions already known */
        BSIM4NumFingerDiff(nf, minSD, &nuIntD, &nuEndD, &nuIntS, &nuEndS);

    T0 = DMCG + DMCI;
    T1 = DMCG + DMCG;
    T2 = DMDG + DMDG;

    PSiso = PDiso = T0 + T0 + Weffcj;
    PSsha = PDsha = T1;
    PSmer = PDmer = T2;

    ASiso = ADiso = T0 * Weffcj;
    ASsha = ADsha = DMCG * Weffcj;
    ASmer = ADmer = DMDG * Weffcj;

    switch(geo)
    {
    case 0:
        *Ps = nuEndS * PSiso + nuIntS * PSsha;
        *Pd = nuEndD * PDiso + nuIntD * PDsha;
        *As = nuEndS * ASiso + nuIntS * ASsha;
        *Ad = nuEndD * ADiso + nuIntD * ADsha;
        break;
    case 1:
        *Ps = nuEndS * PSiso + nuIntS * PSsha;
        *Pd = (nuEndD + nuIntD) * PDsha;
        *As = nuEndS * ASiso + nuIntS * ASsha;
        *Ad = (nuEndD + nuIntD) * ADsha;
        break;
    case 2:
        *Ps = (nuEndS + nuIntS) * PSsha;
        *Pd = nuEndD * PDiso + nuIntD * PDsha;
        *As = (nuEndS + nuIntS) * ASsha;
        *Ad = nuEndD * ADiso + nuIntD * ADsha;
        break;
    case 3:
        *Ps = (nuEndS + nuIntS) * PSsha;
        *Pd = (nuEndD + nuIntD) * PDsha;
        *As = (nuEndS + nuIntS) * ASsha;
        *Ad = (nuEndD + nuIntD) * ADsha;
        break;
    case 4:
        *Ps = nuEndS * PSiso + nuIntS * PSsha;
        *Pd = nuEndD * PDmer + nuIntD * PDsha;
        *As = nuEndS * ASiso + nuIntS * ASsha;
        *Ad = nuEndD * ADmer + nuIntD * ADsha;
        break;
    case 5:
        *Ps = (nuEndS + nuIntS) * PSsha;
        *Pd = nuEndD * PDmer + nuIntD * PDsha;
        *As = (nuEndS + nuIntS) * ASsha;
        *Ad = nuEndD * ADmer + nuIntD * ADsha;
        break;
    case 6:
        *Ps = nuEndS * PSmer + nuIntS * PSsha;
        *Pd = nuEndD * PDiso + nuIntD * PDsha;
        *As = nuEndS * ASmer + nuIntS * ASsha;
        *Ad = nuEndD * ADiso + nuIntD * ADsha;
        break;
    case 7:
        *Ps = nuEndS * PSmer + nuIntS * PSsha;
        *Pd = (nuEndD + nuIntD) * PDsha;
        *As = nuEndS * ASmer + nuIntS * ASsha;
        *Ad = (nuEndD + nuIntD) * ADsha;
        break;
    case 8:
        *Ps = nuEndS * PSmer + nuIntS * PSsha;
        *Pd = nuEndD * PDmer + nuIntD * PDsha;
        *As = nuEndS * ASmer + nuIntS * ASsha;
        *Ad = nuEndD * ADmer + nuIntD * ADsha;
        break;
    case 9: /* geo = 9 and 10 happen only when nf = even */
        *Ps = PSiso + (nf - 1.0) * PSsha;
        *Pd = nf * PDsha;
        *As = ASiso + (nf - 1.0) * ASsha;
        *Ad = nf * ADsha;
        break;
    case 10:
        *Ps = nf * PSsha;
        *Pd = PDiso + (nf - 1.0) * PDsha;
        *As = nf * ASsha;
        *Ad = ADiso + (nf - 1.0) * ADsha;
        break;
    default:
        printf("Warning: Specified GEO = %d not matched\n", geo);
    }
    return 0;
}


int
BSIM4dev::RdseffGeo(double nf, int geo, int rgeo, int minSD, double Weffcj,
                    double Rsh, double DMCG, double DMCI, double DMDG, int Type, double *Rtot)
{
    double Rint=0.0, Rend = 0.0;
    double nuIntD = 0.0, nuEndD = 0.0, nuIntS = 0.0, nuEndS = 0.0;

    if (geo < 9) /* since geo = 9 and 10 only happen when nf = even */
    {
        BSIM4NumFingerDiff(nf, minSD, &nuIntD, &nuEndD, &nuIntS, &nuEndS);

        /* Internal S/D resistance -- assume shared S or D and all wide contacts */
        if (Type == 1)
        {
            if (nuIntS == 0.0)
                Rint = 0.0;
            else
                Rint = Rsh * DMCG / ( Weffcj * nuIntS);
        }
        else
        {
            if (nuIntD == 0.0)
                Rint = 0.0;
            else
                Rint = Rsh * DMCG / ( Weffcj * nuIntD);
        }
    }

    /* End S/D resistance  -- geo dependent */
    switch(geo)
    {
    case 0:
        if (Type == 1) BSIM4RdsEndIso(Weffcj, Rsh, DMCG, DMCI, DMDG,
                                          nuEndS, rgeo, 1, &Rend);
        else           BSIM4RdsEndIso(Weffcj, Rsh, DMCG, DMCI, DMDG,
                                          nuEndD, rgeo, 0, &Rend);
        break;
    case 1:
        if (Type == 1) BSIM4RdsEndIso(Weffcj, Rsh, DMCG, DMCI, DMDG,
                                          nuEndS, rgeo, 1, &Rend);
        else           BSIM4RdsEndSha(Weffcj, Rsh, DMCG, DMCI, DMDG,
                                          nuEndD, rgeo, 0, &Rend);
        break;
    case 2:
        if (Type == 1) BSIM4RdsEndSha(Weffcj, Rsh, DMCG, DMCI, DMDG,
                                          nuEndS, rgeo, 1, &Rend);
        else           BSIM4RdsEndIso(Weffcj, Rsh, DMCG, DMCI, DMDG,
                                          nuEndD, rgeo, 0, &Rend);
        break;
    case 3:
        if (Type == 1) BSIM4RdsEndSha(Weffcj, Rsh, DMCG, DMCI, DMDG,
                                          nuEndS, rgeo, 1, &Rend);
        else           BSIM4RdsEndSha(Weffcj, Rsh, DMCG, DMCI, DMDG,
                                          nuEndD, rgeo, 0, &Rend);
        break;
    case 4:
        if (Type == 1) BSIM4RdsEndIso(Weffcj, Rsh, DMCG, DMCI, DMDG,
                                          nuEndS, rgeo, 1, &Rend);
        else           Rend = Rsh * DMDG / Weffcj;
        break;
    case 5:
        if (Type == 1) BSIM4RdsEndSha(Weffcj, Rsh, DMCG, DMCI, DMDG,
                                          nuEndS, rgeo, 1, &Rend);
        else           Rend = Rsh * DMDG / (Weffcj * nuEndD);
        break;
    case 6:
        if (Type == 1) Rend = Rsh * DMDG / Weffcj;
        else           BSIM4RdsEndIso(Weffcj, Rsh, DMCG, DMCI, DMDG,
                                          nuEndD, rgeo, 0, &Rend);
        break;
    case 7:
        if (Type == 1) Rend = Rsh * DMDG / (Weffcj * nuEndS);
        else           BSIM4RdsEndSha(Weffcj, Rsh, DMCG, DMCI, DMDG,
                                          nuEndD, rgeo, 0, &Rend);
        break;
    case 8:
        Rend = Rsh * DMDG / Weffcj;
        break;
    case 9: /* all wide contacts assumed for geo = 9 and 10 */
        if (Type == 1)
        {
            Rend = 0.5 * Rsh * DMCG / Weffcj;
            if (nf == 2.0)
                Rint = 0.0;
            else
                Rint = Rsh * DMCG / (Weffcj * (nf - 2.0));
        }
        else
        {
            Rend = 0.0;
            Rint = Rsh * DMCG / (Weffcj * nf);
        }
        break;
    case 10:
        if (Type == 1)
        {
            Rend = 0.0;
            Rint = Rsh * DMCG / (Weffcj * nf);
        }
        else
        {
            Rend = 0.5 * Rsh * DMCG / Weffcj;;
            if (nf == 2.0)
                Rint = 0.0;
            else
                Rint = Rsh * DMCG / (Weffcj * (nf - 2.0));
        }
        break;
    default:
        printf("Warning: Specified GEO = %d not matched\n", geo);
    }

    if (Rint <= 0.0)
        *Rtot = Rend;
    else if (Rend <= 0.0)
        *Rtot = Rint;
    else
        *Rtot = Rint * Rend / (Rint + Rend);
    if(*Rtot==0.0)
        printf("Warning: Zero resistance returned from RdseffGeo\n");
    return 0;
}

