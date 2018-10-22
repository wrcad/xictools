
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
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1988 Jeffrey M. Hsu
         1992 Stephen R. Whiteley
****************************************************************************/

#include "frontend.h"
#include "outplot.h"
#include "outdata.h"
#include "cshell.h"
#include "kwords_fte.h"
#include "toolbar.h"
#include "spnumber/spnumber.h"


//
// Initialization functions for WRspice graphics.
//

// Default plotting colors, these are overridden by colorN X resources.
//
const char *DefColorNames[NUMPLOTCOLORS] =
{
    "white", 
    "black", 
    "red", 
    "lime green", 
    "blue", 
    "orange", 
    "magenta", 
    "turquoise", 
    "sienna", 
    "grey", 
    "hot pink", 
    "slate blue", 
    "spring green", 
    "cadet blue", 
    "pink", 
    "indian red", 
    "chartreuse", 
    "khaki", 
    "dark salmon", 
    "rosy brown"
};


// Default colors corresponding to the name array above, as pixel/rgb
// triples.
//
sColor DefColors[NUMPLOTCOLORS] =
{
    sColor( 1, 255, 255, 255 ),
    sColor( 0, 0, 0, 0 ),
    sColor( 2, 255, 0, 0 ),
    sColor( 3, 0, 255, 0 ),
    sColor( 4, 0, 0, 255 ),
    sColor( 5, 255, 0, 255 ),
    sColor( 6, 0, 255, 255 ),
    sColor( 7, 238, 130, 238 ),
    sColor( 8, 160, 82, 45 ),
    sColor( 9, 255, 165, 0 ),
    sColor( 10, 218, 112, 214 ),
    sColor( 11, 238, 130, 238 ),
    sColor( 12, 176, 48, 96 ),
    sColor( 13, 64, 224, 208 ),
    sColor( 14, 255, 192, 203 ),
    sColor( 15, 255, 127, 80 ),
    sColor( 16, 240, 230, 140 ),
    sColor( 17, 255, 255, 0 ),
    sColor( 18, 255, 215, 0 ),
    sColor( 19, 221, 160, 221 )
};


// Set the pixels in the DefColors array.
//
void
SetDefaultColors()
{
    ToolBar()->LoadResourceColors();  // toolkit-specific, loads DefColorNames
    if (GRpkgIf()->MainDev()->numcolors > NUMPLOTCOLORS)
        GRpkgIf()->MainDev()->numcolors = NUMPLOTCOLORS;
    const char *colorstring = DefColorNames[0];
    VTvalue vv;
    if (Sp.GetVar("color0", VTYP_STRING, &vv))
        colorstring = vv.get_string();

    unsigned b_pixel = GRpkgIf()->MainDev()->NameColor(colorstring);
    // note that the pixel and color values are 0 - 255
    int r, g, b;
    GRpkgIf()->GRpkg::RGBofPixel(b_pixel, &r, &g, &b);
    DefColors[0].pixel = b_pixel;
    DefColors[0].red = r;
    DefColors[0].green = g;
    DefColors[0].blue = b;

    unsigned f_pixel;
    if (b_pixel == (unsigned)GRpkgIf()->MainDev()->NameColor("white"))
        f_pixel = GRpkgIf()->MainDev()->NameColor("black");
    else
        f_pixel = GRpkgIf()->MainDev()->NameColor("white");
    if (GRpkgIf()->MainDev()->numcolors <= 2) {
        GRpkgIf()->GRpkg::RGBofPixel(f_pixel, &r, &g, &b);
        DefColors[1].pixel = b_pixel;
        DefColors[1].red = r;
        DefColors[1].green = g;
        DefColors[1].blue = b;
        return; 
    } 
    for (int i = 1; i < GRpkgIf()->MainDev()->numcolors; i++) { 
        char buf[16]; 
        sprintf(buf, "color%d", i); 
        colorstring = DefColorNames[i]; 
        if (Sp.GetVar(buf, VTYP_STRING, &vv)) 
            colorstring = vv.get_string();
        DefColors[i].pixel = GRpkgIf()->MainDev()->NameColor(colorstring);
        if (DefColors[i].pixel == b_pixel)
            DefColors[i].pixel = f_pixel;
        GRpkgIf()->GRpkg::RGBofPixel(DefColors[i].pixel, &r, &g, &b);
        DefColors[i].red = r;
        DefColors[i].green = g;
        DefColors[i].blue = b;
    }
}


struct grAttributes
{
    grAttributes();
    ~grAttributes();

    void setup(const char*);
    bool fixlimits(sDvList*, double*, double*, bool);
    void compress(sDataVec*);

    static double *getnum(const char*, int);
    static char *getword(const char*);
    static bool getflag(const char*);

    double xlim[2];
    double ylim[2];
    double xcompress;
    double xindices[2];
    double xdelta;
    double ydelta;
    GridType gtype;
    PlotType ptype;
    ScaleType format;
    bool nointerp;
    bool ysep;
    bool nogrid;
    bool noplotlogo;
    const char *xlabel;
    const char *ylabel;
    const char *title;
    bool xlimset;
    bool ylimset;
    bool xcompset;
    bool xindset;
    bool xdeltaset;
    bool ydeltaset;
    bool formatset;
    bool gtypeset;
    bool ptypeset;
};

struct sPlotFlags
{
    sPlotFlags(const char **n, int t)
        {
            name = n;
            type = t;
        }

    const char **name;
    int type;
};

namespace {
    sPlotFlags gtypes[] = {
        sPlotFlags(&kw_lingrid, GRID_LIN), 
        sPlotFlags(&kw_loglog, GRID_LOGLOG), 
        sPlotFlags(&kw_xlog, GRID_XLOG), 
        sPlotFlags(&kw_ylog, GRID_YLOG), 
        sPlotFlags(&kw_polar, GRID_POLAR), 
        sPlotFlags(&kw_smith, GRID_SMITH),
        sPlotFlags(&kw_smithgrid, GRID_SMITHGRID)
    };

