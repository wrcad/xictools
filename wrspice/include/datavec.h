
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

#ifndef DATAVEC_H
#define DATAVEC_H

#include <math.h>
#include "ifdata.h"
#include "misc.h"


// references
struct sDataVec;
struct sPlot;
struct sJOB;
struct sOPTIONS;
struct variable;
struct sHtab;
struct wordlist;
struct sTrie;

// Complex numbers
//
struct complex : public IFcomplex
{
    complex(double r=0.0, double i=0.0) { real = r; imag = i; };

    double mag() { return (sqrt(real*real + imag*imag)); };
    double phs() { return (real || imag ? atan2(imag, real) : 0.0); };

    // Compute the square root, in place.
    //
    void csqrt()
        {
            if (real == 0.0) {
                if (imag == 0.0)
                    return;
                if (imag > 0.0) {
                    real = sqrt(0.5*imag);
                    imag = real;
                }
                else {
                    imag = -sqrt(-0.5*imag);
                    real = -imag;
                }
            }
            else if (real > 0.0) {
                if (imag == 0.0)
                    real = sqrt(real);
                else {
                    double d = mag();
                    real = sqrt(0.5*(d + real));
                    imag /= 2.0*real;
                }
            }
            else {
                if (imag == 0.0) {
                    imag = sqrt(-real);
                    real = 0.0;
                }
                else {
                    double d = mag();
                    double t = imag;
                    imag = sqrt(0.5*(d - real));
                    if (t < 0.0)
                        imag = -imag;
                    real = t/(2.0*imag);
                }
            }
        }
};

// Polynomial curve fitting.
//
struct sPoly
{
    sPoly(int d)
        {
            pc_degree = d;
            pc_scratch = new double[(d+1)*(d+2)];
        }

    ~sPoly()
        {
            delete [] pc_scratch;
        }

    bool interp(double*, double*, double*, int, double*, int);
    bool polyfit(double*, double*, double*);
    double peval(double, double*, int = 0);

    void pderiv(double *coeffs)
        {
            for (int i = 0; i < pc_degree; i++)
                coeffs[i] = (i + 1)*coeffs[i + 1];
        }

    int degree()        { return (pc_degree); }
    int dec_degree()    { return (--pc_degree); }

private:
    int pc_degree;
    double *pc_scratch;
};

// Vector types.
enum Utype
{
    UU_NOTYPE,
    UU_TIME,
    UU_FREQUENCY,
    UU_VOLTAGE,
    UU_CURRENT,
    UU_CHARGE,
    UU_FLUX,
    UU_CAP,
    UU_IND,
    UU_RES,
    UU_COND,
    UU_LEN,
    UU_AREA,
    UU_TEMP,
    UU_POWER,
    UU_POLE,
    UU_ZERO,
    UU_SPARAM,
    UU_OUTPUT_N_DENS,
    UU_OUTPUT_NOISE,
    UU_INPUT_N_DENS,
    UU_INPUT_NOISE
};

// Elemental units.
enum Ufund
{
    uuTime,
    uuVoltage,
    uuCurrent,
    uuLength,
    uuTemper
};


struct sUnits
{
    sUnits()
        {
            memset(units, 0, 8*sizeof(char));
        }

    void set(int);
    void set(IFparm*);
    bool set(const char*);
    void operator*(Ufund t) { units[t]++; }
    void operator/(Ufund t) { units[t]--; };
    void operator*(sUnits &u)
        { for (int i = 0; i < 8; i++)
            units[i] += u.units[i]; }
    void operator/(sUnits &u)
        { for (int i = 0; i < 8; i++)
            units[i] -= u.units[i]; }

    // Note that "no units" is equivalent to any units.
    bool operator==(const sUnits &u) const
        { if (isnotype() || u.isnotype()) return (true);
          for (int i = 0; i < 8; i++)
            if (units[i] != u.units[i]) return (false);
          return (true); }
    bool operator==(Utype) const;
    bool isnotype() const
        { for (int i = 0; i < 8; i++)
            if (units[i]) return (false);
          return (true); }
    char *unitstr() const;

private:
    void trial(const sUnits&, int*, bool) const;

