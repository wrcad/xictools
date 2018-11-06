
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
Authors: 1985 Wayne A. Christopher
         1992 Stephen R. Whiteley
****************************************************************************/

#ifndef OUTPLOT_H
#define OUTPLOT_H

#include "config.h"
#include "ftedata.h"
#include "ginterf/graphics.h"

#if defined (HAVE_SETJMP_H) && defined (HAVE_SIGNAL)
#include <signal.h>
#include <setjmp.h>
#endif


//
// General front end stuff for output graphics.
//

#ifndef M_PI
#define M_PI            3.14159265358979323846
#endif

// The graphical interface.  The application takes care of pixel mapping
// so we override the RGBofPixel function.  When X is running, X supplies
// the pixels, in which case the application mapping is updated in
// InitColormap().
//
class SpGrPkg : public GRpkg
{
    bool InitColormap(int mn, int mx, bool dp);
    void RGBofPixel(int p, int *r, int *g, int *b);
};

#define DEF_WIDTH   80  // Line printer width
#define DEF_HEIGHT  66  // Line printer height
#define IPOINTMIN   20  // When we start plotting incremental plots

inline int rnd(double x) { return ((int)(x > 0.0 ? x+0.5 : x-0.5)); }
inline double mylog10(double x) { return (x > 0.0 ? log10(x) : -308.0); }
inline int floorlog(double x)
    { return (x > 0.0 ? (int)floor(log10(x) + 1e-9) : -308); }
inline int ceillog(double x)
    { return (x > 0.0 ? (int)ceil(log10(x) - 1e-9) : -308); }

#define NUMPLOTCOLORS 20    // number of distinct plotting colors
#define MAXNUMTR 18         // max number of traces where yseparate used

enum {GR_PLOT=0, GR_MPLT=1};
#define GR_PLOTstr "plot"
#define GR_MPLTstr "mplot"

// Display scale format.
enum ScaleType { FT_SINGLE=0, FT_GROUP=2, FT_MULTI=3 };

union uGrid
{
    struct {
        char units[32]; // unit labels
        int spacing, numspace, mult;
        double lowlimit, highlimit;
    } lin;
    struct {
        char units[32]; // unit labels
        int hmt, lmt, decsp, subs, pp;
    } log;
    struct {
        int radius, center;
        int hmt, lmt, mag;
    } circular;
};

enum Axis { x_axis, y_axis };


// Regions in plot window for keyed text (text user typed on graph):
//     ---------------------------
//     |      |     U         |  |
//     |      ----------------   |
//     |      |               |  |
//     |      |               |  |
//     |  L   |     P         | R|
//     |      |               |  |
//     |      ----------------   |
//     |      |     D         |  |
//     --------------------------
//
enum KeyedType {KeyedPlot, KeyedLeft, KeyedDown, KeyedRight, KeyedUp};

// Type of label, used internally.  LAyval must be last!
enum LAtype { LAuser=0, LAtitle, LAname, LAdate, LAxlabel, LAylabel,
    LAxunits, LAyunits, LAxval, LAyval };

struct sKeyed
{
    sKeyed()
        {
            text = 0;
            region = KeyedPlot;
            x = y = 0;
            colorindex = 0;
            xform = 0;
            terminated = false;
            fixed = false;
            ignore = false;
            type = LAuser;
            next = 0;
        }

     void update(const sKeyed *k)
        {
            if (fixed)
                return;
            if (!k)
                text[0] = 0;
            else if (type == k->type) {
                if (strcmp(text, k->text)) {
                    delete [] text;
                    text = lstring::copy(k->text);
                }
                region = k->region;
                x = k->x;
                y = k->y;
                colorindex = k->colorindex;
                xform = k->xform;
                terminated = k->terminated;
                fixed = k->fixed;
                ignore = k->ignore;
            }
        }

    ~sKeyed() { delete [] text; }

    char *text;
    KeyedType region;     // region
    int x, y;             // position code for region
    int colorindex;       // index into colors array
    int xform;            // rotation/scale code
    bool terminated;      // flag indicating entry complete
    bool fixed;           // location set by user;
    bool ignore;          // non-selectable, non-editable
    LAtype type;          // type of label
    sKeyed *next;
};

struct sColor
{
    sColor()
        {
            pixel = 0;
            red = 0;
            green = 0;
            blue = 0;
        }

