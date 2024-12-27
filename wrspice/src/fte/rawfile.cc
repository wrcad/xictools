
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
         1992 Stephen R. Whiteley
****************************************************************************/

#include "spglobal.h"
#include "simulator.h"
#include "rawfile.h"
#include "output.h"
#include "cshell.h"
#include "kwords_fte.h"
#include "errors.h"
#include "spnumber/hash.h"
#include "ginterf/graphics.h"


//
// Read and write the ascii and binary rawfile formats.
//

#define DEFPREC 15

cRawOut::cRawOut(sPlot *pl)
{
    ro_plot = pl;
    ro_fp = 0;
    ro_pointPosn = 0;
    ro_prec = 0;
    ro_numdims = 0;
    ro_length = 0;
    memset(ro_dims, 0, MAXDIMS*sizeof(int));
    ro_dlist = 0;
    ro_realflag = false;
    ro_binary = false;
    ro_pad = false;
    ro_no_close = false;
}


cRawOut::~cRawOut()
{
    file_close();
}


bool
cRawOut::file_write(const char *filename, bool app)
{
    bool ascii = Global.AsciiOut();
    VTvalue vv;
    if (Sp.GetVar(kw_filetype, VTYP_STRING, &vv)) {
        if (lstring::cieq(vv.get_string(), "binary"))
            ascii = false;
        else if (lstring::cieq(vv.get_string(), "ascii"))
            ascii = true;
        else {
            GRpkg::self()->ErrPrintf(ET_WARN,
                "strange file type %s (ignored).\n", vv.get_string());
        }
    }
    if (ascii) {
        if (!file_open(filename, app ? "a" : "w", false))
            return (false);
    }
    else {
        if (!file_open(filename, app ? "ab" : "wb", true))
            return (false);
    }
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


// Open the rawfile, return true if successful.
//
bool
cRawOut::file_open(const char *filename, const char *mode, bool binary)
{
    file_close();
    FILE *fp = 0;
    if (filename && *filename) {
        if (!(fp = fopen(filename, mode))) {
            GRpkg::self()->Perror(filename);
            return (false);
        }
    }
    ro_fp = fp;
    ro_pointPosn = 0;
    if (OP.getOutDesc()->outNdgts() > 0)
        ro_prec = OP.getOutDesc()->outNdgts();
    else
        ro_prec = DEFPREC;
    ro_numdims = 0;
    ro_length = 0;
    ro_realflag = true;
    ro_binary = binary;

    bool nopadding = Sp.GetVar(kw_nopadding, VTYP_BOOL, 0);
    ro_pad = !nopadding;
    return (true);
}


// Output the header part of the rawfile.  If this is called from the
// output function, the lengths are all 0, so we save the file position
// of the No.  Points.
//
bool
cRawOut::file_head()
{
    if (!ro_plot)
        return (false);
    fprintf(ro_fp, "Title: %s\n", ro_plot->title());
    fprintf(ro_fp, "Date: %s\n", ro_plot->date());
    fprintf(ro_fp, "Plotname: %s\n", ro_plot->name());

    sDataVec *v = ro_plot->find_vec("all");
    v->sort();
    ro_dlist = v->link();
    v->set_link(0); // so list isn't freed in VecGc()

    // Make sure that the scale is the first in the list.
    //
    bool found_scale = false;
    sDvList *tl, *dl;
    for (tl = 0, dl = ro_dlist; dl; tl = dl, dl = dl->dl_next) {
        if (dl->dl_dvec == ro_plot->scale()) {
            if (tl) {
                tl->dl_next = dl->dl_next;
                dl->dl_next = ro_dlist;
                ro_dlist = dl;
            }
            found_scale = true;
            break;
        }
    }
    if (!found_scale && ro_plot->scale()) {
        dl = new sDvList;
        dl->dl_next = ro_dlist;
        dl->dl_dvec = ro_plot->scale();
        ro_dlist = dl;
    }

    int nvars = 0;
    for (nvars = 0, dl = ro_dlist; dl; dl = dl->dl_next) {
        v = dl->dl_dvec;
        if (v->iscomplex())
            ro_realflag = false;
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
        if (v->length() > ro_length) {
            ro_length = v->length();
            ro_numdims = v->numdims();
            for (int j = 0; j < ro_numdims; j++)
                ro_dims[j] = v->dims(j);
        }
    }

    fprintf(ro_fp, "Flags: %s%s\n",
        ro_realflag ? "real" : "complex", ro_pad ? "" : " unpadded" );
    fprintf(ro_fp, "No. Variables: %d\n", nvars);
    fprintf(ro_fp, "No. Points: ");
    fflush(ro_fp);
    ro_pointPosn = ftell(ro_fp);
    fprintf(ro_fp, "%-8d\n", ro_length);  // save space

    if (ro_numdims > 1) {
        fprintf(ro_fp, "Dimensions: ");
        for (int i = 0; i < ro_numdims; i++) {
            fprintf(ro_fp, "%d%s",  ro_dims[i],
                (i < ro_numdims - 1) ? "," : "");
        }
        fprintf(ro_fp, "\n");
    }

    wordlist *wl;
    if (ro_plot->commands())
        for (wl = ro_plot->commands(); wl; wl = wl->wl_next)
            fprintf(ro_fp, "Command: %s\n", wl->wl_word);
    else
        fprintf(ro_fp, "Command: version %s\n", Sp.Version());

    for (variable *vv = ro_plot->environment(); vv; vv = vv->next()) {
        if (vv->type() == VTYP_BOOL)
            fprintf(ro_fp, "Option: %s\n", vv->name());
        else {
            fprintf(ro_fp, "Option: %s = ", vv->name());
            if (vv->type() == VTYP_LIST)
                fprintf(ro_fp, "( ");
            wl = vv->varwl();
            wordlist::print(wl, ro_fp);
            wordlist::destroy(wl);
            if (vv->type() == VTYP_LIST)
                fprintf(ro_fp, " )");
            putc('\n', ro_fp);
        }
    }
    return (true);
}


// Output the vector names and characteristics.
//
bool
cRawOut::file_vars()
{
    fprintf(ro_fp, "Variables:\n");
    int i;
    sDvList *dl;
    for (i = 0, dl = ro_dlist; dl; dl = dl->dl_next) {
        sDataVec *v = dl->dl_dvec;
        char *t = v->units()->unitstr();
        fprintf(ro_fp, " %d %s %s", i++, v->name(), t);
        delete [] t;
        if (v->flags() & VF_MINGIVEN)
            fprintf(ro_fp, " min=%e", v->minsignal());
        if (v->flags() & VF_MAXGIVEN)
            fprintf(ro_fp, " max=%e", v->maxsignal());
        if (v->defcolor())
            fprintf(ro_fp, " color=%s", v->defcolor());
        if (v->gridtype())
            fprintf(ro_fp, " grid=%d", v->gridtype());
        if (v->plottype())
            fprintf(ro_fp, " plot=%d", v->plottype());

        // Only write dims if they are different from default
        int j = 0;
        if (v->numdims() == ro_numdims) {
            for ( ; j < ro_numdims; j++) {
                if (ro_dims[j] != v->dims(j))
                    break;
            }
        }
        if (j < ro_numdims) {
            fprintf(ro_fp, " dims=");
            for (int k = 0; k < v->numdims(); k++) {
                fprintf(ro_fp, "%d%s",  v->dims(k),
                    (k < v->numdims() - 1) ? "," : "");
            }
        }
        putc('\n', ro_fp);
    }
    if (ro_binary)
        fprintf(ro_fp, "Binary:\n");
    else
        fprintf(ro_fp, "Values:\n");
    return (true);
}


// Output the data, set indx to true index when called from output
// function.  In this case, this function is called repeatedly.
//
bool
cRawOut::file_points(int indx)
{
    if (ro_length == 0)
        // true when called from output routine the first time
        ro_length = 1;
    if (ro_binary) {
        for (int i = 0; i < ro_length; i++) {
            for (sDvList *dl = ro_dlist; dl; dl = dl->dl_next) {
                sDataVec *v = dl->dl_dvec;
                if (v) {
                    // Don't run off the end of this vector's data
                    double dd;
                    if (i < v->length()) {
                        if (ro_realflag) {
                            dd = v->realval(i);
                            fwrite((char*)&dd, sizeof(double), 1, ro_fp);
                        }
                        else if (v->isreal()) {
                            dd = v->realval(i);
                            fwrite((char*)&dd, sizeof(double), 1, ro_fp);
                            dd = 0.0;
                            fwrite((char*)&dd, sizeof(double), 1, ro_fp);
                        }
                        else {
                            dd = v->realval(i);
                            fwrite((char*)&dd, sizeof(double), 1, ro_fp);
                            dd = v->imagval(i);
                            fwrite((char*)&dd, sizeof(double), 1, ro_fp);
                        }
                    }
                    else if (ro_pad) {
                        dd = 0.0;
                        if (ro_realflag)
                            fwrite((char*)&dd, sizeof(double), 1, ro_fp);
                        else {
                            fwrite((char*)&dd, sizeof(double), 1, ro_fp);
                            fwrite((char*)&dd, sizeof(double), 1, ro_fp);
                        }
                    }
                }
            }
        }
    }
    else {
        for (int i = 0; i < ro_length; i++) {
            fprintf(ro_fp, " %d", indx >= 0 ? indx : i);
            for (sDvList *dl = ro_dlist; dl; dl = dl->dl_next) {
                sDataVec *v = dl->dl_dvec;
                if (v) {
                    if (i < v->length()) {
                        if (ro_realflag)
                            fprintf(ro_fp,
                                "\t%.*e\n", ro_prec, v->realval(i));
                        else if (v->isreal())
                            fprintf(ro_fp, "\t%.*e,0.0\n",
                                ro_prec, v->realval(i));
                        else
                            fprintf(ro_fp, "\t%.*e,%.*e\n",
                                ro_prec, v->realval(i),
                                ro_prec, v->imagval(i));
                    }
                    else if (ro_pad) {
                        if (ro_realflag)
                            fprintf(ro_fp, "\t%.*e\n", ro_prec, 0.0);
                        else
                            fprintf(ro_fp, "\t%.*e,%.*e\n",
                                ro_prec, 0.0, ro_prec, 0.0);
                    }
                }
            }
        }
    }
    return (true);
}


// Fill in the point count field.
//
bool
cRawOut::file_update_pcnt(int pointCount)
{
    if (!ro_fp || ro_fp == stdout)
        return (true);
    fflush(ro_fp);
    long place = ftell(ro_fp);
    fseek(ro_fp, ro_pointPosn, SEEK_SET);
    fprintf(ro_fp, "%d", pointCount);
    fseek(ro_fp, place, SEEK_SET);
    fflush(ro_fp);
    return (true);
}


// Close the file.
//
bool
cRawOut::file_close()
{
    sDvList::destroy(ro_dlist);
    ro_dlist = 0;
    if (ro_fp && ro_fp != stdout && !ro_no_close)
        fclose(ro_fp);
    ro_fp = 0;
    return (true);
}
// End of cRawOut functions.


// Read a raw file.  Returns a list of plot structures.  This function
// should be very flexible about what it expects to see in the
// rawfile.  Really all we require is that there be one variables and
// one values section per plot and that the variables precede the
// values.


cRawIn::cRawIn()
{
    ri_fp       = 0;
    ri_title    = 0;
    ri_date     = 0;
    ri_plots    = 0;
    ri_flags    = 0;
    ri_nvars    = 0;
    ri_npoints  = 0;
    ri_numdims  = 0;
    memset(ri_dims, 0, MAXDIMS*sizeof(int));
    ri_padded   = true;
}


cRawIn::~cRawIn()
{
    if (ri_fp && ri_fp != stdin)
        fclose(ri_fp);
    delete [] ri_title;
    delete [] ri_date;
}


namespace {

    inline void
    skip(char **s)
    {
        char *t = *s;
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
cRawIn::raw_read(const char *name)
{
    ri_fp = Sp.PathOpen(name, "rb");
    if (!ri_fp) {
        GRpkg::self()->Perror(name);
        return (0);
    }

    OP.pushPlot();
    TTY.ioPush();
    CP.PushControl();

    sPlot *curpl = 0;
    char buf[BSIZE_SP];
    while (fgets(buf, BSIZE_SP, ri_fp)) {
        char buf2[BSIZE_SP];
        // Figure out what this line is...
        if (lstring::ciprefix("title:", buf)) {
            char *s = buf;
            skip(&s);
            nonl(s);
            delete [] ri_title;
            ri_title = lstring::copy(s);
        }
        else if (lstring::ciprefix("date:", buf)) {
            char *s = buf;
            skip(&s);
            nonl(s);
            delete [] ri_date;
            ri_date = lstring::copy(s);
        }
        else if (lstring::ciprefix("plotname:", buf)) {
            char *s = buf;
            skip(&s);
            nonl(s);
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
            curpl = new sPlot(0);
            curpl->set_next_plot(ri_plots);
            ri_plots = curpl;
            curpl->set_name(s);
            if (!ri_date)
                ri_date = lstring::copy(datestring());
            curpl->set_date(ri_date);
            if (!ri_title)
                ri_title = lstring::copy("default title");
            curpl->set_title(ri_title);
            ri_flags = 0;
            ri_nvars = ri_npoints = 0;
            ri_numdims = 0;
            memset(ri_dims, 0, MAXDIMS*sizeof(int));
            ri_padded = true;
        }
        else if (lstring::ciprefix("flags:", buf)) {
            char *s = buf;
            skip(&s);
            while (lstring::copytok(buf2, &s)) {
                if (lstring::cieq(buf2, "real"))
                    ;
                else if (lstring::cieq(buf2, "complex"))
                    ri_flags |= VF_COMPLEX;
                else if (lstring::cieq(buf2, "unpadded"))
                    ri_padded = false;
                else if (lstring::cieq(buf2, "padded"))
                    ri_padded = true;
                else {
                    GRpkg::self()->ErrPrintf(ET_WARN,
                        "unknown flag %s.\n", buf2);
                }
            }
        }
        else if (lstring::ciprefix("no. variables:", buf)) {
            char *s = buf;
            skip(&s);
            skip(&s);
            ri_nvars = lstring::scannum(s);
        }
        else if (lstring::ciprefix("no. points:", buf)) {
            char *s = buf;
            skip(&s);
            skip(&s);
            ri_npoints = lstring::scannum(s);
        }
        else if (lstring::ciprefix("dimensions:", buf)) {
            char *s = buf;
            skip(&s);
            if (sDataVec::atodims(s, ri_dims, &ri_numdims)) {
                // Something's wrong
                GRpkg::self()->ErrPrintf(ET_WARN,
                    "syntax error in dimensions, ignored.\n");
                ri_numdims = 0;
                continue;
            }
            if (ri_numdims > MAXDIMS) {
                ri_numdims = 0;
                continue;
            }
            // Let's just make sure that the no. of points
            // and the dimensions are consistent.
            //
            int i, j;
            for (j = 0, i = 1; j < ri_numdims; j++)
                i *= ri_dims[j];

            if (ri_npoints && i != ri_npoints) {
                GRpkg::self()->ErrPrintf(ET_WARN,
                "dimensions inconsistent with number of points, ignored.\n");
                ri_numdims = 0;
            }
        }
        else if (lstring::ciprefix("command:", buf)) {
            // Note that we reverse these commands eventually...
            char *s = buf;
            skip(&s);
            nonl(s);
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
        else if (lstring::ciprefix("option:", buf)) {
            char *s = buf;
            skip(&s);
            nonl(s);
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
        else if (lstring::ciprefix("variables:", buf)) {
            // We reverse the dvec list eventually...
            if (!curpl) {
                GRpkg::self()->ErrPrintf(ET_ERROR, "no plot name given.\n");
                ri_plots = 0;
                break;
            }
            char *s = buf;
            skip(&s);
            if (!*s) {
                fgets(buf, BSIZE_SP, ri_fp);
                s = buf;
            }
            if (ri_numdims == 0) {
                ri_numdims = 1;
                ri_dims[0] = ri_npoints;
            }
            // Now read all the variable lines in
            for (int i = 0; i < ri_nvars; i++) {
                sDataVec *v = new sDataVec;
                v->set_next(curpl->tempvecs());
                curpl->set_tempvecs(v);
                if (!curpl->scale())
                    curpl->set_scale(v);
                v->set_flags(ri_flags);
                v->set_plot(curpl);

                v->set_length(0);
                v->set_numdims(0);
                // Length and dims might be changed by options.

                if (!i)
                    curpl->set_scale(v);
                else {
                    fgets(buf, BSIZE_SP, ri_fp);
                    s = buf;
                }
                lstring::advtok(&s);  // The index field
                char *t;
                if ((t = lstring::gettok(&s)) != 0) {
                    v->set_name(t);
                    delete [] t;
                }
                else {
                    GRpkg::self()->ErrPrintf(ET_ERROR,
                        "bad variable line %s.\n", buf);
                }
                char *olds = s;
                t = lstring::gettok(&s); // The units
                if (t) {
                    if (!strchr(t, '='))
                        v->ncunits()->set(t);
                    else
                        s = olds;
                    delete [] t;
                }
                
                // Fix the name...
                if (isdigit(*v->name())) {
                    snprintf(buf2, sizeof(buf2), "v(%s)", v->name());
                    v->set_name(buf2);
                }
                // Now come the strange options...
                while (lstring::copytok(buf2,&s)) {
                    if (lstring::ciprefix("min=", buf2)) {
                        double ms;
                        if (sscanf(buf2 + 4, "%lf", &ms) != 1) {
                            GRpkg::self()->ErrPrintf(ET_ERROR, "bad arg %s.\n",
                                buf2);
                        }
                        else {
                            v->set_minsignal(ms);
                            v->set_flags(v->flags() | VF_MINGIVEN);
                        }
                    }
                    else if (lstring::ciprefix("max=", buf2)) {
                        double ms;
                        if (sscanf(buf2 + 4, "%lf", &ms) != 1) {
                            GRpkg::self()->ErrPrintf(ET_ERROR, "bad arg %s.\n",
                                buf2);
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
                        v->set_gridtype((GridType) lstring::scannum(buf2 + 5));
                    }
                    else if (lstring::ciprefix("plot=", buf2)) {
                        v->set_plottype((PlotType) lstring::scannum(buf2 + 5));
                    }
                    else if (lstring::ciprefix("dims=", buf2)) {
                        v->fixdims(buf2 + 5);
                    }
                    else {
                        GRpkg::self()->ErrPrintf(ET_WARN,
                            "bad var param %s.\n", buf2);
                    }
                }

                // Now we default any missing dimensions
                if (!v->numdims()) {
                    v->set_numdims(ri_numdims);
                    for (int j = 0; j < ri_numdims; j++)
                        v->set_dims(j, ri_dims[j]);
                }
                // And allocate the data array. We would use
                // the desired vector length, but this would
                // be dangerous if the file is invalid.

                if (ri_npoints) {
                    if (v->isreal())
                        v->set_realvec(new double[ri_npoints]);
                    else
                        v->set_compvec(new complex[ri_npoints]);
                    v->set_allocated(ri_npoints);
                }
                else {
                    if (v->isreal())
                        v->set_realvec(new double[SIZE_INCR]);
                    else
                        v->set_compvec(new complex[SIZE_INCR]);
                    v->set_allocated(SIZE_INCR);
                }
            }
        }
        else if (lstring::ciprefix("values:", buf) || 
                lstring::ciprefix("binary:", buf)) {
            if (!curpl) {
                GRpkg::self()->ErrPrintf(ET_ERROR, "no plot name given.\n");
                ri_plots = 0;
                break;
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
            read_data((*buf == 'v' || *buf == 'V') ? false : true, curpl);
        }
        else {
            char *s = buf;
            skip(&s);
            if (*s) {
                GRpkg::self()->ErrPrintf(ET_ERROR,
                    "strange line in rawfile -- load aborted.\n");
                while (ri_plots) {
                    curpl = ri_plots->next_plot();
                    delete ri_plots;
                    ri_plots = curpl;
                }
                CP.PopControl();
                TTY.ioPop();
                OP.popPlot();
                fclose(ri_fp);
                ri_fp = 0;
                return (0);
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
    fclose(ri_fp);
    ri_fp = 0;

    // Reverse the plots list, so will be in file order.
    curpl = ri_plots;
    ri_plots = 0;
    while (curpl) {
        sPlot *pl = curpl;
        curpl = curpl->next_plot();
        pl->set_next_plot(ri_plots);
        ri_plots = pl;
    }

    // make the vectors permanent, and fix dimension
    for (curpl = ri_plots; curpl; curpl = curpl->next_plot()) {
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
    return (ri_plots);
}


void
cRawIn::read_data(bool isbin, sPlot *curpl)
{
    const char *errbr = "bad rawfile, missing data.\n";
    if (isbin) {
        // Binary file
        for (;;) {
            for (sDataVec *v = curpl->tempvecs(); v; v = v->next()) {
                double val1, val2 = 0.0;
                if (v->isreal()) {
                    if (fread((char*)&val1, sizeof(double), 1, ri_fp) != 1) {
                        if (v != curpl->tempvecs())
                            GRpkg::self()->ErrPrintf(ET_ERROR, errbr);
                        return;
                    }
                }
                else {
                    if (fread((char*)&val1, sizeof (double), 1, ri_fp) != 1) {
                        GRpkg::self()->ErrPrintf(ET_ERROR, errbr);
                        return;
                    }
                    if (fread((char*)&val2, sizeof (double), 1, ri_fp) != 1) {
                        GRpkg::self()->ErrPrintf(ET_ERROR, errbr);
                        return;
                    }
                }
                v->add_point(&val1, &val2, SIZE_INCR);
            }
        }
    }
    else {
        // It's an ASCII file
        for (;;) {
            int j;
            if (fscanf(ri_fp, " %d", &j) != 1)
                return;;
            for (sDataVec *v = curpl->tempvecs(); v; v = v->next()) {
                double val1, val2 = 0.0;
                if (v->isreal()) {
                    if (fscanf(ri_fp, " %lf", &val1) != 1) {
                        GRpkg::self()->ErrPrintf(ET_ERROR, errbr);
                        return;
                    }
                }
                else {
                    if (fscanf(ri_fp, " %lf, %lf", &val1, &val2) != 2) {
                        GRpkg::self()->ErrPrintf(ET_ERROR, errbr);
                        return;
                    }
                }
                v->add_point(&val1, &val2, SIZE_INCR);
            }
        }
    }
}
// End of cRawIn functions.

