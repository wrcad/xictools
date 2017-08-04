
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

#include "main.h"
#include "drc.h"
#include "drc_kwords.h"
#include "dsp_layer.h"
#include "dsp_inlines.h"
#include "geo_ylist.h"
#include "geo_polyobj.h"
#include "tech.h"
#include "tech_layer.h"


// Return a list of zoids in zret that represents the figure in od
// expanded or shrunk by dimen.  This uses the DRC test regions to
// either augment the figure if expanding or clip the figure if
// shrinking.  If edgonly is true, return the constructed areas only.
//
XIrt
DRCtestDesc::bloat(const CDo *od, Zlist **zret, bool edgonly)
{
    *zret = 0;
    if (!od || od->type() == CDLABEL || od->type() == CDINSTANCE)
        return (XIok);
    if (td_u.dimen == 0 && td_testdim == 0) {
        *zret = od->toZlist();
        return (XIok);
    }
    PolyObj spo(od, false);
    DRCedgeEval ev(&spo);
    if (!ev.epoly())
        return (XIbad);

    while (ev.advance()) {

        XIrt ret = initEdgeList(&ev);
        if (ret != XIok)
            return (ret);

        ret = polyEdgesTest(&ev, 0);
        if (ret != XIok)
            return (ret);
    }
    if (edgonly) {
        *zret = ev.accumZlist();
        return (XIok);
    }

    if (td_where == DRCoutside) {
        Zlist *z0 = 0;
        if (spo.zlist()) {
            z0 = Zlist::copy(spo.zlist());
            Zlist *ze = z0;
            while (ze->next)
                ze = ze->next;
            ze->next = ev.accumZlist();
        }
        else
            z0 = ev.accumZlist();
        z0 = Zlist::repartition_ni(z0);
        *zret = z0;
        return (XIok);
    }
    Zlist *zc = ev.accumZlist();
    Zlist *zlist = Zlist::copy(spo.zlist());
    XIrt ret = Zlist::zl_andnot(&zlist, zc);
    *zret = zlist;
    return (ret);
}


namespace {
    // Returns 0 - 359, monotonically increasing with angle measured counter-
    // clockwise from x axis.  Faster than a call to a trig function.  Return
    // value is accurate angle at multiples of 45 degrees.
    //
    int
    ang(const Point *p1, const Point *p2)
    {
        int dx = p2->x - p1->x;
        int dy = p2->y - p1->y;
        int adx = dx > 0 ? dx : -dx;
        int ady = dy > 0 ? dy : -dy;
        int a = (90*dy)/(adx + ady);
        if (dx < 0)
            a = 180 - a;
        else if (dy < 0)
            a = 360 + a;
        return (a);
    }
}


// Return the angle outside of the geometry at the corner p2.
//
int
cDRC::outsideAngle(const Point *p1, const Point *p2, const Point *p3, bool cw)
{
    int da;
    if (cw)
        da = ang(p1, p2) - ang(p3, p2);
    else
        da = ang(p3, p2) - ang(p1, p2);
    if (da < 0)
        da += 360;
    else if (da >= 360)
        da -= 360;
    return (da);
}
// End of cDRC functions.


// Static function.
// Return the keyword of the rule type.
//
const char *
DRCtestDesc::ruleName(DRCtype tp)
{
    switch (tp) {
    case drNoRule:
        break;
    case drConnected:
        return (Dkw.Connected());
    case drNoHoles:
        return (Dkw.NoHoles());
    case drExist:
        return (Dkw.Exist());
    case drOverlap:
        return (Dkw.Overlap());
    case drIfOverlap:
        return (Dkw.IfOverlap());
    case drNoOverlap:
        return (Dkw.NoOverlap());
    case drAnyOverlap:
        return (Dkw.AnyOverlap());
    case drPartOverlap:
        return (Dkw.PartOverlap());
    case drAnyNoOverlap:
        return (Dkw.AnyNoOverlap());
    case drMinEdgeLength:
        return (Dkw.MinEdgeLength());
    case drMaxWidth:
        return (Dkw.MaxWidth());
    case drMinWidth:
        return (Dkw.MinWidth());
    case drMinSpace:
        return (Dkw.MinSpace());
    case drMinArea:
        return (Dkw.MinArea());
    case drMaxArea:
        return (Dkw.MaxArea());
    case drMinSpaceTo:
        return (Dkw.MinSpaceTo());
    case drMinSpaceFrom:
        return (Dkw.MinSpaceFrom());
    case drMinOverlap:
        return (Dkw.MinOverlap());
    case drMinNoOverlap:
        return (Dkw.MinNoOverlap());
    case drUserDefinedRule:
        return (Dkw.UserDefinedRule());
    }
    return ("unknown");
}


