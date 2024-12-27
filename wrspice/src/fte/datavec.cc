
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

#include "simulator.h"
#include "datavec.h"
#include "output.h"
#include "cshell.h"
#include "ginterf/graphics.h"


//
// sDataVec functions.
//

bool sDataVec::v_degrees = false;
bool sDataVec::v_temporary = false;

sDataVec::~sDataVec()
{
    if (v_scaldata)
        unscalarize();
    if (v_segmdata)
        unsegmentize();
    delete [] v_name;
    sDvList::destroy(v_link2);
    if (isreal())
        delete [] v_data.real;
    else
        delete [] v_data.comp;
}


// Destroy and reset the vector data.
//
void
sDataVec::reset(int type, int len, sUnits *u, void *data)
{
    if (isreal())
        delete [] v_data.real;
    else
        delete [] v_data.comp;

    v_data.real = 0;
    if (data)
        v_data.real = (double*)data;
    else if (len) {
        if (type & VF_COMPLEX)
            v_data.comp = new complex[len];
        else
            v_data.real = new double[len];
    }

    if (u)
        v_units = *u;

    v_flags = type;
    v_length = v_rlength = len;
    v_numdims = (len > 1);
    memset(v_dims, 0, MAXDIMS*sizeof(int));
}
     

sDataVec *
sDataVec::pad(int len, bool *copied)
{
    if (v_length < len) {
        *copied = true;
        sDataVec *v = new sDataVec(0, v_flags, len, &v_units);
        int i;
        if (isreal()) {
            double *od = v_data.real;
            for (i = 0; i < v_length; i++)
                v->v_data.real[i] = od[i];
            double d = (i ? od[i-1] : 0.0);
            while (i < len)
                v->v_data.real[i++] = d;
        }
        else {
            complex *oc = v_data.comp;
            for (i = 0; i < v_length; i++)
                v->v_data.comp[i] = oc[i];
            complex c = (i ? oc[i-1] : complex(0, 0));
            while (i < len)
                v->v_data.comp[i++] = c;
        }
        return (v);
    }
    else {
        *copied = false;
        return (this);
    }
}


void
sDataVec::newtemp(sPlot *pl)
{
    if (!pl)
        pl = OP.curPlot();
#ifdef FTEDEBUG
    if (Sp.ft_vecdb)
        GRpkg::self()->ErrPrintf(ET_MSGS, "new temporary vector %s\n", v_name);
#endif
    v_flags &= ~VF_PERMANENT;
    if (v_temporary)
        v_flags |= VF_TEMPORARY;
    if (!v_plot)
        v_plot = pl;
    v_next = v_plot->tempvecs();
    v_plot->set_tempvecs(this);
}


void
sDataVec::newperm(sPlot *pl)
{
    if (!pl)
        pl = OP.curPlot();
    if (!pl)
        return;
    if (!v_name)
        v_name = lstring::copy("unnamed");
#ifdef FTEDEBUG
    if (Sp.ft_vecdb)
        GRpkg::self()->ErrPrintf(ET_MSGS, "new permanent vector %s\n", v_name);
#endif
    v_flags |= VF_PERMANENT;
    v_plot = pl;
    pl->new_perm_vec(this);
    if (pl == OP.curPlot())
        CP.AddKeyword(CT_VECTOR, v_name);
}


void
sDataVec::scalarize()
{
    if (!v_scaldata && (v_length > 1) && !no_sxze()) {
        v_scaldata = new scalData(this);

        v_length = 1;
        v_rlength = 1;
        v_numdims = 1;
        memset(v_dims, 0, MAXDIMS*sizeof(int));
        if (isreal()) {
            set_realval(0, realval(v_scaldata->length - 1));
        }
        else {
            set_compval(0, compval(v_scaldata->length - 1));
        }
    }
}


void
sDataVec::unscalarize()
{
    if (v_scaldata) {

        v_length = v_scaldata->length;
        v_rlength = v_scaldata->rlength;
        v_numdims = v_scaldata->numdims;
        memcpy(v_dims, v_scaldata->dims, MAXDIMS*sizeof(int));
        if (isreal())
            set_realval(0, v_scaldata->real);
        else {
            set_realval(0, v_scaldata->real);
            set_imagval(0, v_scaldata->imag);
        }
        delete v_scaldata;
        v_scaldata = 0;
    }
}