    sPlotFlags ptypes[] = {
        sPlotFlags(&kw_linplot, PLOT_LIN), 
        sPlotFlags(&kw_combplot, PLOT_COMB), 
        sPlotFlags(&kw_pointplot, PLOT_POINT)
    };

    sPlotFlags stypes[] = {
        sPlotFlags(&kw_multi, FT_MULTI), 
        sPlotFlags(&kw_single, FT_SINGLE), 
        sPlotFlags(&kw_group, FT_GROUP)
    };
}

#define NumGrTypes (int)(sizeof(gtypes)/sizeof(sPlotFlags))
#define NumPlTypes (int)(sizeof(ptypes)/sizeof(sPlotFlags))
#define NumScTypes (int)(sizeof(stypes)/sizeof(sPlotFlags))


// In order to support hardcopy colors with or without X, we maintain
// a pixel/rgb mapping in DevColors[] with defaults.  This mapping
// supplies rgb values to the hardcopy drivers through the function
// below.  The DefColors mapping is updated if X is running when
// InitColormap is called at startup, and before every plot.
//
void
SpGrPkg::RGBofPixel(int p, int *r, int *g, int *b)
{
    // first see if the pixel is in the DefColors.  It should always
    // be there
    for (int i = 0; i < NUMPLOTCOLORS; i++) {
        if (p == (int)DefColors[i].pixel) {
            *r = DefColors[i].red;
            *g = DefColors[i].green;
            *b = DefColors[i].blue;
            return;
        }
    }
    // in case the pixel is not found, try to get it from X
    GRpkg::RGBofPixel(p, r, g, b);
}


// If X is running, call the application's color table initializer
// after initialing the graphics package color allocation.
//
bool
SpGrPkg::InitColormap(int mn, int mx, bool dp)
{
    bool ret = GRpkg::InitColormap(mn, mx, dp);
    if (!ret && MainDev() && MainDev()->name)
        SetDefaultColors();
    return (ret);
}
// End of SpGrPkg functions.


