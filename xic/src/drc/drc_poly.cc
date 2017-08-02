
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

#include "main.h"
#include "drc.h"
#include "dsp_layer.h"
#include "tech_layer.h"
#include "errorlog.h"


#ifndef M_PI
#define	M_PI  3.14159265358979323846  // pi
#endif

//
//  Some cDRC member functions which relate to polygons.
//


// Quickie test performed during round flash creation.
//
bool
cDRC::diskEval(int rx, int ry, const CDl *ld)
{
    for (DRCtestDesc *td = *tech_prm(ld)->rules_addr(); td; td = td->next()) {
        if (td->type() == drMinWidth && !td->hasRegion()) {
            int h = td->dimen();
            int r = (rx < ry ? rx : ry);
            if (2*r < h) {
                Log()->ErrorLog(mh::DRC,
                    "Disk diameter smaller than minimum dimension.");
                return (false);
            }
        }
    }
    return (true);
}


// Quickie test performed during donut creation.
//
bool
cDRC::donutEval(int r1x, int r1y, int r2x, int r2y, const CDl *ld)
{
    for (DRCtestDesc *td = *tech_prm(ld)->rules_addr(); td; td = td->next()) {
        if (td->type() == drMinWidth && !td->hasRegion()) {
            int dx = abs(r1x - r2x);
            int dy = abs(r1y - r2y);
            if (mmMin(dx, dy) < td->dimen()) {
                Log()->ErrorLog(mh::DRC,
                    "Donut width is smaller than minimum dimension.");
                return (false);
            }
        }
        else if (td->type() == drMinSpace && !td->hasRegion()) {
            int dx = mmMin(r1x, r2x);
            int dy = mmMin(r1y, r2y);
            if (2*mmMin(dx, dy) < td->dimen()) {
                Log()->ErrorLog(mh::DRC,
                    "Donut hole is smaller than minimum space.");
                return (false);
            }
        }
    }
    return (true);
}


// Quickie test performed during arc creation.
//
bool
cDRC::arcEval(int r1x, int r1y, int r2x, int r2y, double a1, double a2,
    const CDl *ld)
{
    int rx = mmMin(r1x, r2x);
    int ry = mmMin(r1y, r2y);
    if (rx == 0 || ry == 0)
        return (true);
    for (DRCtestDesc *td = *tech_prm(ld)->rules_addr(); td; td = td->next()) {
        if (td->type() == drMinWidth && !td->hasRegion()) {
            if (a1 < a2)
                a1 += 2*M_PI;
            if (a1 - a2 < M_PI/2) {
                double h = td->dimen();
                int x1 = (int)(rx*cos(a1));
                int y1 = (int)(ry*sin(a1));
                int x2 = (int)(rx*cos(a2));
                int y2 = (int)(ry*sin(a2));
                double r = (x1-x2)*(double)(x1-x2) + (y1-y2)*(double)(y1-y2);
                if (r < h*(double)h) {
                    Log()->ErrorLog(mh::DRC,
                        "Arc width is smaller than minimum dimension.");
                    return (false);
                }
            }
        }
        else if (td->type() == drMinSpace && !td->hasRegion()) {
            if (a1 < a2)
                a1 += 2*M_PI;
            if (a1 - a2 > 3*M_PI/2) {
                double h = td->dimen();
                int x1 = (int)(rx*cos(a1));
                int y1 = (int)(ry*sin(a1));
                int x2 = (int)(rx*cos(a2));
                int y2 = (int)(ry*sin(a2));
                double r = (x1-x2)*(double)(x1-x2) + (y1-y2)*(double)(y1-y2);
                if (r < h*(double)h) {
                    Log()->ErrorLog(mh::DRC,
                        "Arc gap is smaller than minimum dimension.");
                    return (false);
                }
            }
        }
    }
    return (true);
}

