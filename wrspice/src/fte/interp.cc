
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
 * WRspice Circuit Simulation and Analysis Tool                           *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Wayne A. Christopher
         1992 Stephen R. Whiteley
****************************************************************************/

#include <math.h>
#include <string.h>
#include "ftedata.h"
#include "ttyio.h"
#include "graphics.h"

//#define INTERP_DEBUG


//
// Polynomial interpolation code.
//

// Interpolate data from oscale to nscale.  data is assumed to be olen
// long, ndata will be nlen long.  Returns false if the scales are too
// strange to deal with.  Note that we are guaranteed that either both
// scales are strictly increasing or both are strictly decreasing.
//
bool
sPoly::interp(double *data, double *ndata, double *oscale, int olen,
    double *nscale, int nlen)
{
    if (olen < 2 || nlen < 2) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "lengths too small to interpolate.\n");
        return (false);
    }
    if (pc_degree < 1 || pc_degree > olen) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "degree is %d, can't interpolate.\n",
            pc_degree);
        return (false);
    }

    int sz = pc_degree + 1;
    double *result = new double[sz + sz + sz];
    double *xdata = result + sz;
    double *ydata = xdata + sz;
    memcpy(ydata, data, sz*sizeof(double));
    memcpy(xdata, oscale, sz*sizeof(double));

    // Deal with the first degree pieces
    while (!polyfit(xdata, ydata, result)) {
        // If it doesn't work this time, bump the interpolation.
        // degree down by one.
        if (--pc_degree == 0) {
            GRpkgIf()->ErrPrintf(ET_INTERR, "sPoly::interp: #1.\n");
            delete [] result;
            return (false);
        }
    }

    // Add this part of the curve.  What we do is evaluate the
    // polynomial at those points between the last one and the one
    // that is greatest, without being greater than the leftmost old
    // scale point, or least if the scale is decreasing at the end of
    // the interval we are looking at.
    //
    int sign = (oscale[1] < oscale[0] ? -1 : 1);
    int lastone = -1;
    for (int i = 0; i < pc_degree; i++) {
        int end;
        // See how far we have to go
        for (end = lastone + 1; end < nlen; end++)
            if (nscale[end]*sign > xdata[i]*sign)
                break;
        end--;

        for (int j = lastone + 1; j <= end; j++) {
            if (nscale[j]*sign < oscale[0]*sign)
                ndata[j] = data[0];
            else
                ndata[j] = peval(nscale[j], result);
        }
        lastone = end;
    }

    // Now plot the rest, piece by piece.  l is the last element under
    // consideration.
    //
    for (int l = pc_degree + 1; l < olen; l++) {

        // Shift the old stuff by one and get another value
        int i;
        for (i = 0; i < pc_degree; i++) {
            xdata[i] = xdata[i + 1];
            ydata[i] = ydata[i + 1];
        }
        ydata[i] = data[l];
        xdata[i] = oscale[l];

        while (!polyfit(xdata, ydata, result)) {
            if (--pc_degree == 0) {
                GRpkgIf()->ErrPrintf(ET_INTERR, "sPoly::interp: #2.\n");
                delete [] result;
                return (false);
            }
        }
        int end;
        // See how far we have to go.
        for (end = lastone + 1; end < nlen; end++)
            if (nscale[end]*sign > xdata[i]*sign)
                break;
        end--;

        for (int j = lastone + 1; j <= end; j++) {
            if (nscale[j]*sign < oscale[0]*sign)
                ndata[j] = data[0];
            else
                ndata[j] = peval(nscale[j], result);
        }
        lastone = end;
    }

    // Extend last value if necessary.
    for (int j = lastone + 1; j < nlen; j++)
        ndata[j] = ndata[lastone];

    delete [] result;
    return (true);
}


#ifdef INTERP_DEBUG
namespace {
    void printmat(const char *name, double *mat, int m, int n)
    {
        printf("\n=== Matrix: %s ===\n", name);
        for (int i = 0; i < m; i++) {
            printf(" | ");
            for (int j = 0; j < n; j++)
                printf("%G ", mat[i * n + j]);
            printf("|\n");
        }
        printf("===\n");
    }
}
#endif