void
sDataVec::segmentize()
{
    if (!v_segmdata && (v_numdims > 1) && !no_sxze())  {

        int per = v_dims[1];
        int l = v_length;
        if (l >= per) {
            v_segmdata = new segmData(this);

            int lx = ((l-1)/per)*per;
            int newlen = l - lx;
            v_length = newlen;
            v_rlength = newlen;
            v_numdims = 1;
            memset(v_dims, 0, MAXDIMS*sizeof(int));
            if (isreal()) {
                set_realvec(realvec() + lx);
            }
            else {
                set_compvec(compvec() + lx);
            }
        }
    }
}


void
sDataVec::unsegmentize()
{
    if (v_segmdata) {

        v_length = v_segmdata->length;
        v_rlength = v_segmdata->rlength;
        v_numdims = v_segmdata->numdims;
        memcpy(v_dims, v_segmdata->dims, MAXDIMS*sizeof(int));
        if (isreal())
            set_realvec(v_segmdata->tdata.real);
        else
            set_compvec(v_segmdata->tdata.comp);
        delete v_segmdata;
        v_segmdata = 0;
    }
}


// Create a copy of a vector.
//
sDataVec *
sDataVec::copy() const
{
    const sDataVec *datavec = this;
    if (!datavec)
        return (0);

    sDataVec *nv = new sDataVec(lstring::copy(v_name), v_flags & VF_COPYMASK,
        0, &v_units);
    nv->v_length = v_length;
    nv->v_minsignal = v_minsignal;
    nv->v_maxsignal = v_maxsignal;
    nv->v_rlength = v_rlength;
    nv->v_gridtype = v_gridtype;
    nv->v_plottype = v_plottype;
    nv->v_defcolor = v_defcolor;
    nv->v_plot = v_plot;
    nv->v_scale = v_scale;
    nv->v_numdims = v_numdims;
    for (int j = 0; j < v_numdims; j++)
        nv->v_dims[j] = v_dims[j];
    if (nv->v_length > nv->v_rlength)
        nv->v_rlength = nv->v_length;
    if (nv->v_rlength && (v_data.real != 0)) {
        nv->alloc(isreal(), nv->v_rlength);
        copyto(nv, 0, 0, nv->v_length);
    }
    else
        nv->v_rlength = 0;
    return (nv);
}

void
sDataVec::copyto(sDataVec *dstv, int srcoff, int dstoff, int size) const
{
    if (isreal()) {
        if (dstv->isreal()) {
            double *src = v_data.real + srcoff;
            double *dst = dstv->v_data.real + dstoff;
            while (size--)
                *dst++ = *src++;
        }
        else {
            double *src = v_data.real + srcoff;
            complex *dst = dstv->v_data.comp + dstoff;
            while (size--) {
                dst->real = *src++;
                dst->imag = 0.0;
                dst++;
            }
        }
    }
    else {
        if (dstv->isreal()) {
            complex *src = v_data.comp + srcoff;
            double *dst = dstv->v_data.real + dstoff;
            while (size--) {
                *dst++ = src->real;
                src++;
            }
        }
        else {
            complex *src = v_data.comp + srcoff;
            complex *dst = dstv->v_data.comp + dstoff;
            while (size--)
                *dst++ = *src++;
        }
    }
}


void
sDataVec::alloc(bool real, int size)
{
    if (size) {
        if (real) {
            v_data.real = new double[size];
            memset((void*)v_data.real, 0, size*sizeof(double));
        }
        else {
            v_data.comp = new complex[size];
            memset((void*)v_data.comp, 0, size*sizeof(complex));
        }
    }
    v_rlength = size;
}


