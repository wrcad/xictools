
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

#include "dsp.h"
#include "dsp_window.h"
#include "dsp_inlines.h"
#include "dsp_color.h"


//
// Functions to check and display the grid.
//


// If mark, add a mark for every vertex of odesc that is off-grid in
// this.  Return a count of off-grid vertices found.
//
int
WindowDesc::CheckGrid(const CDo *odesc, bool mark)
{
    if (!odesc)
        return (0);
    int vcnt = 0;
#ifdef HAVE_COMPUTED_GOTO
    COMPUTED_JUMP(odesc)
#else
    CONDITIONAL_JUMP(odesc)
#endif
box:
inst:
    {
        Point_c p(odesc->oBB().left, odesc->oBB().bottom);
        Snap(&p.x, &p.y);
        if (p.x != odesc->oBB().left || p.y != odesc->oBB().bottom) {
            if (mark)
                DSP()->ShowBoxMark(DISPLAY, odesc->oBB().left,
                    odesc->oBB().bottom, HighlightingColor, 20,
                    DSP()->CurMode());
            vcnt++;
        }
        p.set(odesc->oBB().left, odesc->oBB().top);
        Snap(&p.x, &p.y);
        if (p.x != odesc->oBB().left || p.y != odesc->oBB().top) {
            if (mark)
                DSP()->ShowBoxMark(DISPLAY, odesc->oBB().left,
                    odesc->oBB().top, HighlightingColor, 20,
                    DSP()->CurMode());
            vcnt++;
        }
        p.set(odesc->oBB().right, odesc->oBB().top);
        Snap(&p.x, &p.y);
        if (p.x != odesc->oBB().right || p.y != odesc->oBB().top) {
            if (mark)
                DSP()->ShowBoxMark(DISPLAY, odesc->oBB().right,
                    odesc->oBB().top, HighlightingColor, 20,
                    DSP()->CurMode());
            vcnt++;
        }
        p.set(odesc->oBB().right, odesc->oBB().bottom);
        Snap(&p.x, &p.y);
        if (p.x != odesc->oBB().right || p.y != odesc->oBB().bottom) {
            if (mark)
                DSP()->ShowBoxMark(DISPLAY, odesc->oBB().right,
                    odesc->oBB().bottom, HighlightingColor, 20,
                    DSP()->CurMode());
            vcnt++;
        }
        return (vcnt);
    }
poly:
    {
        int num = ((const CDpo*)odesc)->numpts();
        const Point *pts = ((const CDpo*)odesc)->points();
        for (int i = 1; i < num; i++) {
            Point p = pts[i];
            Snap(&p.x, &p.y);
            if (p.x != pts[i].x || p.y != pts[i].y) {
                if (mark)
                    DSP()->ShowBoxMark(DISPLAY, pts[i].x, pts[i].y,
                        HighlightingColor, 20, DSP()->CurMode());
                vcnt++;
            }
        }
        return (vcnt);
    }
wire:
    {
        int num = ((const CDw*)odesc)->numpts();
        const Point *pts = ((const CDw*)odesc)->points();
        for (int i = 0; i < num; i++) {
            Point p = pts[i];
            Snap(&p.x, &p.y);
            if (p.x != pts[i].x || p.y != pts[i].y) {
                if (mark)
                    DSP()->ShowBoxMark(DISPLAY, pts[i].x, pts[i].y,
                        HighlightingColor, 20, DSP()->CurMode());
                vcnt++;
            }
        }
        return (vcnt);
    }
label:
    return (0);
}


#define CCNT 200
#define FCNT 1000

namespace {
    // This twiddles the dash offset so that the grid line patterns are
    // continuous across a partial redraw.
    //
    void
    lsos(GRlineType *l, int os)
    {
        if (!l || !l->mask || l->mask == -1)
            return;
        int plen = 0;
        for (int i = 0; i < l->length; i++)
            plen += l->dashes[i];
        l->offset = plen ? os % plen : 0;
    }
}


