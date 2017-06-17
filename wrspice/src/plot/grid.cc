
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 1996 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL STEPHEN R. WHITELEY BE LIABLE       *
 *   FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION      *
 *   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN           *
 *   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN         *
 *   THE SOFTWARE.                                                        *
 *                                                                        *
 *========================================================================*
 *                                                                        *
 * Whiteley Research Circuit Simulation and Analysis Tool                 *
 *                                                                        *
 *========================================================================*
 $Id: grid.cc,v 2.56 2015/08/20 03:32:10 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1987 UCB
         1992 Stephen R. Whiteley
****************************************************************************/

#include "outplot.h"
#include "frontend.h"
#include "ttyio.h"
#include "texttf.h"
#include "spnumber.h"


//
// Functions to draw the various sorts of grids -- linear, log, polar.
//

namespace {
    void fix_log_trace_limits(double *lo, double *hi, int num)
    {
        int lmt = floorlog(*lo);
        int hmt = ceillog(*hi);
        int decs = hmt - lmt;
        if (!decs) {
            decs++;
            hmt++;
        }
        bool addtop = false;
        while (decs < num) {
            if (addtop) {
                hmt++;
                addtop = false;
            }
            else {
                lmt--;
                addtop = true;
            }
            decs++;
        }
        *lo = pow(10.0, (double)lmt);
        *hi = pow(10.0, (double)hmt);
    }


    // Like ft_minmax(), but consider only points in the visible range.
    //
    void interval_minmax(sDataVec *v, sDataVec *s, double minx, double maxx,
        double *miny, double *maxy)
    {
        double mx = -1.0;
        double mn = 1.0;
        for (int i = 0; i < v->length(); i++) {
            double r = s->realval(i);
            if (r < minx || r > maxx)
                continue;
            double d = v->realval(i);
            if (mx < mn)
                mx = mn = d;
            else {
                if (d < mn) mn = d;
                if (d > mx) mx = d;
            }
        }
        *miny = mn;
        *maxy = mx;
    }

    // Note: scaleunits is never changed in this file.
    const bool scaleunits = true;
}


void
sGraph::gr_linebox(int xl, int yl, int xu, int yu)
{
    yl = yinv(yl);
    yu = yinv(yu);
    gr_dev->Line(xl, yl, xu, yl);
    gr_dev->Line(xu, yl, xu, yu);
    gr_dev->Line(xu, yu, xl, yu);
    gr_dev->Line(xl, yu, xl, yl);
}


void
sGraph::gr_fixgrid()
{
    gr_dev->SetColor(gr_colors[1].pixel);
    gr_dev->setDefaultLinestyle(1);

    if ((gr_datawin.xmin > gr_datawin.xmax)
            || (gr_datawin.ymin > gr_datawin.ymax)) {
        GRpkgIf()->ErrPrintf(ET_INTERR,
            "gr_fixgrid: bad limits: %g, %g, %g, %g.\n", 
            gr_datawin.xmin, gr_datawin.xmax, 
            gr_datawin.ymin, gr_datawin.xmax);
        return;
    }

    sDataVec *scale = ((sDvList*)gr_plotdata)->dl_dvec->scale();
    if (gr_grtype == GRID_POLAR || gr_grtype == GRID_SMITH ||
            gr_grtype == GRID_SMITHGRID || !scale || scale->length() < 2) {
        gr_xmono = false;
    }
    else {
        gr_xmono = true;
        if (scale && scale->length() > 2) {
            bool increasing = (scale->realval(0) < scale->realval(1));
            for (int i = 1; i < scale->length() - 1; i++) {
                if (increasing != (scale->realval(i) < scale->realval(i+1))) {
                    gr_xmono = false;
                    break;
                }
            }
        }
    }

    if (gr_grtype == GRID_POLAR) {
        polargrid();
        return;
    }
    else if (gr_grtype == GRID_SMITH || gr_grtype == GRID_SMITHGRID) {
        smithgrid();
        return;
    }

    double nscale[2];
    if ((gr_grtype == GRID_XLOG) || (gr_grtype == GRID_LOGLOG))
        loggrid(nscale, x_axis);
    else
        lingrid(nscale, x_axis);
    gr_datawin.xmin = nscale[0];
    gr_datawin.xmax = nscale[1];
    if ((gr_grtype == GRID_YLOG) || (gr_grtype == GRID_LOGLOG))
        loggrid(nscale, y_axis);
    else
        lingrid(nscale, y_axis);
    gr_datawin.ymin = nscale[0];
    gr_datawin.ymax = nscale[1];
}


void
sGraph::gr_redrawgrid()
{
    gr_dev->SetColor(gr_colors[1].pixel);
    gr_dev->setDefaultLinestyle(1);

    switch (gr_grtype) {
    case GRID_POLAR:
        drawpolargrid();
        break;

    case GRID_SMITH:
    case GRID_SMITHGRID:
        drawsmithgrid();
        break;

    case GRID_XLOG:
        drawloggrid(x_axis, true);
        drawlingrid(y_axis, true);
        drawloggrid(x_axis, false);
        drawlingrid(y_axis, false);
        break;

    case GRID_YLOG:
        drawlingrid(x_axis, true);
        drawloggrid(y_axis, true);
        drawlingrid(x_axis, false);
        drawloggrid(y_axis, false);
        break;

    case GRID_LOGLOG:
        drawloggrid(x_axis, true);
        drawloggrid(y_axis, true);
        drawloggrid(x_axis, false);
        drawloggrid(y_axis, false);
        break;

    default:
        drawlingrid(x_axis, true);
        drawlingrid(y_axis, true);
        drawlingrid(x_axis, false);
        drawlingrid(y_axis, false);
        break;
    }

    // draw labels
    if (gr_xlabel) {
        if (gr_grtype == GRID_POLAR || gr_grtype == GRID_SMITH ||
                gr_grtype == GRID_SMITHGRID)
            gr_save_text(gr_xlabel, gr_vport.right() +
                2*gr_fontwid - strlen(gr_xlabel)*gr_fontwid,
                gr_vport.bottom() - gr_fonthei, LAxlabel, 1, 0);
        else 
            gr_save_text(gr_xlabel, gr_vport.left() + gr_vport.width()/2,
                gr_vport.bottom() - 3*gr_fonthei, LAxlabel, 1, TXTF_HJC);
    }
    if (gr_ylabel) {
        if (gr_grtype == GRID_POLAR || gr_grtype == GRID_SMITH ||
                gr_grtype == GRID_SMITHGRID)
            gr_save_text(gr_ylabel, gr_vport.left() + gr_vport.width()/2,
                gr_vport.top() + 2*gr_fonthei + gr_fonthei/2, LAylabel, 1,
                TXTF_HJC);
        else
            gr_save_text(gr_ylabel, gr_vport.right(),
                gr_vport.top() + gr_fonthei + gr_fonthei/2, LAylabel, 1,
                TXTF_HJR);
    }
}


// Plot a linear grid. Returns the new hi and lo limits.
//
void
sGraph::lingrid(double *nscale, Axis axis)
{
    if (axis == x_axis) {
        int mag;
        set_lin_grid(nscale, gr_datawin.xmin, gr_datawin.xmax, &gr_xaxis,
            &mag);
        const char *c = 0;
        char *s = gr_xunits.unitstr();
        if (scaleunits && s && *s && (c = SPnum.suffix(mag)) != 0) {
            strcpy(gr_xaxis.lin.units, c);
            strcat(gr_xaxis.lin.units, s);
        }
        else {
            if (mag != 0)
                sprintf(gr_xaxis.lin.units, "e%d", mag);
            else
                gr_xaxis.lin.units[0] = 0;
            if (s && *s)
                strcat(gr_xaxis.lin.units, s);
        }
        delete [] s;
    }
    else {
        set_raw_trace_limits();
        set_lin_trace_limits();
        int mag;
        set_lin_grid(nscale, gr_datawin.ymin, gr_datawin.ymax, &gr_yaxis, &mag);
        const char *c = 0;
        char *s = gr_yunits.unitstr();
        if (scaleunits && s && *s && (c = SPnum.suffix(mag)) != 0) {
            strcpy(gr_yaxis.lin.units, c);
            strcat(gr_yaxis.lin.units, s);
        }
        else {
            if (mag != 0)
                sprintf(gr_yaxis.lin.units, "e%d", mag);
            else
                gr_yaxis.lin.units[0] = 0;
            if (s && *s)
                strcat(gr_yaxis.lin.units, s);
        }
        delete [] s;

        if (gr_format != FT_SINGLE || gr_ysep)
            gr_scalewidth = 0;
        else {
            // find how much space to leave for scale factors
            int nsp = gr_yaxis.lin.numspace;
            double lmt = gr_yaxis.lin.lowlimit;
            double hmt = gr_yaxis.lin.highlimit;
            int mult = gr_yaxis.lin.mult;
            double dval = (hmt - lmt)/nsp;
            double val;
            int slen = 0;
            for (val = lmt; val <= hmt; val += dval) {
                char buf[16];
                // fix numerical problem when val ~ 0
                if (fabs(val) < 1e-7*dval)
                    val = 0;
                sprintf(buf, "%g", val*mult);
                int len = strlen(buf);
                if (len > slen)
                    slen = len;
            }
            gr_scalewidth = slen + 1;
        }
    }
}


