
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA, http://wrcad.com            *
 *  Copyright (C) 2017 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
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
 * printtoraw -- Utility to convert print files to rawfiles.              *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "lstring.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>

// Store trace data
//
struct sTrace
{
    sTrace(char *n) { name = n; points = 0; data = 0; idata = 0; next = 0; }
    ~sTrace() { delete [] name; delete [] data; delete [] idata; }
    void add_point(double);
    void add_point(double, double);

    char *name;         // variable name
    int points;         // number of data points
    double *data;       // real data
    double *idata;      // imaginary data
    sTrace *next;
};


// Add a real data point to the trace
//
void
sTrace::add_point(double d)
{
    if (!(points % 16)) {
        double *nd = new double[points + 16];
        for (int i = 0; i < points; i++)
            nd[i] = data[i];
        delete [] data;
        data = nd;
    }
    data[points] = d;
    points++;
}


// Add a complex data point to the trace
//
void
sTrace::add_point(double re, double im)
{
    if (!(points % 16)) {
        double *rd = new double[points + 16];
        double *id = new double[points + 16];
        for (int i = 0; i < points; i++) {
            rd[i] = data[i];
            id[i] = idata[i];
        }
        delete [] data;
        delete [] idata;
        data = rd;
        idata = id;
    }
    data[points] = re;
    idata[points] = im;
    points++;
}


// Plot data
//
struct sPlot
{
    sPlot() { title = 0; date = 0; plotname = 0; traces = 0; }
    ~sPlot();
    bool parse(FILE*);
    void write_raw(FILE*);

    char *title;        // plot title
    char *date;         // plot date string
    char *plotname;     // plot name
    sTrace *traces;     // traces
};


sPlot::~sPlot()
{
    delete [] title;
    delete [] date;
    delete [] plotname;
    while (traces) {
        sTrace *tn = traces->next;
        delete traces;
        traces = tn;
    }
}


static bool
pfp(char *tok, double *d)
{
    if (sscanf(tok, "%le", d) != 1) {
        fprintf(stderr,
            "Error: expected floating point number, parse failed.\n");
        return (false);
    }
    return (true);
}


