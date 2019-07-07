
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
Authors: 1986 Wayne A. Christopher
         1993 Stephen R. Whiteley
****************************************************************************/

#include "config.h"
#include "simulator.h"
#include "datavec.h"
#include "output.h"
#include "cshell.h"
#include "commands.h"
#include "ttyio.h"
#include "toolbar.h"
#include "inptran.h"
#include "spnumber/spnumber.h"
#include "miscutil/random.h"


//
// The 'compose' command.  This is a more powerful and convenient form of the
// 'let' command.
//

struct sCompose
{
    sCompose() { memset(this, 0, sizeof(sCompose)); }
    bool cmp_parse(wordlist*);
    bool cmp_pattern(wordlist*, int*, double**);
    bool cmp_linsweep(int*, double**);
    bool cmp_logsweep(int*, double**);
    bool cmp_random(int*, double**);
    bool cmp_gauss(int*, double**);

    double start;
    double stop;
    double step;
    double center;
    double span;
    double mean;
    double sd;
    int lin;
    int log;
    int dec;
    int gauss;
    int randm;
    bool startgiven;
    bool stopgiven;
    bool stepgiven;
    bool sssgiven;
    bool lingiven;
    bool centergiven;
    bool spangiven;
    bool meangiven;
    bool sdgiven;
    bool loggiven;
    bool decgiven;
    bool gaussgiven;
    bool randmgiven;
};

namespace {
    void dimxpand(sDataVec*, int*, double*);
    bool cmp_values(wordlist*, int*, double**, complex**, bool*);

    // If the form is plotname.vecname, return the plot and strip the
    // prefix.
    //
    char *stripplot(const char *vname, sPlot **pl)
    {
        if (pl)
            *pl = 0;
        const char *s = strchr(vname, Sp.PlotCatchar());
        if (s && s != vname) {
            int ix = s - vname;
            char *bf = new char[ix + 1];
            strncpy(bf, vname, ix);
            bf[ix] = 0;
            sPlot *p = OP.findPlot(bf);
            delete [] bf;
            if (p) {
                if (pl)
                    *pl = p;
                return (lstring::copy(++s));
            }
        }
        return (0);
    }
}


// The general syntax is 'compose name parm = val ...'
// The possible parms are:
//  start       The value at which the vector should start.
//  stop        The value at which the vector should end.
//  step        The difference between sucessive elements.
//  lin         The number of points, linearly spaced.
//  log         The number of points, logarithmically spaced.
//  dec         The number of points per decade, logarithmically spaced.
//  center      Where to center the range of points.
//  span        The size of the range of points.
//  gauss       The number of points in the gaussian distribution.
//  mean        The mean value for the gass. dist.
//  sd          The standard deviation for the gauss. dist.
//  random      The number of randomly selected points.
//
// The case 'compose name values val val ...' takes the values and creates a
// new vector -- the vals may be arbitrary expressions.
//
// NOTE: most of this doesn't work -- there will be plenty of unused variable
// lint messages...
//
void
CommandTab::com_compose(wordlist *wl)
{
    sCompose sc;
    char *resname = wl->wl_word;
    wl = wl->wl_next;
    int length = 0;
    double *data;
    complex *cdata = 0;
    bool realflag = true;
    if (lstring::eq(wl->wl_word, "values")) {
        if (cmp_values(wl->wl_next, &length, &data, &cdata, &realflag))
            return;
    }
    else if (lstring::eq(wl->wl_word, "pattern")) {
        if (sc.cmp_pattern(wl->wl_next, &length, &data))
            return;
    }
    else {
        if (sc.cmp_parse(wl))
            return;

        // Now see what we have... start and stop are pretty much
        // compatible with everything...
        //
        if (sc.stepgiven && (sc.step == 0.0)) {
            GRpkgIf()->ErrPrintf(ET_ERROR, "step cannot be 0.\n");
            return;
        }
        if (sc.lingiven + sc.loggiven + sc.decgiven +
            sc.randmgiven + sc.gaussgiven > 1) {
            GRpkgIf()->ErrPrintf(ET_ERROR,
                "can have at most one of (lin, log, dec, random, gauss).\n");
            return;
        }
        else if (sc.lingiven + sc.loggiven + sc.decgiven + sc.randmgiven +
                sc.gaussgiven == 0) {
            // Hmm, if we have a start, stop, and step we're ok.
            if (sc.startgiven && sc.stopgiven && sc.stepgiven)
                sc.sssgiven = true;
            else {
                GRpkgIf()->ErrPrintf(ET_ERROR,
                "either one of (lin, log, dec, random, gauss) must be given,\n"
                "or all of (start, stop, and step) must be given.\n");
                return;
            }
        }

        if (sc.lingiven || sc.sssgiven) {
            if (sc.cmp_linsweep(&length, &data))
                return;
        }
        else if (sc.loggiven || sc.decgiven) {
            if (sc.cmp_logsweep(&length, &data))
                return;
        }
        else if (sc.randmgiven) {
            if (sc.cmp_random(&length, &data))
                return;
        }
        else if (sc.gaussgiven) {
            if (sc.cmp_gauss(&length, &data))
                return;
        }
    }
    resname = lstring::copy(resname);
    CP.Unquote(resname);

    sPlot *pl;
    char *vname = stripplot(resname, &pl);
    if (vname) {
        delete [] resname;
        resname = vname;
    }

    sDataVec *n;
    if (pl)
        n = pl->find_vec(resname);
    else
        n = OP.vecGet(resname, 0);
    if (n && (n->flags() & VF_READONLY)) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "specified vector %s is read-only.\n",
            resname);
        delete [] resname;
        return;
    }
    if (n) {
        // Vector already exists.  It the plot is active, deleting and
        // rebuilding may not be safe, so keep the existing vector but
        // replace the data.

        delete [] resname;
        n->reset(realflag ? 0 : VF_COMPLEX, length, 0,
            realflag ? (void*)data : (void*)cdata);
    }
    else {
        sDataVec *result = new sDataVec(resname, realflag ? 0 : VF_COMPLEX,
            length, 0, realflag ? (void*)data : (void*)cdata);
        result->newperm(pl);
    }
    ToolBar()->UpdateVectors(0);
}


