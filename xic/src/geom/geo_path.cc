
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
 * Xic Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "geo.h"
#include "geo_line.h"


// Static function.
// Return true if the path intersects the box.  If touchok is false, a
// point on the line must be in the interior of BB.  If BB has no
// interior, i.e. its width or height < 2, touchok is ignored.
//
// Note:  this should work with BB in window or inverted viewport
// (BB->top < BB->bottom) coordinates.
//
bool
cGEO::path_box_intersect(const Point *pts, int num, const BBox *BB,
    bool touchok)
{
    BBox tBB(*BB);
    if (!touchok && tBB.width() > 1 && abs(tBB.height()) > 1)
        tBB.bloat(-1);
    for (pts++, num--; num; pts++, num--) {
        int x1 = (pts-1)->x;
        int y1 = (pts-1)->y;
        int x2 = pts->x;
        int y2 = pts->y;
        // This returns false if any point of the line is within or
        // on the boundary of the box.
        if (!line_clip(&x1, &y1, &x2, &y2, &tBB))
            return (true);
    }
    return (false);
}


// Static function.
// Return true if the paths intersect.  If touchok is not true, the
// paths must actually cross, and not just touch at an endpoint or
// share colinear pieces.
//
bool
cGEO::path_path_intersect(const Point *pts1, int num1, const Point *pts2,
    int num2, bool touchok)
{
    for (pts1++, num1--; num1; pts1++, num1--) {
        sLine line1 = sLine(pts1-1, pts1);
        int n = num2;
        const Point *p = pts2;
        for (p++, n--; n; p++, n--) {
            sLine line2 = sLine(p-1, p);
            if (touchok) {
                if (line1.lineIntersect(&line2, true))
                    return (true);
            }
            else {
                // note: this doesn't work as advertised.  true is
                // returned if a vertex of l2 falls on l1
                int code;
                if (line1.lineIntersect(&line2, &code))
                    return (true);
                if (code)
                    return (true);
            }
        }
    }
    return (false);
}