// Show the grid in the wdesc window.  Each window can have its own
// grid style, set in the wdesc attributes.
//
void
WindowDesc::ShowGrid()
{
    if (!w_draw)
        return;
    DSPattrib *a = &w_attributes;
    if (!a->grid(w_mode)->displayed()) {
        ShowAxes(DISPLAY);
        return;
    }

    int resol;
    if (w_mode == Physical) {
        double spa = a->grid(Physical)->spacing(Physical);
        int snap = a->grid(Physical)->snap();
        if (snap < 0)
            resol = INTERNAL_UNITS(-spa/snap);
        else
            resol = INTERNAL_UNITS(spa*snap);
    }
    else {
        double spa = a->grid(Electrical)->spacing(Electrical);
        int snap = a->grid(Electrical)->snap();
        if (snap < 0)
            resol = ELEC_INTERNAL_UNITS(-spa/snap);
        else
            resol = ELEC_INTERNAL_UNITS(spa*snap);
    }
    int step = resol * a->grid(w_mode)->coarse_mult();

    // Allow for a different y-scale, used for cross-section display.
    int yresol = mmRnd(resol*YScale());
    int ystep = yresol * a->grid(w_mode)->coarse_mult();

    double thr = DSP()->GridThreshold()*w_draw->Resolution();
    bool show_fine_x = (resol*w_ratio >= thr);
    if (w_mode == Electrical && !show_fine_x)
        return;
    bool show_fine_y = (yresol*w_ratio >= thr);
    if (w_mode == Physical && DSP()->GridNoCoarseOnly() &&
            !show_fine_x && !show_fine_y)
        return;
    bool show_coarse_y = (ystep*w_ratio >= thr);
    bool show_coarse_x = (step*w_ratio >= thr);
    if (!show_coarse_x && !show_coarse_y) {
        ShowAxes(DISPLAY);
        return;
    }

    // The grid origin
    int org_x = w_mode == Physical ? DSP()->PhysVisGridOrigin()->x : 0;
    int org_y = w_mode == Physical ? DSP()->PhysVisGridOrigin()->y : 0;

    int top = ((w_window.top - org_y)/ystep)*ystep + org_y;
    while (top < w_window.top)
        top += ystep;
    int bottom = w_window.bottom;

    int left = ((w_window.left - org_x)/step)*step + org_x;
    while (left > w_window.left)
        left -= step;
    int right = w_window.right;

    int cs = a->grid(w_mode)->dotsize();
    int ccnt = CCNT;
    int fcnt = FCNT;
    if (cs > 0) {
        ccnt += 4*cs;
        fcnt += 4*cs;
    }
    GRmultiPt psc(ccnt);
    GRmultiPt psf(fcnt);

    w_draw->SetColor(DSP()->Color(FineGridColor, w_mode));
    w_draw->SetFillpattern(0);

    int yl, xl;
    int xp, yp;
    if (cs >= 0) {
        // The "dots" grid.

        bool showfine = show_fine_x && show_fine_y;
        bool showcoarse = show_coarse_x && show_coarse_y;
        ccnt = fcnt = 0;
        w_draw->SetLinestyle(0);
        for (yl = top; yl >= bottom; yl -= yresol) {
            LToPy(yl, yp);
            if (ViewportClipBottom(yp, &w_clip_rect) ||
                    ViewportClipTop(yp, &w_clip_rect))
                continue;
            for (xl = left; xl <= right; xl += resol) {
                LToPx(xl, xp);
                if (ViewportClipLeft(xp, &w_clip_rect) ||
                        ViewportClipRight(xp, &w_clip_rect))
                    continue;
                bool yok = (top-yl) % ystep == 0;
                bool xok = (xl-left) % step == 0;
                if ((showfine && (xok || yok)) || (!showfine && xok && yok)) {
                    if (showcoarse) {
                        psc.assign(ccnt++, xp, yp);
                        if (cs > 0) {
                            for (int i = 1; i <= cs; i++) {
                                int xt = xp - i;
                                if (!ViewportClipLeft(xt, &w_clip_rect) &&
                                        !ViewportClipRight(xt, &w_clip_rect))
                                    psc.assign(ccnt++, xt, yp);
                                xt = xp + i;
                                if (!ViewportClipLeft(xt, &w_clip_rect) &&
                                        !ViewportClipRight(xt, &w_clip_rect))
                                    psc.assign(ccnt++, xt, yp);
                                int yt = yp - i;
                                if (!ViewportClipBottom(yt, &w_clip_rect) &&
                                        !ViewportClipTop(yt, &w_clip_rect))
                                    psc.assign(ccnt++, xp, yt);
                                yt = yp + i;
                                if (!ViewportClipBottom(yt, &w_clip_rect) &&
                                        !ViewportClipTop(yt, &w_clip_rect))
                                    psc.assign(ccnt++, xp, yt);
                            }
                        }
                        if (ccnt >= CCNT) {
                            w_draw->SetColor(
                                DSP()->Color(CoarseGridColor, w_mode));
                            w_draw->Pixels(&psc, ccnt);
                            w_draw->SetColor(
                                DSP()->Color(FineGridColor, w_mode));
                            ccnt = 0;
                        }
                    }
                }
                else if (showfine) {
                    psf.assign(fcnt++, xp, yp);
                    if (cs > 0) {
                        for (int i = 1; i <= cs; i++) {
                            int xt = xp - i;
                            if (!ViewportClipLeft(xt, &w_clip_rect) &&
                                    !ViewportClipRight(xt, &w_clip_rect))
                                psf.assign(fcnt++, xt, yp);
                            xt = xp + i;
                            if (!ViewportClipLeft(xt, &w_clip_rect) &&
                                    !ViewportClipRight(xt, &w_clip_rect))
                                psf.assign(fcnt++, xt, yp);
                            int yt = yp - i;
                            if (!ViewportClipBottom(yt, &w_clip_rect) &&
                                    !ViewportClipTop(yt, &w_clip_rect))
                                psf.assign(fcnt++, xp, yt);
                            yt = yp + i;
                            if (!ViewportClipBottom(yt, &w_clip_rect) &&
                                    !ViewportClipTop(yt, &w_clip_rect))
                                psf.assign(fcnt++, xp, yt);
                        }
                    }
                    if (fcnt >= FCNT) {
                        w_draw->Pixels(&psf, fcnt);
                        fcnt = 0;
                    }
                }
            }
        }
        if (fcnt)
            w_draw->Pixels(&psf, fcnt);
        if (ccnt) {
            w_draw->SetColor(DSP()->Color(CoarseGridColor, w_mode));
            w_draw->Pixels(&psc, ccnt);
            w_draw->SetColor(DSP()->Color(FineGridColor, w_mode));
        }
    }
    else {
        // Line grid.

        // Horizontal fine grid lines.
        lsos(&a->grid(w_mode)->linestyle(), w_clip_rect.left);
        w_draw->SetLinestyle(&a->grid(w_mode)->linestyle());

        fcnt = 0;
        if (show_fine_y) {
            for (yl = top; yl >= bottom; yl -= yresol) {

                LToPy(yl, yp);
                if (ViewportClipBottom(yp, &w_clip_rect) ||
                        ViewportClipTop(yp, &w_clip_rect))
                    continue;
                if ((top-yl) % ystep == 0)
                    continue;
                psf.assign(fcnt, w_clip_rect.left, yp);
                fcnt++;
                psf.assign(fcnt, w_clip_rect.right, yp);
                fcnt++;
                if (fcnt >= FCNT) {
                    w_draw->Lines(&psf, fcnt/2);
                    fcnt = 0;
                }
            }
            if (fcnt)
                w_draw->Lines(&psf, fcnt/2);
        }

        // Vertical fine grid lines.
        lsos(&a->grid(w_mode)->linestyle(), w_clip_rect.top);
        w_draw->SetLinestyle(&a->grid(w_mode)->linestyle());

        fcnt = 0;
        if (show_fine_x) {
            for (xl = left; xl <= right; xl += resol) {

                LToPx(xl, xp);
                if (ViewportClipLeft(xp, &w_clip_rect) ||
                        ViewportClipRight(xp, &w_clip_rect))
                    continue;
                if ((xl-left) % step == 0)
                    continue;
                psf.assign(fcnt, xp, w_clip_rect.top);
                fcnt++;
                psf.assign(fcnt, xp, w_clip_rect.bottom);
                fcnt++;
                if (fcnt >= FCNT) {
                    w_draw->Lines(&psf, fcnt/2);
                    fcnt = 0;
                }
            }
            if (fcnt)
                w_draw->Lines(&psf, fcnt/2);
        }

        //
        // coarse grid on top
        //
        w_draw->SetColor(DSP()->Color(CoarseGridColor, w_mode));

        // Horizontal coarse grid lines.
        lsos(&a->grid(w_mode)->linestyle(), w_clip_rect.left);
        w_draw->SetLinestyle(&a->grid(w_mode)->linestyle());

        ccnt = 0;
        if (show_coarse_y) {
            for (yl = top; yl >= bottom; yl -= yresol) {

                LToPy(yl, yp);
                if (ViewportClipBottom(yp, &w_clip_rect) ||
                        ViewportClipTop(yp, &w_clip_rect))
                    continue;
                if ((top-yl) % ystep == 0) {
                    psc.assign(ccnt, w_clip_rect.left, yp);
                    ccnt++;
                    psc.assign(ccnt, w_clip_rect.right, yp);
                    ccnt++;
                    if (ccnt >= CCNT) {
                        w_draw->Lines(&psc, ccnt/2);
                        ccnt = 0;
                    }
                }
            }
            if (ccnt)
                w_draw->Lines(&psc, ccnt/2);
        }

        // Vertical coarse grid lines.
        lsos(&a->grid(w_mode)->linestyle(), w_clip_rect.top);
        w_draw->SetLinestyle(&a->grid(w_mode)->linestyle());

        ccnt = 0;
        if (show_coarse_x) {
            for (xl = left; xl <= right; xl += resol) {

                LToPx(xl, xp);
                if (ViewportClipLeft(xp, &w_clip_rect) ||
                        ViewportClipRight(xp, &w_clip_rect))
                    continue;

                if ((xl-left) % step == 0) {
                    psc.assign(ccnt, xp, w_clip_rect.top);
                    ccnt++;
                    psc.assign(ccnt, xp, w_clip_rect.bottom);
                    ccnt++;
                    if (ccnt >= CCNT) {
                        w_draw->Lines(&psc, ccnt/2);
                        ccnt = 0;
                    }
                }
            }
            if (ccnt)
                w_draw->Lines(&psc, ccnt/2);
        }
    }
    w_draw->SetLinestyle(0);
    ShowAxes(DISPLAY);
}


