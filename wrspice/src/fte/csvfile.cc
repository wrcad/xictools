
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

#include "spglobal.h"
#include "simulator.h"
#include "csvfile.h"
#include "output.h"
#include "cshell.h"
#include "kwords_fte.h"
#include "errors.h"
#include "spnumber/hash.h"
#include "ginterf/graphics.h"

/* TO DO
 * comments level: none, varnames only, verbose.
 * comment char
*/

//
// Read and write comma separated variable (CSV) files.
//

#define DEFPREC 15

#define CSV_CMTCHAR '#'

cCSVout::cCSVout(sPlot *pl)
{
    co_plot = pl;
    co_fp = 0;
    co_pointPosn = 0;
    co_prec = 0;
    co_numdims = 0;
    co_length = 0;
    memset(co_dims, 0, MAXDIMS*sizeof(int));
    co_dlist = 0;
    co_realflag = false;
    co_pad = false;
    co_no_close = false;
    co_nmsmpl = false;
    co_cmtchar = CSV_CMTCHAR; 
    co_cmtlev = CMTLEV_all;
}


cCSVout::~cCSVout()
{
    file_close();
}


// Static function.
// Return true if the file extension is a known CSV extension.
//
bool
cCSVout::is_csv_ext(const char *fname)
{
    if (!fname)
        return (false);
    const char *extn = strrchr(fname, '.');
    if (!extn)
        return (false);
    if (extn == fname)
        return (false);
    extn++;
    if (lstring::cieq(extn, "csv"))
        return (true);
    return (false);
}


bool
cCSVout::file_write(const char *filename, bool app)
{
    if (!file_open(filename, app ? "a" : "w", false))
        return (false);
    if (!file_head())
        return (false);
    if (!file_vars())
        return (false);
    if (!file_points())
        return (false);
    if (!file_close())
        return (false);
    return (true);
}


// Open the csv file, return true if successful.
//
bool
cCSVout::file_open(const char *filename, const char *mode, bool)
{
    file_close();
    FILE *fp = 0;
    if (filename && *filename) {
        if (!(fp = fopen(filename, mode))) {
            GRpkg::self()->Perror(filename);
            return (false);
        }
    }
    co_fp = fp;
    co_pointPosn = 0;
    if (OP.getOutDesc()->outNdgts() > 0)
        co_prec = OP.getOutDesc()->outNdgts();
    else
        co_prec = DEFPREC;
    co_numdims = 0;
    co_length = 0;
    co_realflag = true;

    bool nopadding = Sp.GetVar(kw_nopadding, VTYP_BOOL, 0);
    co_pad = !nopadding;
    return (true);
}


