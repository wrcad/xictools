
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

#include "frontend.h"
#include "outdata.h"
#include "prntfile.h"
#include "cshell.h"
#include "spnumber/spnumber.h"
#include "ginterf/graphics.h"


cPrintIn::cPrintIn()
{
    // Nuthin' to do.
}


cPrintIn::~cPrintIn()
{
    // Nuthin' to do.
}


// Parse a standard-format print file into a new plot, which is
// returned.
//
sPlot *
cPrintIn::read(const char *fname)
{
    FILE *fp = Sp.PathOpen(fname, "r");
    if (!fp) {
        GRpkgIf()->Perror(fname);
        return (0);
    }

    OP.pushPlot();
    TTY.ioPush();
    CP.PushControl();

    sPlot *pl = parse_print(fp);
    fclose(fp);

    CP.PopControl();
    TTY.ioPop();
    OP.popPlot();

    if (pl) {
        // Make the vectors permanent, and fix dimension.
        sDataVec *v, *nv;
        for (v = pl->tempvecs(); v; v = nv) {
            nv = v->next();
            v->set_next(0);
            v->newperm(pl);
            if (v->numdims() == 1)
                v->set_dims(0, v->length());
        }
        pl->set_tempvecs(0);
    }
    return (pl);
}


namespace {
    struct LineSaver
    {
        LineSaver()
            {
                lastlines[0] = 0;
                lastlines[1] = 0;
                lastlines[2] = 0;
                lastlines[3] = 0;
            }

        ~LineSaver()
            {
                delete [] lastlines[0];
                delete [] lastlines[1];
                delete [] lastlines[2];
                delete [] lastlines[3];
            }

        void save(const char *s)
            {
                delete [] lastlines[3];
                lastlines[3] = lastlines[2];
                lastlines[2] = lastlines[1];
                lastlines[1] = lastlines[0];
                lastlines[0] = lstring::copy(s);
            }

        char *lastlines[4];
    };


    bool get_num(char *tok, double *d, int linecnt)
    {
        if (sscanf(tok, "%le", d) != 1) {
            GRpkgIf()->ErrPrintf(ET_ERROR,
            "Error: line %d, expected floating point number, parse failed.\n",
                linecnt);
            return (false);
        }
        return (true);
    }


    bool is_cycle(double *d, int cyc)
    {
        for (int i = 0; i < cyc; i++) {
            if (d[i] != d[cyc + i])
                return (false);
        }
        return (true);
    }
}