bool
sCompose::cmp_parse(wordlist *wl)
{
    // Parse the line...
    const char *msg1 = "bad syntax.\n";
    const char *msg2 = "bad parameter %s = %s.\n";
    while (wl) {
        char *s, *var;
        const char *val;
        if ((s = strchr(wl->wl_word, '=')) && s[1]) {
            // This is var=val
            *s = '\0';
            var = wl->wl_word;
            val = s + 1;
            wl = wl->wl_next;
        }
        else if (strchr(wl->wl_word, '=')) {
            // This is var= val
            *s = '\0';
            var = wl->wl_word;
            wl = wl->wl_next;
            if (wl) {
                val = wl->wl_word;
                wl = wl->wl_next;
            }
            else {
                GRpkgIf()->ErrPrintf(ET_ERROR, msg1);
                return (true);
            }
        }
        else {
            // This is var =val or var = val
            var = wl->wl_word;
            wl = wl->wl_next;
            if (wl) {
                val = wl->wl_word;
                if (*val != '=') {
                    GRpkgIf()->ErrPrintf(ET_ERROR, msg1);
                    return (true);
                }
                val++;
                if (!*val) {
                    wl = wl->wl_next;
                    if (wl) {
                        val = wl->wl_word;
                    }
                    else {
                        GRpkgIf()->ErrPrintf(ET_ERROR, msg1);
                        return (true);
                    }
                }
                wl = wl->wl_next;
            }
            else {
                GRpkgIf()->ErrPrintf(ET_ERROR, msg1);
                return (true);
            }
        }

        double *td;
        if (lstring::cieq(var, "start")) {
            startgiven = true;
            if (!(td = SPnum.parse(&val, false))) {
                GRpkgIf()->ErrPrintf(ET_ERROR, msg2, var, val);
                return (true);
            }
            start = *td;
        }
        else if (lstring::cieq(var, "stop")) {
            stopgiven = true;
            if (!(td = SPnum.parse(&val, false))) {
                GRpkgIf()->ErrPrintf(ET_ERROR, msg2, var, val);
                return (true);
            }
            stop = *td;
        }
        else if (lstring::cieq(var, "step")) {
            stepgiven = true;
            if (!(td = SPnum.parse(&val, false))) {
                GRpkgIf()->ErrPrintf(ET_ERROR, msg2, var, val);
                return (true);
            }
            step = *td;
        }
        else if (lstring::cieq(var, "center")) {
            centergiven = true;
            if (!(td = SPnum.parse(&val, false))) {
                GRpkgIf()->ErrPrintf(ET_ERROR, msg2, var, val);
                return (true);
            }
            center = *td;
        }
        else if (lstring::cieq(var, "span")) {
            spangiven = true;
            if (!(td = SPnum.parse(&val, false))) {
                GRpkgIf()->ErrPrintf(ET_ERROR, msg2, var, val);
                return (true);
            }
            span = *td;
        }
        else if (lstring::cieq(var, "mean")) {
            meangiven = true;
            if (!(td = SPnum.parse(&val, false))) {
                GRpkgIf()->ErrPrintf(ET_ERROR, msg2, var, val);
                return (true);
            }
            mean = *td;
        }
        else if (lstring::cieq(var, "sd")) {
            sdgiven = true;
            if (!(td = SPnum.parse(&val, false))) {
                GRpkgIf()->ErrPrintf(ET_ERROR, msg2, var, val);
                return (true);
            }
            sd = *td;
        }
        else if (lstring::cieq(var, "lin") || lstring::cieq(var, "len") ||
                lstring::cieq(var, "length")) {
            lingiven = true;
            if (!(td = SPnum.parse(&val, false))) {
                GRpkgIf()->ErrPrintf(ET_ERROR, msg2, var, val);
                return (true);
            }
            lin = (int)*td;
            if (lin < 1) {
                GRpkgIf()->ErrPrintf(ET_ERROR, msg2, var, val);
                return (true);
            }
        }
        else if (lstring::cieq(var, "log")) {
            loggiven = true;
            if (!(td = SPnum.parse(&val, false))) {
                GRpkgIf()->ErrPrintf(ET_ERROR, msg2, var, val);
                return (true);
            }
            log = (int)*td;
            if (log < 1) {
                GRpkgIf()->ErrPrintf(ET_ERROR, msg2, var, val);
                return (true);
            }
        }
        else if (lstring::cieq(var, "dec")) {
            decgiven = true;
            if (!(td = SPnum.parse(&val, false))) {
                GRpkgIf()->ErrPrintf(ET_ERROR, msg2, var, val);
                return (true);
            }
            dec = (int)*td;
            if (dec < 1) {
                GRpkgIf()->ErrPrintf(ET_ERROR, msg2, var, val);
                return (true);
            }
        }
        else if (lstring::cieq(var, "gauss")) {
            gaussgiven = true;
            if (!(td = SPnum.parse(&val, false))) {
                GRpkgIf()->ErrPrintf(ET_ERROR, msg2, var, val);
                return (true);
            }
            gauss = (int)*td;
            if (gauss < 1) {
                GRpkgIf()->ErrPrintf(ET_ERROR, msg2, var, val);
                return (true);
            }
        }
        else if (lstring::cieq(var, "random")) {
            randmgiven = true;
            if (!(td = SPnum.parse(&val, false))) {
                GRpkgIf()->ErrPrintf(ET_ERROR, msg2, var, val);
                return (true);
            }
            randm = (int)*td;
            if (randm < 1) {
                GRpkgIf()->ErrPrintf(ET_ERROR, msg2, var, val);
                return (true);
            }
        }
        else {
            GRpkgIf()->ErrPrintf(ET_ERROR,
                "unknown keyword \"%s\".\n", var);
            return (true);
        }
    }
    return (false);
}