    sColor(unsigned int p, unsigned int r, unsigned int g, unsigned int b)
        {
            pixel = p;
            red = r;
            green = g;
            blue = b;
        }

    unsigned int pixel;
    unsigned char red, green, blue;
};

// Codes for keys pressed.
enum { NO_KEY, UP_KEY, DOWN_KEY, LEFT_KEY, RIGHT_KEY, ENTER_KEY,
    BSP_KEY, DELETE_KEY };

struct sVport
{
    sVport()
        {
            vp_xoff = vp_yoff = 0;
            vp_width = vp_height = 0;
        }

    int left()              { return (vp_xoff); }
    int bottom()            { return (vp_yoff); }
    int right()             { return (vp_xoff + vp_width); }
    int top()               { return (vp_yoff + vp_height); }
    int width()             { return (vp_width); }
    int height()            { return (vp_height); }

    void set_left(int l)    { vp_xoff = l; }
    void set_bottom(int b)  { vp_yoff = b; }
    void set_width(int w)   { vp_width = w; }
    void set_height(int h)  { vp_height = h; }

    int vp_xoff;            // x position
    int vp_yoff;            // y position
    int vp_width;           // width of window
    int vp_height;          // height of window
};

struct sDataWin
{
    sDataWin()
        {
            xmin = ymin = 0;
            xmax = ymax = 0;
        }

    double xmin, ymin, xmax, ymax;
};

struct sRmark
{
    sRmark()
        {
            x = y = 0;
            set = mark = false;
        }

    int x, y;      // reference point, viewport coords
    bool set;      // reference has been set
    bool mark;     // the marker is active
};


struct sChkPts;
struct sGrInit;

// Device-independent data structure for plots.
//
struct sGraph
{
    sGraph() { memset(this, 0, sizeof(sGraph)); }
    // No destructor, don't explicitly delete, gr_reset deallocates.

    // graphics package
    GRwbag *gr_new_gx(int);
    int gr_pkg_init();
    void gr_pkg_init_colors();
    bool gr_init_btns();
    bool gr_check_plot_events();
    void gr_redraw();
    void gr_refresh(int, int, int, int, bool = false);
    void gr_popdown();

    // graph.cc
    bool gr_setup_dev(int, const char*);
    int gr_dev_init();
    void gr_reset();
    sGraph* gr_copy();
    void gr_update_keyed(sGraph*, bool);
    void gr_abort();
    bool gr_redraw_direct();
    void gr_redraw_keyed();
    void gr_init_data();
    void *gr_copy_data();
    void gr_destroy_data();
    bool gr_add_trace(sDvList*, int);
    bool gr_delete_trace(int);
    void gr_key_hdlr(const char*, int, int, int);
    void gr_bdown_hdlr(int, int, int);
    void gr_bup_hdlr(int, int, int);
    int gr_select_trace(int, int);
    void gr_replot();
    void gr_zoomin(int, int);
    void gr_zoom(double, double, double, double);
    void gr_end();
    void gr_mark();
    void gr_refmark();
    void gr_setref(int, int);
    void gr_data_to_screen(double, double, int*, int*);
    void gr_screen_to_data(int, int, double*, double*);
    void gr_save_text(const char*, int, int, int, int, int);
    void gr_set_keyed_posn(sKeyed*, int, int);
    void gr_get_keyed_posn(sKeyed*, int*, int*);
    bool gr_get_keyed_bb(sKeyed*, int*, int*, int*, int*);
    void gr_writef(double, sUnits*, int, int, bool = false);
    void gr_draw_last(int);
    void gr_set_ghost(GhostDrawFunc, int, int);
    void gr_show_ghost(bool);
    void gr_ghost_mark(int, int, bool);
    void gr_ghost_zoom(int, int, int, int);
    void gr_ghost_tbox(int, int);
    void gr_ghost_trace(int, int);
    void gr_show_sel_text(bool);
    void gr_show_logo();

    // grid.cc
    void gr_linebox(int, int, int, int);
    void gr_fixgrid();
    void gr_redrawgrid();

    // grsetup.cc
    void setup(sDvList*, sGrInit*);

    // hardcopy.cc
    void gr_hardcopy();

    // mplot.cc
    void mp_prompt(sChkPts*);
    void mp_mark(char);
    void mp_bdown_hdlr(int, int, int);

    int yinv(int y) { return (gr_area.top() - y - 1); }