void
sGraph::set_lin_trace_limits()
{
    sDvList *link;
    int nspa;
    if (gr_ysep) {
        if (gr_grpmin[0] < gr_grpmax[0])
            set_scale(gr_grpmin[0], gr_grpmax[0], gr_grpmin, gr_grpmax, &nspa,
            1e5);
        if (gr_grpmin[1] < gr_grpmax[1])
            set_scale(gr_grpmin[1], gr_grpmax[1], gr_grpmin+1, gr_grpmax+1, &nspa,
            1e5);
        if (gr_grpmin[2] < gr_grpmax[2])
            set_scale(gr_grpmin[2], gr_grpmax[2], gr_grpmin+2, gr_grpmax+2, &nspa,
            1e5);
        for (link = (sDvList*)gr_plotdata; link; link = link->dl_next) {
            sDataVec *dv = link->dl_dvec;
            double mn = dv->minsignal();
            double mx = dv->maxsignal();
            set_scale(mn, mx, &mn, &mx, &nspa, 1e5);
            dv->set_minsignal(mn);
            dv->set_maxsignal(mx);
        }
        gr_numycells = gr_numtraces;
    }
    else {
        if (gr_grpmin[0] < gr_grpmax[0])
            set_scale_4(gr_grpmin[0], gr_grpmax[0], gr_grpmin, gr_grpmax, 1e5);
        if (gr_grpmin[1] < gr_grpmax[1])
            set_scale_4(gr_grpmin[1], gr_grpmax[1], gr_grpmin+1, gr_grpmax+1, 1e5);
        if (gr_grpmin[2] < gr_grpmax[2])
            set_scale_4(gr_grpmin[2], gr_grpmax[2], gr_grpmin+2, gr_grpmax+2, 1e5);
        for (link = (sDvList*)gr_plotdata; link; link = link->dl_next) {
            sDataVec *dv = link->dl_dvec;
            double mn = dv->minsignal();
            double mx = dv->maxsignal();
            set_scale_4(mn, mx, &mn, &mx, 1e5);
            dv->set_minsignal(mn);
            dv->set_maxsignal(mx);
        }
        gr_numycells = 4;
    }
}


namespace {
    // This is a rather ugly hack to see how many significant digits are
    // needed for the scale printout.
    //
    int sigfig(double lmt, double hmt, double dval, double sc)
    {
        int sf = 0;
        char buf[64];
        for (double val = lmt; val < hmt; val += dval) {
            if (fabs(val) < 1e-7*dval)
                val = 0.0;
            sprintf(buf, "%.10f", val*sc);
            char *t = buf + strlen(buf) - 1;
            while (*t == '0')
                t--;
            int i = 0;
            while (t >= buf && *t != '.') {
                t--;
                i++;
            }
            if (i > sf)
                sf = i;
        }
        return (sf);
    }
}


// Call first with dosetup true for both axes so that the viewport width
// and height is consistent when rendering.
//
void
sGraph::drawlingrid(Axis axis, bool dosetup)
{
    const char *msg = "%cdelta is negative -- ignored.\n";
    char *units = 0;
    int nsp = 0, spacing = 0;
    double lmt = 0.0, hmt = 0.0, scale = 1.0;
    if (axis == x_axis) {
        double delta = gr_xdelta;
        if (delta < 0.0) {
            GRpkgIf()->ErrPrintf(ET_WARN, msg, 'x');
            delta = 0.0;
        }
        if (delta == 0.0)
            gr_xaxis.lin.spacing = gr_vport.width() / gr_xaxis.lin.numspace;
        else {
            // The user told us where to put the grid lines.  They will
            // not be equally spaced in this case (i.e, the right edge
            // won't be a line).
            //
            gr_xaxis.lin.numspace =
                (int)((gr_datawin.xmax - gr_datawin.xmin)/delta);
            gr_xaxis.lin.spacing = (int)(gr_vport.width()*delta/
                (gr_datawin.xmax - gr_datawin.xmin));
        }
        units = gr_xaxis.lin.units;
        spacing = gr_xaxis.lin.spacing;
        nsp = gr_xaxis.lin.numspace;
        lmt = gr_xaxis.lin.lowlimit;
        hmt = gr_xaxis.lin.highlimit;
        scale = gr_xaxis.lin.mult;
        gr_vport.set_width(spacing*nsp);
        gr_aspect_x = (gr_datawin.xmax - gr_datawin.xmin)/gr_vport.width();
    }
    else {
        double delta = gr_ydelta;
        if (delta < 0.0) {
            GRpkgIf()->ErrPrintf(ET_WARN, msg, 'y');
            delta = 0.0;
        }
        if (delta == 0.0)
            gr_yaxis.lin.spacing = gr_vport.height()/gr_yaxis.lin.numspace;
        else {
            // The user told us where to put the grid lines.  They will
            // not be equally spaced in this case (i.e, the right edge
            // won't be a line).
            //
            gr_yaxis.lin.numspace =
                (int)((gr_datawin.ymax - gr_datawin.ymin)/delta);
            gr_yaxis.lin.spacing = (int)(gr_vport.height()*delta/
                (gr_datawin.ymax - gr_datawin.ymin));
        }
        if (gr_format != FT_SINGLE || gr_ysep)
            gr_vport.set_height((gr_vport.height()/gr_numycells)*gr_numycells);
        else {
            units = gr_yaxis.lin.units;
            spacing = gr_yaxis.lin.spacing;
            nsp = gr_yaxis.lin.numspace;
            lmt = gr_yaxis.lin.lowlimit;
            hmt = gr_yaxis.lin.highlimit;
            scale = gr_yaxis.lin.mult;
            gr_vport.set_height(spacing*nsp);
            gr_aspect_y =
                (gr_datawin.ymax - gr_datawin.ymin)/gr_vport.height();
        }
    }
    if (dosetup)
        return;

    if (axis == y_axis && (gr_format != FT_SINGLE || gr_ysep)) {
        int sy = gr_vport.height()/gr_numycells;
        gr_dev->setDefaultLinestyle(1);
        if (gr_ysep) {
            int nsy = gr_fonthei/4;
            if (nsy < 2)
                nsy = 2;
            for (int y = 0; y+sy <= gr_vport.height(); y += sy) {
                if (!gr_nogrid || y == 0) {
                    gr_dev->Line(gr_vport.left(),
                        yinv(y+nsy + gr_vport.bottom()),
                        gr_vport.right(), yinv(y+nsy + gr_vport.bottom()));
                    if (!gr_nogrid)
                        gr_dev->Line(gr_vport.left(),
                            yinv(y+sy + gr_vport.bottom()),
                            gr_vport.right(), yinv(y+sy + gr_vport.bottom()));
                    else
                        gr_dev->Line(gr_vport.left() - gr_fontwid/2,
                            yinv(y+sy + gr_vport.bottom()),
                            gr_vport.left(), yinv(y+sy + gr_vport.bottom()));
                }
                else {
                    gr_dev->Line(gr_vport.left() - gr_fontwid/2,
                        yinv(y+nsy + gr_vport.bottom()),
                        gr_vport.left(), yinv(y+nsy + gr_vport.bottom()));
                    gr_dev->Line(gr_vport.left() - gr_fontwid/2,
                        yinv(y+sy + gr_vport.bottom()),
                        gr_vport.left(), yinv(y+sy + gr_vport.bottom()));
                }
            }
        }
        else {
            for (int y = 0; y <= gr_vport.height(); y += sy) {
                if (!gr_nogrid || y == 0)
                    gr_dev->Line(gr_vport.left(), yinv(y + gr_vport.bottom()),
                        gr_vport.right(), yinv(y + gr_vport.bottom()));
                else
                    gr_dev->Line(gr_vport.left() - gr_fontwid/2,
                        yinv(y + gr_vport.bottom()), gr_vport.left(),
                        yinv(y + gr_vport.bottom()));
            }
        }

        // Clear the Y-Units text string, if set.
        sKeyed *kp = 0;
        for (sKeyed *k = gr_keyed; k; k = k->next) {
            if (k->type == LAyunits) {
                if (kp)
                    kp->next = k->next;
                else
                    gr_keyed = k->next;
                delete k;
                break;
            }
            kp = k;
        }
        gr_dev->SetLinestyle(0);
        return;
    }

    int i;
    double dval = (hmt - lmt)/nsp;
    double val;
    hmt += dval/2.0;
    int xf = sigfig(lmt, hmt, dval, scale);

    char buf[64];
    // fudge the right boundary to keep the scale factor visible
    if (axis == x_axis) {
        sprintf(buf, "%.*f", xf, hmt*scale);
        int len = strlen(buf);
        i = len - 7;
        if (i > 0) {
            int w = gr_vport.width() - (i*gr_fontwid)/2;
            gr_xaxis.lin.spacing = w / nsp;
            spacing = gr_xaxis.lin.spacing;
            gr_vport.set_width(spacing*nsp);
        }
    }

    // don't bother switching linestyle for axes
    gr_dev->setDefaultLinestyle(1);
    for (i = 0, val = lmt; val < hmt; i += spacing, val += dval) {
        if (axis == x_axis && gr_ysep) {
            int sy = gr_vport.height()/gr_numycells;
            int nsy = gr_fonthei/4;
            if (nsy < 2)
                nsy = 2;
            if (!gr_nogrid || i == 0) {
                for (int y = 0; y+sy <= gr_vport.height(); y += sy) {
                    gr_dev->Line(gr_vport.left() + i,
                        yinv(y+nsy + gr_vport.bottom()),
                        gr_vport.left() + i, yinv(y+sy + gr_vport.bottom()));
                }
            }
            else
                gr_dev->Line(gr_vport.left() + i,
                    yinv(gr_vport.bottom() + nsy),
                    gr_vport.left() + i,
                    yinv(gr_vport.bottom() - gr_fontwid/2));
        }
        else if (!gr_nogrid || i == 0) {
            if (axis == x_axis)
                gr_dev->Line(gr_vport.left() + i, yinv(gr_vport.bottom()),
                    gr_vport.left() + i, yinv(gr_vport.top()));
            else
                gr_dev->Line(gr_vport.left(), yinv(gr_vport.bottom() + i), 
                    gr_vport.right(), yinv(gr_vport.bottom() + i));
        }
        else {
            if (axis == x_axis)
                gr_dev->Line(gr_vport.left() + i, yinv(gr_vport.bottom()),
                    gr_vport.left() + i,
                    yinv(gr_vport.bottom() - gr_fontwid/2));
            else
                gr_dev->Line(gr_vport.left() - gr_fontwid/2,
                    yinv(gr_vport.bottom() + i), gr_vport.left(), 
                    yinv(gr_vport.bottom() + i));
        }
        // fix numerical problem when val ~ 0
        if (fabs(val) < 1e-7*dval)
            val = 0.0;
        sprintf(buf, "%.*f", xf, val*scale);
        if (axis == x_axis)
            gr_dev->Text(buf, gr_vport.left() + i - (gr_fontwid*strlen(buf))/2, 
                yinv(gr_vport.bottom() - gr_fonthei - gr_fonthei/2), 0);
        else
            gr_dev->Text(buf, gr_vport.left() - gr_fontwid*(strlen(buf) + 1), 
                yinv(gr_vport.bottom() + i - gr_fonthei/2), 0);
    }
    if (axis == x_axis)
        gr_save_text(units, gr_vport.right(),
            gr_vport.bottom() - 3*gr_fonthei, LAxunits, 1, TXTF_HJR);
    else
        gr_save_text(units, effleft() + gr_fontwid,
            gr_vport.top() + gr_fonthei/2, LAyunits, 1, 0);
    gr_dev->SetLinestyle(0);
    gr_dev->Update();
}