bool
sCompose::cmp_pattern(wordlist *wl, int *length, double **datap)
{
    *length = 0;
    *datap = 0;
    double vals[2];
    int fd = 0;
    while (wl) {
        if (fd < 2) {
            const char *t = wl->wl_word;
            double *td = SPnum.parse(&t, false);
            if (td) {
                vals[fd] = *td;
                fd++;
                wl = wl->wl_next;
                continue;
            }
        }
        break;
    }

    int nmax;
    if (fd == 1)
        nmax = rint(vals[0]);
    else if (fd == 2)
        nmax = rint(vals[1]/vals[0]);
    else
        nmax = 0;

    if (*wl->wl_word == 'b' || *wl->wl_word == 'B') {
        // A pattern description, syntax as in pulse/gpulse.
        char *pspec = wordlist::flatten(wl);
        const char *ps = pspec;
        char *errs;
        pbitList *list = pbitList::parse(&ps, &errs);
        delete [] pspec;
        if (!list && errs) {
            GRpkgIf()->ErrPrintf(ET_ERROR, "%s.\n", errs);
            delete [] errs;
            return (true);
        }
        pbitAry pa;
        pa.add(list);
        pbitList::destroy(list);
        if (pa.count() == 0) {
            GRpkgIf()->ErrPrintf(ET_ERROR, "pattern has zero length.\n");
            return (true);
        }

        unsigned long *ary = pa.final();

        if (!nmax || (nmax > pa.count() && pa.rep_start() == 0))
            nmax = pa.count();
        double *data = new double[nmax];
        int j = 0;
        for (int i = 0; i < nmax; i++, j++) {
            if (j == pa.count())
                j = pa.rep_start();
            data[i] = (ary[j/sizeof(unsigned long)] &
                (1 << j%sizeof(unsigned long))) ? 1.0 : 0.0;
        }
        *length = nmax;
        *datap = data;
    }
    else {
        GRpkgIf()->ErrPrintf(ET_ERROR, "expecting bstring, not found.\n");
        return (true);
    }
    return (false);
}


