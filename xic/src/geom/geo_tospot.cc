
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
 * Xic Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "geo.h"
#include "geo_box.h"


// Set the vertices to the center of the grid cells implied by the
// geoSpotSize.
//
void
cGEO::setToSpot(Point *pts, int *numpts) const
{
    if (geoSpotSize <= 0)
        return;

    // compute bounding box
    BBox BB;
    BB.left = BB.right = pts[0].x;
    BB.bottom = BB.top = pts[0].y;
    for (int i = 1; i < *numpts; i++) {
        if (pts[i].x < BB.left)
            BB.left = pts[i].x;
        if (pts[i].x > BB.right)
            BB.right = pts[i].x;
        if (pts[i].y < BB.bottom)
            BB.bottom = pts[i].y;
        if (pts[i].y > BB.top)
            BB.top = pts[i].y;
    }
    int cx = BB.left + BB.right;
    int cy = BB.bottom + BB.top;
    int del = geoSpotSize/2;

    // Set edges to nearest spot boundary
    BB.left = to_spot(BB.left);
    BB.right = to_spot(BB.right);
    BB.bottom = to_spot(BB.bottom);
    BB.top = to_spot(BB.top);

    // compute offsets for centering object
    int dx = (BB.left + BB.right - cx)/2;
    int dy = (BB.bottom + BB.top - cy)/2;

    // shrink box by half a spot
    BB.bloat(-del);

    // clip each vertex to the box, and snap to spot
    for (int i = 0; i < *numpts; i++) {
        pts[i].x += dx;
        pts[i].y += dy;
        if (pts[i].x < BB.left)
            pts[i].x = BB.left;
        else if (pts[i].x > BB.right)
            pts[i].x = BB.right;
        if (pts[i].y < BB.bottom)
            pts[i].y = BB.bottom;
        else if (pts[i].y > BB.top)
            pts[i].y = BB.top;
        pts[i].x = (pts[i].x/geoSpotSize)*geoSpotSize +
            (pts[i].x >= 0 ? del : -del);
        pts[i].y = (pts[i].y/geoSpotSize)*geoSpotSize +
            (pts[i].y >= 0 ? del : -del);
    }

    // remove duplicate vertices
    Point::removeDups(pts, numpts);
}

