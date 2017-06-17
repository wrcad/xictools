
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
 $Id: multidec.cc,v 2.5 2015/06/20 03:16:39 stevew Exp $
 *========================================================================*/

/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1990 Jaijeet Roychowdury
**********/

#include <stdio.h>
#include <math.h>
#include "misc.h"
#include "spmatrix.h"
#include "ttyio.h"

#define THRSH 0.01
#define ABS_THRSH 0
#define DIAG_PIVOTING 1

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

extern void usage(char**);
extern void comments(double, double, double, double, double, double, double,
    double, char*, int, double);
extern double phi(int, double);

#ifndef M_PI
#define M_PI            3.14159265358979323846
#endif

#ifdef WIN32
int optind = 1;
char *optarg;

int
getopt(int ac, char* const *av, const char *optstring)
{
    int ochar = EOF;
    for ( ; optind < ac && av[optind]; optind++) {
        if (*av[optind] != '-')
            continue;
        char *t = strchr(optstring, av[optind][1]);
        if (!t)
            continue;
        ochar = av[optind][1];
        optind++;
        if (t[1] == ':' && optind < ac) {
            optarg = av[optind];
            optind++;
        }
        else
            optarg = 0;
    }
    return (ochar);
}
#endif

int
main (int argc, char **argv)
{
    extern char *optarg;
    int errflg=0, i, j;
    char *options;
    int num, node;

    char *name = 0;
    double l, c, ctot, r=0.0, g=0.0, k=0.0, lm=0.0, cm=0.0, len;
    unsigned gotl=0, gotc=0, gotlen=0;
    unsigned gotname = 0, gotnum = 0;
    int ch;
    while ((ch = getopt(argc, argv, "ul:c:r:g:k:n:o:x:L:")) != EOF)
        switch (ch) {
        case 'o':
            name = new char[strlen(optarg) + 1];
            strcpy(name, optarg);
            gotname = 1;
            break;
        case 'l':
            sscanf(optarg,"%lf", &l);
            gotl = 1;
            break;
        case 'c':
            sscanf(optarg,"%lf", &c);
            gotc = 1;
            break;
        case 'r':
            sscanf(optarg,"%lf", &r);
            break;
        case 'g':
            sscanf(optarg,"%lf", &g);
            break;
        case 'k':
            sscanf(optarg,"%lf", &k);
            break;
        case 'x':
            sscanf(optarg,"%lf", &cm);
            break;
        case 'L':
            sscanf(optarg,"%lf", &len);
            gotlen = 1;
            break;
        case 'n':
            sscanf(optarg,"%d", &num);
            gotnum = 1;
            break;
        case 'u':
            usage(argv);
            exit(1);
            break;
        case '?':
                errflg++;
        }
        if (errflg) {
            usage(argv);
            exit (2);
        }

        if (gotl + gotc + gotname + gotnum + gotlen < 5) {
            fprintf(stderr,
    "l, c, model_name, number_of_conductors and length must be specified.\n");
            fprintf(stderr,"%s -u for details.\n",argv[0]);
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

        for (i = 1; i <= num; i++) {
            matrix[i] = new double[num + 1];
            inverse[i] = new double[num + 1];
        }

        for (i = 1; i <= num; i++) {
            tpeigenvalues[i] = -2.0 * cos(M_PI*i/(num+1));
        }

        for (i = 1; i<= num; i++) {
            for (j = 1; j <= num; j++) {
                matrix[i][j] = phi(i-1, tpeigenvalues[j]);
            }
        }

        double *gammaj = new double[num + 1];
        for (j = 1; j<= num; j++) {
            gammaj[j] = 0.0;
            for (i = 1; i <= num; i++) {
                gammaj[j] += matrix[i][j] * matrix[i][j];
            }
            gammaj[j] = sqrt(gammaj[j]);
        }

        for (j = 1; j <= num; j++) {
            for (i = 1; i <= num; i++) {
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

            for (i = 1; i <= num; i++) {
                for (j = 1; j <= num; j++) {
                    double *elptr = othermatrix->spGetElement(i, j);
                    *elptr = matrix[i][j];
                }
            }

#ifdef DEBUG_LEVEL1
            spPrint(othermatrix,0,1,0);
#endif

            for (i = 1; i <= num; i++)
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

            for (i = 1; i <= num; i++) {
                for (j = 1; j <= num; j++) {
                    rhs[j] = (j==i ? 1.0 : 0.0);
                    irhs[j] = 0.0;
                }
                othermatrix->spSolveTransposed(rhs, solution, irhs, isolution);
                for (j = 1; j <= num;j++) {
                    inverse[i][j] = solution[j];
                }
            }

            delete [] rhs;
            delete [] solution;
        }

        // inverse = M^{-1} set up

        fprintf(stdout, "\n");
        fprintf(stdout, "* Lossy line models\n");

        options = new char[256];
        strcpy(options, "rel=1.2 nocontrol");
        for (i = 1; i <= num; i++) {
            fprintf(stdout,
    ".model mod%d_%s ltra %s r=%0.12g l=%0.12g g=%0.12g c=%0.12g len=%0.12g\n",
                i, name, options, r, l+tpeigenvalues[i]*lm, g,
                ctot-tpeigenvalues[i]*cm, len);
//i, name, options, r, l+tpeigenvalues[i]*lm, g, ctot+tpeigenvalues[i]*cm, len);
    }

    fprintf(stdout, "\n");
    fprintf(stdout,
        "* subcircuit m_%s - modal transformation network for %s\n",
        name, name);
    fprintf(stdout, ".subckt m_%s", name);
    for (i = 1; i <= 2*num; i++) {
        fprintf(stdout, " %d", i);
    }
    fprintf(stdout, "\n");
    for (j = 1; j <= num; j++)
        fprintf(stdout, "v%d %d 0 0v\n", j, j+2*num);

    for (j = 1; j <= num; j++) {
        for (i = 1; i <= num; i++) {
            fprintf(stdout, "f%d 0 %d v%d %0.12g\n",
                (j-1)*num+i, num+j, i, inverse[j][i]);
        }
    }

    node = 3*num+1;
    for (j = 1; j <= num; j++) {
        fprintf(stdout,"e%d %d %d %d 0 %0.12g\n", (j-1)*num+1,
            node, 2*num+j, num+1, matrix[j][1]);
        node++;
        for (i = 2; i < num; i++) {
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
    for (i = 1; i <= 2*num; i++) {
        fprintf(stdout, " %d", i);
    }
    fprintf(stdout, "\n");

    fprintf(stdout, "x1");
    for (i = 1; i <= num; i++)
        fprintf(stdout, " %d", i);
    for (i = 1; i <= num; i++)
        fprintf(stdout, " %d", 2*num+i);
    fprintf(stdout, " m_%s\n", name);

    for (i = 1; i <= num; i++) 
        fprintf(stdout, "o%d %d 0 %d 0 mod%d_%s\n", i, 2*num+i, 3*num+i,
            i, name);

    fprintf(stdout, "x2");
    for (i = 1; i <= num; i++)
        fprintf(stdout, " %d", num+i);
    for (i = 1; i <= num; i++)
        fprintf(stdout, " %d", 3*num+i);
    fprintf(stdout, " m_%s\n", name);

    fprintf(stdout, ".ends %s\n", name);

    delete [] tpeigenvalues;

    for (i = 1; i <= num; i++) {
        delete [] matrix[i];
        delete [] inverse[i];
    }
    delete [] matrix;
    delete [] inverse;
    delete [] name;
    delete [] options;
    return (0);
}


void
usage(char **argv)
{
fprintf(stderr,"Purpose: make subckt. for coupled lines using uncoupled simple lossy lines\n");
fprintf(stderr,"Usage: %s -l<line-inductance> -c<line-capacitance>\n",argv[0]);
fprintf(stderr,"           -r<line-resistance> -g<line-conductance> \n");
fprintf(stderr,"           -k<inductive coeff. of coupling> \n");
fprintf(stderr,"           -x<line-to-line capacitance> -o<subckt-name> \n");
fprintf(stderr,"           -n<number of conductors> -L<length> -u\n");
fprintf(stderr,"Example: %s -n4 -l9e-9 -c20e-12 -r5.3 -x5e-12 -k0.7 -otest -L5.4\n\n",argv[0]);

fprintf(stderr,"See \"Efficient Transient Simulation of Lossy Interconnect\",\n");
fprintf(stderr,"J.S. Roychowdhury and D.O. Pederson, Proc. DAC 91 for details\n");
fprintf(stderr,"\n");
fflush(stderr);
}


void
comments(double r, double l, double g, double c, double ctot, double cm,
    double lm, double k, char *name, int num, double len)
{
fprintf(stdout,"* Subcircuit %s\n",name);
fprintf(stdout,"* %s is a subcircuit that models a %d-conductor transmission line with\n",name,num);
fprintf(stdout,"* the following parameters: l=%g, c=%g, r=%g, g=%g,\n",l,c,r,g);
fprintf(stdout,"* inductive_coeff_of_coupling k=%g, inter-line capacitance cm=%g,\n",k,cm);
fprintf(stdout,"* length=%g. Derived parameters are: lm=%g, ctot=%g.\n",len,lm,ctot);
fprintf(stdout,"* \n");
fprintf(stdout,"* It is important to note that the model is a simplified one - the\n");
fprintf(stdout,"* following assumptions are made: 1. The self-inductance l, the\n");
fprintf(stdout,"* self-capacitance ctot (note: not c), the series resistance r and the\n");
fprintf(stdout,"* parallel capacitance g are the same for all lines, and 2. Each line\n");
fprintf(stdout,"* is coupled only to the two lines adjacent to it, with the same\n");
fprintf(stdout,"* coupling parameters cm and lm. The first assumption implies that edge\n");
fprintf(stdout,"* effects have to be neglected. The utility of these assumptions is\n");
fprintf(stdout,"* that they make the sL+R and sC+G matrices symmetric, tridiagonal and\n");
fprintf(stdout,"* Toeplitz, with useful consequences (see \"Efficient Transient\n");
fprintf(stdout,"* Simulation of Lossy Interconnect\", by J.S.  Roychowdhury and\n");
fprintf(stdout,"* D.O Pederson, Proc. DAC 91).\n\n");
fprintf(stdout,"* It may be noted that a symmetric two-conductor line is\n");
fprintf(stdout,"* represented accurately by this model.\n\n");
fprintf(stdout,"* Subckt node convention:\n");
fprintf(stdout,"* \n");
fprintf(stdout,"*            |--------------------------|\n");
fprintf(stdout,"*      1-----|                          |-----n+1\n");
fprintf(stdout,"*      2-----|                          |-----n+2\n");
fprintf(stdout,"*         :  |   n-wire multiconductor  |  :\n");
fprintf(stdout,"*         :  |          line            |  :\n");
fprintf(stdout,"*    n-1-----|(node 0=common gnd plane) |-----2n-1\n");
fprintf(stdout,"*      n-----|                          |-----2n\n");
fprintf(stdout,"*            |--------------------------|\n\n");
fflush(stdout);
}


double 
phi(int i, double arg)
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


void
fatal(int x)
{
    exit(x);
}