void
sDataVec::resize(int newsize, int extra)
{
    if (newsize < 1)
        newsize = 1;
    if (extra < 0)
        extra = 0;
    int cpsz = SPMIN(v_length, newsize);

    // When increasing the size, grab "extra" elements to avoid the copy
    // that might otherwise occur on subsequent calls.
    if (newsize >= v_length) {
        if (v_rlength >= newsize)
            return;
        newsize += extra;
    }

    if (isreal()) {
        double *oldv = v_data.real;
        v_data.real = new double[newsize];
        memcpy(v_data.real, oldv, cpsz*sizeof(double));
        if (cpsz < newsize)
            memset(v_data.real + cpsz, 0, (newsize - cpsz)*sizeof(double));
        delete [] oldv;
    }
    else {
        complex *oldv = v_data.comp;
        v_data.comp = new complex[newsize];
        memcpy(v_data.comp, oldv, cpsz*sizeof(complex));
        if (cpsz < newsize) {
            memset((void*)(v_data.comp + cpsz), 0,
                (newsize - cpsz)*sizeof(complex));
        }
        delete [] oldv;
    }
    v_length = cpsz;
    v_rlength = newsize;
}


// Return the name of the vector with the plot prefix stripped off. 
// This is no longer trivial since <SEPARATOR> doesn't always mean
// 'plot prefix'.
//
char *
sDataVec::basename() const
{
    if (!v_name)
        return (0);
    char buf[BSIZE_SP];
    if (strchr(v_name, Sp.PlotCatchar())) {
        int i;
        const char *t;
        for (t = v_name, i = 0; *t && *t != Sp.PlotCatchar(); t++)
            buf[i++] = *t;
        buf[i] = '\0';
        if (lstring::cieq(v_plot->type_name(), buf))
            strcpy(buf, t + 1);
        else
            strcpy(buf, v_name);
    }
    else
        strcpy(buf, v_name);
    
    char *t;
    for (t = buf; isspace(*t); t++) ;
    char *s = t;
    for (t = s; *t; t++) ;
    while ((t > s) && isspace(t[-1]))
        *--t = '\0';
    return (lstring::copy(s));
}


namespace {
    // If there are imbedded numeric strings, compare them
    // numerically, not alphabetically.
    //
    int namecmp(const char *s, const char *t)
    {
        for (;;) {
            while ((*s == *t) && !isdigit(*s) && *s)
                s++, t++;
            if (!*s)
                return (0);
            if ((*s != *t) && (!isdigit(*s) || !isdigit(*t)))
                return (*s - *t);
            
            // The beginning of a number... Grab the two numbers 
            // and then compare them...
            //
            int i, j;
            for (i = 0; isdigit(*s); s++)
                i = i * 10 + *s - '0';
            for (j = 0; isdigit(*t); t++)
                j = j * 10 + *t - '0';
            
            if (i != j)
                return (i - j);
        }
    }


    int veccmp(const void *d1p, const void *d2p)
    {
        sDataVec *d1 = *(sDataVec**)d1p;
        sDataVec *d2 = *(sDataVec**)d2p;
        int i;
        if ((i = namecmp(d1->plot()->type_name(),
                d2->plot()->type_name())) != 0)
            return (i);
        return (namecmp(d1->name(), d2->name()));
    }
}


// Sort all the vectors in d, first by plot name and then by vector name.
// Do the right thing with numbers.
//
void
sDataVec::sort()
{
    int i;
    sDvList *dl;
    for (i = 0, dl = v_link2; dl; i++, dl = dl->dl_next) ;
    if (i < 2)
        return;
    sDataVec **array = new sDataVec*[i];
    for (i = 0, dl = v_link2; dl; i++, dl = dl->dl_next)
        array[i] = dl->dl_dvec;
    
    qsort((char*)array, i, sizeof (sDataVec*), veccmp);

    // Now string everything back together...
    for (i = 0, dl = v_link2; dl; i++,dl = dl->dl_next)
        dl->dl_dvec = array[i];
    delete [] array;
}