namespace {
    void clipped_line(WindowDesc *wdesc, int x1, int y1, int x2, int y2)
    {
        if (!cGEO::line_clip(&x1, &y1, &x2, &y2, wdesc->ClipRect()))
            wdesc->Wdraw()->Line(x1, y1, x2, y2);
    }
}


// Show the coordinate axes in the wdesc window.
//
void
WindowDesc::ShowAxes(bool display)
{
    if (w_mode == Electrical)
        return;
    if (!w_draw)
        return;
    if (display == ERASE) {
        int d = (int)(1.0/w_ratio);
        if (d < 1)
            d = 1;
        BBox BB(w_window.left, DSP()->PhysVisGridOrigin()->x - d,
            w_window.right, DSP()->PhysVisGridOrigin()->x + d);
        Redisplay(&BB);
        BB = BBox(DSP()->PhysVisGridOrigin()->y - d, w_window.bottom,
            DSP()->PhysVisGridOrigin()->y + d, w_window.top);
        Redisplay(&BB);
        d = (int)(21.0/w_ratio);
        if (d < 1)
            d = 0;
        BB = BBox(DSP()->PhysVisGridOrigin()->x - d,
            DSP()->PhysVisGridOrigin()->y - d,
            DSP()->PhysVisGridOrigin()->x + d,
            DSP()->PhysVisGridOrigin()->y + d);
        Redisplay(&BB);
    }
    else {
        if (w_attributes.grid(w_mode)->axes() == AxesNone)
            return;
        int xp, yp;
        LToP(DSP()->PhysVisGridOrigin()->x,
            DSP()->PhysVisGridOrigin()->y, xp, yp);
        w_draw->SetLinestyle(0);
        w_draw->SetFillpattern(0);
        if (w_attributes.grid(w_mode)->axes() == AxesMark) {
            int delta = 20;
            // show a mark at the origin
            w_draw->SetColor(DSP()->Color(FineGridColor, w_mode));
            clipped_line(this, xp - delta, yp, xp + delta, yp);
            clipped_line(this, xp, yp - delta, xp, yp + delta);
            w_draw->SetColor(DSP()->Color(CoarseGridColor, w_mode));
            clipped_line(this, xp, yp - delta, xp - delta, yp);
            clipped_line(this, xp - delta, yp, xp, yp + delta);
            clipped_line(this, xp, yp + delta, xp + delta, yp);
            clipped_line(this, xp + delta, yp, xp, yp - delta);
            // x axis
            clipped_line(this, 0, yp, xp - delta, yp);
            clipped_line(this, xp + delta, yp, w_width - 1, yp);
            // y axis
            clipped_line(this, xp, w_height - 1, xp, yp + delta);
            clipped_line(this, xp, yp - delta, xp, 0);
        }
        else {
            w_draw->SetColor(DSP()->Color(CoarseGridColor, w_mode));
            clipped_line(this, 0, yp, w_width - 1, yp);
            clipped_line(this, xp, w_height - 1, xp, 0);
        }
    }
}
// End of WindowDesc functions.