// Set all the plot parameters in gr and fix up the dvecs.
// false is returned on error.  We assume all dvecs are copies so they
// can be modified arbitrarily.
//
bool
SPgraphics::Setup(sGrInit *gr, sDvList **dlptr, const char *attrs,
    sDataVec *scale, const char *hcopy)
{
    grAttributes grs;
    grs.setup(attrs);
    sDvList *dl0 = *dlptr;
    sDvList *dl;
    // If no scale is given, it is assumed that the dvecs already have
    // a scale assigned as below
    if (scale)
        for (dl = dl0; dl; dl = dl->dl_next)
            dl->dl_dvec->set_scale(scale);

    // See if the log flag is set anywhere...
    if (!grs.gtypeset) {
        grs.gtype = GRID_LIN;
        for (dl = dl0; dl; dl = dl->dl_next) {
            sDataVec *d = dl->dl_dvec;
            if (d->scale() && (d->scale()->gridtype() == GRID_XLOG))
                grs.gtype = GRID_XLOG;
        }
        for (dl = dl0; dl; dl = dl->dl_next) {
            sDataVec *d = dl->dl_dvec;
            if (d->gridtype() == GRID_YLOG) {
                if ((grs.gtype == GRID_XLOG) ||
                        (grs.gtype == GRID_LOGLOG))
                    grs.gtype = GRID_LOGLOG;
                else
                    grs.gtype = GRID_YLOG;
            }
        }
        for (dl = dl0; dl; dl = dl->dl_next) {
            sDataVec *d = dl->dl_dvec;
            if (d->gridtype() == GRID_POLAR || d->gridtype() == GRID_SMITH ||
                    d->gridtype() == GRID_SMITHGRID) {
                grs.gtype = d->gridtype();
                break;
            }
        }
    }

    // See if there are any default plot types...  Here, like above, we
    // don't do entirely the best thing when there is a mixed set of
    // default plot types...
    //
    if (!grs.ptypeset) {
        grs.ptype = PLOT_LIN;
        for (dl = dl0; dl; dl = dl->dl_next) {
            sDataVec *d = dl->dl_dvec;
            if (d->plottype() != PLOT_LIN) {
                grs.ptype = d->plottype();
                break;
            }
        }
    }

    // Check and see if this is pole zero stuff
    bool oneval = false;
    if ((dl0->dl_dvec->flags() & VF_POLE) || (dl0->dl_dvec->flags() & VF_ZERO))
        oneval = true;
    
    for (dl = dl0; dl; dl = dl->dl_next) {
        sDataVec *d = dl->dl_dvec;
        if (((d->flags() & VF_POLE) || (d->flags() & VF_ZERO)) !=
                oneval ? 1 : 0) {
            GRpkgIf()->ErrPrintf(ET_ERROR,
        "plot must be either all pole-zero or contain no poles or zeros.\n");
            return (false);
        }
    }
    if (grs.gtype == GRID_POLAR || grs.gtype == GRID_SMITH ||
            grs.gtype == GRID_SMITHGRID)
        oneval = true;

    if (!oneval && scale) {
        // make all vectors the same length
        // if scale not given, it is assumed that this is already done
        int lm = 0;
        for (dl = dl0; dl; dl = dl->dl_next) {
            sDataVec *d = dl->dl_dvec;
            if (d->length() > lm)
                lm = d->length();
        }
        if (grs.ptype == PLOT_LIN) {
            if (scale->length() == 1 || (scale->numdims() > 1 &&
                    scale->dims(scale->numdims() - 1) == 1))
                grs.ptype = PLOT_POINT;
        }
        if (scale->length() < lm)
            scale->extend(lm);
        for (dl = dl0; dl; dl = dl->dl_next) {
            sDataVec *d = dl->dl_dvec;
            if (d->length() != lm)
                d->extend(lm);
        }
    }

    // Now patch up each vector with the compression and the index
    // selection.  If scale not given, this has already been done
    //
    if ((grs.xcompset || grs.xindset) && scale) {
        grs.compress(scale);
        for (dl = dl0; dl; dl = dl->dl_next) {
            sDataVec *d = dl->dl_dvec;
            grs.compress(d);
        }
    }

    // if unit length, do a point plot
    if (dl0->dl_dvec->length() == 1 && grs.ptype == PLOT_LIN)
        grs.ptype = PLOT_POINT;

    // Transform for smith plots.  Do this only for type GRID_SMITH, not
    // for GRID_SMITHGRID.  Skip if vectors have the Smith flag set.
    //
    if (grs.gtype == GRID_SMITH) {
        for (dl = dl0; dl; dl = dl->dl_next)
            if (!(dl->dl_dvec->flags() & VF_SMITH))
                break;
        if (dl) {
            // found one not transformed, copy list
            sDvList *dx0 = 0, *dx = 0;
            for (dl = dl0; dl; dl = dl->dl_next) {
                if (!dx0)
                    dx = dx0 = new sDvList;
                else {
                    dx->dl_next = new sDvList;
                    dx = dx->dl_next;
                }
                dx->dl_dvec = dl->dl_dvec->SmithCopy();
            }
            *dlptr = dx0;
            dl0 = dx0;
        }
    }

    if (!oneval && (grs.gtype == GRID_LIN || grs.gtype == GRID_XLOG ||
            grs.gtype == GRID_YLOG || grs.gtype == GRID_LOGLOG)) {

        // separate complex vectors into real/imag pairs
        for (dl = dl0; dl; dl = dl->dl_next) {
            sDataVec *vr = dl->dl_dvec;
            if (vr->iscomplex()) {
                complex *data = vr->compvec();
                int length = vr->length();
                vr->set_compvec(0);
                vr->set_flags(vr->flags() & ~VF_COMPLEX);
                sDataVec *vi = vr->copy();
                vr->set_realvec(new double[length]);
                vr->set_length(length);
                vr->set_allocated(length);
                vi->set_realvec(new double[length]);
                vi->set_length(length);
                vr->set_allocated(length);
                for (int i = 0; i < length; i++) {
                    vr->set_realval(i, data[i].real);
                    vi->set_realval(i, data[i].imag);
                }
                delete [] data;
                char buf[256];
                sprintf(buf, "re:%s", vr->name());
                vr->set_name(buf);
                sprintf(buf, "im:%s", vi->name());
                vi->set_name(buf);

                sDvList *dx = new sDvList;
                dx->dl_next = dl->dl_next;
                dx->dl_dvec = vi;
                dl->dl_next = dx;
            }
        }
    }

    if (!grs.fixlimits(dl0, gr->xlims, gr->ylims, oneval))
        return (false);
    gr->xlimfixed = grs.xlimset;
    gr->ylimfixed = grs.ylimset;

    gr->xdelta = grs.xdeltaset ? grs.xdelta : 0.0;
    gr->ydelta = grs.ydeltaset ? grs.ydelta : 0.0;
    gr->hcopy = hcopy;
    gr->gridtype = grs.gtype;
    gr->plottype = grs.ptype;

    gr->xlabel = grs.xlabel;
    grs.xlabel = 0;
    if (gr->xlabel)
        gr->free_xlabel = true;
    gr->ylabel = grs.ylabel;
    grs.ylabel = 0;
    if (gr->ylabel)
        gr->free_ylabel = true;
    if (grs.title) {
        gr->title = grs.title;
        grs.title = 0;
        gr->free_title = true;
    }
    else
        gr->title = OP.curPlot()->name();

    char *tpn = new char[strlen(OP.curPlot()->type_name()) +
        strlen(OP.curPlot()->title()) + 3];
    sprintf(tpn, "%s: %s", OP.curPlot()->type_name(),
        OP.curPlot()->title());
    gr->plotname = tpn;

    gr->nointerp = grs.nointerp;
    gr->ysep = grs.ysep;
    gr->noplotlogo = grs.noplotlogo;
    gr->nogrid = grs.nogrid;

    // See if there is one type we can give for the y scale...
    gr->ytype = *dl0->dl_dvec->units();
    for (dl = dl0->dl_next; dl; dl = dl->dl_next) {
        if (!(*dl->dl_dvec->units() == gr->ytype)) {
            gr->ytype.set(UU_NOTYPE);
            break;
        }
    }
    int i;
    for (dl = dl0, i = 0; dl; dl = dl->dl_next, i++) ;
    gr->nplots = i;
    if (!grs.formatset) {
        if (gr->nplots > 1)
            grs.format = FT_MULTI;
        else
            grs.format = FT_SINGLE;
    }
    gr->format = grs.format;

    // Figure out the X name and the X type.  This is sort of bad...
    gr->xname = oneval ? 0 : dl0->dl_dvec->scale()->name();
    if (oneval)
        gr->xtype.set(UU_NOTYPE);
    else
        gr->xtype = *dl0->dl_dvec->scale()->units();

    return (true);
}


//  Start of a new graph.
//  Fill in the data that gets displayed.
//
sGraph *
SPgraphics::Init(sDvList *dl0, sGrInit *gr, sGraph *graph)
{
    bool reuse = false;
    if (!graph)
        graph = NewGraph(GR_PLOT, GR_PLOTstr);
    else
        reuse = true;
    if (!graph)
        return (0);

    graph->setup(dl0, gr);

    if (!reuse && graph->gr_dev_init()) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "can't init viewport for graphics.\n");
        DestroyGraph(graph->id());
        return (0);
    }
    if (GRpkgIf()->CurDev()->devtype != GRmultiWindow) {    
        graph->dev()->Clear();
       if (GRpkgIf()->CurDev()->devtype == GRhardcopy)
            graph->dev()->DefineViewport();
        graph->gr_redraw();
    }
    return (graph);
}
// End of SPgraphics functions.