// This routine takes a multi-dimensional vector and turns it into a family
// of 1-d vectors, linked together with v_link2.  It is here so that plot
// can do intelligent things.
//
sDataVec *
sDataVec::mkfamily()
{
    if (v_numdims < 2)
        return (this);

    int size = v_dims[v_numdims - 1];
    int numvecs = v_length/size;

    char buf[BSIZE_SP];
    int i;
    sDvList *dl, *dl0;
    for (i = 0, dl = dl0 = 0; i < numvecs; i++) {
        if (!dl0)
            dl0 = dl = new sDvList;
        else {
            dl->dl_next = new sDvList;
            dl = dl->dl_next;
        }
        dl->dl_dvec = new sDataVec(v_units);
        dl->dl_dvec->newtemp(v_plot);
    }
    int count[MAXDIMS];
    for (i = 0; i < MAXDIMS; i++)
        count[i] = 0;
    int j;
    for (dl = dl0, j = 0; j < numvecs; j++, dl = dl->dl_next) {
        sDataVec *d = dl->dl_dvec;
        strcpy(buf, v_name);
        for (i = 0; i < v_numdims - 1; i++) {
            int len = strlen(buf);
            snprintf(buf + len, sizeof(buf) - len, "[%d]", count[i]);
        }
        d->v_name = lstring::copy(buf);
        d->v_flags = v_flags;
        d->v_minsignal = v_minsignal;
        d->v_maxsignal = v_maxsignal;
        d->v_gridtype = v_gridtype;
        d->v_plottype = v_plottype;
        d->v_scale = v_scale;
        // Don't copy the default color, since there will be many
        // of these things...
        //
        d->v_numdims = 1;
        d->v_length = size;
        d->alloc(d->isreal(), size);
        copyto(d, size*j, 0, size);

        for (i = v_numdims - 2; i >= 0; i--) {
            if (count[i]++ < v_dims[i])
                break;
            else
                count[i] = 0;
        }
        if (i < 0)
            break;
    }
    if (dl0) {
        sDataVec *vecs = new sDataVec(lstring::copy("list"), 0, 0);
        vecs->v_link2 = dl0;
        vecs->newtemp(v_plot);
        return (vecs);
    }
    return (0);
}


// Extend a data vector to length by replicating the
// last element, or truncate it if it is too long.
//
void
sDataVec::extend(int len)
{
    if (len < 1)
        len = 1;
    if (v_length >= len) {
        v_length = len;
        return;
    }
    if (isreal()) {
        double *oldv = v_data.real;
        v_data.real = new double[len];
        memcpy(v_data.real, oldv, v_length*sizeof(double));

        int i = v_length;
        double d = i > 0 ? oldv[i-1] : 0.0;
        while (i < len)
            v_data.real[i++] = d;
        delete [] oldv;
    }
    else {
        complex *oldv = v_data.comp;
        v_data.comp = new complex[len];
        memcpy(v_data.comp, oldv, v_length*sizeof(complex));

        int i = v_length;
        complex c = i > 0 ? oldv[i-1] : complex(0, 0);
        while (i < len)
            v_data.comp[i++] = c;
        delete [] oldv;
    }
    v_length = len;
}