void
sGraph::loggrid(double *nscale, Axis axis)
{
    if (axis == x_axis) {
        set_log_grid(nscale, gr_datawin.xmin, gr_datawin.xmax, &gr_xaxis);
        strcpy(gr_xaxis.log.units, "log ");
        char *s = gr_xunits.unitstr();
        if (s && *s)
            strcat(gr_xaxis.log.units, s);
        delete [] s;
    }
    else {
        set_raw_trace_limits();
        set_log_trace_limits();
        set_log_grid(nscale, gr_datawin.ymin, gr_datawin.ymax, &gr_yaxis);
        strcpy(gr_yaxis.log.units, "log ");
        char *s = gr_yunits.unitstr();
        if (s && *s)
            strcat(gr_yaxis.log.units, s);
        delete [] s;

        // find how much space to leave for scale factors
        int hmt = gr_yaxis.log.hmt;
        int lmt = gr_yaxis.log.lmt;
        int pp = gr_yaxis.log.pp;
        int j, slen = 0;
        for (j = lmt; j <= hmt; j += pp) {
            char buf[16];
            sprintf(buf, "%d", j);
            int len = strlen(buf);
            if (len > slen)
                slen = len;
        }
        gr_scalewidth = slen + 1;

    }
}


void
sGraph::set_log_trace_limits()
{
    int nspa = 0;
    if (!gr_ysep) {
        uGrid grid;
        if (gr_grpmin[0] < gr_grpmax[0]) {
            set_log_grid(0, gr_grpmin[0], gr_grpmax[0], &grid);
            if (grid.log.hmt - grid.log.lmt > nspa)
                nspa = grid.log.hmt - grid.log.lmt;
        }
        if (gr_grpmin[1] < gr_grpmax[1]) {
            set_log_grid(0, gr_grpmin[1], gr_grpmax[1], &grid);
            if (grid.log.hmt - grid.log.lmt > nspa)
                nspa = grid.log.hmt - grid.log.lmt;
        }
        if (gr_grpmin[2] < gr_grpmax[2]) {
            set_log_grid(0, gr_grpmin[2], gr_grpmax[2], &grid);
            if (grid.log.hmt - grid.log.lmt > nspa)
                nspa = grid.log.hmt - grid.log.lmt;
        }
        for (sDvList *link = (sDvList*)gr_plotdata; link;
                link = link->dl_next) {
            sDataVec *dv = link->dl_dvec;
            set_log_grid(0, dv->minsignal(), dv->maxsignal(), &grid);
            if (grid.log.hmt - grid.log.lmt > nspa)
                nspa = grid.log.hmt - grid.log.lmt;
        }
        gr_numycells = nspa;
    }
    else
        gr_numycells = gr_numtraces;

    if (gr_grpmin[0] < gr_grpmax[0])
        fix_log_trace_limits(&gr_grpmin[0], &gr_grpmax[0], nspa);
    if (gr_grpmin[1] < gr_grpmax[1])
        fix_log_trace_limits(&gr_grpmin[1], &gr_grpmax[1], nspa);
    if (gr_grpmin[2] < gr_grpmax[2])
        fix_log_trace_limits(&gr_grpmin[2], &gr_grpmax[2], nspa);
    for (sDvList *link = (sDvList*)gr_plotdata; link; link = link->dl_next) {
        sDataVec *dv = link->dl_dvec;
        double mn = dv->minsignal();
        double mx = dv->maxsignal();
        fix_log_trace_limits(&mn, &mx, nspa);
        dv->set_minsignal(mn);
        dv->set_maxsignal(mx);
    }
}