// Create a linear sweep...
//
bool
sCompose::cmp_linsweep(int *length, double **datap)
{
    if (stepgiven && startgiven && stopgiven) {
        if (lin) {
            double tstep = (stop - start)/lin;
            if (step != tstep) {
                GRpkgIf()->ErrPrintf(ET_WARN, "bad step -- should be %g.\n",
                    tstep);
                stepgiven = false;
            }
        }
        else {
            if ((int)((stop - start)/step) < 1) {
                GRpkgIf()->ErrPrintf(ET_ERROR, "bad step value.\n");
                return (true);
            }
        }
    } 
    if (!startgiven) {
        if (stopgiven && stepgiven)
            start = stop - step*lin;
        else if (stopgiven)
            start = stop - lin;
        else
            start = 0;
        startgiven = true;
    }
    if (!stopgiven) {
        if (stepgiven)
            stop = start + lin*step;
        else
            stop = start + lin;
        stopgiven = true;
    } 
    if (!stepgiven)
        step = (stop - start)/lin;
    if (lin) {
        double *data = new double[(int)lin];
        int i;
        double tt;
        for (i = 0, tt = start; i < lin; i++, tt += step)
            data[i] = tt;
        *length = (int)lin;
        *datap = data;
    }
    else {
        int len = 1 + abs((int)(1.000000001*(stop - start)/step));
        double *data = new double[(int)len];
        int i;
        double tt;
        if (start < stop) {
            for (i = 0, tt = start; i < len; i++, tt += step)
                data[i] = tt;
        }
        else {
            for (i = 0, tt = start; i < len; i++, tt += step)
                data[i] = tt;
        }
        *length = len;
        *datap = data;
    }
    return (false);
}


// Create a log sweep...
//
bool
sCompose::cmp_logsweep(int *length, double **datap)
{
    if (!startgiven) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "start value is required.\n");
        return (true);
    }
    if (start <= 0) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "start value must be positive.\n");
        return (true);
    }
    if (!stopgiven) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "stop value is required.\n");
        return (true);
    }
    if (stop <= 0) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "stop value must be positive.\n");
        return (true);
    }

    double lstart = log10(start);
    double lstop = log10(stop);

    if (decgiven) {
        if (lstart > lstop) {
            lstart = ceil(lstart);
            lstop = floor(lstop);
        }
        else {
            lstart = floor(lstart);
            lstop = ceil(lstop);
        }
        if (lstop == lstart)
            lstop = lstart + 1;
        int npts = (int)fabs(1.00000001*(lstop - lstart));
        npts *= dec;
        double *data = new double[npts + 1];
        int j = 0;
        double dl = 1.0/dec;
        for (double tt = lstart; ; tt += 1.0) {
            for (int i = 0; i < dec; i++)
                data[j++] = pow(10.0, tt + i*dl);
            if (j == npts) {
                tt += 1.0;
                data[j] = pow(10.0, tt);
                break;
            }
        }
        *length = npts + 1;
        *datap = data;
    }
    else {
        if (log == 1) {
            *length = 1;
            *datap = new double[1];
            **datap = start;
        }
        else {
            double lstep = (lstop - lstart)/(log - 1);
            double *data = new double[log];
            int i;
            double tt;
            for (i = 0, tt = lstart; i < log; i++, tt += lstep)
                data[i] = pow(10.0, tt);
            *length = log;
            *datap = data;
        }
    }
    return (false);
}


// Create a set of random values...
//
bool
sCompose::cmp_random(int *length, double **datap)
{
    if (!centergiven) {
        center = 0.0;
        centergiven = true;
    }
    if (!spangiven) {
        span = 2.0;
        spangiven = true;
    }
    double *data = new double[randm];
    for (int i = 0; i < randm; i++)
        data[i] = (Rnd.random() - 0.5)*span + center;
    *length = randm;
    *datap = data;
    return (false);
}