void
sDataVec::print(sLstr *plstr) const
{
    char buf[BSIZE_SP];
    char ubuf[64];
    char *tt = v_units.unitstr();
    strcpy(ubuf, tt);
    delete [] tt;
    tt = ubuf;
    snprintf(buf, sizeof(buf), "%c %-20s %s[%d]", (v_flags & VF_SELECTED) ?
        '>' : ' ', v_name, isreal() ? "real" : "cplx", v_length);
    if (*tt) {
        int len = strlen(buf);
        snprintf(buf + len, sizeof(buf) - len, ", %s", tt);
    }
    /* old format
    snprintf(buf, "%c   %-20s : %s %s, %d long", (v_flags & VF_SELECTED) ?
        '>' : ' ', v_name, tt, isreal() ? "real" : "complex", v_length);
    */
    if (v_flags & VF_MINGIVEN) {
        int len = strlen(buf);
        snprintf(buf + len, sizeof(buf) - len, ", min=%g", v_minsignal);
    }
    if (v_flags & VF_MAXGIVEN) {
        int len = strlen(buf);
        snprintf(buf + len, sizeof(buf) - len, ", max=%g", v_maxsignal);
    }

    const char *gridname = 0;
    switch (v_gridtype) {
    case GRID_LIN:
        break;
    case GRID_LOGLOG:
        gridname = "loglog";
        break;
    case GRID_XLOG:
        gridname = "xlog";
        break;
    case GRID_YLOG:
        gridname = "ylog";
        break;
    case GRID_POLAR:
        gridname = "polar";
        break;
    case GRID_SMITH:
        gridname = "smith";
        break;
    case GRID_SMITHGRID:
        gridname = "smithgrid";
        break;
    }

    if (gridname) {
        int len = strlen(buf);
        snprintf(buf + len, sizeof(buf) - len, ", grid=%s", gridname);
    }

    const char *plottypename = 0;
    switch (v_plottype) {
    case PLOT_LIN:
        break;
    case PLOT_COMB:
        plottypename = "comb";
        break;
    case PLOT_POINT:
        plottypename = "point";
        break;
    }

    if (plottypename) {
        int len = strlen(buf);
        snprintf(buf + len, sizeof(buf) - len, ", plot=%s", plottypename);
    }

    if (v_defcolor) {
        int len = strlen(buf);
        snprintf(buf + len, sizeof(buf) - len, ", color=%s", v_defcolor);
    }
    if (v_scale) {
        int len = strlen(buf);
        if (v_plot && v_scale->v_plot && v_plot != v_scale->v_plot) {
            snprintf(buf + len, sizeof(buf) - len, ", scale=%s.%s",
                v_scale->v_plot->type_name(), v_scale->v_name);
        }
        else {
            snprintf(buf + len, sizeof(buf) - len, ", scale=%s",
                v_scale->v_name);
        }
    }
    if (v_numdims > 1) {
        int len = strlen(buf);
        snprintf(buf + len, sizeof(buf) - len, ", dims=[");

        for (int i = 0; i < v_numdims; i++) {
            len = strlen(buf);
            snprintf(buf + len, sizeof(buf) - len, "%d%s", v_dims[i],
                (i < v_numdims - 1) ? "," : "]");
        }
    }
    int len = strlen(buf);
    snprintf(buf + len, sizeof(buf) - len, "%s",
        (v_plot && v_plot->scale() == this) ? " [default scale]\n" : "\n");

    if (plstr)
        plstr->add(buf);
    else
        TTY.send(buf);
}


// Returns the minimum and maximum values of a dvec.  If real is true
// look at the real parts, otherwise the imag parts.
// res[0] = min, res[1] = max
//
void
sDataVec::minmax(double *res, bool real) const
{
    double d = real ? realval(0) : imagval(0);
    res[0] = d;
    res[1] = d;
    for (int i = 1; i < v_length; i++) {
        d = real ? realval(i) : imagval(i);
        if (d < res[0]) res[0] = d;
        if (d > res[1]) res[1] = d;
    }
}


namespace {
    inline void SMITHtfm(double re, double im, double *x, double *y)
    {
        double dnom = (re + 1)*(re + 1) + im*im;
        *x = (re*re + im*im - 1)/dnom;
        *y = 2*im/dnom;
    }
}


// Will report the minimum and maximum in "reflection coefficient" space
//
void
sDataVec::SmithMinmax(double *res, bool yval) const
{
    double d, d2;
    SMITHtfm(realval(0), imagval(0), &d, &d2);
    // we are looking for min/max X or Y ralue
    if (yval)
        d = d2;
    res[0] = d;
    res[1] = d;
    for (int i = 1; i < v_length; i++) {
        SMITHtfm(realval(i), imagval(i), &d, &d2);
        // we are looking for min/max X or Y ralue
        if (yval)
            d = d2;
        if (d < res[0]) res[0] = d;
        if (d > res[1]) res[1] = d;
    }
}