    char units[8];
};

// List of data vectors.
//
struct sDvList
{
    sDvList()
        {
            dl_dvec = 0;
            dl_next = 0;
        }

    static void destroy(sDvList *dvl)
        {
            while (dvl) {
                sDvList *dvx = dvl;
                dvl = dvl->dl_next;
                delete dvx;
            }
        }

    sDataVec *dl_dvec;
    sDvList *dl_next;
};

// Dimension label for multi-dimensional plots, with hooks for range
// data.
//
struct sDimen
{
    sDimen(const char *n1, const char *n2)
        {
            d_varname[0] = lstring::copy(n1);
            d_varname[1] = lstring::copy(n2);
            d_start[0] = 0.0;
            d_start[1] = 0.0;
            d_stop[0] = 0.0;
            d_stop[1] = 0.0;
            d_step[0] = 0.0;
            d_step[1] = 0.0;
            d_delta[0] = 0;
            d_delta[1] = 0;
            d_v1 = 0;
            d_v2 = 0;
            d_pf = 0;
            d_size = 0;
        }

    ~sDimen()
        {
            delete [] d_varname[0];
            delete [] d_varname[1];
            delete [] d_v1;
            delete [] d_v2;
            delete [] d_pf;
        }

    void add_point(int i, int j, bool f, int sz1, int sz2)
        {
            if (!d_v1) {
                int sz = sz1 * sz2;
                if (sz <= 0)
                    return;
                d_v1 = new char[sz];
                d_v2 = new char[sz];
                d_pf = new char[sz];
                d_delta[0] = sz1;
                d_delta[1] = sz2;
            }
            d_v1[d_size] = i;
            d_v2[d_size] = j;
            d_pf[d_size] = !f;
            d_size++;  
        }

    const char *varname1()          { return (d_varname[0]); }
    const char *varname2()          { return (d_varname[1]); }

    double start1()                 { return (d_start[0]); }
    void set_start1(double d)       { d_start[0] = d; }

    double start2()                 { return (d_start[1]); }
    void set_start2(double d)       { d_start[1] = d; }

    double stop1()                  { return (d_stop[0]); }
    void set_stop1(double d)        { d_stop[0] = d; }

    double stop2()                  { return (d_stop[1]); }
    void set_stop2(double d)        { d_stop[1] = d; }

    double step1()                  { return (d_step[0]); }
    void set_step1(double d)        { d_step[0] = d; }

    double step2()                  { return (d_step[1]); }
    void set_step2(double d)        { d_step[1] = d; }

    int delta1()                    { return (d_delta[0]); }
    int delta2()                    { return (d_delta[1]); }

    const char *v1()                { return (d_v1); }
    const char *v2()                { return (d_v2); }
    const char *pf()                { return (d_pf); }
    int size()                      { return (d_size); }

private:
    const char *d_varname[2];
    double d_start[2];
    double d_stop[2];
    double d_step[2];
    int d_delta[2];
    char *d_v1;
    char *d_v2;
    char *d_pf;
    int d_size;
};

// General list element for plots.
//
struct sPlotList
{
    sPlotList(sPlot *p, sPlotList *n) { plot = p; next = n; }

    sPlot *plot;
    sPlotList *next;
};

// Base interface class for data file output.
//
class cFileOut
{
public:
    virtual ~cFileOut() { };
    virtual bool file_write(const char*, bool) = 0;
    virtual bool file_open(const char*, const char*, bool) = 0;
    virtual void file_set_fp(FILE*) = 0;
    virtual bool file_head() = 0;
    virtual bool file_vars() = 0;
    virtual bool file_points(int = -1) = 0;
    virtual bool file_update_pcnt(int) = 0;
    virtual bool file_close() = 0;
};