// Output the header part of the csv file.  If this is called from the
// output function, the lengths are all 0, so we save the file position
// of the No.  Points.
//
bool
cCSVout::file_head()
{
    if (!co_plot)
        return (false);
    if (co_cmtchar && co_cmtlev == CMTLEV_all) {
        fprintf(co_fp, "%cTitle: %s\n", co_cmtchar, co_plot->title());
        fprintf(co_fp, "%cDate: %s\n", co_cmtchar, co_plot->date());
        fprintf(co_fp, "%cPlotname: %s\n", co_cmtchar, co_plot->name());
    }

    sDataVec *v = co_plot->find_vec("all");
    v->sort();
    co_dlist = v->link();
    v->set_link(0); // so list isn't freed in VecGc()

    // Make sure that the scale is the first in the list.
    //
    bool found_scale = false;
    sDvList *tl, *dl;
    for (tl = 0, dl = co_dlist; dl; tl = dl, dl = dl->dl_next) {
        if (dl->dl_dvec == co_plot->scale()) {
            if (tl) {
                tl->dl_next = dl->dl_next;
                dl->dl_next = co_dlist;
                co_dlist = dl;
            }
            found_scale = true;
            break;
        }
    }
    if (!found_scale && co_plot->scale()) {
        dl = new sDvList;
        dl->dl_next = co_dlist;
        dl->dl_dvec = co_plot->scale();
        co_dlist = dl;
    }

    int nvars;
    for (nvars = 0, dl = co_dlist; dl; dl = dl->dl_next) {
        v = dl->dl_dvec;
        if (v->iscomplex())
            co_realflag = false;
        nvars++;
        // Find the length and dimensions of the longest vector
        // in the plot.
        // Be paranoid and assume somewhere we may have
        // forgotten to set the dimensions of 1-D vectors.
        //
        if (v->numdims() <= 1) {
            v->set_numdims(1);
            v->set_dims(0, v->length());
        }
        if (v->length() > co_length) {
            co_length = v->length();
            co_numdims = v->numdims();
            for (int j = 0; j < co_numdims; j++)
                co_dims[j] = v->dims(j);
        }
    }

    if (co_cmtchar && co_cmtlev == CMTLEV_all) {
        fprintf(co_fp, "%cFlags: %s%s\n", co_cmtchar,
            co_realflag ? "real" : "complex", co_pad ? "" : " unpadded" );
        fprintf(co_fp, "%cNo. Variables: %d\n", co_cmtchar, nvars);
        fprintf(co_fp, "%cNo. Points: ", co_cmtchar);
        fflush(co_fp);
        co_pointPosn = ftell(co_fp);
        fprintf(co_fp, "%-8d\n", co_length);  // save space

        if (co_numdims > 1) {
            fprintf(co_fp, "%cDimensions: ", co_cmtchar);
            for (int i = 0; i < co_numdims; i++) {
                fprintf(co_fp, "%d%s",  co_dims[i],
                    (i < co_numdims - 1) ? "," : "");
            }
            fprintf(co_fp, "\n");
        }
    }

    wordlist *wl;
    if (co_cmtchar && co_cmtlev == CMTLEV_all) {
        if (co_plot->commands()) {
            for (wl = co_plot->commands(); wl; wl = wl->wl_next)
                fprintf(co_fp, "%cCommand: %s\n", co_cmtchar, wl->wl_word);
        }
        else {
            fprintf(co_fp, "%cCommand: version %s\n", co_cmtchar,
                Sp.Version());
        }

        for (variable *vv = co_plot->environment(); vv; vv = vv->next()) {
            if (vv->type() == VTYP_BOOL)
                fprintf(co_fp, "%cOption: %s\n", co_cmtchar, vv->name());
            else {
                fprintf(co_fp, "%cOption: %s = ", co_cmtchar, vv->name());
                if (vv->type() == VTYP_LIST)
                    fprintf(co_fp, "( ");
                wl = vv->varwl();
                wordlist::print(wl, co_fp);
                wordlist::destroy(wl);
                if (vv->type() == VTYP_LIST)
                    fprintf(co_fp, " )");
                putc('\n', co_fp);
            }
        }
    }
    return (true);
}


// Output the vector names and characteristics.
//
bool
cCSVout::file_vars()
{
    if (co_cmtchar && co_cmtlev == CMTLEV_all) {
        fprintf(co_fp, "%cVariables:\n", co_cmtchar);
    }
    if (co_cmtchar &&
            (co_cmtlev == CMTLEV_names || co_cmtlev == CMTLEV_all)) {
        int i;
        sDvList *dl;
        for (i = 0, dl = co_dlist; dl; dl = dl->dl_next) {
            sDataVec *v = dl->dl_dvec;
            i++;
            if (co_nmsmpl) {
                if (i == 1)
                    fprintf(co_fp, "%s", v->name());
                else
                    fprintf(co_fp, ",%s", v->name());
            }
            else {
                char tbf[256];
                sLstr lstr;
                lstr.add_c('\"');
                lstr.add(v->name());
                char *t = v->units()->unitstr();
                if (t && *t && !isspace(*t)) {
                    snprintf(tbf, 256, " units=%s", t);
                    lstr.add(tbf);
                }
                delete [] t;

                if (v->flags() & VF_MINGIVEN) {
                    snprintf(tbf, 256, " min=%e", v->minsignal());
                    lstr.add(tbf);
                }
                if (v->flags() & VF_MAXGIVEN) {
                    snprintf(tbf, 256, " max=%e", v->maxsignal());
                    lstr.add(tbf);
                }
                if (v->defcolor()) {
                    snprintf(tbf, 256, " color=%s", v->defcolor());
                    lstr.add(tbf);
                }
                if (v->gridtype()) {
                    snprintf(tbf, 256, " grid=%d", v->gridtype());
                    lstr.add(tbf);
                }
                if (v->plottype()) {
                    snprintf(tbf, 256, " plot=%d", v->plottype());
                    lstr.add(tbf);
                }

                // Only write dims if they are different from default
                int j = 0;
                if (v->numdims() == co_numdims) {
                    for ( ; j < co_numdims; j++) {
                        if (co_dims[j] != v->dims(j))
                            break;
                    }
                }
                if (j < co_numdims) {
                    lstr.add(" dims=");
                    for (int k = 0; k < v->numdims(); k++) {
                        snprintf(tbf, 256, "%d%s",  v->dims(k),
                            (k < v->numdims() - 1) ? "," : "");
                        lstr.add(tbf);
                    }
                }
                lstr.add_c('\"');
                if (i == 1)
                    fprintf(co_fp, "%s", lstr.string());
                else
                    fprintf(co_fp, ",%s", lstr.string());
            }
        }
        putc('\n', co_fp);
    }
    if (co_cmtchar && co_cmtlev == CMTLEV_all) {
        fprintf(co_fp, "%cValues:\n", co_cmtchar);
    }
    return (true);
}


