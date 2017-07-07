
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
 $Id: datavec.cc,v 2.5 2015/12/15 22:38:52 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Wayne A. Christopher
         1992 Stephen R. Whiteley
****************************************************************************/

#include "frontend.h"
#include "ftedata.h"
#include "cshell.h"


//
// sDataVec functions.
//

bool sDataVec::v_temporary = false;

sDataVec::~sDataVec()
{
    delete [] v_name;
    sDvList::destroy(v_link2);
    if (isreal())
        delete [] v_data.real;
    else
        delete [] v_data.comp;
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
        pl = Sp.CurPlot();
#ifdef FTEDEBUG
    if (Sp.ft_vecdb)
        GRpkgIf()->ErrPrintf(ET_MSGS, "new temporary vector %s\n", v_name);
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
        pl = Sp.CurPlot();
    if (!pl)
        return;
    if (!v_name)
        v_name = lstring::copy("unnamed");
#ifdef FTEDEBUG
    if (Sp.ft_vecdb)
        GRpkgIf()->ErrPrintf(ET_MSGS, "new permanent vector %s\n", v_name);
#endif
    v_flags |= VF_PERMANENT;
    v_plot = pl;
    pl->new_perm_vec(this);
    if (pl == Sp.CurPlot())
        CP.AddKeyword(CT_VECTOR, v_name);
}


// Create a copy of a vector.
//
sDataVec *
sDataVec::copy()
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
sDataVec::copyto(sDataVec *dstv, int srcoff, int dstoff, int size)
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
            memset(v_data.real, 0, size*sizeof(double));
        }
        else {
            v_data.comp = new complex[size];
            memset(v_data.comp, 0, size*sizeof(complex));
        }
    }
    v_rlength = size;
}


void
sDataVec::resize(int newsize)
{
    if (newsize < 1)
        newsize = 1;
    int sz = SPMIN(v_length, newsize);
    if (isreal()) {
        double *oldv = v_data.real;
        v_data.real = new double[newsize];
        memcpy(v_data.real, oldv, sz*sizeof(double));
        if (sz < newsize)
            memset(v_data.real + sz, 0, (newsize - sz)*sizeof(double));
        delete [] oldv;
    }
    else {
        complex *oldv = v_data.comp;
        v_data.comp = new complex[newsize];
        memcpy(v_data.comp, oldv, sz*sizeof(complex));
        if (sz < newsize)
            memset(v_data.comp + sz, 0, (newsize - sz)*sizeof(complex));
        delete [] oldv;
    }
    v_rlength = newsize;
}


// Return the name of the vector with the plot prefix stripped off. 
// This is no longer trivial since <SEPARATOR> doesn't always mean
// 'plot prefix'.
//
char *
sDataVec::basename()
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
    const sDataVec *datavec = this;
    if (!datavec)
        return (0);
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
        for (i = 0; i < v_numdims - 1; i++)
            sprintf(buf + strlen(buf), "[%d]", count[i]);
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
sDataVec::print(char **retstr)
{
    char buf[BSIZE_SP], buf2[BSIZE_SP];
    char ubuf[64];
    char *tt = v_units.unitstr();
    strcpy(ubuf, tt);
    delete [] tt;
    tt = ubuf;
    sprintf(buf, "%c %-20s %s[%d]", (v_flags & VF_SELECTED) ?
        '>' : ' ', v_name, isreal() ? "real" : "cplx", v_length);
    if (*tt) {
        sprintf(buf2,  ", %s", tt);
        strcat(buf, buf2);
    }
    /* old format
    sprintf(buf, "%c   %-20s : %s %s, %d long", (v_flags & VF_SELECTED) ?
        '>' : ' ', v_name, tt, isreal() ? "real" : "complex", v_length);
    */
    if (v_flags & VF_MINGIVEN) {
        sprintf(buf2, ", min=%g", v_minsignal);
        strcat(buf, buf2);
    }
    if (v_flags & VF_MAXGIVEN) {
        sprintf(buf2, ", max=%g", v_maxsignal);
        strcat(buf, buf2);
    }

    switch (v_gridtype) {
    case GRID_LIN:
        break;

    case GRID_LOGLOG:
        strcat(buf, ", grid=loglog");
        break;

    case GRID_XLOG:
        strcat(buf, ", grid=xlog");
        break;

    case GRID_YLOG:
        strcat(buf, ", grid=ylog");
        break;

    case GRID_POLAR:
        strcat(buf, ", grid=polar");
        break;

    case GRID_SMITH:
        strcat(buf, ", grid=smith");
        break;

    case GRID_SMITHGRID:
        strcat(buf, ", grid=smithgrid");
        break;
    }

    switch (v_plottype) {
    case PLOT_LIN:
        break;

    case PLOT_COMB:
            strcat(buf, ", plot=comb");
        break;

    case PLOT_POINT:
        strcat(buf, ", plot=point");
        break;

    }
    if (v_defcolor) {
        sprintf(buf2, ", color=%s", v_defcolor);
        strcat(buf, buf2);
    }
    if (v_scale) {
        if (v_plot && v_scale->v_plot && v_plot != v_scale->v_plot)
            sprintf(buf2, ", scale=%s.%s", v_scale->v_plot->type_name(),
                v_scale->v_name);
        else
            sprintf(buf2, ", scale=%s", v_scale->v_name);
        strcat(buf, buf2);
    }
    if (v_numdims > 1) {
        sprintf(buf2, ", dims=[");
        strcat(buf, buf2);
        int i;
        for (i = 0; i < v_numdims; i++) {
            sprintf(buf2, "%d%s", v_dims[i],
                (i < v_numdims - 1) ? "," : "]");
            strcat(buf, buf2);
        }
    }
    if (v_plot && v_plot->scale() == this)
        strcat(buf, " [default scale]\n");
    else
        strcat(buf, "\n");
    if (retstr)
        *retstr = lstring::build_str(*retstr, buf);
    else
        TTY.send(buf);
}


// Returns the minimum and maximum values of a dvec.  If real is true
// look at the real parts, otherwise the imag parts.
// res[0] = min, res[1] = max
//
void
sDataVec::minmax(double *res, bool real)
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
sDataVec::SmithMinmax(double *res, bool yval)
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
sDataVec::SmithCopy()
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