#define drcMatching(string) (lstring::cimatch(kw, string))

// Static function.
// Return the type code given the keyword.
//
DRCtype
DRCtestDesc::ruleType(const char *kw)
{
    if (drcMatching(Dkw.Connected()))
        return (drConnected);
    if (drcMatching(Dkw.NoHoles()))
        return (drNoHoles);
    if (drcMatching(Dkw.Exist()))
        return (drExist);
    if (drcMatching(Dkw.Overlap()))
        return (drOverlap);
    if (drcMatching(Dkw.IfOverlap()))
        return (drIfOverlap);
    if (drcMatching(Dkw.NoOverlap()))
        return (drNoOverlap);
    if (drcMatching(Dkw.AnyOverlap()))
        return (drAnyOverlap);
    if (drcMatching(Dkw.PartOverlap()))
        return (drPartOverlap);
    if (drcMatching(Dkw.AnyNoOverlap()))
        return (drAnyNoOverlap);
    if (drcMatching(Dkw.MinArea()))
        return (drMinArea);
    if (drcMatching(Dkw.MaxArea()))
        return (drMaxArea);
    if (drcMatching(Dkw.MinEdgeLength()))
        return (drMinEdgeLength);
    if (drcMatching(Dkw.MaxWidth()))
        return (drMaxWidth);
    if (drcMatching(Dkw.MinWidth()))
        return (drMinWidth);
    if (drcMatching(Dkw.MinSpace()))
        return (drMinSpace);
    if (drcMatching(Dkw.MinSpaceTo()))
        return (drMinSpaceTo);
    if (drcMatching(Dkw.MinSpaceFrom()))
        return (drMinSpaceFrom);
    if (drcMatching(Dkw.MinOverlap()))
        return (drMinOverlap);
    if (drcMatching(Dkw.MinNoOverlap()))
        return (drMinNoOverlap);
    return (drNoRule);
}


// Static function.
// Return true if the rule type requires a target specification.
//
bool
DRCtestDesc::requiresTarget(DRCtype tp)
{
    switch (tp) {
    case drNoRule:
    case drConnected:
    case drNoHoles:
    case drExist:
        break;
    case drOverlap:
    case drIfOverlap:
    case drNoOverlap:
    case drAnyOverlap:
    case drPartOverlap:
    case drAnyNoOverlap:
        return (true);
    case drMinArea:
    case drMaxArea:
        break;
    case drMinEdgeLength:
        return (true);
    case drMaxWidth:
    case drMinWidth:
    case drMinSpace:
        break;
    case drMinSpaceTo:
    case drMinSpaceFrom:
    case drMinOverlap:
    case drMinNoOverlap:
        return (true);
    case drUserDefinedRule:
        // Really don't know about this.
        break;
    }
    return (false);
}


// Static function.
// This will return a matching rule.  By "matching" the rule must
// 1. match the type.
// 2. have no region specification.
// 3. have a simple layer target matching ltarg.
//
// If a match is found, the rule will be replaced in the Virtuoso
// constraint group functions.  Any new rule with the above properties
// should replace an existing rule in this manner.
//
DRCtestDesc *
DRCtestDesc::findRule(CDl *ld, DRCtype type, CDl *ltarg)
{
    for (DRCtestDesc *td = *tech_prm(ld)->rules_addr(); td; td = td->next()) {
        if (td->type() == type && !td->hasRegion()) {
            if (!ltarg || td->targetLayer() == ltarg)
                return (td);
        }
    }
    return (0);
}

