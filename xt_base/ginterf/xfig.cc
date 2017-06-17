
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2005 Whiteley Research Inc, all rights reserved.        *
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
 * This library is available for non-commercial use under the terms of    *
 * the GNU Library General Public License as published by the Free        *
 * Software Foundation; either version 2 of the License, or (at your      *
 * option) any later version.                                             *
 *                                                                        *
 * A commercial license is available to those wishing to use this         *
 * library or a derivative work for commercial purposes.  Contact         *
 * Whiteley Research Inc., 456 Flora Vista Ave., Sunnyvale, CA 94086.     *
 * This provision will be enforced to the fullest extent possible under   *
 * law.                                                                   *
 *                                                                        *
 * This library is distributed in the hope that it will be useful,        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 * Library General Public License for more details.                       *
 *                                                                        *
 * You should have received a copy of the GNU Library General Public      *
 * License along with this library; if not, write to the Free             *
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.     *
 *========================================================================*
 *                                                                        *
 * Ginterf Graphical Interface Library                                    *
 *                                                                        *
 *========================================================================*
 $Id: xfig.cc,v 2.36 2010/07/09 02:41:26 stevew Exp $
 *========================================================================*/

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include "xfig.h"
#include "texttf.h"


// Driver code for the xfig format
// This supplies version 3.2 format output

#ifndef M_PI
#define	M_PI  3.14159265358979323846  // pi
#endif

// point size of font;
#define FONTPTS 12

namespace ginterf
{
    HCdesc XFdesc(
        "XF",                       // drname
        "Xfig line draw, color",    // descr
        "xfig_line_draw_color",     // keyword
        "xfig",                     // alias
        "-f %s -r %d -t 1 -w %f -h %f -x %f -y %f", // format
        HClimits(
            0.0, 22.0,  // minxoff, maxxoff
            0.0, 22.0,  // minyoff, maxyoff
            1.0, 23.5,  // minwidth, maxwidth
            1.0, 23.5,  // minheight, maxheight
                        // flags
            HCdontCareXoff | HCdontCareYoff | HCnoCanRotate | HCfileOnly |
                HCfixedResol,
            0           // resols
        ),
        HCdefaults(
            0.0, 0.0,   // defxoff, defyoff
            4.0, 4.0,   // defwidth, defheight
            0,          // command string to print file
            0,          // defresol (index into resols)
            HClegOff,   // initially no legend
            HCbest      // initially set best orientation
        ),
        true);      // line draw
}


bool
XFdev::Init(int *ac, char **av)
{
    if (!ac || !av)
        return (true);
    HCdata *hd = new HCdata;
    hd->filename = 0;
    hd->width = 8.0;
    hd->height = 10.5;
    hd->resol = 1200;

    if (HCdevParse(hd, ac, av)) {
        delete hd;
        return (true);
    }

    // ignore any resolution change
    hd->resol = 1200;

    if (!hd->filename || !*hd->filename) {
        delete hd;
        return (true);
    }
    HCcheckEntries(hd, &XFdesc);

    delete data;
    data = hd;
    numlinestyles = 0;
    numcolors = 32;

    return (false);
}


GRdraw *
XFdev::NewDraw(int)
{
    FILE *plotfp;
    if (data->filename && *data->filename)
        plotfp = fopen(data->filename, "wb");
    else
        return (0);
    if (!plotfp) {
        GRpkgIf()->Perror(data->filename);
        return (0);
    }
    XFparams *xf = new XFparams;
    xf->fp = plotfp;
    xf->dev = this;

    width = (int)(data->resol*data->width);
    height = (int)(data->resol*data->height);
    xoff = (int)(data->xoff*data->resol);
    yoff = (int)(data->yoff*data->resol);

    return (xf);
}


void
XFparams::Halt()
{
    fclose(fp);
    delete this;
}


void
XFparams::ResetViewport(int wid, int hei)
{
    if (wid == dev->width && hei == dev->height)
        return;
    if (wid <= 0 || hei <= 0)
        return;
    dev->width = wid;
    dev->data->width = ((double)dev->width)/dev->data->resol;
    dev->height = hei;
    dev->data->height = ((double)dev->height)/dev->data->resol;
}