// The information for a particular set of vectors that come from one
// plot.
//
struct sPlot
{
    // Type of plot: unknown, simulation data, constants, exec local data.
    enum PLtype { PLnone, PLsimrun, PLconst, PLexec };

    // plots.cc
    sPlot(const char*);
    ~sPlot();

    void new_perm_vec(sDataVec*);
    sDataVec *get_perm_vec(const char*) const;
    wordlist *list_perm_vecs() const;
    int num_perm_vecs() const;
    sDataVec *find_vec(const char*);
    sDataVec *remove_perm_vec(const char*);
    void remove_vec(const char*);
    int num_perm_vecs();
    void set_case(bool);
    void new_plot();
    void add_plot();
    bool add_segment(const sPlot*, char**);
    void run_commands();
    bool compare(const sPlot*, char**);
    void destroy();
    void clear_selected();
    void add_note(const char*);
    bool set_dims(int, const int*);

    // linear.cc
    void linearize(wordlist*);
    void lincopy(sDataVec*, double*, int, sPlot*);

    // postcoms.cc
    sDvList *write(sDvList*, bool, const char*);

    const char *title()                     const { return (pl_title); }
    void set_title(const char *c)
        {
            char *s = lstring::copy(c);
            delete [] pl_title;
            pl_title = s;
        }

    const char *date()                      const { return (pl_date); }
    void set_date(const char *c)
        {
            char *s = lstring::copy(c);
            delete [] pl_date;
            pl_date = s;
        }

    const char *name()                      const { return (pl_name); }
    void set_name(const char *c)
        {
            char *s = lstring::copy(c);
            delete [] pl_name;
            pl_name = s;
        }

    const char *type_name()                 const { return (pl_typename); }
    void set_type_name(const char *c)
        {
            char *s = lstring::copy(c);
            delete [] pl_typename;
            pl_typename = s;
        }

    const char *circuit()                   const { return (pl_circuit); }
    void set_circuit(const char *c)
        {
            char *s = lstring::copy(c);
            delete [] pl_circuit;
            pl_circuit = s;
        }

    sPlot *next_plot()                      { return (pl_next); }
    void set_next_plot(sPlot *n)            { pl_next = n; }

    sDataVec *tempvecs()                    { return (pl_dvecs); }
    void set_tempvecs(sDataVec *v)          { pl_dvecs = v; }

    sDataVec *scale()                       { return (pl_scale); }
    void set_scale(sDataVec *v)             { pl_scale = v; }

    wordlist *commands()                    { return (pl_commands); }
    void set_commands(wordlist *c)          { pl_commands = c; }

    variable *environment()                 { return (pl_env); }
    void set_environment(variable *v)       { pl_env = v; }

    sDimen *dimensions()                    { return (pl_dims); }
    void set_dimensions(sDimen *d)
        {
            delete pl_dims;
            pl_dims = d;
        }

    sOPTIONS *options()                     { return (pl_ftopts); }
    void set_options(sOPTIONS *o)           { pl_ftopts = o; }

    void range(double *start, double *stop, double *step)
        {
            if (start)
                *start = pl_start;
            if (stop)
                *stop = pl_stop;
            if (step)
                *step = pl_step;
        }
    void set_range(double start, double stop, double step)
        {
            pl_start = start;
            pl_stop = stop;
            pl_step = step;
        }

    sTrie **ccom()                          { return (&pl_ccom); }

    int num_dimensions()                    const { return (pl_ndims); }
    void set_num_dimensions(int n)          { pl_ndims = n; }

    int fftsc_ix()                          const { return (pl_fftsc_ix); }
    void set_fftsc_ix(int n)                { pl_fftsc_ix = n; }

    bool active()                           const { return (pl_active); }
    void set_active(bool b)                 { pl_active = b; }

    bool written()                          const { return (pl_written); }
    void set_written(bool b)                { pl_written = b; }

    wordlist *notes()                       const { return (pl_notes); }