void
sGraph::setup(sDvList *dl0, sGrInit *gr)
{
    gr_command = gr->command;
    gr_oneval = (gr->xname ? false : true);
    gr_xlimfixed = gr->xlimfixed;
    gr_ylimfixed = gr->ylimfixed;

    // Communicate filename to hardcopy driver.
    if (gr->hcopy)
        gr_filename = gr->hcopy;

    VTvalue vv;
    gr_degree = DEF_polydegree;
    if (Sp.GetVar(kw_polydegree, VTYP_NUM, &vv))
        gr_degree = vv.get_int();

    if (Sp.GetVar(kw_ticmarks, VTYP_NUM, &vv))
        gr_ticmarks = vv.get_int();
    else {
        if (Sp.GetVar(kw_ticmarks, VTYP_BOOL, 0))
            gr_ticmarks = 10;
        else
            gr_ticmarks = 0;
    }

    // set upper and lower limits
    gr_rawdata.xmin = gr->xlims[0];
    gr_rawdata.xmax = gr->xlims[1];
    gr_rawdata.ymin = gr->ylims[0];
    gr_rawdata.ymax = gr->ylims[1];

    gr_pltype = gr->plottype;
    gr_grtype = gr->gridtype;
    gr_xunits = gr->xtype;
    gr_yunits = gr->ytype;
    gr_xdelta = gr->xdelta;
    gr_ydelta = gr->ydelta;
    gr_numtraces = gr->nplots;
    gr_ysep = gr->ysep;
    gr_noplotlogo = gr->noplotlogo;
    gr_nogrid = gr->nogrid;

    if (!gr_oneval) {
        if (gr->xlabel)
            gr_xlabel = lstring::copy(gr->xlabel);
        else
            gr_xlabel = lstring::copy(gr->xname);
        if (gr->ylabel)
            gr_ylabel = lstring::copy(gr->ylabel);
    }
    else {
        bool rev = (dl0->dl_dvec->flags() & (VF_POLE | VF_ZERO)) &&
            (gr->gridtype ==  GRID_LIN || gr->gridtype == GRID_LOGLOG ||
            gr->gridtype == GRID_XLOG || gr->gridtype == GRID_YLOG);

        if (gr->xlabel)
            gr_xlabel = lstring::copy(gr->xlabel);
        else
            gr_xlabel = lstring::copy(rev ? "imag" : "real");
        if (gr->ylabel)
            gr_ylabel = lstring::copy(gr->ylabel);
        else
            gr_ylabel = lstring::copy(rev ? "real" : "imag");
    }

    gr_title = lstring::copy(gr->title);
    gr_plotname = gr->plotname;  // already malloc'd
    if (dl0->dl_dvec->plot())
        gr_date = lstring::copy(dl0->dl_dvec->plot()->date());
    else
        gr_date = lstring::copy(datestring());

    gr_plotdata = (void*)dl0;
    gr_plotdata = gr_copy_data();

    if (GP.TmpGraph()) {
        // this is set for a zoomin
        if (GP.TmpGraph()->gr_selections) {
            gr_selections = new char[GP.TmpGraph()->gr_selsize];
            gr_selsize = GP.TmpGraph()->gr_selsize;
            memcpy(gr_selections, GP.TmpGraph()->gr_selections, gr_selsize);
        }
    }

    if (gr_grtype == GRID_POLAR || gr_grtype == GRID_SMITH ||
            gr_grtype == GRID_SMITHGRID || gr_oneval) {
        gr_format = FT_SINGLE;
        gr_ysep = false;
    }
    else
        gr_format = gr->format;

    if (gr_numtraces == 1 || gr_numtraces > MAXNUMTR) {
        gr_format = FT_SINGLE;
        gr_ysep = false;
    }
}
// End of sGraph functions.