// Output the data, set indx to true index when called from output
// function.  In this case, this function is called repeatedly.
//
bool
cCSVout::file_points(int indx)
{
    (void)indx;
    if (co_length == 0)
        // true when called from output routine the first time
        co_length = 1;
    for (int i = 0; i < co_length; i++) {
        for (sDvList *dl = co_dlist; dl; dl = dl->dl_next) {
            sDataVec *v = dl->dl_dvec;
            if (v) {
                if (i < v->length()) {
                    if (dl != co_dlist)
                        putc(',', co_fp);
                    if (co_realflag)
                        fprintf(co_fp, "%.*e", co_prec, v->realval(i));
                    else if (v->isreal())
                        fprintf(co_fp, "%.*e,0.0", co_prec, v->realval(i));
                    else {
                        fprintf(co_fp, "%.*e,%.*e",
                            co_prec, v->realval(i),
                            co_prec, v->imagval(i));
                    }
                }
                else if (co_pad) {
                    if (dl != co_dlist)
                        putc(',', co_fp);
                    if (co_realflag)
                        fprintf(co_fp, "%.*e", co_prec, 0.0);
                    else {
                        fprintf(co_fp, "%.*e,%.*e",
                            co_prec, 0.0, co_prec, 0.0);
                    }
                }
            }
        }
        putc('\n', co_fp);
    }
    return (true);
}


// Fill in the point count field.
//
bool
cCSVout::file_update_pcnt(int pointCount)
{
    if (!co_fp || co_fp == stdout)
        return (true);
    fflush(co_fp);
    long place = ftell(co_fp);
    fseek(co_fp, co_pointPosn, SEEK_SET);
    fprintf(co_fp, "%d", pointCount);
    fseek(co_fp, place, SEEK_SET);
    fflush(co_fp);
    return (true);
}


// Close the file.
//
bool
cCSVout::file_close()
{
    sDvList::destroy(co_dlist);
    co_dlist = 0;
    if (co_fp && co_fp != stdout && !co_no_close)
        fclose(co_fp);
    co_fp = 0;
    return (true);
}
// End of cRawOut functions.


// Read a CSV file.  Returns a list of plot structures.  This function
// should be very flexible about what it expects to see in the
// csvfile.  Really all we require is that there be one variables and
// one values section per plot and that the variables precede the
// values.


cCSVin::cCSVin()
{
    ci_fp           = 0;
    ci_title        = 0;
    ci_date         = 0;
    ci_plots        = 0;
    ci_flags        = 0;
    ci_nvars        = 0;
    ci_npoints      = 0;
    ci_numdims      = 0;
    memset(ci_dims, 0, MAXDIMS*sizeof(int));
    ci_padded       = true;
}


cCSVin::~cCSVin()
{
    if (ci_fp && ci_fp != stdin)
        fclose(ci_fp);
    delete [] ci_title;
    delete [] ci_date;

}


namespace {

    inline void
    skip(const char **s)
    {
        const char *t = *s;
        while (*t && !isspace(*t))
            t++;
        while (isspace(*t))
            t++;
        *s = t;
    }

    inline void
    nonl(char *buf)
    {
        char *s = strrchr(buf, '\n');
        if (s) {
            if (s > buf && *(s-1) == '\r')
                *(s-1) = 0;
            else
                *s = 0;
        }
    }
}