    PLtype type()                           const { return ((PLtype)pl_type); }
    void set_type(PLtype t)                 { pl_type = (char)t; }

private:
    char *pl_title;         // The title card.
    char *pl_date;          // Date.
    char *pl_name;          // The plot name.
    char *pl_typename;      // Tran1, op2, etc.
    char *pl_circuit;       // The name of originating ckt, if any.
    wordlist *pl_notes;     // Added note strings for this plot.

    sPlot *pl_next;         // List of plots.
    sHtab *pl_hashtab;      // Hash head for permanent vectors.
    sDataVec *pl_dvecs;     // The data vectors in this plot.
    sDataVec *pl_scale;     // The "scale" for the rest of the vectors.
    wordlist *pl_commands;  // Commands to execute for this plot.
    variable *pl_env;       // The 'environment' for this plot.
    sTrie *pl_ccom;         // The ccom struct for this plot.
    sDimen *pl_dims;        // Dimension labels.
    sOPTIONS *pl_ftopts;    // Spice opts set by shell.

    double pl_start;        // The "tran params" for this plot.
    double pl_stop;
    double pl_step;

    int pl_ndims;           // Number of dimensions.
    int pl_fftsc_ix;        // Index to keep FFT scale names unique.
    bool pl_active;         // True when the plot is being used.
    bool pl_written;        // Some or all of the vecs have been saved.
    char pl_type;           // Type of data.
};

// Structure:  sDataVec
//
// A (possibly multi-dimensional) data vector.  The data is
// represented internally by a 1-d array.  The number of dimensions
// and the size of each dimension is recorded, along with v_length,
// the total size of the array.  If the dimensionality is 0 or 1,
// v_length is significant instead of v_numdims and v_dims, and the
// vector is handled in the old manner.
//
#define MAXDIMS 8

// sDataVec flags
#define VF_COMPLEX   0x1      // The data are complex
#define VF_SMITH     0x2      // Data transformed for Smith plot
#define VF_POLE      0x4      // PZ data: poles
#define VF_ZERO      0x8      // PZ data: zeros
#define VF_ROLLOVER  0x10     // Vector has rolled-over data
#define VF_NOSXZE    0x20     // Do not scalarize or segmentize
#define VF_COPYMASK  0x3f     // Flags that are generally copied

#define VF_PERMANENT 0x40     // Don't garbage collect this vector
#define VF_MINGIVEN  0x80     // The v_minsignal value is valid
#define VF_MAXGIVEN  0x100    // The v_maxsignal value is valid
#define VF_SELECTED  0x200    // Vector is currently selected
#define VF_READONLY  0x400    // Vector can't be changed
#define VF_TEMPORARY 0x800    // Vector will be freed immediately
#define VF_STRING    0x1000   // Vector has string data in v_defcolor.

// Grid types.
// SMITHGRID is only a smith grid, SMITH transforms the data.
enum GridType
{
    GRID_LIN,
    GRID_LOGLOG,
    GRID_XLOG,
    GRID_YLOG,
    GRID_POLAR,
    GRID_SMITH,
    GRID_SMITHGRID
};

// Plot types.
enum PlotType
{
    PLOT_LIN,
    PLOT_COMB,
    PLOT_POINT
};

struct sDataVec
{
    // Temp storage of vector data for scalarizing.
    //
    struct scalData
    {
        scalData(sDataVec *v)
            {
                if (v) {
                    length = v->length();
                    rlength = v->allocated();
                    numdims = v->numdims();
                    for (int i = 0; i < MAXDIMS; i++)
                        dims[i] = v->dims(i);
                    real = v->realval(0);
                    imag = v->imagval(0);

                }
                else {
                    length = 0;
                    rlength = 0;
                    numdims = 0;
                    memset(dims, 0, MAXDIMS*sizeof(int));
                    real = 0.0;
                    imag = 0.0;
                }
            }