    sKeyed *get_keyed(int num)
        {
            for (sKeyed *k = gr_keyed; k; k = k->next) {
                if (k->type == num)
                    return (k);
            }
            return (0);
        }

    // The following functions get and set the plot trace name text.
    //
    char *get_txt(int num)
        {
            sKeyed *k = get_keyed(LAyval + num);
            return (k ? k->text : 0);
        }

    void set_txt(int num, char *text)
        {
            sKeyed *k = get_keyed(LAyval + num);
            if (k)
                k->text = text;
        }

    int id()                        { return (gr_id); }
    void newid(int gid)
        {
            // used in graphdb.cc
            gr_id = gid;
            gr_degree = 1;
            gr_linestyle = -1;
        }

    int apptype()                   { return (gr_apptype); }
    void set_apptype(int a)         { gr_apptype = a; }

    GRdraw *dev()                   { return (gr_dev); }
    void set_dev(GRdraw *d)         { gr_dev = d; }
    void halt()
        {
            if (gr_dev)
                gr_dev->Halt();
            gr_dev = 0;
        }

    wordlist *command()             { return (gr_command); }
    const char *plotname()          { return (gr_plotname); }
    const char *title()             { return (gr_title); }

    void *plotdata()                { return (gr_plotdata); }
    void set_plotdata(void *d)      { gr_plotdata = d; }

    sVport &area()                  { return (gr_area); }
    int numtraces()                 { return (gr_numtraces); }

    void set_fontsize()
        {
            gr_dev->TextExtent(0, &gr_fontwid, &gr_fonthei);
        }

    sDataWin &datawin()             { return (gr_datawin); }
    sDataWin &rawdata()             { return (gr_rawdata); }
    double grpmin(int i)            { return (gr_grpmin[i]); }
    double grpmax(int i)            { return (gr_grpmax[i]); }
    uGrid &xaxis()                  { return (gr_xaxis); }
    uGrid &yaxis()                  { return (gr_yaxis); }

    int effleft()                   { return (gr_area.left() + gr_dimwidth); }
    int effwid()                    { return (gr_area.width() - gr_dimwidth); }

    sRmark &reference()             { return (gr_reference); }

    sColor &color(int i)            { return (gr_colors[i]); }

    PlotType plottype()             { return (gr_pltype); }
    void set_plottype(PlotType t)   { gr_pltype = t; }
    GridType gridtype()             { return (gr_grtype); }
    void set_gridtype(GridType t)   { gr_grtype = t; }
    ScaleType format()              { return (gr_format); }
    void set_format(ScaleType t)    { gr_format = t; }

    bool yseparate()                { return (gr_ysep); }
    void set_yseparate(bool b)      { gr_ysep = b; }
    bool xmonotonic()               { return (gr_xmono); }
    void set_noevents(bool b)       { gr_noevents = b; }

    bool dirty()                    { return (gr_dirty); }
    void set_dirty(bool b)          { gr_dirty = b; }

    int cmdmode()                   { return (gr_cmdmode); }
    void set_cmdmode(int m)         { gr_cmdmode = m; }

    void clear_selections()
        {
            delete [] gr_selections;
            gr_selections = 0;
            gr_selsize = 0;
            gr_sel_flat = false;
        }

#ifdef WIN32
    bool destroyed()                { return (gr_destroyed); }
    void set_destroyed(bool b)      { gr_destroyed = b; }
    bool redraw_queued()            { return (gr_redraw_queued); }
    void set_redraw_queued(bool b)  { gr_redraw_queued = b; }
    void msw_stop()
        {
            gr_cpage = 0;
            gr_stop = true;
        }
    int in_redraw()                { return (gr_in_redraw); }
#endif

    // This is a hack to delete the units strings from the saved text
    // list.  They will be regenerated, with possibly new contents, in
    // the redraw.
    //
    void clear_saved_text()
        {
            sKeyed *kp = 0, *kn;
            for (sKeyed *k = gr_keyed; k; k = kn) {
                kn = k->next;
                if (k->type == LAxunits || k->type == LAyunits) {
                    if (!kp)
                        gr_keyed = kn;
                    else
                        kp->next = kn;
                    delete k;
                }
                else
                    kp = k;
            }
        }

    // Clear the units text, latype is LAxunits or LAyunits.
    //
    void clear_units_text(LAtype latype)
        {
            sKeyed *kp = 0;
            for (sKeyed *k = gr_keyed; k; k = k->next) {
                if (k->type == latype) {
                    if (!k->fixed) {
                        if (!kp)
                            gr_keyed = k->next;
                        else
                            kp->next = k->next;
                        delete k;
                    }
                    break;
                }
                kp = k;
            }
        }

private:

    // graph.cc
    bool dv_scale_icon_hdlr(int, int, int);
    bool dv_dims_map_hdlr(int, int, int, bool);
    sDvList *dv_copy_data();
    void dv_destroy_data();
    bool dv_redraw();
    void dv_initdata();
    void dv_resize();
    void dv_trace(bool);
    void dv_legend(int, sDataVec*);
    void dv_set_trace(sDataVec*, sDataVec*, int);
    void dv_plot_trace(sDataVec*, sDataVec*, int);
    void dv_plot_interval(sDataVec*, double*, double*, sPoly*, bool);
    void dv_point(sDataVec*, double, double, double, double, int);
    void dv_erase_factors();
    bool dv_find_where(sDataVec*, int, double*, int*);
    void dv_find_y(sDataVec*, sDataVec*, int, double, double*);
    void dv_pl_environ(double, double, double, double, bool);
    void dv_find_selections();

    static int timeout(void*);

    // grid.cc
    void lingrid(double*, Axis);
    void set_lin_trace_limits();
    void drawlingrid(Axis, bool);
    void loggrid(double*, Axis);
    void set_log_trace_limits();
    void drawloggrid(Axis, bool);
    void set_raw_trace_limits();
    void polargrid();
    void drawpolargrid();
    void smithgrid();
    void drawsmithgrid();
    void set_scale_4(double, double, double*, double*, double) const;
    void set_scale(double, double, double*, double*, int*, double) const;
    void set_lin_grid(double*, double, double, uGrid*, int*) const;
    void set_log_grid(double*, double, double, uGrid*) const;
    void add_deg_label(int, int, int, int, int, int, int);
    void add_rad_label(int, double, int, int);
    void arc_set(double, double, double, double, double, int, int,
        char*, char*, int, int, int, int, int*);
    double clip_arc(double, double, int, int, int, int, int, int, int);

    // mplot.cc
    void mp_free_sel();
    bool mp_redraw();
    void mp_initdata();
    sChkPts *mp_copy_data();
    void mp_destroy_data();
    void mp_xbox(int, int, int, int);
    void mp_pbox(int, int, int, int);
    void mp_writeg(double, int, int, int);

    int gr_id;                      // identifier for this graph
    int gr_apptype;                 // type code for this graph
    GRdraw *gr_dev;                 // device dependent part
    const char *gr_filename;        // for hardcopy

    wordlist *gr_command;           // command line copy
    const char *gr_plotname;        // name of plot this graph is in
    const char *gr_title;           // title for graph, from plot
    const char *gr_date;            // plot date
    void *gr_plotdata;              // normalized data

    sVport gr_area;                 // overall display area
    sVport gr_vport;                // window of the plotting area
    int gr_numtraces;               // number of dvecs to plot
    int gr_scalewidth;              // width of left side scale text in chars
    int gr_fontwid;                 // font size
    int gr_fonthei;
    sDataWin gr_datawin;            // plotting range
    sDataWin gr_rawdata;            // raw data range
    double gr_aspect_x;             // cache (datawindow size)/(viewport size)
    double gr_aspect_y;
    double gr_grpmin[3];            // scales for type classes
    double gr_grpmax[3];
    uGrid gr_xaxis;
    uGrid gr_yaxis;

    sRmark gr_reference;            // the reference mark

    int gr_linestyle;               // current line style
    int gr_color;                   // current pen color
    sColor gr_colors[NUMPLOTCOLORS];  // color table
    int gr_ticmarks;                // mark every ticmark'th point
    int gr_field;                   // character width of factor box

    PlotType gr_pltype;             // defined in ftedata.h
    GridType gr_grtype;             // defined in ftedata.h
    ScaleType gr_format;            // format: F_?

    const char *gr_xlabel;          // for grid axes
    const char *gr_ylabel;
    int gr_degree;                  // degree of polynomial interpretation

    int gr_dimwidth;                // correction factor for dimens list

    // used int grid.cc
    sUnits gr_xunits;
    sUnits gr_yunits;
    double gr_xdelta;               // if non-zero, user-specified deltas
    double gr_ydelta;
    int gr_numycells;               // used for multiple y scales