// Output the initializing lines of the file.  This has to be done after
// the viewport width/height are known, ahead of any geometry.
//
void
XFparams::DefineViewport()
{
    // init line
    fprintf(fp, "#FIG 3.2\n");
    fprintf(fp, "Portrait\n");
    fprintf(fp, "Flush Left\n");
    fprintf(fp, "Inches\n");
    fprintf(fp, "Letter\n");
    fprintf(fp, "100.0\n");    // ???
    fprintf(fp, "Single\n");
    fprintf(fp, "-2\n");
    fprintf(fp, "1200 2\n");

    // create color mapping
    pix_list *p0 = GRappIf()->ListPixels();
        // returns pixels used by application
    int colornum = 32;  // start of user defined colors
    pix_list *pn;
    for (pix_list *p = p0; p; p = pn) {
        pn = p->next;
        if (p->r == 255 && p->g == 255 && p->b == 255)
            p->r = p->g = p->b = 0;
        fprintf(fp, "0 %d #%02x%02x%02x\n", colornum, p->r, p->g, p->b);
        ctab.add(p->pixel, colornum++);
        delete p;
    }
    numcolors = colornum - 32;
}


void
XFparams::Pixel(int x, int y)
{
    Line(x, y, x, y);
}


void
XFparams::Pixels(GRmultiPt *data, int num)
{
    if (num < 1)
        return;
    data->data_ptr_init();
    while (num--) {
        int x = data->data_ptr_x();
        int y = data->data_ptr_y();
        Line(x, y, x, y);
        data->data_ptr_inc();
    }
}


void
XFparams::Line(int x1, int y1, int x2, int y2)
{
    // clip out unchanged points
    if (x1 == x2 && y1 == y2 && x1 == lastx && y1 == lasty)
        return;

    if (x2 == lastx && y2 == lastx) {
        int xt = x1;
        x1 = x2;
        x2 = xt;
        int yt = y1;
        y1 = y2;
        y2 = yt;
    }
    int thickness = 1;
    int pen_color = color;
    int fill_color = 7;
    int pen_style = -1;
    int area_fill = -1;
    double style_val = linestyle ? 4.0 : 0.0;
    int join_style = 2;  // round
    int cap_style = 1;  // round
    int radius = -1;
    int forward_arrow = 0;
    int backward_arrow = 0;

    fprintf(fp, "2 1 %d %d %d %d %d %d %d %g %d %d %d %d %d %d\n",
        linestyle,
        thickness,
        pen_color,
        fill_color,
        depth,
        pen_style,
        area_fill,
        style_val,
        join_style,
        cap_style,
        radius,
        forward_arrow,
        backward_arrow,
        2);
    fprintf(fp, "\t %d %d %d %d\n", x1, y1, x2, y2);

    // validate cache
    lastx = x2;
    lasty = y2;
}


void
XFparams::PolyLine(GRmultiPt *data, int num)
{
    if (num < 2)
        return;
    int thickness = 1;
    int pen_color = color;
    int fill_color = 7;
    int pen_style = -1;
    int area_fill = -1;
    double style_val = linestyle ? 4.0 : 0.0;
    int join_style = 0;
    int cap_style = 0;
    int radius = -1;
    int forward_arrow = 0;
    int backward_arrow = 0;
    fprintf(fp, "2 1 %d %d %d %d %d %d %d %g %d %d %d %d %d %d\n",
        linestyle,
        thickness,
        pen_color,
        fill_color,
        depth,
        pen_style,
        area_fill,
        style_val,
        join_style,
        cap_style,
        radius,
        forward_arrow,
        backward_arrow,
        num);
    fprintf(fp, "\t");
    data->data_ptr_init();
    while (num--) {
        fprintf(fp, " %d %d", data->data_ptr_x(), data->data_ptr_y());
        data->data_ptr_inc();
    }
    fprintf(fp, "\n");
}


void
XFparams::Lines(GRmultiPt *data, int num)
{
    if (num < 1)
        return;
    data->data_ptr_init();
    while (num--) {
        int x = data->data_ptr_x();
        int y = data->data_ptr_y();
        data->data_ptr_inc();
        Line(x, y, data->data_ptr_x(), data->data_ptr_y());
        data->data_ptr_inc();
    }
}