#define SIZE_INCR 10

sPlot *
cCSVin::csv_read(const char *name)
{
    ci_fp = Sp.PathOpen(name, "rb");
    if (!ci_fp) {
        GRpkg::self()->Perror(name);
        return (0);
    }

    OP.pushPlot();
    TTY.ioPush();
    CP.PushControl();

    sPlot *curpl = 0;
    char buf[BSIZE_SP];
    int rawline = 0;
    const char cmtchar = CSV_CMTCHAR;
    while (fgets(buf, BSIZE_SP, ci_fp)) {
        const char *s = buf;
        bool cmt_line = false;
        while (*s == cmtchar || isspace(*s)) {
            if (*s == cmtchar)
                cmt_line = true;
            s++;
        }
        char buf2[BSIZE_SP];
        if (cmt_line) {
            // A comment line, look for saved info.
            if (lstring::ciprefix("title:", s)) {
                skip(&s);
                nonl(buf);
                delete [] ci_title;
                ci_title = lstring::copy(s);
            }
            else if (lstring::ciprefix("date:", s)) {
                skip(&s);
                nonl(buf);
                delete [] ci_date;
                ci_date = lstring::copy(s);
            }
            else if (lstring::ciprefix("plotname:", s)) {
                skip(&s);
                nonl(buf);
                if (curpl) {    // reverse commands list
                    wordlist *nwl, *wl = curpl->commands();
                    curpl->set_commands(0);
                    for ( ; wl && wl->wl_next; wl = nwl) {
                        nwl = wl->wl_next;
                        wl->wl_next = curpl->commands();
                        curpl->set_commands(wl);
                    }
                }

                // Start a new plot.
                rawline = 0;
                curpl = new sPlot(0);
                curpl->set_next_plot(ci_plots);
                ci_plots = curpl;
                curpl->set_name(s);
                if (!ci_date)
                    ci_date = lstring::copy(datestring());
                curpl->set_date(ci_date);
                if (!ci_title)
                    ci_title = lstring::copy("default title");
                curpl->set_title(ci_title);
                ci_flags = 0;
                ci_nvars = ci_npoints = 0;
                ci_numdims = 0;
                memset(ci_dims, 0, MAXDIMS*sizeof(int));
                ci_padded = true;
            }
            else if (lstring::ciprefix("flags:", s)) {
                skip(&s);
                while (lstring::copytok(buf2, &s)) {
                    if (lstring::cieq(buf2, "real"))
                        ;
                    else if (lstring::cieq(buf2, "complex"))
                        ci_flags |= VF_COMPLEX;
                    else if (lstring::cieq(buf2, "unpadded"))
                        ci_padded = false;
                    else if (lstring::cieq(buf2, "padded"))
                        ci_padded = true;
                    else {
                        GRpkg::self()->ErrPrintf(ET_WARN,
                            "unknown flag %s.\n", buf2);
                    }
                }
            }
            else if (lstring::ciprefix("no. variables:", s)) {
                skip(&s);
                skip(&s);
                ci_nvars = lstring::scannum(s);
            }
            else if (lstring::ciprefix("no. points:", s)) {
                skip(&s);
                skip(&s);
                ci_npoints = lstring::scannum(s);
            }
            else if (lstring::ciprefix("dimensions:", s)) {
                skip(&s);
                if (sDataVec::atodims(s, ci_dims, &ci_numdims)) {
                    // Something's wrong
                    GRpkg::self()->ErrPrintf(ET_WARN,
                        "syntax error in dimensions, ignored.\n");
                    ci_numdims = 0;
                    continue;
                }
                if (ci_numdims > MAXDIMS) {
                    ci_numdims = 0;
                    continue;
                }
                // Let's just make sure that the no. of points
                // and the dimensions are consistent.
                //
                int i, j;
                for (j = 0, i = 1; j < ci_numdims; j++)
                    i *= ci_dims[j];

                if (ci_npoints && i != ci_npoints) {
                    GRpkg::self()->ErrPrintf(ET_WARN,
                "dimensions inconsistent with number of points, ignored.\n");
                    ci_numdims = 0;
                }
            }
            else if (lstring::ciprefix("command:", s)) {
                // Note that we reverse these commands eventually...
                skip(&s);
                nonl(buf);
                if (curpl) {
                    wordlist *wl = new wordlist;
                    wl->wl_word = lstring::copy(s);
                    wl->wl_next = curpl->commands();
                    if (curpl->commands())
                        curpl->commands()->wl_prev = wl;
                    curpl->set_commands(wl);
                }
                else {
                    GRpkg::self()->ErrPrintf(ET_ERROR,
                        "misplaced Command: line.\n");
                }
                // Now execute the command if we can
                CP.EvLoop(s);
            }
            else if (lstring::ciprefix("option:", s)) {
                skip(&s);
                nonl(buf);
                if (curpl) {
                    wordlist *wl = CP.LexString(s);
                    variable *vv;
                    for (vv = curpl->environment(); vv && vv->next();
                            vv = vv->next()) ;
                    if (vv)
                        vv->set_next(CP.ParseSet(wl));
                    else
                        curpl->set_environment(CP.ParseSet(wl));
                    wordlist::destroy(wl);
                }
                else {
                    GRpkg::self()->ErrPrintf(ET_ERROR,
                        "misplaced Command: line.\n");
                }
            }
            continue;
        }
        if (!*s)
            continue;
        if (rawline == 0) {
            rawline++;
            // Process the vector name list.

            // We reverse the dvec list eventually...
            if (!curpl) {
                GRpkg::self()->ErrPrintf(ET_ERROR, "no plot name given.\n");
                ci_plots = 0;
                break;
            }
            if (ci_numdims == 0) {
                ci_numdims = 1;
                ci_dims[0] = ci_npoints;
            }
            // Now read all the variables in
            int i = 0;
            while (char *tok = lstring::gettok(&s, ",")) {
                const char *nmxtra = tok;
                if (*nmxtra == '"') {
                    nmxtra++;
                    char *e = tok + strlen(tok) - 1;
                    if (*e == '"')
                        *e = 0;
                }

                char *nm = lstring::gettok(&nmxtra);
                sDataVec *v = new sDataVec;
                v->set_name(nm);
                delete [] nm;
                v->set_next(curpl->tempvecs());
                curpl->set_tempvecs(v);
                if (!curpl->scale())
                    curpl->set_scale(v);
                v->set_flags(ci_flags);
                v->set_plot(curpl);

                v->set_length(0);
                v->set_numdims(0);
                // Length and dims might be changed by options.

                if (!i)
                    curpl->set_scale(v);
                i++;

                // Fix the name...
                if (isdigit(*v->name())) {
                    snprintf(buf2, sizeof(buf2), "v(%s)", v->name());
                    v->set_name(buf2);
                }
                // Now come the strange options...
                if (nmxtra && *nmxtra) {
                    while (lstring::copytok(buf2, &nmxtra)) {
                        if (lstring::ciprefix("units=", buf2)) {
                            const char *tt = buf2 + 6;
                            char *un = lstring::gettok(&tt);
                            if (un && *un)
                                v->ncunits()->set(un);
                            delete [] un;
                        }
                        else if (lstring::ciprefix("min=", buf2)) {
                            double ms;
                            if (sscanf(buf2 + 4, "%lf", &ms) != 1) {
                                GRpkg::self()->ErrPrintf(ET_ERROR,
                                    "bad arg %s.\n", buf2);
                            }
                            else {
                                v->set_minsignal(ms);
                                v->set_flags(v->flags() | VF_MINGIVEN);
                            }
                        }
                        else if (lstring::ciprefix("max=", buf2)) {
                            double ms;
                            if (sscanf(buf2 + 4, "%lf", &ms) != 1) {
                                GRpkg::self()->ErrPrintf(ET_ERROR,
                                    "bad arg %s.\n", buf2);
                            }
                            else {
                                v->set_maxsignal(ms);
                                v->set_flags(v->flags() | VF_MAXGIVEN);
                            }
                        }
                        else if (lstring::ciprefix("color=", buf2)) {
                            v->set_defcolor(buf2 + 6);
                        }
                        else if (lstring::ciprefix("scale=", buf2)) {
                            // This is bad, but...
                            v->set_scale((sDataVec*)lstring::copy(buf2 + 6));
                        }
                        else if (lstring::ciprefix("grid=", buf2)) {
                            v->set_gridtype((GridType) lstring::scannum(
                                buf2 + 5));
                        }
                        else if (lstring::ciprefix("plot=", buf2)) {
                            v->set_plottype((PlotType) lstring::scannum(
                                buf2 + 5));
                        }
                        else if (lstring::ciprefix("dims=", buf2)) {
                            v->fixdims(buf2 + 5);
                        }
                        else {
                            GRpkg::self()->ErrPrintf(ET_WARN,
                                "bad var param %s.\n", buf2);
                        }
                    }
                }
                delete [] tok;

                // Now we default any missing dimensions
                if (!v->numdims()) {
                    v->set_numdims(ci_numdims);
                    for (int j = 0; j < ci_numdims; j++)
                        v->set_dims(j, ci_dims[j]);
                }
                // And allocate the data array. We would use
                // the desired vector length, but this would
                // be dangerous if the file is invalid.

                if (ci_npoints) {
                    if (v->isreal())
                        v->set_realvec(new double[ci_npoints]);
                    else
                        v->set_compvec(new complex[ci_npoints]);
                    v->set_allocated(ci_npoints);
                }
                else {
                    if (v->isreal())
                        v->set_realvec(new double[SIZE_INCR]);
                    else
                        v->set_compvec(new complex[SIZE_INCR]);
                    v->set_allocated(SIZE_INCR);
                }
            }

            // We'd better reverse the dvec list now...
            sDataVec *v = curpl->tempvecs(), *nv;
            curpl->set_tempvecs(0);
            for ( ; v; v = nv) {
                nv = v->next();
                v->set_next(curpl->tempvecs());
                curpl->set_tempvecs(v);
            }
            // And fix the scale pointers
            for (v = curpl->tempvecs(); v; v = v->next()) {
                if (v->special_scale()) {
                    for (nv = curpl->tempvecs(); nv; nv = nv->next()) {
                        if (lstring::cieq((char*)v->special_scale(),
                                nv->name())) {
                            delete [] (char*)v->special_scale();
                            v->set_scale(nv);
                            break;
                        }
                    }
                    if (!nv) {
                        Sp.Error(E_NOVEC, 0, (char*)v->scale());
                        v->set_scale(0);
                    }
                }
            }
        }
        else {
            const char *errbr = "bad file, missing data.\n";
            for (sDataVec *v = curpl->tempvecs(); v; v = v->next()) {
                char *tok = lstring::gettok(&s, ",");
                double val1, val2 = 0.0;
                if (sscanf(tok, "%lf", &val1) != 1) {
                    GRpkg::self()->ErrPrintf(ET_ERROR, errbr);
                    delete [] tok;
                    break;
                }
                delete [] tok;
                if (!v->isreal()) {
                    tok = lstring::gettok(&s, ",");
                    if (sscanf(tok, "%lf", &val2) != 1) {
                        GRpkg::self()->ErrPrintf(ET_ERROR, errbr);
                        delete [] tok;
                        break;
                    }
                    delete [] tok;
                }
                v->add_point(&val1, &val2, SIZE_INCR);
            }
        }
    }

    if (curpl) {    // reverse commands list
        wordlist *nwl, *wl = curpl->commands();
        curpl->set_commands(0);
        for ( ; wl && wl->wl_next; wl = nwl) {
            nwl = wl->wl_next;
            wl->wl_next = curpl->commands();
            curpl->set_commands(wl);
        }
    }

    CP.PopControl();
    TTY.ioPop();
    OP.popPlot();
    fclose(ci_fp);
    ci_fp = 0;

    // Reverse the plots list, so will be in file order.
    curpl = ci_plots;
    ci_plots = 0;
    while (curpl) {
        sPlot *pl = curpl;
        curpl = curpl->next_plot();
        pl->set_next_plot(ci_plots);
        ci_plots = pl;
    }

    // make the vectors permanent, and fix dimension
    for (curpl = ci_plots; curpl; curpl = curpl->next_plot()) {
        sDataVec *v, *nv;
        for (v = curpl->tempvecs(); v; v = nv) {
            nv = v->next();
            v->set_next(0);
            v->newperm(curpl);
            if (v->numdims() == 1)
                v->set_dims(0, v->length());
        }
        curpl->set_tempvecs(0);
    }
    return (ci_plots);
}
// End of cCSVin functions.