// Call first with dosetup true for both axes so that the viewport width
// and height is consistent when rendering
//
void
sGraph::drawloggrid(Axis axis, bool dosetup)
{
    int hmt = 0, lmt = 0, decsp = 0, subs = 0, pp = 0;
    if (axis == x_axis) {
        gr_xaxis.log.decsp = gr_vport.width()*gr_xaxis.log.pp/
            (gr_xaxis.log.hmt - gr_xaxis.log.lmt);
        hmt = gr_xaxis.log.hmt;
        lmt = gr_xaxis.log.lmt;
        decsp = gr_xaxis.log.decsp;
        subs = gr_xaxis.log.subs;
        pp = gr_xaxis.log.pp;
        gr_vport.set_width(decsp*(hmt - lmt)/pp);
        gr_aspect_x = (gr_datawin.xmax - gr_datawin.xmin)/gr_vport.width();
    }
    else {
        gr_yaxis.log.decsp = gr_vport.height()*gr_yaxis.log.pp/
            (gr_yaxis.log.hmt - gr_yaxis.log.lmt);
        if (gr_format != FT_SINGLE || gr_ysep)
            gr_vport.set_height((gr_vport.height()/gr_numycells)*gr_numycells);
        else {
            hmt = gr_yaxis.log.hmt;
            lmt = gr_yaxis.log.lmt;
            decsp = gr_yaxis.log.decsp;
            subs = gr_yaxis.log.subs;
            pp = gr_yaxis.log.pp;
            gr_vport.set_height(decsp*(hmt - lmt)/pp);
        }
        gr_aspect_y = (gr_datawin.ymax - gr_datawin.ymin)/gr_vport.height();
    }
    if (dosetup)
        return;

    if (axis == y_axis && (gr_format != FT_SINGLE || gr_ysep)) {
        gr_dev->setDefaultLinestyle(1);
        int sy = gr_vport.height()/gr_numycells;
        if (gr_ysep) {
            int nsy = gr_fonthei/4;
            if (nsy < 2)
                nsy = 2;
            for (int y = 0; y+sy <= gr_vport.height(); y += sy) {
                if (!gr_nogrid || y == 0) {
                    gr_dev->Line(gr_vport.left(),
                        yinv(y+nsy + gr_vport.bottom()),
                        gr_vport.right(), yinv(y+nsy + gr_vport.bottom()));
                    if (!gr_nogrid)
                        gr_dev->Line(gr_vport.left(),
                            yinv(y+sy + gr_vport.bottom()),
                            gr_vport.right(), yinv(y+sy + gr_vport.bottom()));
                    else
                        gr_dev->Line(gr_vport.left() - gr_fontwid/2,
                            yinv(y+sy + gr_vport.bottom()),
                            gr_vport.left(), yinv(y+sy + gr_vport.bottom()));
                }
                else {
                    gr_dev->Line(gr_vport.left() - gr_fontwid/2,
                        yinv(y+nsy + gr_vport.bottom()),
                        gr_vport.left(), yinv(y+nsy + gr_vport.bottom()));
                    gr_dev->Line(gr_vport.left() - gr_fontwid/2,
                        yinv(y+sy + gr_vport.bottom()),
                        gr_vport.left(), yinv(y+sy + gr_vport.bottom()));
                }
            }
        }
        else {
            for (int y = 0; y <= gr_vport.height(); y += sy) {
                if (!gr_nogrid || y == 0)
                    gr_dev->Line(gr_vport.left(), yinv(y + gr_vport.bottom()), 
                        gr_vport.right(), yinv(y + gr_vport.bottom()));
                else
                    gr_dev->Line(gr_vport.left() - gr_fontwid/2,
                        yinv(y + gr_vport.bottom()), gr_vport.left(),
                        yinv(y + gr_vport.bottom()));
            }
        }
        gr_dev->SetLinestyle(0);
        return;
    }

    // Now plot every pp'th decade line, with subs lines between them
    int i, j;
    for (i = 0, j = lmt; j <= hmt; i += decsp, j += pp) {
        if (axis == x_axis && gr_ysep) {
            int sy = gr_vport.height()/gr_numycells;
            int nsy = gr_fonthei/4;
            if (nsy < 2)
                nsy = 2;
            if (!gr_nogrid || i == 0) {
                for (int y = 0; y+sy <= gr_vport.height(); y += sy) {
                    gr_dev->Line(gr_vport.left() + i,
                        yinv(y+nsy + gr_vport.bottom()),
                        gr_vport.left() + i, yinv(y+sy + gr_vport.bottom()));
                }
            }
            else
                gr_dev->Line(gr_vport.left() + i,
                    yinv(gr_vport.bottom() + nsy),
                    gr_vport.left() + i,
                    yinv(gr_vport.bottom() - gr_fontwid/2));
        }
        else if (!gr_nogrid || i == 0) {
            if (axis == x_axis)
                gr_dev->Line(gr_vport.left() + i, yinv(gr_vport.bottom()), 
                    gr_vport.left() + i, yinv(gr_vport.top()));
            else
                gr_dev->Line(gr_vport.left(), yinv(gr_vport.bottom() + i), 
                    gr_vport.right(), yinv(gr_vport.bottom() + i));
        }
        else {
            if (axis == x_axis) {
                gr_dev->Line(gr_vport.left() + i, yinv(gr_vport.bottom()),
                    gr_vport.left() + i,
                    yinv(gr_vport.bottom() - gr_fontwid/2));
            }
            else
                gr_dev->Line(gr_vport.left() - gr_fontwid/2,
                    yinv(gr_vport.bottom() + i), gr_vport.left(), 
                    yinv(gr_vport.bottom() + i));
        }
        char buf[16];
        sprintf(buf, "%d", j);
        if (axis == x_axis)
            gr_dev->Text(buf, gr_vport.left() + i - strlen(buf) / 2, 
                yinv(gr_vport.bottom() - gr_fonthei - gr_fonthei/2), 0);
        else
            gr_dev->Text(buf, gr_vport.left() - gr_fontwid*(strlen(buf) + 1), 
                yinv(gr_vport.bottom() + i - gr_fonthei/2), 0);

        if (j != hmt) {
            // Now draw the other lines
            for (int k = 1; k <= subs; k++) {
                int l = (int)(ceil((double) k * 10 / subs));
                if (l == 10)
                    break;
                int m = (int)(decsp * log10((double ) l)) + i;
                if (!gr_nogrid) {
                    if (axis == x_axis) {
                        if (gr_ysep) {
                            int sy = gr_vport.height()/gr_numycells;
                            int nsy = gr_fonthei/4;
                            if (nsy < 2)
                                nsy = 2;
                            for (int y = 0; y+sy <= gr_vport.height();
                                    y += sy) {
                                gr_dev->Line(gr_vport.left() + m,
                                    yinv(y+nsy + gr_vport.bottom()),
                                    gr_vport.left() + m,
                                    yinv(y+sy + gr_vport.bottom()));
                            }
                        }
                        else
                            gr_dev->Line(gr_vport.left() + m,
                                yinv(gr_vport.bottom()), gr_vport.left() + m, 
                                yinv(gr_vport.top()));
                    }
                    else
                        gr_dev->Line(gr_vport.left(), 
                            yinv(gr_vport.bottom() + m), gr_vport.right(), 
                            yinv(gr_vport.bottom() + m));
                }
            }
        }
    }
    if (axis == x_axis)
        gr_save_text(gr_xaxis.log.units, gr_vport.right(),
            gr_vport.bottom() - 3*gr_fonthei, LAxunits, 1, TXTF_HJR);
    else
        gr_save_text(gr_yaxis.log.units, effleft() + gr_fontwid,
            gr_vport.top() + gr_fonthei/2, LAyunits, 1, 0);
    gr_dev->Update();
}


void
sGraph::set_raw_trace_limits()
{
    if (!(gr_scale_flags & 2)) {
        gr_grpmin[0] = 1;
        gr_grpmax[0] = -1;
    }
    if (!(gr_scale_flags & 4)) {
        gr_grpmin[1] = 1;
        gr_grpmax[1] = -1;
    }
    if (!(gr_scale_flags & 8)) {
        gr_grpmin[2] = 1;
        gr_grpmax[2] = -1;
    }
    double ymin = 1;
    double ymax = -1;
    unsigned mask = 16;
    for (sDvList *link = (sDvList*)gr_plotdata; link; link = link->dl_next) {
        mask <<= 1;
        double dd[2], lo, hi;
        if (link->dl_dvec->scale() && !gr_oneval)
            interval_minmax(link->dl_dvec, link->dl_dvec->scale(),
                gr_rawdata.xmin, gr_rawdata.xmax, &lo, &hi);
        else {
            link->dl_dvec->minmax(dd, true);
            lo = dd[0];
            hi = dd[1];
        }
        if (*link->dl_dvec->units() == UU_VOLTAGE) {
            if (!(gr_scale_flags & 2)) {
                if (gr_grpmin[0] > gr_grpmax[0]) {
                    gr_grpmin[0] = lo;
                    gr_grpmax[0] = hi; 
                }       
                else {
                    if (lo < gr_grpmin[0]) gr_grpmin[0] = lo;
                    if (hi > gr_grpmax[0]) gr_grpmax[0] = hi;
                }
            }
        }
        else if (*link->dl_dvec->units() == UU_CURRENT) {
            if (!(gr_scale_flags & 4)) {
                if (gr_grpmin[1] > gr_grpmax[1]) {
                    gr_grpmin[1] = lo;
                    gr_grpmax[1] = hi;
                }
                else {
                    if (lo < gr_grpmin[1]) gr_grpmin[1] = lo;
                    if (hi > gr_grpmax[1]) gr_grpmax[1] = hi;
                }
            }
        }
        else {
            if (!(gr_scale_flags & 8)) {
                if (gr_grpmin[2] > gr_grpmax[2]) {
                    gr_grpmin[2] = lo;
                    gr_grpmax[2] = hi;
                }
                else {
                    if (lo < gr_grpmin[2]) gr_grpmin[2] = lo;
                    if (hi > gr_grpmax[2]) gr_grpmax[2] = hi;
                }
            }
        }
        if (!(gr_scale_flags & mask)) {
            link->dl_dvec->set_minsignal(lo);
            link->dl_dvec->set_maxsignal(hi);
        }
        if (ymax < ymin) {
            ymin = lo;
            ymax = hi;
        }
        else {
            if (lo < ymin)
                ymin = lo;
            if (hi > ymax)
                ymax = hi;
        }
    }
    if (gr_format != FT_SINGLE) {
        if (!(gr_scale_flags & 16)) {
            gr_datawin.ymin = ymin;
            gr_datawin.ymax = ymax;
        }
    }
}