void
XFparams::Box(int x1, int y1, int x2, int y2)
{
    int thickness = 1;
    int pen_color = color;
    int fill_color = 7;
    int pen_style = -1;
    int area_fill = fillpattern;
    double style_val = 0.0;
    int join_style = 0;
    int cap_style = 0;
    int radius = -1;
    int forward_arrow = 0;
    int backward_arrow = 0;
    fprintf(fp, "2 2 %d %d %d %d %d %d %d %g %d %d %d %d %d %d\n",
        linestyle,
        thickness,
        pen_color,
        fill_color,
        depth,
        pen_style,
        area_fill,
        style_val,
        join_style,
        cap_style,
        radius,
        forward_arrow,
        backward_arrow,
        5);
    fprintf(fp, "\t %d %d %d %d %d %d %d %d %d %d\n",
        x1, y1, x1, y2, x2, y2, x2, y1, x1, y1);
}


void
XFparams::Boxes(GRmultiPt *data, int num)
{
    if (num < 1)
        return;
    data->data_ptr_init();
    while (num--) {
        int x = data->data_ptr_x();
        int y = data->data_ptr_y();
        data->data_ptr_inc();
        int w = data->data_ptr_x();
        int h = data->data_ptr_y();
        Box(x, y, x + w - 1, y + h - 1);
        data->data_ptr_inc();
    }
}


void
XFparams::Arc(int x0, int y0, int rx, int ry, double theta1, double theta2)
{
    if (rx <= 0 || ry <= 0)
        return;
    if (theta1 >= theta2)
        theta2 = 2 * M_PI + theta2;

    int thickness = 1;
    int pen_color = color;
    int fill_color = 7;
    int pen_style = -1;
    int area_fill = -1;
    double style_val = 0.0;
    int cap_style = 0;
    int direction = 1;
    int forward_arrow = 0;
    int backward_arrow = 0;

    if (fabs(theta1 + 2*M_PI - theta2) < 1e-6) {
        // closed figure
        int xs = x0 + (int)(rx*cos(theta1));
        int ys = y0 - (int)(ry*sin(theta1));
        int xe = x0 + (int)(rx*cos(theta2));
        int ye = y0 - (int)(ry*sin(theta2));

        fprintf(fp,
            "1 1 %d %d %d %d %d %d %d %g %d 0.0 %d %d %d %d %d %d %d %d\n",
            linestyle,
            thickness,
            pen_color,
            fill_color,
            depth,
            pen_style,
            area_fill,
            style_val,
            direction,
            x0,
            y0,
            rx,
            ry,
            xs,
            ys,
            xe,
            ye);
        return;
    }

    // Xfig doesn't handle elliptical arcs.  This will approximate with
    // a circular arc.

    int x1 = x0 + (int)(rx*cos(theta1));
    int y1 = y0 - (int)(ry*sin(theta1));
    int x2 = x0 + (int)(rx*cos(0.5*(theta1 + theta2)));
    int y2 = y0 - (int)(ry*sin(0.5*(theta1 + theta2)));
    int x3 = x0 + (int)(rx*cos(theta2));
    int y3 = y0 - (int)(ry*sin(theta2));

    fprintf(fp,
        "5 1 %d %d %d %d %d %d %d %g %d %d %d %d %d %d %d %d %d %d %d %d\n",
        linestyle,
        thickness,
        pen_color,
        fill_color,
        depth,
        pen_style,
        area_fill,
        style_val,
        cap_style,
        direction,
        forward_arrow,
        backward_arrow,
        x0, y0,
        x1, y1,
        x2, y2,
        x3, y3);
}


void
XFparams::Polygon(GRmultiPt *data, int n)
{
    if (n < 4)
        return;
    int thickness = 1;
    int pen_color = color;
    int fill_color = 7;
    int pen_style = -1;
    int area_fill = fillpattern;
    double style_val = 0.0;
    int join_style = 0;
    int cap_style = 0;
    int radius = -1;
    int forward_arrow = 0;
    int backward_arrow = 0;
    fprintf(fp, "2 3 %d %d %d %d %d %d %d %g %d %d %d %d %d %d\n",
        linestyle,
        thickness,
        pen_color,
        fill_color,
        depth,
        pen_style,
        area_fill,
        style_val,
        join_style,
        cap_style,
        radius,
        forward_arrow,
        backward_arrow,
        n);
    fprintf(fp, "\t");
    data->data_ptr_init();
    while (n--) {
        fprintf(fp, " %d %d", data->data_ptr_x(), data->data_ptr_y());
        data->data_ptr_inc();
    }
    fprintf(fp, "\n");
}