void
GridDesc::set(const GridDesc &gd)
{
    *this = gd;
    DSPmainDraw(defineLinestyle(&g_linestyle, g_linestyle.mask))
}


// Generate a test string representing the structure settings.  The
// format is
//
//   spacing snap linestyle [cross_size] [-a axes] [-d displayed]
//     [-t on_top] [-m coarse_mult]
//
char *
GridDesc::desc_string()
{
    // This part is backwards compatible with the grid register save
    // format in the tech file in 3.2.x releaases.
    sLstr lstr;
    lstr.add_d(g_spacing, 4, false);
    lstr.add_c(' ');
    lstr.add_i(g_snap);
    lstr.add_c(' ');
    lstr.add_u(g_linestyle.mask);
    lstr.add_c(' ');
    lstr.add_u(g_cross_size);

    lstr.add(" -a ");
    lstr.add_u(g_coarse_mult);
    lstr.add(" -d ");
    lstr.add_u(g_displayed);
    lstr.add(" -t ");
    lstr.add_u(g_on_top);
    lstr.add(" -m ");
    lstr.add_u(g_coarse_mult);
    return (lstr.string_trim());
}


namespace {
    bool parse_bool(const char *str)
    {
        if (isdigit(*str))
            return (*str != '0');
        if (*str == 't' || *str == 'T')
            return (true);
        if (*str == 'y' || *str == 'Y')
            return (true);
        if (*str == 'f' || *str == 'F')
            return (false);
        if (*str == 'n' || *str == 'N')
            return (false);
        if (lstring::ciprefix("on", str))
            return (true);
        if (lstring::ciprefix("of", str))
            return (false);
        return (true);
    }
}