// Polar grids.
//
void
sGraph::polargrid()
{
    // Figure out the minimum and maximum radii we're dealing with
    double mx = (gr_datawin.xmin + gr_datawin.xmax)/2;
    double my = (gr_datawin.ymin + gr_datawin.ymax)/2;
    double d = sqrt(mx * mx + my * my);
    double maxrad = d + (gr_datawin.xmax - gr_datawin.xmin)/2;
    double minrad = d - (gr_datawin.xmax - gr_datawin.xmin)/2;

    if (maxrad == 0.0)
        maxrad = 1.0;
    if ((gr_datawin.xmin < 0) && (gr_datawin.ymin < 0) && 
            (gr_datawin.xmax > 0) && (gr_datawin.ymax > 0))
        minrad = 0;

    // Make sure that the range is square
    mx = gr_datawin.xmax - gr_datawin.xmin;
    my = gr_datawin.ymax - gr_datawin.ymin;
    if (mx > my) {
        gr_datawin.ymin -= (mx - my) / 2;
        gr_datawin.ymax += (mx - my) / 2;
    }
    else if (mx < my) {
        gr_datawin.xmin -= (my - mx) / 2;
        gr_datawin.xmax += (my - mx) / 2;
    }

    int mag = floorlog(maxrad);
    double tenpowmag = pow(10.0, (double)mag);
    int hmt = (int)(maxrad / tenpowmag);
    int lmt = (int)(minrad / tenpowmag);
    if (hmt * tenpowmag < maxrad)
        hmt++;
    if (lmt * tenpowmag > minrad)
        lmt--;

    gr_xaxis.circular.hmt = hmt;
    gr_xaxis.circular.lmt = lmt;
    gr_xaxis.circular.mag = mag;
}


void
sGraph::drawpolargrid()
{
    // Make sure that our area is square
    if (gr_vport.width() > gr_vport.height())
        gr_vport.set_width(gr_vport.height());
    else
        gr_vport.set_height(gr_vport.width());

    // Make sure that the borders are even
    if (gr_vport.width() & 1) {
        gr_vport.set_width(gr_vport.width() + 1);
        gr_vport.set_height(gr_vport.height() + 1);
    }
    gr_aspect_x = (gr_datawin.xmax - gr_datawin.xmin)/gr_vport.width();
    gr_aspect_y = (gr_datawin.ymax - gr_datawin.ymin)/gr_vport.height();

    gr_xaxis.circular.center = gr_vport.width()/2 + gr_vport.left();
    gr_yaxis.circular.center = gr_vport.height()/2 + gr_vport.bottom();
    gr_xaxis.circular.radius = gr_vport.width()/2;

    int hmt = gr_xaxis.circular.hmt;
    int lmt = gr_xaxis.circular.lmt;
    int mag = gr_xaxis.circular.mag;
    double tenpowmag = pow(10.0, (double)mag);

    int step = 1;
    if (lmt == 0 && hmt > 5) {
        if (!(hmt % 2))
            step = 2;
        else if (!(hmt % 3))
            step = 3;
        else
            step = 1;
    }
    double pixperunit = gr_xaxis.circular.radius*2 /
        (gr_datawin.xmax - gr_datawin.xmin);

    int relcx = (int)(-(gr_datawin.xmin + gr_datawin.xmax)/2 * pixperunit);
    int relcy = (int)(-(gr_datawin.ymin + gr_datawin.ymax)/2 * pixperunit);
    int dist = (int)sqrt((double) (relcx * relcx + relcy * relcy));

    gr_dev->SetLinestyle(0);
    gr_dev->Arc(gr_xaxis.circular.center, yinv(gr_yaxis.circular.center),
        gr_xaxis.circular.radius, gr_xaxis.circular.radius, 0.0, 0.0);
    gr_dev->setDefaultLinestyle(1);

    // Now draw the circles.
    int i;
    double theta;
    int relrad, degs;
    int x1, y1, x2, y2;
    char buf[64];
    for (i = lmt; (relrad = (int)(i*tenpowmag*pixperunit)) <= 
            dist + gr_xaxis.circular.radius; i += step) {
        clip_arc(0.0, 0.0, gr_xaxis.circular.center + relcx, 
            gr_yaxis.circular.center + relcy, relrad,
            gr_xaxis.circular.center, gr_yaxis.circular.center, 
            gr_xaxis.circular.radius, 0);
        // Toss on the label
        if (relcx || relcy)
            theta = atan2((double) relcy, (double) relcx);
        else
            theta = M_PI;
        if (i && (relrad > dist - gr_xaxis.circular.radius))
            add_rad_label(i, theta, (int)(gr_xaxis.circular.center -
                (relrad - dist) * cos(theta)), (int)(gr_yaxis.circular.center -
                (relrad - dist) * sin(theta)));
    }

    // Now draw the spokes.  We have two possible cases -- first, the
    // origin may be inside the area -- in this case draw 12 spokes.
    // Otherwise, draw several spokes at convenient places.
    //
    if ((gr_datawin.xmin <= 0.0)
            && (gr_datawin.xmax >= 0.0)
            && (gr_datawin.ymin <= 0.0)
            && (gr_datawin.ymax >= 0.0)) {
        for (i = 0; i < 12; i++) {
            x1 = gr_xaxis.circular.center + relcx;
            y1 = gr_yaxis.circular.center + relcy;
            x2 = x1 + (int)(gr_xaxis.circular.radius * 2*cos(i * M_PI / 6));
            y2 = y1 + (int)(gr_xaxis.circular.radius * 2*sin(i * M_PI / 6));
            if (!clip_to_circle(&x1, &y1, &x2, &y2, 
                    gr_xaxis.circular.center, gr_yaxis.circular.center, 
                    gr_xaxis.circular.radius)) {
                gr_dev->Line(x1, yinv(y1), x2, yinv(y2)); 
                    // Add a label here
                    add_deg_label(i*30, x2, y2, x1, y1,
                        gr_xaxis.circular.center, gr_yaxis.circular.center);
            }
        }
    }
    else {
        // Figure out the angle that we have to fill up
        theta = 2 * asin((double) gr_xaxis.circular.radius/dist);
        theta = theta * 180 / M_PI;   // Convert to degrees
        
        // See if we should put lines at 30, 15, 5, or 1 degree 
        // increments.
        //
        if (theta / 30 > 3)
            degs = 30;
        else if (theta / 15 > 3)
            degs = 15;
        else if (theta / 5 > 3)
            degs = 5;
        else
            degs = 1;

        // We'll be cheap
        for (i = 0; i < 360; i+= degs) {
            x1 = gr_xaxis.circular.center + relcx;
            y1 = gr_yaxis.circular.center + relcy;
            x2 = x1 + (int)(dist * 2 * cos(i * M_PI / 180));
            y2 = y1 + (int)(dist * 2 * sin(i * M_PI / 180));
            if (!clip_to_circle(&x1, &y1, &x2, &y2, 
                    gr_xaxis.circular.center, gr_yaxis.circular.center, 
                    gr_xaxis.circular.radius)) {
                gr_dev->Line(x1, yinv(y1), x2, yinv(y2));
                // Put on the label.
                add_deg_label(i, x2, y2, x1, y1,
                    gr_xaxis.circular.center, gr_yaxis.circular.center);
            }
        }
    }

    sprintf(buf, "e%d", mag);
    gr_save_text(buf, gr_xaxis.circular.center + gr_xaxis.circular.radius, 
        gr_yaxis.circular.center - gr_xaxis.circular.radius, LAxunits, 1, 0);
    gr_dev->Update();
}


