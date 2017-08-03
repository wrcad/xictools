
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
STAG version 2.6
Copyright 2000 owned by the United Kingdom Secretary of State for Defence
acting through the Defence Evaluation and Research Agency.
Developed by :     Jim Benson,
                   Department of Electronics and Computer Science,
                   University of Southampton,
                   United Kingdom.
With help from :   Nele D'Halleweyn, Bill Redman-White, and Craig Easson.

Based on STAG version 2.1
Developed by :     Mike Lee,
With help from :   Bernard Tenbroek, Bill Redman-White, Mike Uren, Chris Edwards
                   and John Bunyan.
Acknowledgements : Rupert Howes and Pete Mole.
**********/

#include "soi3defs.h"

#define SOI3nextModel      next()
#define SOI3nextInstance   next()
#define SOI3instances      inst()
#define CKTterr(a, b, c) (b)->terr(a, c)


int
SOI3dev::trunc(sGENmodel *genmod, sCKT *ckt, double *timeStep)
{
    sSOI3model *model = static_cast<sSOI3model*>(genmod);
    sSOI3instance *here;

    for( ; model != NULL; model = model->SOI3nextModel)
    {
        for(here=model->SOI3instances;here!=NULL;here = here->SOI3nextInstance)
        {
#ifdef SIMETRIX_VERSION
            /* NTL modifications 27.4.98 add last parameter - (not actually used in DDI version) */
            CKTterr(here->SOI3qgf,ckt,timeStep,0);
            CKTterr(here->SOI3qd,ckt,timeStep,0);
            CKTterr(here->SOI3qs,ckt,timeStep,0);
            /* end NTL */
#else /* SIMETRIX_VERSION */
            CKTterr(here->SOI3qgf,ckt,timeStep);
            CKTterr(here->SOI3qd,ckt,timeStep);
            CKTterr(here->SOI3qs,ckt,timeStep);
#endif /* SIMETRIX_VERSION */
        }
    }
    return(OK);
}