// Constructor, initialize state according to variables.
//
grAttributes::grAttributes()
{
    double *dd;
    if ((dd = getnum(kw_xlimit, 2)) != 0) {
        xlim[0] = dd[0];
        xlim[1] = dd[1];
        xlimset = true;
    }
    else {
        xlim[0] = 0.0;
        xlim[1] = 0.0;
        xlimset = false;
    }

    if ((dd = getnum(kw_ylimit, 2)) != 0) {
        ylim[0] = dd[0];
        ylim[1] = dd[1];
        ylimset = true;
    }
    else {
        ylim[0] = 0.0;
        ylim[1] = 0.0;
        ylimset = false;
    }

    if ((dd = getnum(kw_xcompress, 1)) != 0) {
        xcompress = dd[0];
        xcompset = true;
    }
    else {
        xcompress = 0.0;
        xcompset = false;
    }

    if ((dd = getnum(kw_xindices, 2)) != 0) {
        xindices[0] = dd[0];
        xindices[1] = dd[1];
        xindset = true;
    }
    else {
        xindices[0] = 0.0;
        xindices[1] = 0.0;
        xindset = false;
    }

    if ((dd = getnum(kw_xdelta, 1)) != 0) {
        xdelta = dd[0];
        xdeltaset = true;
    }
    else {
        xdelta = 0.0;
        xdeltaset = false;
    }

    if ((dd = getnum(kw_ydelta, 1)) != 0) {
        ydelta = dd[0];
        ydeltaset = true;
    }
    else {
        ydelta = 0.0;
        ydeltaset = false;
    }

    // Get the grid type and the point type.  Note we can't do if-else
    // here because we want to catch all the grid types.
    //
    gtypeset = false;
    int i;
    for (i = 0; i < NumGrTypes; i++) {
        if (getflag(*gtypes[i].name)) {
            if (gtypeset)
                GRpkgIf()->ErrPrintf(ET_WARN, "too many grid types given.\n");
            else {
                gtype = (GridType)gtypes[i].type;
                gtypeset = true;
            }
        }
    }
    if (!gtypeset) {
        VTvalue vv;
        if (Sp.GetVar(kw_gridstyle, VTYP_STRING, &vv)) {
            for (i = 0; i < NumGrTypes; i++) {
                if (lstring::eq(vv.get_string(), *gtypes[i].name)) {
                    gtype = (GridType)gtypes[i].type;
                    break;
                }
            }
            if (i == NumGrTypes) {
                GRpkgIf()->ErrPrintf(ET_WARN, "strange grid type %s.\n",
                    vv.get_string());
                gtype = GRID_LIN;
            }
            gtypeset = true;
        }
        else
            gtype = GRID_LIN;
    }

    // Now get the point type
    ptypeset = false;
    for (i = 0; i < NumPlTypes; i++) {
        if (getflag(*ptypes[i].name)) {
            if (ptypeset)
                GRpkgIf()->ErrPrintf(ET_WARN, "too many plot types given.\n");
            else {
                ptype = (PlotType)ptypes[i].type;
                ptypeset = true;
            }
        }
    }
    if (!ptypeset) {
        VTvalue vv;
        if (Sp.GetVar(kw_plotstyle, VTYP_STRING, &vv)) {
            for (i = 0; i < NumPlTypes; i++) {
                if (lstring::eq(vv.get_string(), *ptypes[i].name)) {
                    ptype = (PlotType)ptypes[i].type;
                    break;
                }
            }
            if (i == NumPlTypes) {
                GRpkgIf()->ErrPrintf(ET_WARN, "strange plot type %s.\n",
                    vv.get_string());
                ptype = PLOT_LIN;
            }
            ptypeset = true;
        }
        else
            ptype = PLOT_LIN;
    }

    // The scale type
    formatset = false;
    for (i = 0; i < NumScTypes; i++) {
        if (getflag(*stypes[i].name)) {
            if (formatset)
                GRpkgIf()->ErrPrintf(ET_WARN, "too many scale types given.\n");
            else {
                format = (ScaleType)stypes[i].type;
                formatset = true;
            }
        }
    }
    if (!formatset) {
        VTvalue vv;
        if (Sp.GetVar(kw_scaletype, VTYP_STRING, &vv)) {
            for (i = 0; i < NumScTypes; i++) {
                if (lstring::eq(vv.get_string(), *stypes[i].name)) {
                    format = (ScaleType)stypes[i].type;
                    break;
                }
            }
            if (i == NumScTypes) {
                GRpkgIf()->ErrPrintf(ET_WARN, "strange scale type %s.\n",
                    vv.get_string());
                format = FT_MULTI;
            }
            formatset = true;
        }
        else
            format = FT_MULTI;
    }

    nointerp = getflag(kw_nointerp);
    ysep = getflag(kw_ysep);
    noplotlogo = getflag(kw_noplotlogo);
    nogrid = getflag(kw_nogrid);

    xlabel = getword(kw_xlabel);
    ylabel = getword(kw_ylabel);
    title = getword(kw_title);
}


grAttributes::~grAttributes()
{
    delete [] xlabel;
    delete [] ylabel;
    delete [] title;
}


