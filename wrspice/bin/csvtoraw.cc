
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
 * printtoraw -- Utility to convert print files to rawfiles.              *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "miscutil/lstring.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>

//
// WRspice number parsing.
//

#define UNITS_CATCHAR() '#'
#define UNITS_SEPCHAR() '_'

namespace {
    // Static function.
    // Grab a units string into buf, advance the pointer.  The buf length
    // is limited to 32 chars including the null byte.
    //
    bool get_unitstr(const char **s, char *buf)
    {
        int i = 0;
        const char *t = *s;
        bool had_alpha = false;
        bool had_cat = false;
        if (isalpha(*t) || *t == UNITS_CATCHAR() || *t == UNITS_SEPCHAR()) {
            if (*t != UNITS_CATCHAR()) {
                buf[i++] = *t;
                if (*t != UNITS_SEPCHAR())
                    had_alpha = true;
            }
            for (t++; i < 31; t++) {
                if (isalpha(*t)) {
                    had_alpha = true;
                    buf[i++] = *t;
                    continue;
                }
                if (*t == UNITS_SEPCHAR()) {
                    had_alpha = false;
                    buf[i++] = *t;
                    continue;
                }
                if (had_alpha && isdigit(*t)) {
                    buf[i++] = *t;
                    continue;
                }
                if (!had_cat && *t == UNITS_CATCHAR()) {
                    had_alpha = false;
                    buf[i++] = UNITS_SEPCHAR();
                    continue;
                }
                break;
            }
            buf[i] = 0;
            *s = t;
            return (true);
        }
        return (false);
    }


    // This serves two purposes:  1) faster than calling pow(), and 2)
    // more accurate than some crappy pow() implementations.
    //
#define ETABSIZE 24
    double np_powers[] = {
        1.0e+0,
        1.0e+1,
        1.0e+2,
        1.0e+3,
        1.0e+4,
        1.0e+5,
        1.0e+6,
        1.0e+7,
        1.0e+8,
        1.0e+9,
        1.0e+10,
        1.0e+11,
        1.0e+12,
        1.0e+13,
        1.0e+14,
        1.0e+15,
        1.0e+16,
        1.0e+17,
        1.0e+18,
        1.0e+19,
        1.0e+20,
        1.0e+21,
        1.0e+22,
        1.0e+23
    };

}


