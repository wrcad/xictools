
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
Authors: 1987 Wayne A. Christopher
         1992 Stephen R. Whiteley
****************************************************************************/

#include "simulator.h"
#include "graph.h"
#include "output.h"
#include "cshell.h"
#include "kwords_fte.h"

//
// Line-printer (ASCII) plots.
//


#define FUDGE        7
#define MARGIN_BASE  11
#define LCHAR        '.'
#define MCHAR        'X'
#define PCHARS       "+*=$%!0123456789"


namespace {
    // Figure out where a point should go, given the limits of the
    // plotting area and the type of scale (log or linear).
    //
    int findpoint(double pt, double *lims, int maxp, int minp, bool islog)
    {
        if (pt < lims[0])
            pt = lims[0];
        if (pt > lims[1])
            pt = lims[1];
        if (islog) {
            double tl = mylog10(lims[0]);
            double th = mylog10(lims[1]);
            return ((int)(((mylog10(pt) - tl)/(th - tl)) *
                (maxp - minp)) + minp);
        }
        return ((int)(((pt - lims[0])/(lims[1] - lims[0])) *
            (maxp - minp)) + minp);
    }
}


// We should really deal with the xlog and delta arguments.  This
// function is full of magic numbers that make the formatting correct.
//
void
SPgraphics::AsciiPlot(sDvList *dl0, const char *grp)
{
    sGrInit *gr = (sGrInit*)grp;

    // ANSI C does not specify how many digits are in an exponent for %c
    // We assumed it was 2.  If it's more, shift starting position over.
    //
    char buf[BSIZE_SP];
    int margin = MARGIN_BASE;
    int omargin = margin;
    sprintf(buf, "%1.1e", 0.0);        // expect 0.0e+00
    int shift = strlen(buf) - 7;
    margin += shift;

    sDataVec *xscale = dl0->dl_dvec->scale();
    sPlot *plot = dl0->dl_dvec->plot();

    // Make sure the margin is correct.
    bool novalue = Sp.GetVar(kw_noasciiplotvalue, VTYP_BOOL, 0);
    if (!novalue && !OP.vecEq(xscale, dl0->dl_dvec))
        margin *= 2;

    bool ylogscale = false;
    if ((xscale->gridtype() == GRID_YLOG) ||
            (xscale->gridtype() == GRID_LOGLOG))
        ylogscale = true;

    int maxy, height;
    TTY.winsize(&maxy, &height);
    maxy -= 2;  // This keeps line1/line2 below from being long enough to
                // befoul the line count in "more".
    bool nobreakp;
    if (Sp.GetFlag(FT_NOPAGE))
        nobreakp = true;
    else
        nobreakp = Sp.GetVar(kw_nobreak, VTYP_BOOL, 0);
    maxy -= (margin + FUDGE);
    int maxx = xscale->length();
    double xrange[2];
    xrange[0] = gr->xlims[0];
    xrange[1] = gr->xlims[1];
    double yrange[2];
    yrange[0] = gr->ylims[0];
    yrange[1] = gr->ylims[1];

    if (maxx <= 0) {
        GRpkg::self()->ErrPrintf(ET_WARN, "no points to plot.\n");
        return;
    }
    else if (maxx < 2) {
        GRpkg::self()->ErrPrintf(ET_ERROR,
            "asciiplot can't handle scale with length < 2.\n");
        return;
    }

    sDvList *dl;
    int i;
    for (dl = dl0, i = 0; dl; dl = dl->dl_next)
        dl->dl_dvec->set_linestyle((PCHARS[i] ? PCHARS[i++] : '#'));

    // Now allocate the field and stuff.
    char *field = new char[(maxy + 1)*(maxx + 1)];
    char *line1 = new char[maxy + margin + FUDGE + 10];
    char *line2 = new char[maxy + margin + FUDGE + 10];
    double *values = 0;
    if (!novalue)
        values = new double[maxx];
    
    // Clear the field, put the lines in the right places, and create
    // the headers.
    //
    int j;
    for (i = 0, j = (maxx + 1) * (maxy + 1); i < j; i++)
        field[i] = ' ';
    for (i = 0, j = maxy + margin + FUDGE; i < j; i++) {
        line1[i] = '-';
        line2[i] = ' ';
    }
    line1[j] = line2[j] = '\0';

    // The following is similar to the stuff in grid.cc
    if ((xrange[0] > xrange[1]) || (yrange[0] > yrange[1])) {
        GRpkg::self()->ErrPrintf(ET_INTERR,
            "ft_agraf: bad limits %g, %g, %g, %g.\n", 
            xrange[0], xrange[1], yrange[0], yrange[1]);
        return;
    }

    int mag;
    double tenpowmag;
    if (gr->ylims[1] == 0.0) {
        mag = (int) floor(mylog10(- gr->ylims[0]));
        tenpowmag = pow(10.0, (double) mag);
    }
    else if (gr->ylims[0] == 0.0) { 
        mag = (int) floor(mylog10(gr->ylims[1]));
        tenpowmag = pow(10.0, (double) mag);
    }
    else {
        double diff = gr->ylims[1] - gr->ylims[0];
        mag = (int) floor(mylog10(diff));
        tenpowmag = pow(10.0, (double) mag);
    }

    int lmt = (int) floor(gr->ylims[0] / tenpowmag);
    yrange[0] = gr->ylims[0] = lmt * tenpowmag;
    int hmt = (int) ceil(gr->ylims[1] / tenpowmag);
    yrange[1] = gr->ylims[1] = hmt * tenpowmag;

    int dst = hmt - lmt;
    // This is a strange case; I don't know why it's here
    if (dst == 11)
        dst = 12;

    if (dst == 1) {
        dst = 10;
        mag++;
        hmt *= 10;
        lmt *= 10;
    }

    int nsp;
    for (nsp = 4; nsp < 8; nsp++)
        if (!(dst % nsp))
            break;
    if (nsp == 8)
        for (nsp = 2; nsp < 4; nsp++)
            if (!(dst % nsp))
                break;
    int spacing = maxy/nsp;

    // Reset the max X coordinate to deal with round-off error.
    int omaxy = maxy + 1;
    maxy = spacing * nsp;

    for (i = 0, j = lmt; j <= hmt; i += spacing, j += dst/nsp) {
        for (int k = 0; k < maxx; k++)
            field[k*omaxy + i] = LCHAR;
        line1[i + margin + 2*shift] = '|';
        sprintf(buf, "%.2e", j*pow(10.0, (double)mag));
        memcpy(&line2[i + margin - ((j < 0) ? 2 : 1) - shift], buf,
                strlen(buf));
    }
    line1[i - spacing + margin + 1] = '\0';

    for (i = 1; i < omargin - 1 && xscale->name()[i - 1]; i++)
        line2[i] = xscale->name()[i - 1];
    if (!novalue)
        for (i = omargin + 1;
        i < margin - 2 && (dl0->dl_dvec->name()[i - omargin - 1]);
        i++)
            line2[i] = dl0->dl_dvec->name()[i - omargin - 1];

    bool xlog =
        ((gr->gridtype == GRID_XLOG) || (gr->gridtype == GRID_LOGLOG));

    // Now the buffers are all set up properly. Plot points for each
    // vector using interpolation. For each point on the x-axis, find the
    // two bracketing points in xscale, and then interpolate their
    // y values for each vector.

    int upper = 0,  lower = 0;
    for (i = 0; i < maxx; i++) {
        double x;
        if (gr->nointerp)
            x = xscale->realval(i);
        else if (xlog && xrange[0] > 0.0 && xrange[1] > 0.0)
            x = xrange[0]*pow(10.0, mylog10(xrange[1]/xrange[0])*i/(maxx - 1));
        else
            x = xrange[0] + (xrange[1] - xrange[0])*i/(maxx - 1);
        while (upper < xscale->length() - 1 && xscale->realval(upper) < x)
            upper++;
        while (lower < xscale->length() - 1 && xscale->realval(lower)  < x)
            lower++;
        if (xscale->realval(lower) > x && lower > 0)
            lower--;
        double x1 = xscale->realval(lower);
        double x2 = xscale->realval(upper);
        if (x1 > x2) {
            GRpkg::self()->ErrPrintf(ET_ERROR, "X scale (%s) not monotonic.\n", 
                xscale->name());
            return;
        }
        for (dl = dl0; dl; dl = dl->dl_next) {
            sDataVec *v = dl->dl_dvec;
            double yy1 = v->realval(lower);
            double y2 = v->realval(upper);
            double y;
            if (x1 == x2)
                y = yy1;
            else
                y = yy1 + (y2 - yy1)*(x - x1)/(x2 - x1);
            if (!novalue && (dl == dl0))
                values[i] = y;
            int ypt = findpoint(y, yrange, maxy, 0, ylogscale);
            char c = field[omaxy * i + ypt];
            if ((c == ' ') || (c == LCHAR))
                field[omaxy*i + ypt] = (char)v->linestyle();
            else
                field[omaxy*i + ypt] = MCHAR;
        }
    }

    TTY.init_more();
    for (i = 0; i < omaxy + margin; i++)
        TTY.send("-");
    TTY.send("\n");
    int curline = 6;
    i = (omaxy + margin - strlen(plot->title()))/2;
    while (i-- > 0)
        TTY.send(" ");
    strcpy(buf, plot->title());
    buf[maxy + margin] = '\0';  // Cut off if too wide
    TTY.send(buf);
    TTY.send("\n");
    curline++;
    sprintf(buf, "%s %s", plot->name(), plot->date());
    buf[maxy + margin] = '\0';
    i = (omaxy + margin - strlen(buf)) / 2;
    while (i-- > 0)
        TTY.send(" ");
    TTY.send(buf);
    TTY.send("\n\n");
    curline += 2;
    TTY.send("Legend:  ");
    i = 0;
    j = (maxx + margin - 8)/20;
    if (j == 0)
        j = 1;
    for (dl = dl0; dl; dl = dl->dl_next) {
        sDataVec *v = dl->dl_dvec;
        TTY.printf("%c = %-17s", (char)v->linestyle(), v->name());
        if (!(++i % j) && v->link()) {
            TTY.send("\n         ");
            curline++;
        }
    }
    TTY.send("\n");
    curline++;
    for (i = 0; i < omaxy + margin; i++)
        TTY.send("-");
    TTY.send("\n");
    curline++;
    i = 0;
    TTY.printf("%s\n%s\n", line2, line1);
    curline += 2;
    for (i = 0; i < maxx; i++) {
        double x;
        if (gr->nointerp)
            x = xscale->realval(i);
        else if (xlog && xrange[0] > 0.0 && xrange[1] > 0.0)
            x = xrange[0]*pow(10.0, mylog10(xrange[1]/xrange[0])*i/(maxx - 1));
        else
            x = xrange[0] + (xrange[1] - xrange[0])*i/(maxx - 1);
        if (x < 0.0)
            TTY.printf("%.3e ", x);
        else
            TTY.printf(" %.3e ", x);
        if (!novalue) {
            if (values[i] < 0.0)
                TTY.printf("%.3e ", values[i]);
            else
                TTY.printf(" %.3e ", values[i]);
        }
        char cb = field[(i + 1) * omaxy];
        field[(i + 1) * omaxy] = '\0';
        TTY.send(&field[i * omaxy]);
        field[(i + 1) * omaxy] = cb;
        TTY.send("\n");
        curline++;
        if (((curline % height) == 0) && (i < maxx - 1) && !nobreakp) {
//            TTY.printf("%s\n%s\n\014%s\n%s\n", line1, line2, line2, line1);
            TTY.printf("%s\n%s\n\n%s\n%s\n", line1, line2, line2, line1);
            curline = 7;
        }
    }
    TTY.printf("%s\n%s\n", line1, line2);

    delete [] field;
    delete [] line1;
    delete [] line2;
    if (!novalue)
        delete [] values;
}