        int length;
        int rlength;
        int numdims;
        int dims[MAXDIMS];
        double real;
        double imag;
    };

    // Temp storage of vector data for segmentizing.
    //
    struct segmData
    {
        segmData(sDataVec *v)
            {
                if (v) {
                    length = v->length();
                    rlength = v->allocated();
                    numdims = v->numdims();
                    for (int i = 0; i < MAXDIMS; i++)
                        dims[i] = v->dims(i);
                    if (v->isreal())
                        tdata.real = v->realvec();
                    else
                        tdata.comp = v->compvec();
                }
                else {
                    length = 0;
                    rlength = 0;
                    numdims = 0;
                    memset(dims, 0, MAXDIMS*sizeof(int));
                    tdata.real = 0;
                }
            }

        int length;
        int rlength;
        int numdims;
        int dims[MAXDIMS];
        union { double *real; complex *comp; } tdata;
    };

    sDataVec(int t = UU_NOTYPE)
        {
            v_data.real = 0;

            v_name = 0;
            v_units.set(t);
            v_minsignal = v_maxsignal = 0.0;

            v_plot = 0;
            v_scale = 0;
            v_next = 0;
            v_link2 = 0;
            v_defcolor = 0;
            v_scaldata = 0;
            v_segmdata = 0;

            v_flags = 0;
            v_length = 0;
            v_rlength = 0;
            v_gridtype = GRID_LIN;
            v_plottype = PLOT_LIN;
            v_linestyle = 0;
            v_color = 0;
            v_numdims = 0;
            memset(v_dims, 0, MAXDIMS*sizeof(int));
        }

    sDataVec(const sUnits &u)
        {
            v_data.real = 0;

            v_name = 0;
            v_units = u;
            v_minsignal = v_maxsignal = 0.0;

            v_plot = 0;
            v_scale = 0;
            v_next = 0;
            v_link2 = 0;
            v_defcolor = 0;
            v_scaldata = 0;
            v_segmdata = 0;

            v_flags = 0;
            v_length = 0;
            v_rlength = 0;
            v_gridtype = GRID_LIN;
            v_plottype = PLOT_LIN;
            v_linestyle = 0;
            v_color = 0;
            v_numdims = 0;
            memset(v_dims, 0, MAXDIMS*sizeof(int));
        }

    sDataVec(char *vname, int type, int len, const sUnits *u = 0,
        void *data = 0)
        {
            v_data.real = 0;
            if (data)
                v_data.real = (double*)data;
            else if (len) {
                if (type & VF_COMPLEX)
                    v_data.comp = new complex[len];
                else
                    v_data.real = new double[len];
            }

            v_name = vname;
            if (u)
                v_units = *u;
            else
                v_units.set(UU_NOTYPE);
            v_minsignal = v_maxsignal = 0.0;

            v_plot = 0;
            v_scale = 0;
            v_next = 0;
            v_link2 = 0;
            v_defcolor = 0;
            v_scaldata = 0;
            v_segmdata = 0;

            v_flags = type;
            v_length = v_rlength = len;
            v_gridtype = GRID_LIN;
            v_plottype = PLOT_LIN;
            v_linestyle = 0;
            v_color = 0;
            v_numdims = (len > 1);
            memset(v_dims, 0, MAXDIMS*sizeof(int));
        }

    ~sDataVec();

    // datavec.cc
    void reset(int, int, sUnits* = 0, void* = 0);
    sDataVec *pad(int, bool*);
    void newtemp(sPlot* = 0);
    void newperm(sPlot* = 0);
    void scalarize();
    void unscalarize();
    void segmentize();
    void unsegmentize();
    sDataVec *copy() const;
    void copyto(sDataVec*, int, int, int) const;
    void alloc(bool, int);
    void resize(int, int = 0);
    char *basename() const;
    void sort();
    sDataVec *mkfamily();
    void extend(int);
    void print(sLstr*) const;
    void minmax(double*, bool) const;
    void SmithMinmax(double*, bool) const;
    sDataVec *SmithCopy() const;

