
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
 * WRspice Circuit Simulation and Analysis Tool                           *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: UCB CAD Group
         1993 Stephen R. Whiteley
****************************************************************************/

#include "pzdefs.h"
#include "device.h"
#include "spmatrix.h"


int
sPZAN::PZload(sCKT *ckt, IFcomplex *s)
{
    sPZAN *pzan = static_cast<sPZAN*>(ckt->CKTcurJob);
    int size = ckt->CKTmatrix->spGetSize(1);
    for (int i = 0; i <= size; i++) {
        ckt->CKTrhs[i] = 0.0;
        ckt->CKTirhs[i] = 0.0;
    }

    ckt->CKTmatrix->spSetComplex();
    if (ckt->CKTmatrix->spDataAddressChange()) {
        int error = ckt->resetup();
        if (error)
            return (error);
    }
    ckt->CKTmatrix->spClear();
    sCKTmodGen mgen(ckt->CKTmodels);
    for (sGENmodel *m = mgen.next(); m; m = mgen.next()) {
        int error = DEV.device(m->GENmodType)->pzLoad(m, ckt, s);
        if (error)
            return (error);
    }

#ifdef notdef
    printf("*** Before PZ adjustments *\n");
    ckt->CKTmatrix->spPrint(0, 1, 1);
#endif

    if (pzan->PZbalance_col && pzan->PZsolution_col)
        ckt->CKTmatrix->spAddCol(pzan->PZbalance_col, pzan->PZsolution_col);
        // AC sources ?? XXX

    if (pzan->PZsolution_col)
        ckt->CKTmatrix->spZeroCol(pzan->PZsolution_col);

    // Driving function (current source)
    if (pzan->PZdrive_pptr)
        *pzan->PZdrive_pptr = 1.0;
    if (pzan->PZdrive_nptr)
        *pzan->PZdrive_nptr = -1.0;

#ifdef notdef
    printf("*** After PZ adjustments *\n");
    ckt->CKTmatrix->spPrint(0, 1, 1);
#endif

    return(OK);
}
