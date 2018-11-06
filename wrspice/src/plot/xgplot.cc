
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
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1988 Jeffrey M. Hsu
         1992 Stephen R. Whiteley
****************************************************************************/

#include "graph.h"
#include "cshell.h"
#include "kwords_fte.h"
#include "frontend.h"

#define XG_MAXVECTORS 64


// Produce output for the xgraph program.
//
void
SPgraphics::Xgraph(sDvList *dl0, sGrInit *gr)
{
    // Sanity checking
    int numVecs;
    sDvList *dl;
    for (dl = dl0, numVecs = 0; dl; dl = dl->dl_next)
        numVecs++;
    if (numVecs == 0)
        return;
    else if (numVecs > XG_MAXVECTORS) {
        TTY.err_printf("Error: too many vectors for Xgraph.\n");
        return;
    }
    int linewidth = 1;
    VTvalue vv;
    if (Sp.GetVar(kw_xglinewidth, VTYP_NUM, &vv) && vv.get_int() > 0)
        linewidth = vv.get_int();
    bool markers = Sp.GetVar(kw_xgmarkers, VTYP_BOOL, 0);

    // Make sure the gridtype is supported
    bool xlog, ylog;
    switch (gr->gridtype) {
    case GRID_LIN:
        xlog = ylog = false;
        break;
    case GRID_XLOG:
        xlog = true;
        ylog = false;
        break;
    case GRID_YLOG:
        ylog = true;
        xlog = false;
        break;
    case GRID_LOGLOG:
        xlog = ylog = true;
        break;
    default:
        TTY.err_printf("Error: grid type unsupported by Xgraph.\n");
        return;
    }

    // Open the output file
    FILE *file = fopen(gr->hcopy, "w");
    if (!file) {
        GRpkgIf()->Perror(gr->hcopy);
        return;
    }

    // Set up the file header
    if (gr->plotname)
        fprintf(file, "TitleText: %s\n", gr->plotname);
    if (gr->xlabel)
        fprintf(file, "XUnitText: %s\n", gr->xlabel);
    if (gr->ylabel)
        fprintf(file, "YUnitText: %s\n", gr->ylabel);

    if (gr->nogrid)
        fprintf(file, "Ticks: True\n");
    if (xlog) {
        fprintf( file, "LogX: True\n" );
        fprintf(file, "XLowLimit:  %e\n", log10(gr->xlims[0]));
        fprintf(file, "XHighLimit: %e\n", log10(gr->xlims[1]));
    }
    else {
        fprintf(file, "XLowLimit:  %e\n", gr->xlims[0]);
        fprintf(file, "XHighLimit: %e\n", gr->xlims[1]);
    }
    if (ylog) {
        fprintf( file, "LogY: True\n" );
        fprintf(file, "YLowLimit:  %e\n", log10(gr->ylims[0]));
        fprintf(file, "YHighLimit: %e\n", log10(gr->ylims[1]));
    }
    else {
        fprintf(file, "YLowLimit:  %e\n", gr->ylims[0]);
        fprintf(file, "YHighLimit: %e\n", gr->ylims[1]);
    }
    fprintf(file, "LineWidth: %d\n", linewidth);
    fprintf(file, "BoundBox: True\n");
    if (gr->plottype == PLOT_COMB) {
        fprintf(file, "BarGraph: True\n");
        fprintf(file, "NoLines: True\n");
    }
    else if (gr->plottype == PLOT_POINT) {
        if (markers)
            fprintf(file, "Markers: True\n");
        else
            fprintf(file, "LargePixels: True\n");
        fprintf( file, "NoLines: True\n" );
    }

    // Write out the data
    for (dl = dl0; dl; dl = dl->dl_next) {
        sDataVec *v = dl->dl_dvec;
        sDataVec *scale = v->scale();
        if (v->name())
            fprintf(file, "\"%s\"\n", v->name());
        for (int i = 0; i < scale->length(); i++) {
            double xval = scale->realval(i);
            double yval = v->realval(i);
            fprintf(file, "% e % e\n", xval, yval);
        }
        fprintf(file, "\n");
    }
    fclose(file);
    char buf[BSIZE_SP];
    sprintf(buf, "xgraph %s &", gr->hcopy);
    CP.System(buf);
}