// Parse a number.  This will handle things like 10M, etc.  If the
// number should not have any unrelated trailing text, then
// no_units_txt is true.  The string pointer is advanced past the
// number.  If no_units_txt is false the argument is advanced to the
// end of the word.
//
// Returns 0 if no number can be found or if there are unrecognized
// trailing characters when no_units_txt is true, otherwise returns a
// pointer to static data.
//
double *num_parse(const char **line, bool no_units_txt)
{
    double mantis = 0;
    int expo1 = 0;
    int expo2 = 0;
    int sign = 1;
    int expsgn = 1;
#define ENDCHAR(c)  (!c || isspace(c) || ispunct(c))

    static double np_num = 0.0;

    if (!line || !*line)
        return (0);
    const char *here = *line;
    if (*here == '+')
        // Plus, so do nothing except skip it.
        here++;
    if (*here == '-') {
        // Minus, so skip it, and change sign.
        here++;
        sign = -1;
    }

    // We don't want to recognise "P" as 0P, or .P as 0.0P...
    if  (*here == '\0' || (!isdigit(*here) && *here != '.') ||
            (*here == '.' && !isdigit(*(here+1)))) {
        // Number looks like just a sign!
        return (0);
    }
    while (isdigit(*here)) {
        // Digit, so accumulate it.
        mantis = 10*mantis + (*here - '0');
        here++;
    }
    if (ENDCHAR(*here)) {
        // Reached the end of token - done.
        *line = here;
        np_num = mantis*sign;
        return (&np_num);
    }
    if (*here == '.') {
        // Found a decimal point!
        here++; // skip to next character
        if (ENDCHAR(*here)) {
            // Number ends in the decimal point.
            *line = here;
            np_num = mantis*sign;
            return (&np_num);
        }
        while (isdigit(*here)) {
            // Digit, so accumulate it.
            mantis = 10*mantis + (*here - '0');
            expo1--;
            here++;
        }
        if (ENDCHAR(*here)) {
            // Reached the end of token - done.
            *line = here;
            if (-expo1 < ETABSIZE)
                np_num = sign*mantis/np_powers[-expo1];
            else
                np_num = sign*mantis*pow(10.0, (double)expo1);
            return (&np_num);
        }
    }

    // Now look for "E","e",etc to indicate an exponent.
    if (*here == 'e' || *here == 'E' || *here == 'd' || *here == 'D') {
        // Have an exponent, so skip the e.
        here++;
        // Now look for exponent sign.
        if (*here == '+')
            // just skip +
            here++;
        if (*here == '-') {
            // Skip over minus sign.
            here++;
            // Make a negative exponent.
            expsgn = -1;
        }
        // Now look for the digits of the exponent.
        while (isdigit(*here)) {
            expo2 = 10*expo2 + (*here - '0');
            here++;
        }
    }

    // Now we have all of the numeric part of the number, time to
    // look for the scale factor (alphabetic).
    //
    char ch = isupper(*here) ? tolower(*here) : *here;
    switch (ch) {
    case 'a':
        expo1 -= 18;
        here++;
        break;
    case 'f':
        expo1 -= 15;
        here++;
        break;
    case 'p':
        expo1 -= 12;
        here++;
        break;
    case 'n':
        expo1 -= 9;
        here++;
        break;
    case 'u':
        expo1 -= 6;
        here++;
        break;
    case 'm':
        // Special case for m, may be m or mil or meg.
        if ((*(here+1) == 'E' || *(here+1) == 'e') &&
                (*(here+2) == 'G' || *(here+2) == 'g')) {
            expo1 += 6;
            here += 3;
        }
        else if ((*(here+1) == 'I' || *(here+1) == 'i') &&
                (*(here+2) == 'L' || *(here+2) == 'l')) {
            expo1 -= 6;
            here += 3;
            mantis = mantis*25.4;
        }
        else {
            // Not either special case, so just m => 1e-3.
            expo1 -= 3;
            here++;
        }
        break;
    case 'k':
        expo1 += 3;
        here++;
        break;
    case 'g':
        expo1 += 9;
        here++;
        break;
    case 't':
        expo1 += 12;
        here++;
        break;
    default:
        break;
    }

    char buf[32];

    if (no_units_txt && (*here == UNITS_CATCHAR() ||
            *here == UNITS_SEPCHAR() || isalpha(*here)))
        return (0);

    get_unitstr(&here, buf);

    expo1 += expsgn*expo2;
    if (expo1 >= 0) {
        if (expo1 < ETABSIZE)
            np_num = sign*mantis*np_powers[expo1];
        else
            np_num = sign*mantis*pow(10.0, (double)expo1);
    }
    else {
        if (-expo1 < ETABSIZE)
            np_num = sign*mantis/np_powers[-expo1];
        else
            np_num = sign*mantis*pow(10.0, (double)expo1);
    }
    *line = here;
    return (&np_num);
}


// cvstoraw:  convert a space/comma separated file to a rawfile.
//
// Usage:  csvtoraw [filename]
// Output goes to standard output.  Input from the file is a name
// was given, or stdin otherwise.
//
// A "csv" (comma-separated values) file is assumed to have a form as
// described below.
// o Any lines that start with white space or a comment character
//   ahead of the header line are ignored.  Comment characters are *#!.
// o The header line contains a number of words, space and/or comma
//   separated, these are the vector names.  If they contain a comment
//   character or comma the word should be double-quoted.
// o Lines that follow are ignored if the first non-space character is
//   a comment character, comma, or the line is blank.
// o Otherwise the lines should contain the same number of numbers as
//   words in the header line.  Any number format as used in WRspice
//   is fine, numbers are separated by spaces and/or commas.
// o The leftmost logical column will be taken as the scale vector.