    // math functions

    // cmath1.cc
    sDataVec *v_rms();
    sDataVec *v_sum();
    sDataVec *v_mag();
    sDataVec *v_ph();
    sDataVec *v_j();
    sDataVec *v_real();
    sDataVec *v_imag();
    sDataVec *v_pos();
    sDataVec *v_db();
    sDataVec *v_log10();
    sDataVec *v_log();
    sDataVec *v_ln();
    sDataVec *v_exp();
    sDataVec *v_sqrt();
    sDataVec *v_sin();
    sDataVec *v_cos();
    sDataVec *v_tan();
    sDataVec *v_asin();
    sDataVec *v_acos();
    sDataVec *v_atan();
    sDataVec *v_sinh();
    sDataVec *v_cosh();
    sDataVec *v_tanh();
    sDataVec *v_asinh();
    sDataVec *v_acosh();
    sDataVec *v_atanh();
    sDataVec *v_j0();
    sDataVec *v_j1();
    sDataVec *v_jn();
    sDataVec *v_y0();
    sDataVec *v_y1();
    sDataVec *v_yn();
    sDataVec *v_cbrt();
    sDataVec *v_erf();
    sDataVec *v_erfc();
    sDataVec *v_gamma();
    sDataVec *v_norm();
    sDataVec *v_uminus();
    sDataVec *v_mean();
    sDataVec *v_size();
    sDataVec *v_vector();
    sDataVec *v_unitvec();
    sDataVec *v_rnd();
    sDataVec *v_ogauss();
    sDataVec *v_exponential();
    sDataVec *v_chisq();
    sDataVec *v_erlang();
    sDataVec *v_tdist();
    sDataVec *v_beta();
    sDataVec *v_binomial();
    sDataVec *v_poisson();
    sDataVec *v_not();
    sDataVec *v_sgn();
    sDataVec *v_floor();
    sDataVec *v_ceil();
    sDataVec *v_rint();
    sDataVec *v_interpolate(sDataVec*, bool=false);
    sDataVec *v_interpolate();
    sDataVec *v_deriv();
    sDataVec *v_integ();
    sDataVec *v_fft();
    sDataVec *v_ifft();
    sDataVec *v_undefined();

    // Measurements
    void find_range(double, double, int*, int*);
    sDataVec *v_mmin(sDataVec**);
    sDataVec *v_mmax(sDataVec**);
    sDataVec *v_mpp(sDataVec**);
    sDataVec *v_mavg(sDataVec**);
    sDataVec *v_mrms(sDataVec**);
    sDataVec *v_mpw(sDataVec**);
    sDataVec *v_mrft(sDataVec**);

    // HSPICE inspired
    sDataVec *v_hs_unif(sDataVec**);
    sDataVec *v_hs_aunif(sDataVec**);
    sDataVec *v_hs_gauss(sDataVec**);
    sDataVec *v_hs_agauss(sDataVec**);
    sDataVec *v_hs_limit(sDataVec**);
    sDataVec *v_hs_pow(sDataVec**);
    sDataVec *v_hs_pwr(sDataVec**);
    sDataVec *v_hs_sign(sDataVec**);

    // math ops

    // cmath2.cc
    sDataVec *v_plus(sDataVec*);
    sDataVec *v_minus(sDataVec*);
    sDataVec *v_times(sDataVec*);
    sDataVec *v_mod(sDataVec*);
    sDataVec *v_divide(sDataVec*);
    sDataVec *v_comma(sDataVec*);
    sDataVec *v_power(sDataVec*);
    sDataVec *v_eq(sDataVec*);
    sDataVec *v_gt(sDataVec*);
    sDataVec *v_lt(sDataVec*);
    sDataVec *v_ge(sDataVec*);
    sDataVec *v_le(sDataVec*);
    sDataVec *v_ne(sDataVec*);
    sDataVec *v_and(sDataVec*);
    sDataVec *v_or(sDataVec*);