// Reset state according to tokens in string.
//
void
grAttributes::setup(const char *attr)
{
    if (!attr)
        return;
    const char *s = attr;
    char *tok;
    while ((tok = lstring::gettok(&s, "=")) != 0) {
        if (lstring::cieq(tok, kw_lingrid)) {
            gtype = GRID_LIN;
            gtypeset = true;
        }
        else if (lstring::cieq(tok, kw_xlog)) {
            gtype = GRID_XLOG;
            gtypeset = true;
        }
        else if (lstring::cieq(tok, kw_ylog)) {
            gtype = GRID_YLOG;
            gtypeset = true;
        }
        else if (lstring::cieq(tok, kw_loglog)) {
            gtype = GRID_LOGLOG;
            gtypeset = true;
        }
        else if (lstring::cieq(tok, kw_polar)) {
            gtype = GRID_POLAR;
            gtypeset = true;
        }
        else if (lstring::cieq(tok, kw_smith)) {
            gtype = GRID_SMITH;
            gtypeset = true;
        }
        else if (lstring::cieq(tok, kw_smithgrid)) {
            gtype = GRID_SMITHGRID;
            gtypeset = true;
        }

        else if (lstring::cieq(tok, kw_linplot)) {
            ptype = PLOT_LIN;
            ptypeset = true;
        }
        else if (lstring::cieq(tok, kw_pointplot)) {
            ptype = PLOT_POINT;
            ptypeset = true;
        }
        else if (lstring::cieq(tok, kw_combplot)) {
            ptype = PLOT_COMB;
            ptypeset = true;
        }

        else if (lstring::cieq(tok, kw_multi)) {
            format = FT_MULTI;
            formatset = true;
        }
        else if (lstring::cieq(tok, kw_single)) {
            format = FT_SINGLE;
            formatset = true;
        }
        else if (lstring::cieq(tok, kw_group)) {
            format = FT_GROUP;
            formatset = true;
        }

        else if (lstring::cieq(tok, kw_xlimit)) {
            delete [] tok;
            const char *s1, *s2;
            if (*s == '"') {
                s1 = ",\"";
                s2 = "\"";
            }
            else {
                s1 = ",";
                s2 = 0;
            }
            tok = lstring::gettok(&s, s1);
            bool ok = false;
            if (tok) {
                const char *t = tok;
                double *v = SPnum.parse(&t, false);
                if (v) {
                    double d = *v;
                    delete [] tok;
                    tok = lstring::gettok(&s, s2);
                    if (tok) {
                        t = tok;
                        v = SPnum.parse(&t, false);
                        if (v) {
                            xlim[0] = d;
                            xlim[1] = *v;
                            xlimset = true;
                            ok = true;
                        }
                    }
                }
            }
            if (!ok)
                GRpkgIf()->ErrPrintf(ET_WARN, "parse error for %s.\n",
                    kw_xlimit);
        }
        else if (lstring::cieq(tok, kw_ylimit)) {
            delete [] tok;
            const char *s1, *s2;
            if (*s == '"') {
                s1 = ",\"";
                s2 = "\"";
            }
            else {
                s1 = ",";
                s2 = 0;
            }
            tok = lstring::gettok(&s, s1);
            bool ok = false;
            if (tok) {
                const char *t = tok;
                double *v = SPnum.parse(&t, false);
                if (v) {
                    double d = *v;
                    delete [] tok;
                    tok = lstring::gettok(&s, s2);
                    if (tok) {
                        t = tok;
                        v = SPnum.parse(&t, false);
                        if (v) {
                            ylim[0] = d;
                            ylim[1] = *v;
                            ylimset = true;
                            ok = true;
                        }
                    }
                }
            }
            if (!ok)
                GRpkgIf()->ErrPrintf(ET_WARN, "parse error for %s.\n",
                    kw_ylimit);
        }
        else if (lstring::cieq(tok, kw_xcompress)) {
            delete [] tok;
            tok = lstring::gettok(&s);
            bool ok = false;
            if (tok) {
                const char *t = tok;
                double *v = SPnum.parse(&t, false);
                if (v) {
                    xcompress = *v;
                    xcompset = true;
                    ok = true;
                }
            }
            if (!ok)
                GRpkgIf()->ErrPrintf(ET_WARN, "parse error for %s.\n",
                    kw_xcompress);
        }
        else if (lstring::cieq(tok, kw_xindices)) {
            delete [] tok;
            const char *s1, *s2;
            if (*s == '"') {
                s1 = ",\"";
                s2 = "\"";
            }
            else {
                s1 = ",";
                s2 = 0;
            }
            tok = lstring::gettok(&s, s1);
            bool ok = false;
            if (tok) {
                const char *t = tok;
                double *v = SPnum.parse(&t, false);
                if (v) {
                    double d = *v;
                    delete [] tok;
                    tok = lstring::gettok(&s, s2);
                    if (tok) {
                        t = tok;
                        v = SPnum.parse(&t, false);
                        if (v) {
                            xindices[0] = d;
                            xindices[1] = *v;
                            xindset = true;
                            ok = true;
                        }
                    }
                }
            }
            if (!ok)
                GRpkgIf()->ErrPrintf(ET_WARN, "parse error for %s.\n",
                    kw_xindices);
        }
        else if (lstring::cieq(tok, kw_xdelta)) {
            delete [] tok;
            tok = lstring::gettok(&s);
            bool ok = false;
            if (tok) {
                const char *t = tok;
                double *v = SPnum.parse(&t, false);
                if (v) {
                    xdelta = *v;
                    xdeltaset = true;
                    ok = true;
                }
            }
            if (!ok)
                GRpkgIf()->ErrPrintf(ET_WARN, "parse error for %s.\n",
                    kw_xdelta);
        }
        else if (lstring::cieq(tok, kw_ydelta)) {
            delete [] tok;
            tok = lstring::gettok(&s);
            bool ok = false;
            if (tok) {
                const char *t = tok;
                double *v = SPnum.parse(&t, false);
                if (v) {
                    ydelta = *v;
                    ydeltaset = true;
                    ok = true;
                }
            }
            if (!ok)
                GRpkgIf()->ErrPrintf(ET_WARN, "parse error for %s.\n",
                    kw_ydelta);
        }
        else if (lstring::cieq(tok, kw_xlabel)) {
            delete [] tok;
            tok = lstring::getqtok(&s);
            if (tok) {
                delete [] ylabel;
                ylabel = tok;
                tok = 0;
            }
            else
                GRpkgIf()->ErrPrintf(ET_WARN, "parse error for %s.\n",
                    kw_xlabel);
        }
        else if (lstring::cieq(tok, kw_ylabel)) {
            delete [] tok;
            tok = lstring::getqtok(&s);
            if (tok) {
                delete [] xlabel;
                xlabel = tok;
                tok = 0;
            }
            else
                GRpkgIf()->ErrPrintf(ET_WARN, "parse error for %s.\n",
                    kw_ylabel);
        }
        else if (lstring::cieq(tok, kw_title)) {
            delete [] tok;
             tok = lstring::getqtok(&s);
            if (tok) {
                delete [] title;
                title = tok;
                tok = 0;
            }
            else
                GRpkgIf()->ErrPrintf(ET_WARN, "parse error for %s.\n",
                    kw_title);
        }
        else if (lstring::cieq(tok, kw_nointerp))
            nointerp = true;
        else if (lstring::cieq(tok, kw_ysep))
            ysep = true;
        else if (lstring::cieq(tok, kw_noplotlogo))
            noplotlogo = true;
        else if (lstring::cieq(tok, kw_nogrid))
            nogrid = true;
        delete [] tok;
    }
}