// Parse a WRspice print file into the plot struct, return true
// on success
//
bool
sPlot::parse(FILE *fp)
{
    char buf[1024];
    char *lastlines[4];
    lastlines[0] = 0;
    lastlines[1] = 0;
    lastlines[2] = 0;
    lastlines[3] = 0;
    sTrace *tc = 0;
    int dashcnt = 0;
    int pointcnt = 0;
    bool skipsc = false;

    while (fgets(buf, 1024, fp) != 0) {
        char *tok;
        if (lstring::prefix("----------", buf)) {
            dashcnt++;
            continue;
        }
        delete [] lastlines[3];
        lastlines[3] = lastlines[2];
        lastlines[2] = lastlines[1];
        lastlines[1] = lastlines[0];
        lastlines[0] = lstring::copy(buf);

        if (dashcnt < 2)
            continue;

        char *s = buf;
        while (isspace(*s))
            s++;
        if (!*s || !isdigit(*s))
            continue;

        int tcnt = 0;
        sTrace *te = tc;
        while ((tok = lstring::gettok(&s)) != 0) {
            tcnt++;
            if (tcnt == 1) {
                int n;
                if (sscanf(tok, "%d", &n) != 1) {
                    for (int i = 0; i < 4; i++)
                        delete [] lastlines[i];
                    fprintf(stderr,
                        "Error: expected integer, parse failed.\n");
                    return (false);
                }
                delete [] tok;
                if (n == 0) {
                    if (tc) {
                        while (te->next)
                            te = te->next;
                        tc = 0;
                        skipsc = true;
                    }
                    else {
                        char *e = lastlines[3];
                        while (isspace(*e))
                            e++;
                        if (*e)
                            title = lstring::copy(e);
                        else
                            title = lstring::copy("untitled");
                        e = title + strlen(title) - 1;
                        while (e >= title && isspace(*e))
                            *e-- = 0;

                        e = lastlines[2];
                        while (isspace(*e))
                            e++;
                        // awful way to find start of date
                        char *dt = strstr(e, "  ");
                        if (dt) {
                            *dt++ = 0;
                            while (isspace(*dt))
                                dt++;
                        }
                        if (dt && *dt)
                            date = lstring::copy(dt);
                        else
                            date = lstring::copy("undated");
                        plotname = lstring::copy(e);
                        e = date + strlen(date) - 1;
                        while (e >= date && isspace(*e))
                            *e-- = 0;
                        e = plotname + strlen(plotname) - 1;
                        while (e >= plotname && isspace(*e))
                            *e-- = 0;
                    }
                    int tcnt1 = 0;
                    char *s1 = lastlines[1];
                    while ((tok = lstring::gettok(&s1)) != 0) {
                        tcnt1++;
                        if (tcnt1 <= (traces ? 2 : 1)) {
                            delete [] tok;
                            continue;
                        }
                        if (!traces)
                            traces = te = new sTrace(tok);
                        else {
                            te->next = new sTrace(tok);
                            te = te->next;
                        }
                        if (!tc)
                            tc = te;
                    }
                    te = tc;
                    pointcnt = 0;
                }
                else if (n != pointcnt) {
                    for (int i = 0; i < 4; i++)
                        delete [] lastlines[i];
                    fprintf(stderr, "Error: index not monotonic.\n");
                    return (false);
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
                if (!pfp(tok, &re)) {
                    for (int i = 0; i < 4; i++)
                        delete [] lastlines[i];
                    return (false);
                }
                delete [] tok;
                tok = lstring::gettok(&s);
                double im;
                if (!tok || !pfp(tok, &im)) {
                    for (int i = 0; i < 4; i++)
                        delete [] lastlines[i];
                    return (false);
                }
                te->add_point(re, im);
            }
            else {
                double d;
                if (!pfp(tok, &d)) {
                    for (int i = 0; i < 4; i++)
                        delete [] lastlines[i];
                    return (false);
                }
                te->add_point(d);
            }
            delete [] tok;
            te = te->next;
            if (!te)
                break;
        }
        pointcnt++;
    }
    for (int i = 0; i < 4; i++)
        delete [] lastlines[i];
    return (true);
}


static bool
is_cycle(double *d, int cyc)
{
    for (int i = 0; i < cyc; i++) {
        if (d[i] != d[cyc + i])
            return (false);
    }
    return (true);
}


// Write out the plot data in rawfile format
//
void
sPlot::write_raw(FILE *fp)
{
    fprintf(fp, "Title: %s\n", title);
    fprintf(fp, "Date: %s\n", date);
    fprintf(fp, "Plotname: %s\n", plotname);
    bool cplx = false;
    int novars = 0;
    for (sTrace *t = traces; t; t = t->next) {
        novars++;
        if (t->idata)
            cplx = true;
    }

    fprintf(fp, "Flags: %s\n", cplx ? "complex" : "real");
    fprintf(fp, "No. Variables: %d\n", novars);

    int numpts = 0;
    for (sTrace *t = traces; t; t = t->next) {
        if (t->points > numpts)
            numpts = t->points;
    }
    for (sTrace *t = traces; t; t = t->next) {
        if (t->points < numpts) {
            double d = t->data[t->points-1];
            double *nd = new double[numpts];
            for (int i = 0; i < t->points; i++)
                nd[i] = t->data[i];
            for (int i = t->points; i < numpts; i++)
                nd[i] = d;
            delete [] t->data;
            t->data = nd;
            if (t->idata) {
                d = t->idata[t->points-1];
                nd = new double[numpts];
                for (int i = 0; i < t->points; i++)
                    nd[i] = t->idata[i];
                for (int i = t->points; i < numpts; i++)
                    nd[i] = d;
                delete [] t->idata;
                t->idata = nd;
            }
            t->points = numpts;
        }
    }
    fprintf(fp, "No. points: %d\n", numpts);

    // Try to identify dimensionality, this could stand improvement.
    // This will recognize only 2-d plots.
    if (!cplx) {
        double *d = traces[0].data;
        int np = numpts/2;
        for (int i = 1; i <= np; i++) {
            if (*d == traces[0].data[i]) {
                if (is_cycle(traces[0].data, i) && (numpts % i) == 0) {
                    fprintf(fp, "Dimensions: %d,%d\n", numpts/i, i);
                    break;
                }
            }
        }
    }

    fprintf(fp, "variables:\n");
    int cnt = 0;
    for (sTrace *t = traces; t; t = t->next) {
        fprintf(fp, " %d %s", cnt, t->name);
        // set units for some common cases
        if (lstring::cieq(t->name, "time"))
            fprintf(fp, " S");
        else if (lstring::cieq(t->name, "frequency")) {
            fprintf(fp, " Hz");
            if (t->data[0] > 0.0)
                fprintf(fp, " grid=2");  // log scale
        }
        else if (lstring::ciprefix("v(", t->name) ||
                lstring::ciprefix("vm(", t->name))
            fprintf(fp, " V");
        else if (lstring::ciprefix("i(", t->name) ||
                lstring::ciprefix("im(", t->name))
            fprintf(fp, " A");
        fprintf(fp, "\n");
        cnt++;
    }
    fprintf(fp, "Values:\n");
    for (int i = 0; i < numpts; i++) {
        if (cplx) {
            fprintf(fp, " %-8d %.15e,%.15e\n", i, traces->data[i],
                traces->idata ? traces->idata[i] : 0.0);
            for (sTrace *t = traces->next; t; t = t->next)
                fprintf(fp," %-8s %.15e,%.15e\n", " ", t->data[i],
                    t->idata ? t->idata[i] : 0.0);
        }
        else {
            fprintf(fp, " %-8d %.15e\n", i, traces->data[i]);
            for (sTrace *t = traces->next; t; t = t->next)
                fprintf(fp," %-8s %.15e\n", " ", t->data[i]);
        }
    }
}


int
main(int argc, char **argv)
{
    FILE *fp = 0;
    if (argc == 2) {
        fp = fopen(argv[1], "r");
        if (!fp) {
            perror(argv[1]);
            return (1);
        }
    }
    else if (argc > 2) {
        fprintf(stderr,
            "printtoraw: convert WRspice print output to rawfile.\n"
            "Copyright (c) Whiteley Research Inc. 2002\n"
            "Usage: printoraw [print_file]\n\n");
        return (1);
    }
    sPlot plot;
    if (!plot.parse(fp ? fp : stdin))
        return (1);
    if (fp)
        fclose(fp);
    plot.write_raw(stdout);
    return (0);
}

