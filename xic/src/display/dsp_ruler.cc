
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

#include "dsp.h"
#include "dsp_window.h"
#include "dsp_color.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"


// Whether or not snapping starts out on.
#define DEF_SNAPPING true

namespace {
    struct ESnapStuff
    {
        ESnapStuff()
            {
                setup(0);
            }

        void setup(const DSPattrib *a)
            {
                if (a) {
                    es_mode = a->edge_snapping();
                    es_off_grid = a->edge_off_grid();
                    es_non_manh = a->edge_non_manh();
                    es_wire_edge = a->edge_wire_edge();
                    es_wire_path = a->edge_wire_path();
                }
                else {
                    // Defaults for the Ruler command.

                    es_mode = EdgeSnapSome;
                    es_off_grid = true;
                    es_non_manh = true;
                    es_wire_edge = true;
                    es_wire_path = true;
                }
            }

        void apply(DSPattrib *a)
            {
                if (a) {
                    a->set_edge_snapping(es_mode);
                    a->set_edge_off_grid(es_off_grid);
                    a->set_edge_non_manh(es_non_manh);
                    a->set_edge_wire_edge(es_wire_edge);
                    a->set_edge_wire_path(es_wire_path);
                }
            }

    private:
        EdgeSnapMode es_mode;
        bool es_off_grid;
        bool es_non_manh;
        bool es_wire_edge;
        bool es_wire_path;
    };

    ESnapStuff ES;
    ESnapStuff ESbak[DSP_NUMWINS];
    bool Snapping = DEF_SNAPPING;
    bool InSnapCmdBak;
    unsigned char Wflags;
}


void
cDisplay::RulerSetSnapDefaults(const DSPattrib *a, const bool *snap)
{
    ES.setup(a);
    if (snap)
        Snapping = *snap;
}


void
cDisplay::RulerGetSnapDefaults(DSPattrib *a, bool *snap, bool origdef)
{
    if (origdef) {
        ESnapStuff e;
        e.apply(a);
        if (snap)
            *snap = DEF_SNAPPING;
    }
    else {
        ES.apply(a);
        if (snap)
            *snap = Snapping;
    }
}


void
cDisplay::StartRulerCmd()
{
    InSnapCmdBak = InEdgeSnappingCmd();
    SetInEdgeSnappingCmd(true);

    SetNoGridSnapping(!Snapping);
    Wflags = 0;
    for (int i = 0; i < DSP_NUMWINS; i++) {
        WindowDesc *wdesc = DSP()->Window(i);
        if (!wdesc)
            continue;
        Wflags |= (1 << i);
        DSPattrib *a = wdesc->Attrib();
        ESbak[i].setup(a);
        ES.apply(a);
        wdesc->Wbag()->PopUpGrid(0, MODE_UPD);
    }
}


void
cDisplay::EndRulerCmd()
{
    SetInEdgeSnappingCmd(InSnapCmdBak);
    SetNoGridSnapping(false);

    for (int i = 0; i < DSP_NUMWINS; i++) {
        WindowDesc *wdesc = DSP()->Window(i);
        if (!wdesc)
            continue;
        DSPattrib *a = wdesc->Attrib();
        if (!i)
            ES.setup(a);
        if (Wflags & (1 << i))
            ESbak[i].apply(a);
        else
            ESbak[0].apply(a);
        wdesc->Wbag()->PopUpGrid(0, MODE_UPD);
    }
}


namespace {
    void set_scale(double, double, double*, double*, int*);
    bool ismod(double, double);
}


#define pshort 8
#define plong 12
#define XOR_LINES

