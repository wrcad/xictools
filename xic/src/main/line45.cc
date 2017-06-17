
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2007 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  WHITELEY RESEARCH INCORPORATED PROPRIETARY SOFTWARE                   *
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
 *                                                                        *
 * XIC Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id: line45.cc,v 5.8 2017/03/14 01:26:47 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "events.h"
#include "editif.h"
#include "tech.h"


// Changes one end of a line to make it vertical, horizontal, or diagonal.
//
//    x1, y1 = coordinates of one end of line (fixed)
//    x2, y2 = pointers to coordinates of other end of line (movable)
//
// New end point will have the same x or y coordinate as old point
//
void
cMain::To45(int x1, int y1, int *x2, int *y2)
{
    int d[4];
    d[2] = x1 - *x2;
    d[0] = y1 - *y2;
    d[1] = (*y2 - y1) - (*x2 - x1);
    d[3] = (*y2 - y1) + (*x2 - x1);

    if (d[0]*d[1]*d[2]*d[3] == 0) return;
    int c = 0;
    for (int i = 1; i <= 3; i++)
        if (abs(d[i]) < abs(d[c])) c = i;

    switch (c) {
    case 0:
        *y2 = y1;
        break;
    case 2:
        *x2 = x1;
        break;
    case 1:
        if (d[1] > 0) {
            if (x1 > *x2) *x2 = *x2 + d[1];
            else *y2 = *y2 - d[1];
        }
        else {
            if (x1 > *x2) *y2 = *y2 - d[1];
            else *x2 = *x2 + d[1];
        }
        break;
    case 3:
        if (d[3] > 0) {
            if (x1 > *x2) *y2 = *y2 - d[3];
            else *x2 = *x2 - d[3];
        }
        else {
            if (x1 > *x2) *x2 = *x2 - d[3];
            else *y2 = *y2 - d[3];
        }
        break;
    }
}


#define DELSLP 0.05

// Snap the endpoint to a 45 multiple if the angle is close
//
bool
cMain::To45snap(int *x, int *y, int refx, int refy)
{
    int dx = abs(*x - refx);
    int dy = abs(*y - refy);
    if (!dx && !dy)
        return (false);
    if (EV()->IsConstrained())
        // reverse logic - no constraint if set
        return (true);
    else if (Tech()->IsConstrain45()) {
        To45(refx, refy, x, y);
        return (true);
    }
    if (!dx || (double)dx/dy < DELSLP)
        *x = refx;
    else if (!dy || (double)dy/dx < DELSLP)
        *y = refy;
    else {
        double d = (double)dx/dy - 1.0;
        if (d > -2*DELSLP && d < 2*DELSLP) {
            if (dx < dy)
                *y = refy + ((*y - refy)/dx)*dx;
            else
                *x = refx + ((*x - refx)/dy)*dy;
        }
    }
    return (true);
}


// Return true if x1,y1 and x2,y2 are in vertical or horizontal
// alignment.
//
bool
cMain::IsManhattan(int x1, int y1, int x2, int y2)
{
    if (x1 == x2 || y1 == y2)
        return (true);
    return (false);
}