sPlot *
cPrintIn::parse_print(FILE *fp)
{
    char buf[1024];
    LineSaver ls;
    sPlot *pl = 0;
    sDataVec *vend = 0, *vblock = 0;
    int dashcnt = 0;
    int pointcnt = 0;
    int linecnt = 0;
    bool skipsc = false;

    while (fgets(buf, 1024, fp) != 0) {
        linecnt++;
        char *tok;
        if (lstring::prefix("----------", buf)) {
            dashcnt++;
            continue;
        }
        ls.save(buf);

        if (dashcnt < 2)
            continue;

        char *s = buf;
        while (isspace(*s))
            s++;
        if (!*s || !isdigit(*s))
            continue;

        sDataVec *vcur = vblock;
        int tcnt = 0;
        while ((tok = lstring::gettok(&s)) != 0) {
            tcnt++;
            if (tcnt == 1) {
                int n;
                if (sscanf(tok, "%d", &n) != 1) {
                    GRpkgIf()->ErrPrintf(ET_ERROR,
                        "Error: line %d, expected integer, parse failed.\n",
                        linecnt);
                    delete [] tok;
                    delete pl;
                    return (0);
                }
                if (n == 0) {
                    delete [] tok;
                    if (!vend) {
                        pl = new sPlot(0);
                        char *e = ls.lastlines[3];
                        while (isspace(*e))
                            e++;
                        char *title;
                        if (*e)
                            title = lstring::copy(e);
                        else
                            title = lstring::copy("untitled");
                        e = title + strlen(title) - 1;
                        while (e >= title && isspace(*e))
                            *e-- = 0;
                        pl->set_title(title);
                        delete [] title;

                        e = ls.lastlines[2];
                        while (isspace(*e))
                            e++;
                        // awful way to find start of date
                        char *dt = strstr(e, "  ");
                        if (dt) {
                            *dt++ = 0;
                            while (isspace(*dt))
                                dt++;
                        }
                        char *date;
                        if (dt && *dt)
                            date = lstring::copy(dt);
                        else
                            date = lstring::copy("undated");
                        char *plotname = lstring::copy(e);
                        e = date + strlen(date) - 1;
                        while (e >= date && isspace(*e))
                            *e-- = 0;
                        pl->set_date(date);
                        delete [] date;
                        e = plotname + strlen(plotname) - 1;
                        while (e >= plotname && isspace(*e))
                            *e-- = 0;
                        pl->set_name(plotname);
                        delete [] plotname;
                    }
                    else
                        skipsc = true;

                    vblock = 0;
                    int tcnt1 = 0;
                    char *s1 = ls.lastlines[1];
                    while ((tok = lstring::gettok(&s1)) != 0) {
                        tcnt1++;
                        if (tcnt1 <= (vend ? 2 : 1)) {
                            delete [] tok;
                            continue;
                        }
                        sDataVec *vec = new sDataVec(UU_NOTYPE);
                        vec->set_name(tok);
                        vec->set_plot(pl);
                        if (!vend) {
                            pl->set_scale(vec);
                            pl->set_tempvecs(vec);
                            vend = vec;
                        }
                        else {
                            while (vend->next())
                                vend = vend->next();
                            vend->set_next(vec);
                        }
                        if (!vblock) {
                            vblock = vec;
                            vcur = vec;
                        }
                        delete [] tok;
                    }
                    pointcnt = 0;
                }
                else if (n != pointcnt) {
                    GRpkgIf()->ErrPrintf(ET_ERROR,
                        "Error: line %d, index not monotonic.\n", linecnt);
                    delete pl;
                    return (0);
                }
                continue;
            }

            char *e = tok + strlen(tok) - 1;
            if (tcnt == 2 && skipsc) {
                if (*e == ',') {
                    delete [] tok;
                    tok = lstring::gettok(&s);
                }
                delete [] tok;
                continue;
            }
            if (*e == ',') {
                // complex data
                *e = 0;
                double re;
                if (!get_num(tok, &re, linecnt)) {
                    delete pl;
                    return (0);
                }
                delete [] tok;
                tok = lstring::gettok(&s);
                double im;
                if (!tok || !get_num(tok, &im, linecnt)) {
                    delete pl;
                    return (0);
                }
                if (vcur->length() == vcur->allocated()) {
                    complex *cold = vcur->compvec();
                    complex *cnew = new complex[vcur->length() + 64];
                    vcur->set_compvec(cnew);
                    if (cold) {
                        memcpy(cnew, cold, vcur->length()*sizeof(complex));
                        delete [] cold;
                    }
                    vcur->set_allocated(vcur->length() + 64);
                }
                int len = vcur->length();
                vcur->compvec()[len].real = re;
                vcur->compvec()[len].imag = im;
                vcur->set_length(len + 1);
            }
            else {
                double d;
                if (!get_num(tok, &d, linecnt)) {
                    delete pl;
                    return (0);
                }
                if (vcur->length() == vcur->allocated()) {
                    double *vold = vcur->realvec();
                    double *vnew = new double[vcur->length() + 64];
                    vcur->set_realvec(vnew);
                    if (vold) {
                        memcpy(vnew, vold, vcur->length()*sizeof(double));
                        delete [] vold;
                    }
                    vcur->set_allocated(vcur->length() + 64);
                }
                int len = vcur->length();
                vcur->realvec()[len] = d;
                vcur->set_length(len + 1);
            }
            delete [] tok;
            vcur = vcur->next();
            if (!vcur)
                break;
        }
        pointcnt++;
    }
    if (pl) {
        // Try to identify dimensionality, this could stand improvement.
        // This will recognize only 2-d plots.
        double *d = pl->scale()->realvec();
        int numpts = pl->scale()->length();
        int halfnp = numpts/2;
        int numdims = 0;
        int dims[2];
        for (int i = 1; i <= halfnp; i++) {
            if (d[0] == d[i]) {
                if (is_cycle(d, i) && (numpts % i) == 0) {
                    numdims = 2;
                    dims[0] = numpts/i;
                    dims[1] = i;
                    break;
                }
            }
        }
        if (numdims) {
            for (sDataVec *v = pl->tempvecs(); v; v = v->next()) {
                v->set_numdims(numdims);
                for (int j = 0; j < numdims; j++)
                    v->set_dims(j, dims[j]);
            }
        }
    }
    return (pl);
}
// End of cPrintIn functions.


cColIn::cColIn()
{
    // Nuthin' to do.
}


cColIn::~cColIn()
{
    // Nuthin' to do.
}


// Parse a file containing columns of data values.  Assume that extra
// columns can be placed in a separate block, as in a SPICE print
// file.  However, we handle at most one rollover block, and the
// number of columns in the rollover must be less than the number of
// main columns.
//
sPlot *
cColIn::read(const char *fname, int ncols, int xcols)
{
    if (ncols < 1) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "Column count is zero or negative.\n");
        return (0);
    }

    FILE *fp = Sp.PathOpen(fname, "r");
    if (!fp) {
        GRpkgIf()->Perror(fname);
        return (0);
    }

    OP.pushPlot();
    TTY.ioPush();
    CP.PushControl();

    sPlot *pl = parse_cols(fp, ncols, xcols);
    fclose(fp);

    CP.PopControl();
    TTY.ioPop();
    OP.popPlot();

    if (pl) {
        // Make the vectors permanent, and fix dimension.
        sDataVec *v, *nv;
        for (v = pl->tempvecs(); v; v = nv) {
            nv = v->next();
            v->set_next(0);
            v->newperm(pl);
            if (v->numdims() == 1)
                v->set_dims(0, v->length());
        }
        pl->set_tempvecs(0);
    }
    return (pl);
}