void
sGraph::smithgrid()
{
    double mx = gr_datawin.xmax - gr_datawin.xmin;
    double my = gr_datawin.ymax - gr_datawin.ymin;
    if (mx > my) {
        gr_datawin.ymin -= (mx - my)/2;
        gr_datawin.ymax += (mx - my)/2;
    }
    else if (mx < my) {
        gr_datawin.xmin -= (my - mx)/2;
        gr_datawin.xmax += (my - mx)/2;
    }
    gr_scalewidth = 7;
}


// Maximum number of circles.
#define CMAX  50

void
sGraph::drawsmithgrid()
{
    // Make sure that our area is square
    if (gr_vport.width() > gr_vport.height())
        gr_vport.set_width(gr_vport.height());
    else
        gr_vport.set_height(gr_vport.width());

    // Make sure that the borders are even
    if (gr_vport.width() & 1) {
        gr_vport.set_width(gr_vport.width() + 1);
        gr_vport.set_height(gr_vport.height() + 1);
    }
    gr_aspect_x = (gr_datawin.xmax - gr_datawin.xmin)/gr_vport.width();
    gr_aspect_y = (gr_datawin.ymax - gr_datawin.ymin)/gr_vport.height();

    gr_xaxis.circular.center = gr_vport.width()/2 + gr_vport.left();
    gr_yaxis.circular.center = gr_vport.height()/2 + gr_vport.bottom();
    gr_xaxis.circular.radius = gr_vport.width()/2;
    // Figure out the minimum and maximum radii we're dealing with
    double mx = (gr_datawin.xmin + gr_datawin.xmax)/2;
    double my = (gr_datawin.ymin + gr_datawin.ymax)/2;
    double d = sqrt(mx*mx + my*my);
    double maxrad = d + (gr_datawin.xmax - gr_datawin.xmin)/2;

    int mag = floorlog(maxrad);
    double pixperunit = gr_vport.width()/(gr_datawin.xmax - 
            gr_datawin.xmin);

    int xoff = (int)(-pixperunit*(gr_datawin.xmin + gr_datawin.xmax)/2);
    int yoff = (int)(-pixperunit*(gr_datawin.ymin + gr_datawin.ymax)/2);

    // Sweep the range from 10e-20 to 10e20.  If any arcs fall into the
    // picture, plot the arc set.
    //
    int j = 0, i = 0;
    for (mag = -20; mag < 20; mag++) {
        i = (int)(gr_xaxis.circular.radius *
            pow(10.0, (double)mag)/maxrad);
        if (i > 10) {
            j = 1;
            break;
        }
        else if (i > 5) {
            j = 2;
            break;
        }
        else if (i > 2) {
            j = 5;
            break;
        }
    }
    int k = 1;

    int xplace = gr_vport.right();
    gr_dev->SetLinestyle(0);

    // Now plot all the arc sets.  Go as high as 5 times the radius that
    // will fit on the screen.  The base magnitude is one more than 
    // the least magnitude that will fit...
    //
    int basemag;
    if (i > 20)
        basemag = mag;
    else
        basemag = mag + 1;
    // Go back one order of magnitude and have a closer look
    mag -= 2;
    j *= 10;
    double rnrm[CMAX];
    double dphi[CMAX];
    double ir[CMAX], rr[CMAX], ki[CMAX], kr[CMAX], ks[CMAX];
    int zheight, plen;
    char plab[32], nlab[32];
    while (mag < 20) {
        i = (int)(j * pow(10.0, (double)mag) * pixperunit/2);
        if (i/5 > gr_xaxis.circular.radius + ((xoff > 0) ? xoff : -xoff))
            break;
        rnrm[k] = j * pow(10.0, (double)(mag - basemag));
        dphi[k] = 2.0 * atan(rnrm[k]);
        ir[k] = pixperunit * (1 + cos(dphi[k])) / sin(dphi[k]);
        rr[k] = pixperunit * 0.5 * (((1 - rnrm[k]) / (1 + rnrm[k])) + 1);
        sprintf(plab, "%g", rnrm[k]);
        plen = strlen(plab);

        // See if the label will fit on the upper xaxis.
        // wait for some k, so we don't get fooled
        if (k > 6) {
        if ((int)(gr_xaxis.circular.radius - xoff - pixperunit + 2*rr[k]) <
            plen * gr_fontwid + 2)
            break;
        }
        // See if the label will fit on the lower xaxis.
        // First look at the leftmost circle possible
        if ((int)(pixperunit - 2*rr[k] + gr_xaxis.circular.radius + xoff +
                fabs((double) yoff)) < plen * gr_fontwid + 4) { 
            if (j == 95) {
                j = 10;
                mag++;
            }
            else {
                if (j < 20)
                    j += 1;
                else 
                    j += 5;
            }
            continue;
        }
        // Then look at the circles following in the viewport.
        if (k > 1 && (int)2*(rr[k-1] - rr[k]) < plen * gr_fontwid + 4) {
            if (j == 95) {
                j = 10;
                mag++;
            }
            else {
                if (j < 20)
                    j += 1;
                else 
                    j += 5;
            }
            continue;
        }
        if (j == 95) {
            j = 10;
            mag++;
        }
        else {
            if (j < 20)
                j += 1;
            else 
                j += 5;
        }
        ki[k-1] = ir[k];
        kr[k-1] = rr[k];
        k++;
        if (k == CMAX) {
            GRpkgIf()->ErrPrintf(ET_WARN, "smith grid too complex\n");
            break;
        }
    }
    k--;

    // Now adjust the clipping radii
    for (i = 0; i < k; i++)
        ks[i] = ki[i];
    for (i = k-1, j = k-1; i >= 0; i -= 2, j--) {
        ki[i] = ks[j];
        if (i > 0)
            ki[i-1] = ks[j];
    }
    for (i = 0; i < k; i++)
        ks[i] = kr[i];
    for (i = k-1, j = k-1; (i >= 0) && (dphi[i] > M_PI / 2); i -= 2, j--) {
        kr[i] = ks[j];
        if (i > 0)
            kr[i-1] = ks[j];
    }
    for ( ; i >= 0; i--, j--)
        kr[i] = ks[j];

    if ((yoff > -gr_xaxis.circular.radius) &&
            (yoff < gr_xaxis.circular.radius)) {
        zheight = (int)(gr_xaxis.circular.radius *
            cos(asin((double)yoff/gr_xaxis.circular.radius)));
        zheight = (zheight > 0) ? zheight : - zheight;
    }
    else
        zheight = gr_xaxis.circular.radius;

#define RAD_TO_DEG (180/M_PI)
    for (ki[k] = kr[k] = (double) 0; k > 0; k--) {
        sprintf(plab, "%g", rnrm[k]);
        sprintf(nlab, "-%g", rnrm[k]);
        arc_set(rr[k], kr[k], ir[k], ki[k], pixperunit,
            xoff, yoff, plab, nlab,
            (int) (0.5 + RAD_TO_DEG * (M_PI - dphi[k])),
            (int) (0.5 + RAD_TO_DEG * (M_PI + dphi[k])),
            gr_xaxis.circular.center - zheight,
            gr_xaxis.circular.center + zheight, &xplace);
    }
    if (mag == 20) {
        GRpkgIf()->ErrPrintf(ET_INTERR, " smithgrid: screwed up!\n");
        return;
    }

    gr_dev->SetLinestyle(0);

    gr_dev->Arc(gr_xaxis.circular.center, yinv(gr_yaxis.circular.center),
        gr_xaxis.circular.radius, gr_xaxis.circular.radius, 0.0, 0.0);

    if ((yoff > -gr_xaxis.circular.radius) &&
            (yoff < gr_xaxis.circular.radius)) {
        zheight = (int)(gr_xaxis.circular.radius *
            cos(asin((double) yoff/gr_xaxis.circular.radius)));
        if (zheight < 0)
            zheight = - zheight;
        gr_dev->Line(gr_xaxis.circular.center - zheight,
            yinv(gr_yaxis.circular.center + yoff),
            gr_xaxis.circular.center + zheight,
            yinv(gr_yaxis.circular.center + yoff));
        gr_dev->Text("0", gr_xaxis.circular.center + zheight + gr_fontwid,
            yinv(gr_yaxis.circular.center + yoff - gr_fonthei/2), 0);
        gr_dev->Text("o", gr_xaxis.circular.center + zheight + gr_fontwid*2,
            yinv(gr_yaxis.circular.center + yoff), 0);
        gr_dev->Text("180", gr_xaxis.circular.center - zheight - gr_fontwid*5,
            yinv(gr_yaxis.circular.center + yoff - gr_fonthei/2), 0);
        gr_dev->Text("o", gr_xaxis.circular.center - zheight - gr_fontwid*2,
            yinv(gr_yaxis.circular.center + yoff), 0);
    }
}