// Static function
// Parse the description string, setting gd accordingly.  Return true
// if success, false if a syntax or other error.
//
bool
GridDesc::parse(const char *str, GridDesc &gd)
{
    // spacing
    char *tok = lstring::gettok(&str);
    double spa;
    if (!tok || sscanf(tok, "%lf", &spa) != 1 || spa <= 0.0 || spa > 10000.0) {
        Errs()->add_error("error parsing spacing parameter");
        delete [] tok;
        return (false);
    }
    gd.set_spacing(spa);
    delete [] tok;

    // snap
    tok = lstring::gettok(&str);
    int snap;
    if (!tok || sscanf(tok, "%d", &snap) != 1 || snap < -10 || snap > 10 ||
            snap == 0) {
        Errs()->add_error("error parsing snap parameter");
        delete [] tok;
        return (false);
    }
    gd.set_snap(snap);
    delete [] tok;

    // linestyle mask
    tok = lstring::gettok(&str);
    unsigned int ls;
    if (!tok || sscanf(tok, "%u", &ls) != 1) {
        delete [] tok;
        Errs()->add_error("error parsing linestyle mask");
        return (false);
    }
    gd.linestyle().mask = ls;
    delete [] tok;

    while ((tok = lstring::gettok(&str)) != 0) {
        unsigned int m;
        if (lstring::ciprefix("-a", tok)) {
            delete [] tok;
            tok = lstring::gettok(&str);
            if (!tok)
                break;
            if (sscanf(tok, "%u", &m) != 1 || m > 2) {
                Errs()->add_error("error parsing axes style");
                delete [] tok;
                return (false);
            }
            gd.set_axes((AxesType)m);
            delete [] tok;
            continue;
        }
        if (lstring::ciprefix("-d", tok)) {
            delete [] tok;
            tok = lstring::gettok(&str);
            if (!tok)
                break;
            gd.set_displayed( parse_bool(tok));
            delete [] tok;
            continue;
        }
        if (lstring::ciprefix("-t", tok)) {
            delete [] tok;
            tok = lstring::gettok(&str);
            if (!tok)
                break;
            gd.set_show_on_top(parse_bool(tok));
            delete [] tok;
            continue;
        }
        if (lstring::ciprefix("-m", tok)) {
            delete [] tok;
            tok = lstring::gettok(&str);
            if (!tok)
                break;
            if (sscanf(tok, "%u", &m) != 1 || m < 1 || m > 50) {
                Errs()->add_error("error parsing coarse mult parameter");
                delete [] tok;
                return (false);
            }
            gd.set_coarse_mult(m);
            delete [] tok;
            continue;
        }
        if (sscanf(tok, "%u", &m) == 1 && m <= GRD_MAX_CRS) {
            gd.set_dotsize(m);
            delete [] tok;
            continue;
        }
        Errs()->add_error("unknown or invalid parameter");
        delete [] tok;
        return (false);
    }
    return (true);
}