sDataVec *
sDataVec::SmithCopy() const
{
    // Transform for smith plots
    sDataVec *d = copy();
    d->newtemp();

    if (d->v_flags & VF_SMITH)
        return (d);
    d->v_flags |= VF_SMITH;

    if (isreal()) {
        for (int j = 0; j < v_length; j++) {
            double re = v_data.real[j];
            d->v_data.real[j] = (re - 1) / (re + 1);
        }
    }
    else {
        for (int j = 0; j < d->v_length; j++) {
            // (re - 1, im) / (re + 1, im)

            double re = v_data.comp[j].real;
            double im = v_data.comp[j].imag;

            double dnom = (re+1.0)*(re+1.0) + im*im;
            double x = (re*re + im*im - 1.0)/dnom;
            double y = 2.0*im/dnom;

            d->v_data.comp[j].real = x;
            d->v_data.comp[j].imag = y;
        }
    }
    return (d);
}


void
sDataVec::add_point(double *val1, double *val2, int size_incr)
{
    if (length() >= allocated())
        resize(length() + size_incr);
    if (isreal())
        set_realval(length(), *val1);
    else {
        set_realval(length(), *val1);
        set_imagval(length(), *val2);
    }
    set_length(length() + 1);
}


// s is a string of the form d1,d2,d3...
//
void
sDataVec::fixdims(const char *s)
{
    int dims[MAXDIMS];
    memset(dims, 0, MAXDIMS*sizeof(int));
    int ndims = 0;
    if (atodims(s, dims, &ndims)) {
        // Something's wrong
        GRpkg::self()->ErrPrintf(ET_WARN,
            "syntax error in dimensions, ignored.\n");
        return;
    }
    for (int i = 0; i < MAXDIMS; i++)
        set_dims(i, dims[i]);
    set_numdims(ndims);
}


// Static function.
// Read a string of one of the following forms into a dimensions array:
//  [12][1][10]
//  [12,1,10]
//  12,1,10
//  12, 1, 10
//  12 , 1 , 10
// Basically, we require that all brackets be matched, that all
// numbers be separated by commas or by "][", that all whitespace
// is ignored, and the beginning [ and end ] are ignored if they
// exist.  The only valid characters in the string are brackets,
// commas, spaces, and digits.  If any dimension is blank, its
// entry in the array is set to 0.
//  
//  Return 0 on success, 1 on failure.
//
int
sDataVec::atodims(const char *p, int *data, int *outlength)
{
    if (!data || !outlength)
        return (1);

    if (!p) {
        *outlength = 0;
        return (0);
    }

    while (*p && isspace(*p))
        p++;

    int needbracket = 0;
    if (*p == '[') {
        p++;
        while (*p && isspace(*p))
            p++;
        needbracket = 1;
    }

    int length = 0;
    int state = 0;
    int err = 0;
    char sep = '\0';

    while (*p && state != 3) {
        switch (state) {
        case 0: // p just at or before a number
            if (length >= MAXDIMS) {
                if (length == MAXDIMS)
                    GRpkg::self()->ErrPrintf(ET_ERROR,
                        "maximum of %d dimensions allowed.\n", MAXDIMS);
                length += 1;
            }
            else if (!isdigit(*p))
                data[length++] = 0;    // This position was empty
            else {
                data[length++] = atoi(p);
                while (isdigit(*p))
                    p++;
            }
            state = 1;
            break;

        case 1: // p after a number, looking for ',' or ']'
            if (sep == '\0')
                sep = *p;
            if (*p == ']' && *p == sep) {
                p++;
                state = 2;
            }
            else if (*p == ',' && *p == sep) {
                p++;
                state = 0;
            }
            else  // Funny character following a #
                break;
            break;

        case 2: // p after a ']', either at the end or looking for '['
            if (*p == '[') {
                p++;
                state = 0;
            }
            else
                state = 3;
            break;
        }

        while (*p && isspace(*p))
            p++;
    }

    *outlength = length;
    if (length > MAXDIMS)
        return (1);
    if (state == 3)   // We finished with a closing bracket
        err = !needbracket;
    else if (*p)      // We finished by hitting a bad character after a #
        err = 1;
    else              // We finished by exhausting the string
        err = needbracket;
    if (err)
        *outlength = 0;
    return (err);
}