// Create a gaussian distribution...
//
bool
sCompose::cmp_gauss(int *length, double **datap)
{
    if (!meangiven) {
        mean = 0;
        meangiven = true;
    }
    if (!sdgiven) {
        sd = 1.0;
        sdgiven = true;
    }

    double *data = new double[gauss];
    for (int i = 0; i < gauss; i++)
        data[i] = Rnd.gauss()*sd + mean;
    *length = gauss;
    *datap = data;
    return (false);
}


namespace {
    // Copy the data from a vector into a buffer with larger dimensions.
    //
    void dimxpand(sDataVec *v, int *newdims, double *data)
    {
        int dims = v->numdims();
        if (dims == 0)
            dims = 1;

        int i;
        int ncount[MAXDIMS], ocount[MAXDIMS];
        for (i = 0; i < MAXDIMS; i++)
            ncount[i] = ocount[i] = 0;
        
        complex *cdata = (complex*)data;
        bool realflag = v->isreal();
        for (;;) {
            int o, n;
            for (o = n = i = 0; i < dims; i++) {
                int j, t, u;
                for (j = i, t = u = 1; j < dims; j++) {
                    t *= v->dims(j);
                    u *= newdims[j];
                }
                o += ocount[i] * t;
                n += ncount[i] * u;
            }

            if (realflag)
                data[n] = v->realval(o);
            else
                cdata[n]= v->compval(o);

            // Now find the next index element...
            for (i = dims - 1; i >= 0; i--) {
                if ((ocount[i] < v->dims(i) - 1) &&
                        (ncount[i] < newdims[i] - 1)) {
                    ocount[i]++;
                    ncount[i]++;
                    break;
                }
                else
                    ocount[i] = ncount[i] = 0;
            }
            if (i < 0)
                break;
        }
    }


    bool cmp_values(wordlist *wl, int *length, double **datap,
        complex **cdatap, bool *realflag)
    {
        pnlist *pl = Sp.GetPtree(wl, true);
        if (!pl)
            return (true);
        sDvList *dl0 = Sp.DvList(pl);
        if (!dl0)
            return (true);

        // Now make sure these are all of the same dimensionality.  We
        // can coerce the sizes...
        //
        int dim = dl0->dl_dvec->numdims();
        if (dim < 2)
            dim = 0;
        if (dim >= MAXDIMS) {
            GRpkgIf()->ErrPrintf(ET_ERROR, "max dimensionality is %d.\n",
                MAXDIMS);
            sDvList::destroy(dl0);
            return (true);
        }
        int len = dim ? 1 : dl0->dl_dvec->length();
        bool rflg = true;
        if (dl0->dl_dvec->iscomplex())
            rflg = false;

        sDvList *dl;
        for (dl = dl0->dl_next; dl; dl = dl->dl_next) {
            sDataVec *v = dl->dl_dvec;
            int i = v->numdims();
            if (i < 2) {
                i = 0;
                len += v->length();
            }
            else
                len++;
            if (i != dim) {
                GRpkgIf()->ErrPrintf(ET_ERROR,
                    "all vectors must be of the same dimensionality.\n");
                sDvList::destroy(dl0);
                return (true);
            }
            if (v->iscomplex())
                rflg = false;
        }
        int i;
        int dims[MAXDIMS];
        for (i = 0; i < dim; i++) {
            dims[i] = dl0->dl_dvec->dims(i);
            for (dl = dl0->dl_next; dl; dl = dl->dl_next) {
                sDataVec *v = dl->dl_dvec;
                if (v->dims(i) > dims[i])
                    dims[i] = v->dims(i);
            }
        }
        dim++;
        dims[dim - 1] = len;
        int blocksize;
        for (i = 0, blocksize = 1; i < dim - 1; i++)
            blocksize *= dims[i];
        double *data = 0;
        complex *cdata = 0;
        if (rflg)
            data = new double[len*blocksize];
        else
            cdata = new complex[len*blocksize];

        // Now copy all the data over... If the sizes are too small
        // then the extra elements are left as 0.
        //
        for (i = 0, dl = dl0; dl; dl = dl->dl_next) {
            sDataVec *v = dl->dl_dvec;
            if (dim == 1) {
                for (int j = 0; j < v->length(); j++) {
                    if (rflg && v->isreal())
                        data[i] = v->realval(j);
                    else
                        cdata[i] = v->compval(j);
                    i++;
                }
                continue;
            }
            dimxpand(v, dims, (rflg ? (data + i * blocksize) : 
                    (double *) (cdata + i * blocksize)));
            i++;
        }
        len *= blocksize;
        *length = len;
        *realflag = rflg;
        if (rflg)
            *datap = data;
        else
            *cdatap = cdata;
        sDvList::destroy(dl0);
        return (false);
    }
}