    bool isreal()               const { return (!(v_flags & VF_COMPLEX)); }
    bool iscomplex()            const { return (v_flags & VF_COMPLEX); }

    bool has_data()             const { return (v_data.real != 0); }

    double realval(int i) const
        {
            return (isreal() ? v_data.real[i] : v_data.comp[i].real);
        }

    void set_realval(int i, double d)
        {
            if (isreal())
                v_data.real[i] = d;
            else
                v_data.comp[i].real = d;
        }

    double *realvec()           const { return (isreal() ? v_data.real : 0); }

    void set_realvec(double *v, bool clean = false)
        {
            v_flags &= ~VF_COMPLEX;
            if (clean && v != v_data.real)
                delete [] v_data.real;
            v_data.real = v;
        }

    double imagval(int i) const
        {
            return (isreal() ? 0.0 : v_data.comp[i].imag);
        }

    void set_imagval(int i, double d)
        {
            if (isreal())
                ;
            else
                v_data.comp[i].imag = d;
        }

    complex compval(int i) const
        {
            if (isreal())
                return (complex(v_data.real[i], 0.0));
            else
                return (v_data.comp[i]);
        }

    void set_compval(int i, complex c)
        {
            if (isreal())
                v_data.real[i] = c.real;
            else
                v_data.comp[i] = c;
        }

    complex *compvec()          const { return (isreal() ? 0 : v_data.comp); }

    void set_compvec(complex *c, bool clean = false)
        {
            v_flags |= VF_COMPLEX;
            if (clean && c != v_data.comp)
                delete [] v_data.comp;
            v_data.comp = c;
        }

    bool no_sxze()              const { return (v_flags & VF_NOSXZE); }
    void set_no_sxze(bool b)
        {
            if (b)
                v_flags |= VF_NOSXZE;
            else
                v_flags &= ~VF_NOSXZE;
        }

    bool scalarized()           const { return (v_scaldata != 0); }
    bool segmentized()          const { return (v_segmdata != 0); }

    // Length within smallest block if multi-dimensional.
    //
    int unscalarized_length() const
        {
            if (v_scaldata) {
                int l = v_scaldata->length;
                if (v_scaldata->numdims > 1)
                    l %= v_scaldata->dims[1];
                return (l);
            }
            int l = v_length;
            if (v_numdims > 1)
                l %= v_dims[1];
            return (l);
        }

    double unscalarized_prev_real() const
        {
            if (v_scaldata) {
                int n = v_scaldata->length - 2;
                return (n > 0 ? realval(n) : v_scaldata->real);
            }
            if (v_length > 1)
                return (realval(v_length - 2));
            if (v_length == 1)
                return (realval(0));
            return (0.0);
            
        }

    double unscalarized_prev_imag() const
        {
            if (v_scaldata) {
                int n = v_scaldata->length - 2;
                return (n > 0 ? imagval(n) : v_scaldata->imag);
            }
            if (v_length > 1)
            return (imagval(v_length - 2));
            if (v_length == 1)
                return (imagval(0));
            return (0.0);
        }

    double unscalarized_first() const
        {
            if (v_scaldata)
                return (v_scaldata->real);
            return (realval(0));
        }

    double absval(int i) const
        {
            return (isreal() ? fabs(v_data.real[i]) :
                sqrt(v_data.comp[i].real*v_data.comp[i].real +
                    v_data.comp[i].imag*v_data.comp[i].imag));
        }

    void realloc(int len)
        {
            if (isreal()) {
                delete [] v_data.real;
                v_data.real = new double[len];
            }
            else {
                delete [] v_data.comp;
                v_data.comp = new complex[len];
            }
            v_rlength = len;
        }

    const char *name()              const { return (v_name); }
    void set_name(const char *n)
        {
            char *s = lstring::copy(n);
            delete [] v_name;
            v_name = s;
        }

