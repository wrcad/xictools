
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
 * multidec -- Lossy transmission line decomposition tool.                *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1990 Jaijeet Roychowdury
**********/

#include <stdio.h>
#include <math.h>
#include "misc.h"
#include "ttyio.h"
#include "sparse/spmatrix.h"
#include "spnumber/spnumber.h"

#define THRSH 0.01
#define ABS_THRSH 0
#define DIAG_PIVOTING 1
#ifndef M_PI
#define M_PI            3.14159265358979323846
#endif

#undef DEBUG_LEVEL1

// Interface to the TTY.  The sparse package is built for WRspice. 
// and uses the TTYio system.  We have to supply a trivial interface.
//
bool sTTYioIF::isInteractive()          { return (false); }
int sTTYioIF::getchar()                 { return (0); }
bool sTTYioIF::getWaiting()             { return (false); }
void sTTYioIF::setWaiting(bool)         { }
wordlist *sTTYioIF::getWordlist()       { return (0); }
void sTTYioIF::stripList(wordlist*)     { }
sTTYioIF TTYif;


namespace {
    double *fparse(const char**, bool=false);

    void usage(char **argv)
    {
        fprintf(stderr,
"Purpose: make subckt. for coupled lines using uncoupled simple lossy lines\n"
"Usage: %s -l<line-inductance> -c<line-capacitance>\n"
"           -r<line-resistance> -g<line-conductance>\n"
"           -k<inductive coeff. of coupling>\n"
"           -x<line-to-line capacitance> -o<subckt-name>\n"
"           -n<number of conductors> -L<length> -u\n"
        , argv[0]);
        fprintf(stderr,
"Example: %s -n4 -l9e-9 -c20e-12 -r5.3 -x5e-12 -k0.7 -otest -L5.4\n\n"
        , argv[0]);

        fprintf(stderr,
"See \"Efficient Transient Simulation of Lossy Interconnect\",\n"
"J.S. Roychowdhury and D.O. Pederson, Proc. DAC 91 for details\n\n"
        );
        fflush(stderr);
    }


    void comments(double r, double l, double g, double c, double ctot,
        double cm, double lm, double k, const char *name, int num, double len)
    {
        fprintf(stdout, "* Subcircuit %s\n", name);
        fprintf(stdout,
"* %s is a subcircuit that models a %d-conductor transmission line with\n"
            , name, num);
        fprintf(stdout,
"* the following parameters: l=%g, c=%g, r=%g, g=%g,\n"
            , l, c, r, g);
        fprintf(stdout,
"* inductive_coeff_of_coupling k=%g, inter-line capacitance cm=%g,\n"
            , k, cm);
        fprintf(stdout,
"* length=%g. Derived parameters are: lm=%g, ctot=%g.\n"
            , len, lm, ctot);
        fprintf(stdout, "* \n");
        fprintf(stdout,
"* It is important to note that the model is a simplified one - the\n"
"* following assumptions are made: 1. The self-inductance l, the\n"
"* self-capacitance ctot (note: not c), the series resistance r and the\n"
"* parallel conductance g are the same for all lines, and 2. Each line\n"
"* is coupled only to the two lines adjacent to it, with the same\n"
"* coupling parameters cm and lm. The first assumption implies that edge\n"
"* effects have to be neglected. The utility of these assumptions is\n"
"* that they make the sL+R and sC+G matrices symmetric, tridiagonal and\n"
"* Toeplitz, with useful consequences (see \"Efficient Transient\n"
"* Simulation of Lossy Interconnect\", by J.S.  Roychowdhury and\n"
"* D.O Pederson, Proc. DAC 91).\n\n"
        );

        fprintf(stdout,
"* It may be noted that a symmetric two-conductor line is\n"
"* represented accurately by this model.\n\n"
"* Subckt node convention:\n"
"* \n"
"*            |--------------------------|\n"
"*      1-----|                          |-----n+1\n"
"*      2-----|                          |-----n+2\n"
"*         :  |   n-wire multiconductor  |  :\n"
"*         :  |          line            |  :\n"
"*    n-1-----|(node 0=common gnd plane) |-----2n-1\n"
"*      n-----|                          |-----2n\n"
"*            |--------------------------|\n\n"
        );
        fflush(stdout);
    }