// Display the ruler.  This is used for both highlighting display and
// ghost drawing.  In the latter case, we have to use a special refresh
// mode since simple XOR drawing may not work with text.
//
void
sRuler::show(bool d_or_e, bool ghost_erase)
{
    if (win_num < 0 || win_num >= DSP_NUMWINS)
        return;
    WindowDesc *wdesc = DSP()->Window(win_num);
    if (!wdesc)
        return;
    if (!wdesc->Wdraw())
        return;
    double ys = wdesc->YScale();
    int x1 = p1.x;
    int y1 = mmRnd(p1.y*ys);
    int x2 = p2.x;
    int y2 = mmRnd(p2.y*ys);
    DSP()->TPoint(&x1, &y1);
    DSP()->TPoint(&x2, &y2);

    if (DSPpkg::self()->IsDualPlane())
        wdesc->Wdraw()->SetXOR(d_or_e ? GRxHlite : GRxUnhlite);
    else {
        if (d_or_e) {
#ifdef XOR_LINES
#else
            if (ghost_erase) {
                // The first arg is always DISPLAY when ghost drawing.
                BBox BB;
                if (bbox(BB, wdesc))
                    wdesc->GhostUpdate(&BB);
                return;
            }
#endif
            wdesc->Wdraw()->SetColor(
                DSP()->Color(HighlightingColor, wdesc->Mode()));
        }
        else {
            // This function will be reentered from Refresh to redraw
            // other rulers.
            BBox BB;
            if (bbox(BB, wdesc))
                wdesc->Update(&BB);
            return;
        }
    }

    double dx = x1 - x2;
    double dy = y1 - y2;
    double len = sqrt(dx*dx + dy*dy/ys/ys);
    if (len != 0.0) {
        double ratio = wdesc->Ratio() * sqrt(dx*dx + dy*dy)/len;
        len /= CDphysResolution;

        double lnew, unew;
        int n;
        set_scale(loff, loff + len, &lnew, &unew, &n);
        double ds = (unew - lnew)/(n*5);

        // keep the gradations large enough to read
        double pixs = ds*CDphysResolution*ratio;
        while (pixs < 6.0) {
            pixs *= 2.0;
            if (!ismod(lnew, 2.0*ds))
                lnew += ds;
            ds *= 2.0;
        }

        // If the second scale factor isn't visible, don't show any scale
        // factors, just lines.
        bool too_short = (len < 5*ds);

        double ang = atan2(-dx, dy);
        wdesc->LToP(x1, y1, x1, y1);
        wdesc->LToP(x2, y2, x2, y2);
        wdesc->Wdraw()->SetLinestyle(0);
        wdesc->Wdraw()->SetFillpattern(0);
        double ca = cos(ang);
        double sa = sin(ang);
        x2 = x1 + INTERNAL_UNITS(len*sa*ratio);
        y2 = y1 + INTERNAL_UNITS(len*ca*ratio);
        int xt1 = x1;
        int yt1 = y1;
        if (!cGEO::line_clip(&xt1, &yt1, &x2, &y2, &wdesc->Viewport()))
            wdesc->Wdraw()->Line(xt1, yt1, x2, y2);
        len *= 1 + 1e-9;
        len += loff;
        for (double d = lnew; d < len; d += ds) {
            if (d < loff)
                continue;
            int xx1 = x1 + INTERNAL_UNITS((d-loff)*sa*ratio);
            int yy1 = y1 + INTERNAL_UNITS((d-loff)*ca*ratio);
            bool longgrad = ismod(d, 5*ds);
            double t = longgrad && !too_short ? plong : pshort;
            if (mirror)
                t = -t;
            int xx2 = xx1 - mmRnd(t*ca);
            int yy2 = yy1 + mmRnd(t*sa);
            xt1 = xx2;
            yt1 = yy2;
            if (!cGEO::line_clip(&xx1, &yy1, &xt1, &yt1,
                    &wdesc->Viewport()))
                wdesc->Wdraw()->Line(xx1, yy1, xt1, yt1);
            if (longgrad && !too_short) {
                int tx = mirror ? -5 : 5;
                xx2 -= mmRnd(tx*ca);
                yy2 += mmRnd(tx*sa);
                char tbuf[32];
                mmDtoA(tbuf, d, 3, true);
                int w, h;
                wdesc->Wdraw()->TextExtent(tbuf, &w, &h);
                if (xx1 > xx2)
                    xx2 -= w;
                if (yy1 < yy2)
                    yy2 += h;
                if (yy2 < wdesc->ViewportHeight() - 1 &&
                        yy2 - h > 0 &&
                        xx2 > 0 &&
                        xx2 + w < wdesc->ViewportWidth() - 1) {
#ifdef XOR_LINES
                    if (ghost_erase) {
                        BBox BB(xx2, yy2, xx2+w, yy2-h);
                        wdesc->GhostUpdate(&BB);
                    }
                    else
#endif

                    wdesc->Wdraw()->Text(tbuf, xx2, yy2, 0);
                }
            }
        }
    }
    if (DSPpkg::self()->IsDualPlane())
        wdesc->Wdraw()->SetXOR(GRxNone);
}


