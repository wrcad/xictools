
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
Authors: 1982 Giles Billingsley
         1986 Wayne A. Christopher
         1993 Stephen R. Whiteley
****************************************************************************/

#include "graph.h"


//
// Some functions to do clipping of polygons, etc to boxes.  Most of
// this code was rescued from MFB.
//

#define CODEMINX 1
#define CODEMINY 2
#define CODEMAXX 4
#define CODEMAXY 8
#define CODE(x,y,c)  c = 0;\
                     if (x < l)\
                         c = CODEMINX;\
                     else if (x > r)\
                         c = CODEMAXX;\
                     if (y < b)\
                         c |= CODEMINY;\
                     else if (y > t)\
                         c |= CODEMAXY;

// Static function.
// clip_line will clip a line to a rectangular area.  The returned
// value is 'true' if the line is out of the AOI (therefore does not
// need to be displayed) and 'false' if the line is in the AOI.
//
bool
sGraph::clip_line(int *pX1, int *pY1, int *pX2, int *pY2,
    int l, int b, int r, int t)
{
    int x1 = *pX1;
    int y1 = *pY1;
    int x2 = *pX2;
    int y2 = *pY2;
    int x=0, y=0;
    int c1, c2;
    CODE(x1, y1, c1)
    CODE(x2, y2, c2)
    while (c1 || c2) {
        if (c1 & c2)
            return (true); // Line is invisible
        int c;
        if (!(c = c1))
            c = c2;
        if (c & CODEMINX) {
            y = y1+(y2-y1)*(l-x1)/(x2-x1);
            x = l;
        }
        else if (c & CODEMAXX) {
            y = y1+(y2-y1)*(r-x1)/(x2-x1);
            x = r;
        }
        else if (c & CODEMINY) {
            x = x1+(x2-x1)*(b-y1)/(y2-y1);
            y = b;
        }
        else if (c & CODEMAXY) {
            x = x1+(x2-x1)*(t-y1)/(y2-y1);
            y = t;
        }
        if (c == c1) {
            x1 = x;
            y1 = y;
            CODE(x, y, c1)
        }
        else {
            x2 = x;
            y2 = y; 
            CODE(x, y, c2)
        }
    }
    *pX1 = x1;
    *pY1 = y1;
    *pX2 = x2;
    *pY2 = y2;
    return (false); // Line is at least partially visible
}


// Static function.
// This function will clip a line to a circle, returning true if the
// line is entirely outside the circle.  Note that we have to be
// careful not to switch the points around, since in grid.c we need to
// know which is the outer point for putting the label on.
//
bool
sGraph::clip_to_circle(int *x1, int *y1, int *x2, int *y2,
    int cx, int cy, int rad)
{
    // Get the angles between the origin and the endpoints
    double theta1;
    if ((*x1-cx) || (*y1-cy))
        theta1 = atan2((double) *y1 - cy, (double) *x1 - cx);
    else
        theta1 = M_PI;
    double theta2;
    if ((*x2-cx) || (*y2-cy))
        theta2 = atan2((double) *y2 - cy, (double) *x2 - cx);
    else
        theta2 = M_PI;

    if (theta1 < 0.0)
        theta1 = 2 * M_PI + theta1;
    if (theta2 < 0.0)
        theta2 = 2 * M_PI + theta2;

    double dtheta = theta2 - theta1;
    if (dtheta > M_PI)
        dtheta = dtheta - 2 * M_PI;
    else if (dtheta < - M_PI)
        dtheta = 2 * M_PI - dtheta;

    // Make sure that p1 is the first point
    bool flip = false;
    if (dtheta < 0) {
        double tt = theta1;
        theta1 = theta2;
        theta2 = tt;
        int i = *x1;
        *x1 = *x2;
        *x2 = i;
        i = *y1;
        *y1 = *y2;
        *y2 = i;
        flip = true;
        dtheta = -dtheta;
    }

    // Figure out the distances between the points.
    double a = sqrt((double)((*x1-cx)*(*x1-cx) + (*y1-cy)*(*y1-cy)));
    double b = sqrt((double)((*x2-cx)*(*x2-cx) + (*y2-cy)*(*y2-cy)));
    double c = sqrt((double)((*x1-*x2)*(*x1-*x2) + (*y1-*y2)*(*y1-*y2)));

    // We have three cases now -- either the midpoint of the line is
    // closest to the origon, or point 1 or point 2 is.  Actually the
    // midpoint won't in general be the closest, but if a point besides
    // one of the endpoints is closest, the midpoint will be closer than
    // both endpoints.
    //
    double tx = (*x1 + *x2)/2;
    double ty = (*y1 + *y2)/2;
    double dt = sqrt((double)((tx - cx)*(tx - cx) + (ty - cy)*(ty - cy)));
    double perplen;
    if ((dt < a) && (dt < b)) {
        // This is weird -- round-off errors I guess
        double tt = (a*a + c*c - b*b)/(2*a*c);
        if (tt > 1.0)
            tt = 1.0;
        else if (tt < -1.0)
            tt = -1.0;
        double alpha = acos(tt);
        perplen = a * sin(alpha);
    }
    else if (a < b)
        perplen = a;
    else
        perplen = b;

    // Now we should see if the line is outside of the circle.
    if (perplen >= rad)
        return (true);

    // It's at least partially inside.
    if (a > rad) {
        double tt = (a*a + c*c - b*b)/(2*a*c);
        if (tt > 1.0)
            tt = 1.0;
        else if (tt < -1.0)
            tt = -1.0;
        double alpha = acos(tt);
        double gamma = asin(sin(alpha)*a/rad);
        if (gamma < M_PI/2)
            gamma = M_PI - gamma;
        double beta = M_PI - alpha - gamma;
        *x1 = cx + (int)(rad*cos(theta1 + beta));
        *y1 = cy + (int)(rad*sin(theta1 + beta));
    }
    if (b > rad) {
        double tt = (c*c + b*b - a*a)/(2*b*c);
        if (tt > 1.0)
            tt = 1.0;
        else if (tt < -1.0)
            tt = -1.0;
        double alpha = acos(tt);
        double gamma = asin(sin(alpha)*b/rad);
        if (gamma < M_PI/2)
            gamma = M_PI - gamma;
        double beta = M_PI - alpha - gamma;
        *x2 = cx + (int)(rad*cos(theta2 - beta));
        *y2 = cy + (int)(rad*sin(theta2 - beta));
    }
    if (flip) {
        int i = *x1;
        *x1 = *x2;
        *x2 = i;
        i = *y1;
        *y1 = *y2;
        *y2 = i;
    }
    return (false);
}