    inline double phi(int i, double arg)
    {
        double  rval;
        switch (i) {
        case 0:
            rval = 1.0;
            break;
        case 1:
            rval = arg;
            break;
        default:
            rval = arg*phi(i-1,arg) - phi(i-2,arg);
        }
        return rval;
    }
}

#define CHECK(val) if (!(val)) {fprintf(stderr, \
        "Number parse error. Aborting\n\n"); exit(10); }

int
main (int argc, char **argv)
{
    extern char *optarg;

    int num=0, errflg=0;
    char *name = 0;
    double l=0.0, c, ctot, r=0.0, g=0.0, k=0.0, lm=0.0, cm=0.0, len;
    bool gotl=false, gotc=false, gotlen=false;
    bool gotname=false, gotnum=false;
    int ch;
    while ((ch = getopt(argc, argv, "ul:c:r:g:k:n:o:x:L:")) != EOF) {
        double *d;
        const char *argtok = optarg;
        switch (ch) {
        case 'o':
            name = new char[strlen(optarg) + 1];
            strcpy(name, optarg);
            gotname = true;
            break;
        case 'l':
            //sscanf(optarg,"%lf", &l);
            d = fparse(&argtok);
            CHECK(d)
            l = *d;
            gotl = true;
            break;
        case 'c':
            //sscanf(optarg,"%lf", &c);
            d = fparse(&argtok);
            CHECK(d)
            c = *d;
            gotc = true;
            break;
        case 'r':
            //sscanf(optarg,"%lf", &r);
            d = fparse(&argtok);
            CHECK(d)
            r = *d;
            break;
        case 'g':
            //sscanf(optarg,"%lf", &g);
            d = fparse(&argtok);
            CHECK(d)
            g = *d;
            break;
        case 'k':
            //sscanf(optarg,"%lf", &k);
            d = fparse(&argtok);
            CHECK(d)
            k = *d;
            break;
        case 'x':
            //sscanf(optarg,"%lf", &cm);
            d = fparse(&argtok);
            CHECK(d)
            cm = *d;
            break;
        case 'L':
            //sscanf(optarg,"%lf", &len);
            d = fparse(&argtok);
            CHECK(d)
            len = *d;
            gotlen = true;
            break;
        case 'n':
            if (sscanf(optarg,"%d", &num) == 1)
                gotnum = true;
            break;
        case 'u':
            usage(argv);
            exit(1);
            break;
        case '?':
            errflg++;
        }
    }

    if (errflg) {
        usage(argv);
        exit (2);
    }

    if (gotl + gotc + gotname + gotnum + gotlen < 5) {
        fprintf(stderr,
"l, c, model_name, number_of_conductors and length must be specified.\n");
        fprintf(stderr, "%s -u for details.\n", argv[0]);
        fflush(stdout);
        exit(1);
    }

    if ( (k<0.0?-k:k) >= 1.0 ) {
        fprintf(stderr,"Error: |k| must be less than 1.0\n");
        fflush(stderr);
        exit(1);
    }

    if (num == 1) {
        fprintf(stdout,"* single conductor line\n");
        fflush(stdout);
        exit(1);
    }

    lm = l*k;
    switch (num) {
    case 1:
        ctot = c;
        break;
    case 2:
        ctot = c + cm;
        break;
    default:
        ctot = c + 2*cm;
        break;
    }

    comments(r, l, g, c, ctot, cm, lm, k, name, num, len);

    double **matrix = new double*[num + 1];
    double **inverse = new double*[num + 1];
    double *tpeigenvalues = new double[num + 1];

    for (int i = 1; i <= num; i++) {
        matrix[i] = new double[num + 1];
        inverse[i] = new double[num + 1];
    }

    for (int i = 1; i <= num; i++) {
        tpeigenvalues[i] = -2.0 * cos(M_PI*i/(num+1));
    }

    for (int i = 1; i<= num; i++) {
        for (int j = 1; j <= num; j++) {
            matrix[i][j] = phi(i-1, tpeigenvalues[j]);
        }
    }

    double *gammaj = new double[num + 1];
    for (int j = 1; j<= num; j++) {
        gammaj[j] = 0.0;
        for (int i = 1; i <= num; i++) {
            gammaj[j] += matrix[i][j] * matrix[i][j];
        }
        gammaj[j] = sqrt(gammaj[j]);
    }

    for (int j = 1; j <= num; j++) {
        for (int i = 1; i <= num; i++) {
            matrix[i][j] /= gammaj[j];
        }
    }

    delete [] gammaj;

    // matrix = M set up

    {
        double *rhs = new double[num + 1];
        double *irhs = new double[num + 1];
        double *solution = new double[num + 1];
        double *isolution = new double[num + 1];

        spMatrixFrame *othermatrix =
            new spMatrixFrame(num, SP_NO_KLU | SP_NO_SORT);

        for (int i = 1; i <= num; i++) {
            for (int j = 1; j <= num; j++) {
                double *elptr = othermatrix->spGetElement(i, j);
                *elptr = matrix[i][j];
            }
        }

#ifdef DEBUG_LEVEL1
        spPrint(othermatrix,0,1,0);
#endif

        for (int i = 1; i <= num; i++)
            rhs[i] = 0.0;
        rhs[1] = 1.0;

        int err = othermatrix->spOrderAndFactor(rhs,
            THRSH, ABS_THRSH, DIAG_PIVOTING);
        othermatrix->spErrorMessage(stderr, 0);

        int singular_row, singular_col;
        switch (err) {
        case spNO_MEMORY:
            fprintf(stderr,"No memory in spOrderAndFactor\n");
            fflush(stderr);
            exit(1);
        case spSINGULAR:
            othermatrix->spWhereSingular(&singular_row, &singular_col);
            fprintf(stderr,
                "Singular matrix: problem in row %d and col %d\n",
                singular_row, singular_col);
            fflush(stderr);
            exit(1);
        case spSMALL_PIVOT:
            fprintf(stderr,"* Warning: matrix is illconditioned.\n");
            fflush(stderr);
            break;
        default: break;
        }

        for (int i = 1; i <= num; i++) {
            for (int j = 1; j <= num; j++) {
                rhs[j] = (j==i ? 1.0 : 0.0);
                irhs[j] = 0.0;
            }
            othermatrix->spSolveTransposed(rhs, solution, irhs, isolution);
            for (int j = 1; j <= num;j++) {
                inverse[i][j] = solution[j];
            }
        }

        delete [] rhs;
        delete [] solution;
    }

    // inverse = M^{-1} set up

    fprintf(stdout, "\n");
    fprintf(stdout, "* Lossy line models\n");

    //char *options = new char[256];
    //strcpy(options, "rel=1.2 nocontrol");
    char *options = new char[8];
    options[0] = 0;
    for (int i = 1; i <= num; i++) {
        fprintf(stdout,
".model mod%d_%s tra %s r=%0.12g l=%0.12g g=%0.12g c=%0.12g len=%0.12g\n",
            i, name, options, r, l+tpeigenvalues[i]*lm, g,
            ctot-tpeigenvalues[i]*cm, len);
    }

    fprintf(stdout, "\n");
    fprintf(stdout,
        "* subcircuit m_%s - modal transformation network for %s\n",
        name, name);
    fprintf(stdout, ".subckt m_%s", name);
    for (int i = 1; i <= 2*num; i++) {
        fprintf(stdout, " %d", i);
    }
    fprintf(stdout, "\n");
    for (int j = 1; j <= num; j++)
        fprintf(stdout, "v%d %d 0 0v\n", j, j+2*num);

    for (int j = 1; j <= num; j++) {
        for (int i = 1; i <= num; i++) {
            fprintf(stdout, "f%d 0 %d v%d %0.12g\n",
                (j-1)*num+i, num+j, i, inverse[j][i]);
        }
    }

    int node = 3*num+1;
    for (int j = 1; j <= num; j++) {
        fprintf(stdout,"e%d %d %d %d 0 %0.12g\n", (j-1)*num+1,
            node, 2*num+j, num+1, matrix[j][1]);
        node++;
        for (int i = 2; i < num; i++) {
            fprintf(stdout,"e%d %d %d %d 0 %0.12g\n", (j-1)*num+i,
                node, node-1, num+i, matrix[j][i]);
            node++;
        }
        fprintf(stdout,"e%d %d %d %d 0 %0.12g\n", j*num, j, node-1,
            2*num,matrix[j][num]);
    }
    fprintf(stdout, ".ends m_%s\n", name);

    fprintf(stdout, "\n");
    fprintf(stdout, "* Subckt %s\n", name);
    fprintf(stdout, ".subckt %s", name);
    for (int i = 1; i <= 2*num; i++) {
        fprintf(stdout, " %d", i);
    }
    fprintf(stdout, "\n");

    fprintf(stdout, "x1");
    for (int i = 1; i <= num; i++)
        fprintf(stdout, " %d", i);
    for (int i = 1; i <= num; i++)
        fprintf(stdout, " %d", 2*num+i);
    fprintf(stdout, " m_%s\n", name);

    for (int i = 1; i <= num; i++) 
        fprintf(stdout, "t%d %d 0 %d 0 mod%d_%s\n", i, 2*num+i, 3*num+i,
            i, name);

    fprintf(stdout, "x2");
    for (int i = 1; i <= num; i++)
        fprintf(stdout, " %d", num+i);
    for (int i = 1; i <= num; i++)
        fprintf(stdout, " %d", 3*num+i);
    fprintf(stdout, " m_%s\n", name);

    fprintf(stdout, ".ends %s\n", name);

    delete [] tpeigenvalues;

    for (int i = 1; i <= num; i++) {
        delete [] matrix[i];
        delete [] inverse[i];
    }
    delete [] matrix;
    delete [] inverse;
    delete [] name;
    delete [] options;
    return (0);
}