// Compute the ruler's bounding box in wdesc.  If the bounding box is
// valid, return true.  The BB coords are in Viewport space!
//
bool
sRuler::bbox(BBox &BB, WindowDesc *wdesc)
{
    double ys = wdesc->YScale();
    int x1 = p1.x;
    int y1 = mmRnd(p1.y*ys);
    int x2 = p2.x;
    int y2 = mmRnd(p2.y*ys);
    DSP()->TPoint(&x1, &y1);
    DSP()->TPoint(&x2, &y2);

    double dx = x1 - x2;
    double dy = y1 - y2;
    double len = sqrt(dx*dx + dy*dy/ys/ys);
    if (len != 0.0) {
        double ratio = wdesc->Ratio() * sqrt(dx*dx + dy*dy)/len;
        len /= CDphysResolution;

        double lnew, unew;
        int n;
        set_scale(loff, loff + len, &lnew, &unew, &n);
        double ds = (unew - lnew)/(n*5);

        // keep the gradations large enough to read
        double pixs = ds*CDphysResolution*ratio;
        while (pixs < 6.0) {
            pixs *= 2.0;
            if (!ismod(lnew, 2.0*ds))
                lnew += ds;
            ds *= 2.0;
        }

        // If the second scale factor isn't visible, don't show any scale
        // factors, just lines.
        bool too_short = (len < 5*ds);

        double ang = atan2(-dx, dy);
        wdesc->LToP(x1, y1, x1, y1);
        wdesc->LToP(x2, y2, x2, y2);
        double ca = cos(ang);
        double sa = sin(ang);
        BB.left = BB.right = x1;
        BB.bottom = BB.top = y1;
        x2 = x1 + INTERNAL_UNITS(len*sa*ratio);
        y2 = y1 + INTERNAL_UNITS(len*ca*ratio);
        BB.add(x2, y2);
        len *= 1 + 1e-9;
        len += loff;
        for (double d = lnew; d < len; d += ds) {
            if (d < loff)
                continue;
            int xx1 = x1 + INTERNAL_UNITS((d-loff)*sa*ratio);
            int yy1 = y1 + INTERNAL_UNITS((d-loff)*ca*ratio);
            bool longgrad = ismod(d, 5*ds);
            double t = longgrad && !too_short ? plong : pshort;
            if (mirror)
                t = -t;
            int xx2 = xx1 - mmRnd(t*ca);
            int yy2 = yy1 + mmRnd(t*sa);
            BB.add(xx2, yy2);
            if (longgrad && !too_short) {
                int tx = mirror ? -5 : 5;
                xx2 -= mmRnd(tx*ca);
                yy2 += mmRnd(tx*sa);
                char tbuf[32];
                mmDtoA(tbuf, d, 3, true);
                int w, h;
                wdesc->Wdraw()->TextExtent(tbuf, &w, &h);
                if (xx1 > xx2)
                    xx2 -= w;
                if (yy1 < yy2)
                    yy2 += h;
                BB.add(xx2, yy2);
                BB.add(xx2 + w, yy2 - h);
            }
        }

        BB.bloat(1);
        // invert for viewport coords
        int tmp = BB.bottom;
        BB.bottom = BB.top;
        BB.top = tmp;
        return (true);
    }
    return (false);
}
// End of sRuler functions


namespace {
    // Find nice 1-2-5 scale for data.
    //
    void
    set_scale(double l, double u, double *lnew, double *unew, int *n)
    {
        *n = 4;
        int j = 0;
        double x;
        if (u  < l) {
            x = l;
            l = u;
            u = x;
        }
        else if (u == l) {
            if (u == 0.0 && l == 0.0) {
                *lnew = -0.5;
                *unew = 0.5;
                return;
            }
            l -= 0.1*fabs(l);
            u += 0.1*fabs(u);
        }
        x = u - l;
        l += x*.001;
        u -= x*.001;
        x = u - l;
        double e = floor(log10(x));
        double m = x / pow(10.0, e);

        if      (m <= 2) j = 1;
        else if (m <= 4) j = 2;
        else if (m <= 8) j = 4;
        else             j = 8;

        if      (m > j*1.75) *n = 8;
        else if (m > j*1.5)  *n = 7;
        else if (m > j*1.25) *n = 6;
        else if (m > j*1.0)  *n = 5;
        else                 *n = 4;

        double s = *n * j * pow(10.0, e) / 4.0;
        if (*n == 8) *n = 4;
        double del = s / *n;
        x = l / del;
        while (s + del*floor(x) < u) {
            s += del;
            *n += 1;
            if (*n == 8) *n = 4;
            del = s / *n;
            x =  l / del;
        }
        *lnew = del * floor(x);
        *unew = *lnew + s;
    }


    // Return true if n/d is effectively an integer
    //
    bool
    ismod(double n, double d)
    {
        if (d != 0.0) {
            double a = n/d;
            if (a - floor(a) < 1e-9)
                return (true);
            if (ceil(a) - a < 1e-9)
                return (true);
        }
        return (false);
    }
}