    bool gr_ysep;                   // separate traces along vertical
    bool gr_nogrid;                 // don't show grid
    bool gr_xmono;                  // scale is monotonic
    bool gr_xlimfixed;              // x limits fixed
    bool gr_ylimfixed;              // y limits fixed
    bool gr_stop;                   // stop flag
    bool gr_oneval;                 // true if plotting one value
                                    //  against itself (real vs imaginary)
    bool gr_noevents;               // suppress event checking
    bool gr_noplotlogo;             // suppress WRspice logo

    // used in rendering code
    bool gr_dirty;                  // plot needs update;
    bool gr_seltext;                // text is selected
    bool gr_nolinecache;            // skip caching for iplot
#ifdef WIN32
    bool gr_destroyed;              // destroy requested
    bool gr_redraw_queued;          // redraw pending
#endif

    // Characters the user typed on graph.
    sKeyed *gr_keyed;               // list head for saved text
    int gr_timer_id;                // id of move-mode timer for keyed text

    int gr_pressx;                  // button press point
    int gr_pressy;
#define Moving       1
#define ZoomIn       2
#define ShiftMode   16
#define ControlMode 32
    unsigned int gr_cmdmode;        // in-command flags
    sDvList *gr_cmd_data;           // transient trace data storage

    char *gr_selections;            // trace display mask
    short gr_selsize;               // selection size
    bool gr_sel_flat;               // selection list is flat
    bool gr_sel_show;               // displaying selection list
    bool gr_sel_drag;               // in drag, to selecte/deselect range
    int gr_sel_x;                   // drag start x
    int gr_sel_y;                   // drag start y

    unsigned int gr_scale_flags;    // suppress rescaling for scale icon funcs
                                    //  0       x-scale
                                    //  1-3     class0-2
                                    //  4       aggregate
                                    //  others  vector min/max

    short gr_cpage;                 // dimension list paging
    short gr_npage;

    int gr_in_redraw;               // redrawing call depth

#if defined (HAVE_SETJMP_H) && defined (HAVE_SIGNAL)
public:
    void(*oldhdlr)(int);            // old FPE handler
    jmp_buf jmpbuf;
#endif
};


// Structures for operating range analysis plotting.

enum MDtype { MDnormal, MDmonte, MDdimens };

// Linked list of x-y file points to plot.
struct sChkPts
{
    sChkPts() { memset(this, 0, sizeof(sChkPts)); }

    ~sChkPts()
        {
            delete [] v1;
            delete [] v2;
            delete [] pf;
            delete [] sel;
            delete [] date;
            delete [] filename;
            delete [] plotname;
            delete [] param1;
            delete [] param2;
        }


    double minv1;   // min/max data values
    double maxv1;
    double minv2;
    double maxv2;
    MDtype type;    // data data
    bool flat;      // linear data flag
    bool pfset;     // pass/fail valid
    const char *param1;   // parameter 1 name
    const char *param2;   // parameter 1 name
    const char *filename; // file or vector name
    const char *plotname; // plot name of vector
    char *date;     // file date
    char *v1;       // value 1 steps
    char *v2;       // value 2 steps
    char *pf;       // boolean pass/fail values
    char *sel;      // boolean selected status
    int delta1;     // number of steps 1
    int delta2;     // number of steps 2
    int size;       // length of list
    int rsize;      // allocated length
    int xc;         // screen center x
    int yc;         // screen center y
    int d;          // screen cell size
    int d1;         // current step 1
    int d2;         // current step 2
    sChkPts *next;
};


// Structure to pass to gr_init(), etc.
//
struct sGrInit
{
    sGrInit()
        {
            xlims[0] = 0.0;
            xlims[1] = 0.0;
            ylims[0] = 0.0;
            ylims[1] = 0.0;
            xdelta = 0;
            ydelta = 0;        
            xname = 0;
            plotname = 0;       
            title = 0;
            hcopy = 0;
            xlabel = 0;
            ylabel = 0;
            command = 0;
            nplots = 0;
            gridtype = GRID_LIN;
            plottype = PLOT_LIN;
            format = FT_SINGLE;
            nointerp = false;
            ysep = false;
            noplotlogo = false;
            nogrid = false;

            free_title = false;
            free_xlabel = false;
            free_ylabel = false;
        }

    ~sGrInit()
        {
            if (free_title)
                delete [] title;
            if (free_xlabel)
                delete [] xlabel;
            if (free_ylabel)
                delete [] ylabel;
        }