void
XFparams::Zoid(int yl, int yu, int xll, int xul, int xlr, int xur)
{
    if (yl >= yu)
        return;
    int points[10];
    int n = 0;
    points[n++] = xll;
    points[n++] = yl;
    points[n++] = xul;
    points[n++] = yu;
    if (xur > xul) {
        points[n++] = xur;
        points[n++] = yu;
    }
    points[n++] = xlr;
    points[n++] = yl;
    if (xll < xlr) {
        points[n++] = xll;
        points[n++] = yl;
    }
    if (n < 8)
        return;

    int thickness = 1;
    int pen_color = color;
    int fill_color = 7;
    int pen_style = -1;
    int area_fill = fillpattern;
    double style_val = 0.0;
    int join_style = 0;
    int cap_style = 0;
    int radius = -1;
    int forward_arrow = 0;
    int backward_arrow = 0;
    fprintf(fp, "2 3 %d %d %d %d %d %d %d %g %d %d %d %d %d %d\n",
        linestyle,
        thickness,
        pen_color,
        fill_color,
        depth,
        pen_style,
        area_fill,
        style_val,
        join_style,
        cap_style,
        radius,
        forward_arrow,
        backward_arrow,
        n/2);
    fprintf(fp, "\t");
    for (int i = 0; i < n; i += 2)
        fprintf(fp, " %d %d", points[i], points[i+1]);
    fprintf(fp, "\n");
}


void
XFparams::Text(const char *text, int x, int y, int xform, int width, int ht)
{
    if (!width || !ht)
        return;

    // xform bits:
    // 0-1, 0-no rotation, 1-90, 2-180, 3-270.
    // 2, mirror y after rotation
    // 3, mirror x after rotation and mirror y
    // ---- above are legacy ----
    // 4, shift rotation to 45, 135, 225, 315
    // 5-6 horiz justification 00,11 left, 01 center, 10 right
    // 7-8 vert justification 00,11 bottom, 01 center, 10 top
    // 9-10 font (gds)

// how to do mirror?
    if (text == 0)
        return;
    double angle = 0.0;
    switch (xform & TXTF_ROT) {
    case 0:
        break;
    case 1:
        angle = M_PI/2;
        break;
    case 2:
        angle = M_PI;
        break;
    case 3:
        angle = 3*M_PI/2;
        break;
    }
    int justification = 0;
    if (xform & TXTF_HJR)
        justification = 2;
    else if (xform & TXTF_HJC)
        justification = 1;
    if (xform & TXTF_VJT)
        y += (1200*FONTPTS)/72;
    else if (xform & TXTF_VJC)
        y += (1200*FONTPTS)/(2*72);
    int pen_style = -1;
    int font = 5;
    int font_size = FONTPTS;
    int font_flags = 0;
    int height = 0;
    int length = 0;
    fprintf(fp, "4 %d %d %d %d %d %d %g %d %d %d %d %d %s\\001\n",
        justification,
        color,
        depth,
        pen_style,
        font,
        font_size,
        angle,
        font_flags,
        height,
        length,
        x, y,
        text);
}


void
XFparams::TextExtent(const char *text, int *x, int *y)
{
    *y = (1200*FONTPTS)/72;
    *x = (6*(*y))/10;
    if (text)
        *x *= strlen(text);
}


void
XFparams::SetColor(int pixel)
{
    color = ctab.get(pixel);
    depth = numcolors - color + 31;
}


void
XFparams::SetLinestyle(const GRlineType *lineptr)
{
    if (!lineptr)
        linestyle = 0;
    else
        linestyle = 1;
}


void
XFparams::SetFillpattern(const GRfillType *fillp)
{
    if (!fillp) {
        fillpattern = -1;
        return;
    }
    int lindex = GRappIf()->FillTypeIndex(fillp);
    if (lindex <= 0) {
        fillpattern = -1;
        return;
    }
    int ft = GRappIf()->FillStyle(_devXF_, lindex);
    if (ft > 0)
        fillpattern = ft;
    else
        fillpattern = -1;
}


void
XFparams::DisplayImage(const GRimage*, int, int, int, int)
{
    GRpkgIf()->HCabort("HPGL driver doesn't suppport image mode.");
}


// Return the resolution, relative to the on-screen resolution, needed
// for size thresholding.
//
double
XFparams::Resolution()
{
    return (dev->data->resol/GR_SCREEN_RESOL);
}