// Find 4 segment scale for data.  Arg fig limits significant figures,
// and is a power of 10, eg 1e5, or 0.0 for no limit.
//
void
sGraph::set_scale_4(double l, double u, double *lnew, double *unew,
    double fig) const
{
    double x;
    if (u < l) {
        x = l;
        l = u;
        u = x;
    }
    else if (u == l) {
        if (l == 0.0 && u == 0.0) {
            *lnew = -1e-12;
            *unew = 1e-12;
            return;
        }
        l -= 0.1*fabs(l);
        u += 0.1*fabs(u);
    }
    else if (fig > 1.0 && SPMAX(fabs(l), fabs(u))/(u - l) > fig) {
        fig = 1.0/fig;
        l *= (l > 0.0 ? 1.0 - fig : 1.0 + fig);
        u *= (u > 0.0 ? 1.0 + fig : 1.0 - fig);
    }
    x = u - l;
    l += x*.001;
    u -= x*.001;
    x = u - l;  
    double e = floor(log10(x));
    double m = x/pow(10.0, e);
    
    double j = 0.0;
    if      (m <= 1.0)  j = 0.1;
    else if (m <= 1.25) j = 0.2;
    else if (m <= 1.5)  j = 0.4;
    else if (m <= 1.75) j = 0.8;
    else if (m <= 2)    j = 1.0;  
    else if (m <= 4)    j = 2.0;
    else if (m <= 8)    j = 4.0;
    else                j = 8.0;
                    
    int n;
    if      (m > j*1.75) n = 8;
    else if (m > j*1.5)  n = 7;
    else if (m > j*1.25) n = 6;
    else if (m > j*1.0)  n = 5;
    else                 n = 4;

    double s = n*j*pow(10.0, e)/4.0;
    if (n == 8)
        n = 4;
    double del = s/n;
    x = l/del;
    while (n != 4 || s + del*floor(x) < u) {
        s += del;
        n += 1;
        if (n == 8)
            n = 4;
        del = s/n;
        x = l/del;
    }
    *lnew = del*floor(x);
    *unew = *lnew + s;
}


// Find nice 1-2-5 scale for data.  Arg fig limits significant figures,
// and is a power of 10, eg 1e5, or 0.0 for no limit.
//
void
sGraph::set_scale(double l, double u, double *lnew, double *unew, int *n,
    double fig) const
{
    *n = 4;
    double x;
    if (u < l) {
        x = l;
        l = u;
        u = x;
    }
    else if (u == l) {
        if (l == 0.0 && u == 0.0) {
            *lnew = -1e-12;
            *unew = 1e-12;
            *n = 4;
            return;
        }
        l -= 0.1*fabs(l);
        u += 0.1*fabs(u);
    }
    else if (fig > 1.0 && SPMAX(fabs(l), fabs(u))/(u - l) > fig) {
        fig = 1.0/fig;
        l *= (l > 0.0 ? 1.0 - fig : 1.0 + fig);
        u *= (u > 0.0 ? 1.0 + fig : 1.0 - fig);
    }
    x = u - l;
    l += x*.001;
    u -= x*.001;
    x = u - l;
    double e = floor(log10(x));
    double m = x / pow(10.0, e);

    double j = 0.0;
    if      (m <= 1.0)  j = 0.1;
    else if (m <= 1.25) j = 0.2;
    else if (m <= 1.5)  j = 0.4;
    else if (m <= 1.75) j = 0.8;
    else if (m <= 2)    j = 1.0;
    else if (m <= 4)    j = 2.0;
    else if (m <= 8)    j = 4.0;
    else                j = 8.0;

    if      (m > j*1.75) *n = 8;
    else if (m > j*1.5)  *n = 7;
    else if (m > j*1.25) *n = 6;
    else if (m > j*1.0)  *n = 5;
    else                 *n = 4;

    double s = (*n)*j*pow(10.0, e)/4.0;
    if (*n == 8)
        *n = 4;
    double del = s / *n;
    x = l/del;
    while (s + del*floor(x) < u) {
        s += del;
        *n += 1;
        if (*n == 8)
            *n = 4;
        del = s/(*n);
        x = l/del;
    }
    *lnew = del*floor(x);
    *unew = *lnew + s;
}


void
sGraph::set_lin_grid(double *nscale, double lo, double hi, uGrid *grid,
    int *magn) const
{
    set_scale(lo, hi, &lo, &hi, &grid->lin.numspace, 0.0);
    if (nscale) {
        nscale[0] = lo;
        nscale[1] = hi;
    }

    double mx = SPMAX(fabs(hi), fabs(lo));
    int mag = floorlog(mx);
    double tenpowmag = pow(10.0, (double) mag);

    grid->lin.mult = 1;
    grid->lin.lowlimit = lo / tenpowmag;
    grid->lin.highlimit = hi / tenpowmag;
    if (scaleunits) {
        if ((mag % 3) && !((mag - 1) % 3)) {
            grid->lin.mult = 10;
            mag--;
        }
        else if ((mag % 3) && !((mag + 1) % 3)) {
            grid->lin.mult = 100;
            mag -= 2;
        }
    }
    if (magn)
        *magn = mag;
}


void
sGraph::set_log_grid(double *nscale, double lo, double hi, uGrid *grid) const
{
    // How many orders of magnitude.  We are already guaranteed that hi
    // and lo are positive. We want to have something like 8 grid lines
    // total, so if there are few orders of magnitude put in intermediate
    // lines.
    //
    grid->log.lmt = floorlog(lo);
    grid->log.hmt = ceillog(hi);
    int decs = grid->log.hmt - grid->log.lmt;
    if (!decs) {
        decs++;
        grid->log.hmt++;
    }
    grid->log.subs = 8 / decs;
    if (decs > 10) {
        grid->log.pp = decs / 10 + 1;
        while (decs % grid->log.pp) {
            grid->log.hmt++;
            decs++;
        }
    }
    else
        grid->log.pp = 1;
    if (nscale) {
        nscale[0] = pow(10.0, (double) grid->log.lmt);
        nscale[1] = pow(10.0, (double) grid->log.hmt);
    }
}


#define LOFF    5
#define MINDIST 10

// Put a degree label on the screen, with 'deg' as the label, near
// point (x, y) such that the perpendicular to (cx, cy) and (x, y)
// doesn't overwrite the label.  If the distance between the center
// and the point is too small, don't put the label on.
//
void
sGraph::add_deg_label(int deg, int x, int y, int cx, int cy, int lx, int ly)
{
    if (sqrt((double) (x - cx)*(x - cx) + (y - cy)*(y - cy)) < MINDIST)
        return;
    char buf[32];
    sprintf(buf, "%d", deg);
    int w = gr_fontwid*(strlen(buf) + 1);
    int h = (int)(gr_fonthei*1.5);
    double angle = atan2((double)(y - ly), (double)(x - lx));
    int d = (int)(fabs(cos(angle))*w/2 + fabs(sin(angle))*h/2) + LOFF;
    
    x += (int)(d*cos(angle) - w/2);
    y += (int)(d*sin(angle) - h/2);
 
    gr_dev->Text(buf, x, yinv(y), 0);
    gr_dev->Text("o", x + strlen(buf)*gr_fontwid, yinv(y + gr_fonthei/2), 0);
}


// This function adds the radial labels.  An attempt is made to
// displace the label from the circles.  Put the label just inside the
// circle, along the line from the logical center to the physical
// center.
//
void
sGraph::add_rad_label(int lab, double theta, int x, int y)
{
    char buf[32];
    sprintf(buf, "%d", lab);
    int fw = strlen(buf) * gr_fontwid + 2;
    int fh = gr_fonthei + 2;
    theta += M_PI;

    if ((theta >= 0 && theta < M_PI/2) || theta >= M_PI + M_PI) {
        y -= fh;
        x -= (int)(fw * cos(theta));
    }
    else if (theta >= M_PI/2 && theta < M_PI) {
        y -= fh;
        x -= (int)(fw * (1 + cos(theta))) - 3;
    }
    else if (theta >= M_PI && theta < 3*M_PI/2)
        x -= (int)(fw * (1 + cos(theta))) - 3;
    else
        x -= (int)(fw * cos(theta));
    
    gr_dev->Text(buf, x, yinv(y), 0);
}