bool
grAttributes::fixlimits(sDvList *dl0, double *xlims, double *ylims,
    bool oneval)
{
    // Figure out the proper x- and y-axis limits
    bool ynotset = false;
    if (ylimset) {
        ylims[0] = ylim[0];
        ylims[1] = ylim[1];
    }
    else if (oneval && !xlimset && (gtype == GRID_POLAR ||
            gtype == GRID_SMITH || gtype == GRID_SMITHGRID))
        ynotset = true;
    else {
        ylims[0] = 0;
        ylims[1] = 0;
        sDvList *dl;
        for (dl = dl0; dl; dl = dl->dl_next) {
            sDataVec *d = dl->dl_dvec;
            double dd[2];
            d->minmax(dd, true);

            if ((d->flags() & VF_MINGIVEN) && dd[0] < d->minsignal())
                dd[0] = d->minsignal();
            if ((d->flags() & VF_MAXGIVEN) && dd[1] > d->maxsignal())
                dd[1] = d->maxsignal();

            if (dl == dl0) {
                ylims[0] = dd[0];
                ylims[1] = dd[1];
            }
            else {
                if (dd[0] < ylims[0]) ylims[0] = dd[0];
                if (dd[1] > ylims[1]) ylims[1] = dd[1];
            }
        }
    }

    bool xnotset = false;
    if (xlimset) {
        xlims[0] = xlim[0];
        xlims[1] = xlim[1];
    }
    else if (oneval && !ylimset && (gtype == GRID_POLAR ||
            gtype == GRID_SMITH || gtype == GRID_SMITHGRID))
        xnotset = true;
    else {
        xlims[0] = 0;
        xlims[1] = 0;
        sDvList *dl;
        for (dl = dl0; dl; dl = dl->dl_next) {
            sDataVec *d = dl->dl_dvec->scale();
            double dd[2];
            d->minmax(dd, true);

            if ((d->flags() & VF_MINGIVEN) && dd[0] < d->minsignal())
                dd[0] = d->minsignal();
            if ((d->flags() & VF_MAXGIVEN) && dd[1] > d->maxsignal())
                dd[1] = d->maxsignal();

            if (dl == dl0) {
                xlims[0] = dd[0];
                xlims[1] = dd[1];
            }
            else {
                if (dd[0] < xlims[0]) xlims[0] = dd[0];
                if (dd[1] > xlims[1]) xlims[1] = dd[1];
            }
        }
    }

    // Do some coercion of the limits to make them reasonable
    if (!xnotset) {
        if ((xlims[0] == 0) && (xlims[1] == 0)) {
            xlims[0] = -1e-12;
            xlims[1] = 1e-12;
        }
        if (xlims[0] > xlims[1]) {
            double tt = xlims[1];
            xlims[1] = xlims[0];
            xlims[0] = tt;
        }
        if (xlims[0] == xlims[1]) {
            xlims[0] *= (xlims[0] > 0) ? 1.0 - 1e-4 : 1.0 + 1e-4;
            xlims[1] *= (xlims[1] > 0) ? 1.0 + 1e-4 : 1.0 - 1e-4;
        }
    }
    if (!ynotset) {
        if ((ylims[0] == 0) && (ylims[1] == 0)) {
            ylims[0] = -1e-12;
            ylims[1] = 1e-12;
        }
        if (ylims[0] > ylims[1]) {
            double tt = ylims[1];
            ylims[1] = ylims[0];
            ylims[0] = tt;
        }
        // ensure five sig figs for Y data
        if (ylims[0] == ylims[1]
            || fabs(ylims[0])/(ylims[1]-ylims[0]) > 1.0e4
            || fabs(ylims[1])/(ylims[1]-ylims[0]) > 1.0e4) {
            ylims[0] *= (ylims[0] > 0) ? 1.0 - 1e-4 : 1.0 + 1e-4;
            ylims[1] *= (ylims[1] > 0) ? 1.0 + 1e-4 : 1.0 - 1e-4;
        }
    }

    if ((gtype == GRID_XLOG || gtype == GRID_LOGLOG) && xlims[0] < 0.0) {
        GRpkgIf()->ErrPrintf(ET_ERROR,
            "X values must be >= 0 for log scale.\n");
        return (false);
    }
    if ((gtype == GRID_YLOG || gtype == GRID_LOGLOG) && ylims[0] < 0.0) {
        GRpkgIf()->ErrPrintf(ET_ERROR,
            "Y values must be >= 0 for log scale.\n");
        return (false);
    }

    // Fix the plot limits for smith and polar grids
    if (gtype == GRID_POLAR && !xlimset && !ylimset) {
        // (0, 0) in the center of the screen
        double rad = 0.0;
        for (sDvList *dl = dl0; dl; dl = dl->dl_next) {
            sDataVec *d = dl->dl_dvec;
            for (int i = 0; i < d->length(); i++) {
                double r = d->absval(i);
                if (r > rad)
                    rad = r;
            }
        }
        rad *= 1.1;
        if (rad == 0.0)
            rad = 1.0;
        xlims[0] = - rad;
        xlims[1] = rad;
        ylims[0] = - rad;
        ylims[1] = rad;
    }
    else if (gtype == GRID_SMITH || gtype == GRID_SMITHGRID) {
        if (!xlimset && !ylimset) {
            xlims[0] = -1.0;
            xlims[1] = 1.0;
            ylims[0] = -1.0;
            ylims[1] = 1.0;
        }
        else {
            // final check and coersion, don't want to call the plotting
            // routines with bad limits
            
            if (xlims[0] < -1.01 || xlims[0] > 1.01 ||
                    xlims[1] < -1.01 || xlims[1] > 1.01 ||
                    ylims[0] < -1.01 || ylims[0] > 1.01 ||
                    ylims[1] < -1.01 || ylims[1] > 1.01)
                GRpkgIf()->ErrPrintf(ET_WARN,
                    "Smith data out of range [-1, 1], clipped.\n");
            if (xlims[1] > 1.0)
                xlims[1] = 1.0;
            else if (xlims[1] < -0.95)
                xlims[1] = -0.95;
            if (xlims[0] < -1.0)
                xlims[0] = -1.0;
            else if (xlims[0] > 0.95)
                xlims[0] = 0.95;
            if (xlims[0] > xlims[1]) {
                double tt = xlims[0];
                xlims[0] = xlims[1];
                xlims[1] = tt;
            }

            if (ylims[1] > 1.0)
                ylims[1] = 1.0;
            else if (ylims[1] < -0.95)
                ylims[1] = -0.95;
            if (ylims[0] < -1.0)
                ylims[0] = -1.0;
            else if (ylims[0] > 0.95)
                ylims[0] = 0.95;
            if (ylims[0] > ylims[1]) {
                double tt = ylims[0];
                ylims[0] = ylims[1];
                ylims[1] = tt;
            }
        }
    }
    return (true);
}


