
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
Authors: 1985 Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/

#include "simulator.h"
#include "circuit.h"
#include "ttyio.h"
#include "sparse/spmatrix.h"
#include "ginterf/graphics.h"


//  Initialize the numerical iteration package.
//
int
sCKT::NIinit()
{
    CKTniState = NIUNINITIALIZED;

    // The spMatrixFrame is created here as a real matrix.  Call spSetComplex
    // after the initial DCOP for ACAN and similar.

    int flags = 0;
    if (CKTcurTask) {
        if (CKTcurTask->TSKnoKLU)
            flags |= SP_NO_KLU;
        if (CKTcurTask->TSKnoMatrixSort)
            flags |= SP_NO_SORT;
        if (CKTcurTask->TSKextPrec)
            flags |= SP_EXT_PREC;
        if (Sp.GetTranTrace() || Sp.GetFlag(FT_SIMDB))
            flags |= SP_TRACE;
        if (!CKTcurTask->TSKtranslate)
            flags |= SP_NOMAPTR;
        if (CKTcurTask->TSKnocacheelts)
            flags |= SP_NOCACHEELTS;
    }
    CKTmatrix = new spMatrixFrame(0, flags);
    return (CKTmatrix->spError());
}


// Print the matrix in human readable form.  This will use the WRspice
// TTY system.
//
void
sCKT::NIprint(bool reordered, bool data, bool header)
{

    if (CKTmatrix && CKTmatrix->spGetSize(1) > 0)
        CKTmatrix->spPrint(reordered, data, header);
    else
        TTY.err_printf("Matrix not found.\n");
}


// A debugging aid, dump the matrix in human-readable form to a file
// whose name is passed.
//
void
sCKT::NIdbgPrint(bool reordered, bool data, bool header, const char *fname)
{
    if (!fname)
        fname = "wrspice.matrix";
    FILE *fp = fopen(fname, "w");
    if (fp) {
        if (CKTmatrix && CKTmatrix->spGetSize(1) > 0)
            CKTmatrix->spPrint(reordered, data, header, fp);
        else
            fprintf(fp, "Matrix not found.\n");
        fclose(fp);
    }
    else
        TTY.err_printf("Can't open file %s.\n", fname);
}


namespace {
    void dbl_realloc(double **d, int sz)
    {
        delete [] *d;
        if (sz > 0) {
            *d = new double[sz];
            memset(*d, 0, sz*sizeof(double));
        }
        else
            *d = 0;
    }
}


//  Perform reinitialization necessary for the numerical iteration
//  package - the matrix has now been fully accessed once, so we know
//  how big it is, so allocate RHS vector.
//
int
sCKT::NIreinit()
{
    int size = CKTmatrix->spGetSize(1);
    int nsize = CKTnodeTab.numNodes() - 1;
    if (size < nsize) {
        GRpkg::self()->ErrPrintf(ET_ERROR,
            "node table and matrix sizes are inconsistent (%d, %d).\n"
            "Something is wrong with the circuit topology.\n", nsize, size);
        return (E_BADMATRIX);
    }

    size++;
    dbl_realloc(&CKTrhs, size);
    dbl_realloc(&CKTrhsOld, size);
    dbl_realloc(&CKTrhsSpare, size);
    dbl_realloc(&CKTirhs, size);
    dbl_realloc(&CKTirhsOld, size);
    dbl_realloc(&CKTirhsSpare, size);
    for (int i = 0; i < 8; i++)
        dbl_realloc(CKTsols + i, size);

    CKTniState = NISHOULDREORDER | NIACSHOULDREORDER | NIPZSHOULDREORDER;
    return (OK);
}


// Delete the data structures allocated for numeric integration.
//
void
sCKT::NIdestroy()
{
    delete CKTmatrix;
    CKTmatrix = 0;

    dbl_realloc(&CKTrhs, 0);
    dbl_realloc(&CKTrhsOld, 0);
    dbl_realloc(&CKTrhsSpare, 0);
    dbl_realloc(&CKTirhs, 0);
    dbl_realloc(&CKTirhsOld, 0);
    dbl_realloc(&CKTirhsSpare, 0);
    for (int i = 0; i < 8; i++)
        dbl_realloc(CKTsols + i, 0);
    dbl_realloc(&CKToldSol, 0);
    dbl_realloc(&CKToldState0, 0);
}