namespace {

    // This serves two purposes:  1) faster than calling pow(), and 2)
    // more accurate than some crappy pow() implementations.
    //
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
#define ETABSIZE 24

    double np_num;

#define UNITS_CATCHAR() '#'
#define UNITS_SEPCHAR() '_'

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


    // Parse a number, ripped from WRspice.
    //
    double *fparse(const char **line, bool whole)
    {
        double mantis = 0;
        int expo1 = 0;
        int expo2 = 0;
        int sign = 1;
        int expsgn = 1;

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
        if (*here == '\0') {
            // Reached the end of token - done.
            *line = here;
            np_num = mantis*sign;
            return (&np_num);
        }
        if (*here == '.') {
            // Found a decimal point!
            here++; // skip to next character
            if (*here == '\0') {
                // Number ends in the decimal point.
                *line = here;
                np_num = mantis*sign;
                return (&np_num);
            }
            while (isdigit(*here)) {
                // Digit, so accumulate it.
                mantis = 10*mantis + (*here - '0');
                expo1--;
                if (*here == '\0') {
                    // Reached the end of token - done.
                    *line = here;
                    if (-expo1 < ETABSIZE)
                        np_num = sign*mantis/np_powers[-expo1];
                    else
                        np_num = sign*mantis*pow(10.0, (double)expo1);
                    return (&np_num);
                }
                here++;
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

        if (whole && *here != '\0')
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
}