// Do the parse.  If xcols < 0 or xcols >= ncols, grab and save ncols
// numbers, ignoring any additional numbers in a line.  If xcols == 0,
// grab and save ncols numbers from lines with exactly ncols numbers
// only.  If 0 < xcols < ncols, take the total number of columns to be
// ncols+xcols.  Save the values from lines with ncols numbers and
// lines with xcols numbers.
//
// The use of positive xcols is for the case where additional columns
// are printed on separate lines to avoid too-long lines in the file. 
// There is expectation that the extra colums have the same number of
// values as the "regular" columns, and that the extra values are
// printed after the regular values.
//
sPlot *
cColIn::parse_cols(FILE *fp, int ncols, int xcols)
{
    sPlot *pl = 0;
    if (ncols <= 0)
        return (0);
    if (xcols >= ncols)
        xcols = -1;
    double *vals = new double[ncols];
    GCarray<double*> gv_vals(vals);
    for (;;) {
        bool err;
        char *line = Sp.ReadLine(fp, &err);
        if (!line)
            break;
        const char *s = line;
        int i = 0;
        for ( ; i < ncols; i++) {
            while (isspace(*s) || *s == ',')
                s++;
            double *d = SPnum.parse(&s, false);
            if (!d)
                break;
            vals[i] = *d;
        }
        if (i == ncols && xcols >= 0) {
            while (isspace(*s) || *s == ',')
                s++;
            double *d = SPnum.parse(&s, false);
            if (d)
                i = 0;
        }
        delete [] line;
        if (i < ncols && (!pl || xcols <= 0 || i != xcols))
            continue;
        int cols = i;
        if (!pl) {
            pl = new sPlot(0);
            pl->set_title("column data");
            pl->set_date(datestring());
            pl->set_name("column data import");
            sDataVec *vec = new sDataVec(UU_NOTYPE);
            pl->set_scale(vec);
            pl->set_tempvecs(vec);
            vec->set_plot(pl);
            vec->set_name("column_0");
            sDataVec *vend = vec;
            for (i = 1; i < ncols; i++) {
                char tbf[16];
                sprintf(tbf, "column_%d", i);
                vec = new sDataVec(UU_NOTYPE);
                vec->set_plot(pl);
                vec->set_name(tbf);
                vend->set_next(vec);
                vend = vend->next();
            }
            if (xcols > 0) {
                for (i = ncols; i < ncols + xcols; i++) {
                    char tbf[16];
                    sprintf(tbf, "column_%d", i);
                    vec = new sDataVec(UU_NOTYPE);
                    vec->set_plot(pl);
                    vec->set_name(tbf);
                    vend->set_next(vec);
                    vend = vend->next();
                }
            }
        }
        i = 0;
        for (sDataVec *v = pl->tempvecs(); v; v = v->next()) {
            if ((cols == ncols && i < ncols) ||
                    (cols == xcols && i >= ncols)) {
                if (v->length() == v->allocated()) {
                    double *vold = v->realvec();
                    double *vnew = new double[v->length() + 64];
                    v->set_realvec(vnew);
                    if (vold) {
                        memcpy(vnew, vold, v->length()*sizeof(double));
                        delete [] vold;
                    }
                    v->set_allocated(v->length() + 64);
                }
                int len = v->length();
                int ix = i;
                if (ix > ncols)
                    ix -= ncols;
                v->realvec()[len] = vals[ix];
                v->set_length(len + 1);
            }
            i++;
        }
    }
    if (pl) {
        if (xcols > 0) {
            // This makes sense only if the lengths of the extra vectors
            // is the same as the others.  Enforce this.
            int len = pl->tempvecs()->length();
            int i = 0;
            for (sDataVec *v = pl->tempvecs(); v; v = v->next()) {
                if (i >= ncols) {
                    if (v->length() > len)
                        v->set_length(len);
                    else if (v->length() < len) {
                        double *vold = v->realvec();
                        double *vnew = new double[len];
                        v->set_realvec(vnew);
                        memset(vnew, 0, len*sizeof(double));
                        memcpy(vnew, vold, v->length()*sizeof(double));
                        delete [] vold;
                        v->set_length(len);
                        v->set_allocated(len);
                    }
                }
                i++;
            }
        }
        // Try to identify dimensionality, this could stand improvement.
        // This will recognize only 2-d plots.
        double *d = pl->scale()->realvec();
        int numpts = pl->scale()->length();
        int halfnp = numpts/2;
        int numdims = 0;
        int dims[2];
        for (int i = 1; i <= halfnp; i++) {
            if (d[0] == d[i]) {
                if (is_cycle(d, i) && (numpts % i) == 0) {
                    numdims = 2;
                    dims[0] = numpts/i;
                    dims[1] = i;
                    break;
                }
            }
        }
        if (numdims) {
            for (sDataVec *v = pl->tempvecs(); v; v = v->next()) {
                v->set_numdims(numdims);
                for (int j = 0; j < numdims; j++)
                    v->set_dims(j, dims[j]);
            }
        }
    }
    return (pl);
}
// End of cColIn functions.