    const sUnits *units()           const { return (&v_units); }
    sUnits *ncunits()               { return (&v_units); }

    double minsignal()              const { return (v_minsignal); }
    void set_minsignal(double d)    { v_minsignal = d; }

    double maxsignal()              const { return (v_maxsignal); }
    void set_maxsignal(double d)    { v_maxsignal = d; }

    sPlot *plot()                   const { return (v_plot); }
    void set_plot(sPlot *p)         { v_plot = p; }

    sDataVec *scale() const
        {
            if (v_scale)
                return (v_scale);
            if (v_plot)
                return (v_plot->scale());
            return (0);
        }
    sDataVec *special_scale()       const { return (v_scale); }
    void set_scale(sDataVec *s)     { v_scale = s; }

    sDataVec *next()                const { return (v_next); }
    void set_next(sDataVec *n)      { v_next = n; }

    sDvList *link()                 const { return (v_link2); }
    void set_link(sDvList *l)       { v_link2 = l; }

    const char *defcolor()          const { return (v_defcolor); }
    void set_defcolor(const char *c)
        {
            char *s = lstring::copy(c);
            delete [] v_defcolor;
            v_defcolor = s;
        }

    int flags()                     const { return (v_flags); }
    void set_flags(int f)           { v_flags = f; }

    int length()                    const { return (v_length); }
    void set_length(int l)          { v_length = l; }

    int allocated()                 const { return (v_rlength); }
    void set_allocated(int a)       { v_rlength = a; }

    GridType gridtype()             const { return (v_gridtype); }
    void set_gridtype(GridType g)   { v_gridtype = g; }

    PlotType plottype()             const { return (v_plottype); }
    void set_plottype(PlotType p)   { v_plottype = p; }

    int linestyle()                 const { return (v_linestyle); }
    void set_linestyle(int l)       { v_linestyle = l; }

    int color()                     const { return (v_color); }
    void set_color(int c)           { v_color = c; }

    int numdims()                   const { return (v_numdims); }
    void set_numdims(int n)         { v_numdims = n; }

    int dims(int i)                 const { return (v_dims[i]); }
    void set_dims(int i, int d)     { v_dims[i] = d; }

    // This flag determines whether degrees or radians are used.  The
    // radtodeg and degtorad macros are no-ops if this is false.
    static bool degrees()           { return (v_degrees); }
    static void set_degrees(bool b) { v_degrees = b; }
    static double radtodeg(double c) 
        {
            return (v_degrees ? (c/M_PI)*180 : c);
        }
    static double degtorad(double c)
        {
            return (v_degrees ? (c*M_PI)/180 : c);
        }

    static void set_temporary(bool b) { v_temporary = b; }

private:
    union {
        double *real;
        complex *comp;
    } v_data;               // Vector's data.

    char *v_name;           // Vector's name.
    sUnits v_units;         // Vector's units struct.
    double v_minsignal;     // Minimum value to plot.
    double v_maxsignal;     // Maximum value to plot.

    sPlot *v_plot;          // The plot structure (if it has one).
    sDataVec *v_scale;      // If this has a non-standard scale...
    sDataVec *v_next;       // Link for list of plot vectors.
    sDvList *v_link2;       // Extra link for things like print.
    char *v_defcolor;       // The name of a color to use.
    scalData *v_scaldata;
    segmData *v_segmdata;

    int v_flags;            // Flags (a combination of VF_*).
    int v_length;           // Length of the vector.
    int v_rlength;          // How much space we really have.
    GridType v_gridtype;    // One of GRID_*.
    PlotType v_plottype;    // One of PLOT_*.
    int v_linestyle;        // What line style we are using.
    int v_color;            // What color we are using.
    int v_numdims;          // How many dims -- 0 = scalar (len = 1).
    int v_dims[MAXDIMS];    // The actual size in each dimension.

    static bool v_degrees;
    static bool v_temporary;
};

#endif // FTEDATA_H