// Takes pc_degree+1 doubles, and fills in result with the
// coefficients of the polynomial that will fit them.
//
bool
sPoly::polyfit(double *xdata, double *ydata, double *result)
{
    int n = pc_degree + 1;
    double *mat1 = pc_scratch;
    double *mat2 = pc_scratch + n*n;
    memset(result, 0, n*sizeof(double));
    memset(mat1, 0, n*n*sizeof (double));
    memcpy(mat2, ydata, n*sizeof(double));

    // Fill in the matrix with x^k for 0 <= k <= degree for each point.
    int i, l = 0;
    for (i = 0; i < n; i++) {
        double d = 1.0;
        for (int j = 0; j < n; j++) {
            mat1[l] = d;
            d *= xdata[i];
            l += 1;
        }
    }

    // Do Gauss-Jordan elimination on mat1.
    for (i = 0; i < n; i++) {
        // choose largest pivot
        int j, lindex;
        double largest;
        for (j = i, largest = mat1[i*n + i], lindex = i; j < n; j++) {
            if (fabs(mat1[j*n + i]) > largest) {
                largest = fabs(mat1[j*n + i]);
                lindex = j;
            }
        }
        if (lindex != i) {
            // swap rows i and lindex
            for (int k = 0; k < n; k++) {
                double d = mat1[i*n + k];
                mat1[i*n + k] = mat1[lindex*n + k];
                mat1[lindex*n + k] = d;
            }
            double d = mat2[i];
            mat2[i] = mat2[lindex];
            mat2[lindex] = d;
        }
        // Make sure we have a non-zero pivot
        if (mat1[i*n + i] == 0.0) {
            // this should be rotated
            return (false);
        }
        for (j = i + 1; j < n; j++) {
            double d = mat1[j*n + i]/mat1[i*n + i];
            for (int k = 0; k < n; k++)
                mat1[j*n + k] -= d*mat1[i*n + k];
            mat2[j] -= d*mat2[i];
        }
    }

    for (i = n - 1; i > 0; i--)
        for (int j = i - 1; j >= 0; j--) {
            double d = mat1[j*n + i]/mat1[i*n + i];
            for (int k = 0; k < n; k++)
                mat1[j*n + k] -= 
                        d*mat1[i*n + k];
            mat2[j] -= d*mat2[i];
        }
    
    // Now write the stuff into the result vector.
    for (i = 0; i < n; i++) {
        result[i] = mat2[i]/mat1[i*n + i];
        // GRpkgIf()->ErrPrintf(ET_MSGS, "result[%d] = %G\n", i, result[i]);
    }

#define ABS_TOL 0.001
#define REL_TOL 0.001

    // Let's check and make sure the coefficients are ok.  If they
    // aren't, just return false.  This is not the best way to do it.
    //
    for (i = 0; i < n; i++) {
        double d = peval(xdata[i], result);
        if (fabs(d - ydata[i]) > ABS_TOL) {
#ifdef INTERP_DEBUG
            GRpkgIf()->ErrPrintf(ET_MSGS,
                "Error: polyfit: x = %le, y = %le, int = %le\n",
                xdata[i], ydata[i], d);
            printmat("mat1", mat1, n, n);
            printmat("mat2", mat2, n, 1);
#endif
            return (false);
        }
        else if (fabs(d - ydata[i]) / (fabs(d) > ABS_TOL ? fabs(d) :
                ABS_TOL) > REL_TOL) {
#ifdef INTERP_DEBUG
            GRpkgIf()->ErrPrintf(ET_MSGS,
                "Error: polyfit: x = %le, y = %le, int = %le\n",
                xdata[i], ydata[i], d);
            printmat("mat1", mat1, n, n);
            printmat("mat2", mat2, n, 1);
#endif
            return (false);
        }
    }
    return (true);
}


double
sPoly::peval(double x, double *coeffs, int degr)
{
    if (!coeffs)
        return (0.0);    // Should not happen
    if (!degr)
        degr = pc_degree;
    double y = coeffs[degr];  // there are (degr+1) coeffs
    for (int i = degr - 1; i >= 0; i--) {
        y *= x;
        y += coeffs[i];
    }
    return (y);
}