// Collapse every *xcomp elements into one, and use only the elements
// between xind[0] and xind[1].  Do this for each block of a multi-
// dimensional vector.
//
void
grAttributes::compress(sDataVec *d)
{
    int bsize, blocks;
    if (d->numdims() > 1) {
        bsize = d->dims(d->numdims()-1);
        blocks = d->length()/bsize;
    }
    else {
        bsize = d->length();
        blocks = 1;
    }

    int ilo = (int) xindices[0];
    int ihi = (int) xindices[1];

    if ((ilo <= ihi) && (ilo > 0) && (ilo < bsize) &&
            (ihi > 1) && (ihi <= bsize)) {

        int i = d->length() - blocks*bsize;
        if (i > ihi)
            blocks++;
        if (blocks) {
            int newlen = ihi - ilo;
            if (d->isreal()) {
                double *dd = new double[newlen*blocks];
                double *dst = dd;
                double *src = d->realvec() + ilo;
                for (i = 0; i < blocks; i++) {
                    DCOPY(src, dst, newlen);
                    src += bsize;
                    dst += newlen;
                }
                d->set_realvec(dd, true);
            }
            else {
                complex *cc = new complex[newlen*blocks];
                complex *dst = cc;
                complex *src = d->compvec() + ilo;
                for (i = 0; i < blocks; i++) {
                    CCOPY(src, dst, newlen);
                    src += bsize;
                    dst += newlen;
                }
                d->set_compvec(cc, true);
            }
            d->set_length(newlen*blocks);
            if (d->numdims() <= 1) {
                d->set_numdims(1);
                d->set_dims(0, newlen);
            }
            else
                d->set_dims(d->numdims()-1, newlen);
        }
    }

    int cfac = (int) xcompress;
    if ((cfac > 1) && (cfac < bsize)) {
        int newlen = bsize/cfac;
        for (int j = 0; j < blocks; j++) {
            for (int i = 0; i < newlen; i++)
                if (d->isreal())
                    d->set_realval(i + j*newlen, d->realval(i*cfac + j*bsize));
                else
                    d->set_compval(i + j*newlen, d->compval(i*cfac + j*bsize));
        }
        d->set_length(blocks*newlen);
        if (d->numdims() <= 1)
            d->set_dims(0, d->length());
        else
            d->set_dims(d->numdims()-1, d->length());
    }
}


// Static function.
double *
grAttributes::getnum(const char *name, int nargs)
{
    static double dd[2];
    char buf[32];
    sprintf(buf, "_temp_%s", name);
    if (nargs == 1) {
        VTvalue vv;
        if (Sp.GetVar(buf, VTYP_REAL, &vv)) {
            dd[0] = vv.get_real();
            return (dd);
        }
        if (Sp.GetVar(name, VTYP_REAL, &vv, Sp.CurCircuit())) {
            dd[0] = vv.get_real();
            return (dd);
        }
        return (0);
    }
    else if (nargs == 2) {
        variable *v = Sp.GetRawVar(buf);
        if (!v)
            v = Sp.GetRawVar(name, Sp.CurCircuit());
        if (!v)
            return (0);
        if (v->type() == VTYP_STRING) {
            const char *s = v->string();
            double *d = SPnum.parse(&s, false);
            if (!d) {
                GRpkgIf()->ErrPrintf(ET_ERROR, "bad min value for %s.\n",
                    name);
                return (0);
            }
            dd[0] = *d;
            while (*s && !isdigit(*s) && *s != '-' && *s != '+') s++;
            d = SPnum.parse(&s, false);
            if (!d) {
                GRpkgIf()->ErrPrintf(ET_ERROR, "bad max value for %s.\n",
                    name);
                return (0);
            }
            dd[1] = *d;
            return (dd);
        }
        if (v->type() == VTYP_LIST) {
            v = v->list();
            if (!v || !v->next()) {
                GRpkgIf()->ErrPrintf(ET_ERROR, "bad list for %s.\n", name);
                return (0);
            }
            if (v->type() == VTYP_NUM)
                dd[0] = v->integer();
            else if (v->type() == VTYP_REAL)
                dd[0] = v->real();
            else {
                GRpkgIf()->ErrPrintf(ET_ERROR, "bad min list value for %s.\n",
                    name);
                return (0);
            }
            v = v->next();
            if (v->type() == VTYP_NUM)
                dd[1] = v->integer();
            else if (v->type() == VTYP_REAL)
                dd[1] = v->real();
            else {
                GRpkgIf()->ErrPrintf(ET_ERROR, "bad max list value for %s.\n",
                    name);
                return (0);
            }
            return (dd);
        }
        else
            GRpkgIf()->ErrPrintf(ET_ERROR, "bad type for %s.\n", name);
    }
    return (0);
}


// Static function.
bool
grAttributes::getflag(const char *name)
{
    char buf1[32];
    sprintf(buf1, "_temp_%s", name);
    if (Sp.GetVar(buf1, VTYP_BOOL, 0))
        return (true);
    if (Sp.GetVar(name, VTYP_BOOL, 0, Sp.CurCircuit()))
        return (true);
    return (false);
}


// Static function.
char *
grAttributes::getword(const char *name)
{
    char buf1[64];
    sprintf(buf1, "_temp_%s", name);
    VTvalue vv;
    if (Sp.GetVar(buf1, VTYP_STRING, &vv))
        return (lstring::copy(vv.get_string()));
    if (Sp.GetVar(name, VTYP_STRING, &vv, Sp.CurCircuit()))
        return (lstring::copy(vv.get_string()));
    return (0);
}