    double xlims[2];      // The size of the screen
    double ylims[2];
    double xdelta;        // Line increments for the scale
    double ydelta;        
    sUnits xtype;         // The types of the data graphed
    sUnits ytype;
    const char *xname;    // What to label things
    const char *plotname;       
    const char *title;          
    const char *hcopy;    // The raster file
    const char *xlabel;   // Labels for axes
    const char *ylabel;         
    wordlist *command;    // comand line that created plot
    int nplots;           // How many plots there will be
    GridType gridtype;    // The grid type
    PlotType plottype;    //  and the plot type
    ScaleType format;     // multi, single, or group scales
    bool nointerp;        // skip interpolation
    bool ysep;            // separate multiple traces along vertical
    bool noplotlogo;      // don't show WRspice logo
    bool nogrid;          // don't show grid
    bool xlimfixed;       // x limits were specified
    bool ylimfixed;       // y limits were specified

    bool free_title;      // GC flags
    bool free_xlabel;
    bool free_ylabel;
};

// Generator
struct sGgen
{
    sGgen(int i, sGraph *l) { num = i; graph = l; }
    sGraph *next();

private:
    sGraph *graph;
    int num;
};

struct SPgraphics
{
    SPgraphics()
        {
            spg_cur = 0;
            spg_echogr = 0;
            spg_sourceGraph = 0;
            spg_tmpGraph = 0;
            spg_mainThread = 0;
            spg_mplotOn = false;
            spg_running_id = 1;
        };

    // doplot.cc
    bool Plot(wordlist*, sGraph*, const char*, const char*, int);

    // grsetup.cc
    bool Setup(sGrInit*, sDvList**, const char*, sDataVec*, const char*);
    sGraph *Init(sDvList*, sGrInit*, sGraph* = 0);

    // asciplot.cc
    void AsciiPlot(sDvList*, const char*);

    // xgplot.cc
    void Xgraph(sDvList*, sGrInit*);

    // graphdb.cc
    sGraph *NewGraph();
    sGraph *NewGraph(int, const char*);
    sGraph *FindGraph(int);
    bool DestroyGraph(int);
    void FreeGraphs();
    void SetGraphContext(int);
    void PushGraphContext(sGraph*);
    void PopGraphContext();
    void PlotPosition(int*, int*);
    sGgen *InitGgen();

    // mplot.cc
    bool MpParse(const char*, int*, int*, char*, char*, char*) const;
    int MpInit(int, int, double, double, double, double, bool, const sPlot*);
    int MpWhere(int, int, int);
    int MpMark(int, char);
    int MpDone(int);

    // plotutil.cc
    int Getchar(int, int, int*);
    bool isMainThread();
    int Checkup();
    void ReturnEvent(int*, int*, int*, int*);
    void HaltFullScreenGraphics();
    int PopUpXterm(const char*);

    sGraph *Cur()               { return (spg_cur); }
    sGraph *SourceGraph()       { return (spg_sourceGraph); }
    sGraph *TmpGraph()          { return (spg_tmpGraph); }
    int RunningId()             { return (spg_running_id); }

    void SetSourceGraph(sGraph *g)  { spg_sourceGraph = g; }
    void SetTmpGraph(sGraph *g)     { spg_tmpGraph = g; }
    void SetMplotOn(bool b)         { spg_mplotOn = b; }

private:
    sGraph *spg_cur;            // the current sGraph
    sGraph *spg_echogr;         // used to route text during iplot
    sGraph *spg_sourceGraph;    // graph with button-down selection
    sGraph *spg_tmpGraph;       // parent of zoomin
    void   *spg_mainThread;     // main application thread id
    bool spg_mplotOn;           // margin plot while running
    int spg_running_id;         // counter for sGraph id assignment
};

//
// Definitions for external symbols for output graphics.
// XXX put these somewhere

// grsetup.cc
extern void SetDefaultColors();

// ui-specific
extern void LoadResourceColors();

extern SPgraphics GP;
extern const char *DefPointchars;
extern sColor DefColors[NUMPLOTCOLORS];     // default plotting colors...
extern const char *DefColorNames[NUMPLOTCOLORS];  // ... and their names
extern HCcb wrsHCcb;

// clip.cc
extern bool clip_line(int*, int*, int*, int*, int, int, int, int);
extern bool clip_to_circle(int*, int*, int*, int*, int, int, int);

#endif // OUTPLOT_H