// Store trace data
//
struct sTrace
{
    sTrace(char *n) : name(n), points(0), data(0), idata(0), next(0) { }
    ~sTrace()
        {
            delete [] name;
            delete [] data;
            delete [] idata;
        }
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
    bool parse_csv(FILE*);
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


namespace {
    bool iscmt(char c)
    {
        return (c == '*' || c == '#' || c == '!');
    }

    bool issep(char c)
    {
        return (isspace(c) || c == ','); 
    }

    char *getline(FILE *fp)
    {
        sLstr lstr;
        int c;
        while ((c = fgetc(fp)) != EOF) {
            if (c == '\n')
                break;
            lstr.add_c(c);
        }
        return (lstr.string_trim());
    }
}


// Parse a "csv" file.
// > Any lines that start with white space or a comment character
//   ahead of the header line are ignored.  Comment characters are *#!.
// > The header line contains a number of words, space and/or comma
//   separated, these are the vector names.  If they contain a comment
//   character or comma the word should be double-quoted.
// > Lines that follow are ignored if the first non-space character is
//   a comment character, comma, or the line is blank.
// > Otherwise the lines should contain the same number of numbers as
//   words in the header line.  Any number format as used in WRspice
//   is fine, numbers are separated by spaces and/or commas.
// > The leftmost logical column will be taken as the scale vector.
//
bool
sPlot::parse_csv(FILE *fp)
{
    sTrace *te = 0;
    int nwords = 0;
    int linecnt = 0;
    for (;;) {
        char *str = getline(fp);
        if (!str)
            break;
        linecnt++;
        if (issep(*str) || iscmt(*str)) {
            delete [] str;
            continue;
        }
        const char *s = str;
        char *tok;
        while ((tok = lstring::getqtok(&s, ",")) != 0) {
            if (!te)
                te = traces = new sTrace(tok);
            else {
                te->next = new sTrace(tok);
                te = te->next;
            }
            nwords++;
        }
        if (nwords == 0)
            return (false);
        delete [] str;
        break;
    }

    // We're past the header line, nothing left but numbers.
    int vsize = 0;
    for (;;) {
top:
        char *str = getline(fp);
        if (!str)
            break;
        linecnt++;
        const char *s = str;
        while (isspace(*s))
            s++;
        if (iscmt(*s)) {
            delete str;
            continue;
        }
        te = traces;
        double *d = 0;
        while ((d = num_parse(&s, false)) != 0) {
            if (!te) {
                printf(
            "unexpected token on line %d, ignoring rest of line.\n", linecnt);
                delete [] str;
                goto top;
            }
                
            te->add_point(*d);
            te = te->next;
            while (issep(*s))
                s++;
        }
        vsize++;
    }
    if (traces == 0)
        return (false);
    printf("Found %d vectors of length %d.\n", nwords, vsize);
    title = lstring::copy("CSV Data");
    date =  lstring::copy("undated");
    plotname = lstring::copy("unnamed");
    return (true);
}


namespace {
    bool is_cycle(double *d, int cyc)
    {
        for (int i = 0; i < cyc; i++) {
            if (d[i] != d[cyc + i])
                return (false);
        }
        return (true);
    }
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
            "csvtoraw: convert comma or space-separated data to rawfile.\n"
            "Copyright (c) Whiteley Research Inc. 2024\n"
            "Usage: csvtoraw [print_file]\n\n");
        return (1);
    }
    sPlot plot;
    if (!plot.parse_csv(fp ? fp : stdin))
        return (1);
    if (fp)
        fclose(fp);
    plot.write_raw(stdout);
    return (0);
}