// Draw one arc set.  The arcs should have radius rad.  The outermost
// circle is described by (centx, centy) and maxrad, and the distance
// from the right side of the bounding circle to the logical center of
// the other circles in pixels is xoffset (positive brings the
// negative plane into the picture).  plab and nlab are the labels to
// put on the positive and negative X-arcs, respectively...  If the
// X-axis isn't on the screen, then we have to be clever...
//
void
sGraph::arc_set(double rad, double prevrad, double irad, double iprevrad,
    double radoff, int xoffset, int yoffset, char *plab, char *nlab,
    int pdeg, int ndeg, int pxmin, int pxmax, int *xplace)
{
    (void)nlab;
    double angle = atan2(iprevrad,  rad);
    double iangle = atan2(prevrad,  irad);

    // Let's be lazy and just draw everything -- we won't get called too
    // much and the circles get clipped anyway...
    //
    gr_dev->SetColor(gr_colors[18].pixel);

    double rclip = clip_arc(2*angle, 2*M_PI - 2*angle,
        (int) (gr_xaxis.circular.center + xoffset + radoff - rad),
        (int) (gr_yaxis.circular.center + yoffset), (int)rad,
        gr_xaxis.circular.center, gr_yaxis.circular.center,
        gr_xaxis.circular.radius, 0);

    // Draw the upper and lower circles
    gr_dev->SetColor(gr_colors[19].pixel);
    double aclip = clip_arc(M_PI*1.5 + 2*iangle, M_PI*1.5 - 2*iangle,
        (int) (gr_xaxis.circular.center + xoffset + radoff),
        (int) (gr_yaxis.circular.center + yoffset + irad), (int)irad,
         gr_xaxis.circular.center, gr_yaxis.circular.center,
         gr_xaxis.circular.radius, 1);
    if ((aclip > M_PI / 180) && (pdeg > 1)) {
        int xlab = gr_xaxis.circular.center +
            xoffset + (int)(radoff + irad*cos(aclip));
        int ylab = gr_yaxis.circular.center +
            yoffset + (int)(irad * (1 + sin(aclip)));
        if ((ylab - gr_yaxis.circular.center) > gr_fonthei) {
            gr_dev->SetColor(gr_colors[1].pixel);
            add_deg_label(pdeg, xlab, ylab,
                gr_xaxis.circular.center, gr_yaxis.circular.center,
                gr_xaxis.circular.center, gr_yaxis.circular.center);
            gr_dev->SetColor(gr_colors[19].pixel);
        }
    }
    aclip = clip_arc(M_PI/2 + 2*iangle, M_PI/2 - 2*iangle,
        (int)(gr_xaxis.circular.center + xoffset + radoff),
        (int)(gr_yaxis.circular.center + yoffset - irad), (int)irad,
        gr_xaxis.circular.center, gr_yaxis.circular.center,
        gr_xaxis.circular.radius, (iangle == 0) ? 2 : 0);
    if ((aclip >= 0 && aclip < 2*M_PI - M_PI/180) && (pdeg < 359)) {
        int xlab = gr_xaxis.circular.center +
            xoffset + (int)(radoff + irad*cos(aclip));
        int ylab = gr_yaxis.circular.center +
            yoffset + (int)(irad * (sin(aclip) - 1));
        gr_dev->SetColor(gr_colors[1].pixel);
        add_deg_label(ndeg, xlab, ylab,
            gr_xaxis.circular.center, gr_yaxis.circular.center,
            gr_xaxis.circular.center, gr_yaxis.circular.center);
        gr_dev->SetColor(gr_colors[19].pixel);
    }
    
    // Now toss the labels on...
    gr_dev->SetColor(gr_colors[1].pixel);

    if ((yoffset > - .8*gr_xaxis.circular.radius) &&
            (yoffset < .8*gr_xaxis.circular.radius)) {
        int x = gr_xaxis.circular.center + xoffset + (int)(radoff - 2*rad) -
            gr_fontwid * strlen(plab) - 2;
        if ((x > pxmin) && (x < pxmax))
            gr_dev->Text(plab, x,
                yinv(gr_yaxis.circular.center + yoffset - gr_fonthei - 1), 0);
    }
    else if (rclip != -1 && *xplace > gr_vport.left()) {
        // "Real" arc was shown, print label and advance.
        // The labels are printed left (highest) to right, while there
        // is space.
        //
        *xplace -= strlen(plab)*gr_fontwid;
        gr_dev->Text(plab, *xplace, yinv(gr_vport.top() + 2*gr_fonthei), 0);
        *xplace -= gr_fontwid;
    }
}


// This routine draws an arc and clips it to a circle.  It's hard to
// figure out how it works without looking at the piece of scratch
// paper I have in front of me, so let's hope it doesn't break.
//
double
sGraph::clip_arc(double start, double end, int cx, int cy, int rad,
    int clipx, int clipy, int cliprad, int flag)
{
    int x = cx - clipx;
    int y = cy - clipy;
    double dist = sqrt((double) (x * x + y * y));

    if (!rad || !cliprad)
        return (-1);
    if (dist + rad < cliprad) {
        // The arc is entirely in the boundary
        gr_dev->Arc(cx, yinv(cy), rad, rad, start, end);
        return (flag ? start : end);
    }
    else if ((dist - rad >= cliprad) || (rad - dist >= cliprad)) {
        // The arc is outside of the boundary
        return (-1);
    }
    // Now let's figure out the angles at which the arc crosses the
    // circle. We know dist != 0.
    //
    double phi = atan2((double)y , (double)x);
    double theta = M_PI + phi;
    double alpha =
        (double)(dist*dist + rad*rad - cliprad*cliprad)/(2*dist*rad);
    if (alpha > 1.0)
        alpha = 0.0;
    else if (alpha < -1.0)
        alpha = M_PI;
    else
        alpha = acos(alpha);

    double a1 = theta + alpha;
    double a2 = theta - alpha;
    while (a1 < 0)
        a1 += M_PI * 2;
    while (a2 < 0)
        a2 += M_PI * 2;
    while (a1 >= M_PI * 2)
        a1 -= M_PI * 2;
    while (a2 >= M_PI * 2)
        a2 -= M_PI * 2;

    double tx = cos(start)*rad + x;
    double ty = sin(start)*rad + y;
    double d = sqrt((double)(tx*tx + ty*ty));
    bool in = (d > cliprad) ? false : true;

    // Now begin with start.  If the point is in, draw to either end, a1, 
    // or a2, whichever comes first.
    //
    d = M_PI * 3;
    if ((end < d) && (end > start))
        d = end;
    if ((a1 < d) && (a1 > start))
        d = a1;
    if ((a2 < d) && (a2 > start))
        d = a2;
    if (d == M_PI * 3) {
        d = end;
        if (a1 < d)
            d = a1;
        if (a2 < d)
            d = a2;
    }

    double sclip = -1,  eclip = -1;
    if (in) {
        if (start > d) {
            double tmp = start;
            start = d;
            d = tmp;
        }
        gr_dev->Arc(cx, yinv(cy), rad, rad, start, d);
        sclip = start;
        eclip = d;
    }
    if (d == end)
        return (flag ? sclip : eclip);
    if (a1 != a2)
        in = in ? false : true;

    // Now go from here to the next point.
    double l = d;
    d = M_PI * 3;
    if ((end < d) && (end > l))
        d = end;
    if ((a1 < d) && (a1 > l))
        d = a1;
    if ((a2 < d) && (a2 > l))
        d = a2;
    if (d == M_PI * 3) {
        d = end;
        if (a1 < d)
            d = a1;
        if (a2 < d)
            d = a2;
    }

    if (in) {
        gr_dev->Arc(cx, yinv(cy), rad, rad, l, d);
        sclip = l;
        eclip = d;
    }
    if (d == end)
        return (flag ? sclip : eclip);
    in = in ? false : true;
    
    // And from here to the end.
    if (in) {
        gr_dev->Arc(cx, yinv(cy), rad, rad, d, end);
        if (flag != 2) {
            sclip = d;
            eclip = end;
        }
    }
    return (flag%2 ? sclip : eclip);
}

