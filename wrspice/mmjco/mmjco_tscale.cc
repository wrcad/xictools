//=================================================================
// Code to create a table of tca-fit sets at various temperatures,
// and to create interpolated fit sets for arbitrary temperature.
//
// Stephen R. Whiteley, wrcad.com,  Synopsys, Inc.  2/25/2022
//=================================================================
// Released under Apache License, Version 2.0.
//   http://www.apache.org/licenses/LICENSE-2.0
//

#include "mmjco_tscale.h"

#define DEBUG

// Create the data for a new fit table for temperature t via
// interpolating from the presently stored tables.
//
double * 
mmjco_mtdb::new_tab(double t)
{
    const double *tvals = mt_temps;
    double *fvals = new double[mt_ntemps];
    double *data = new double[mt_nterms*6];

    int nvals = mt_nterms*6;
    for (int i = 0; i < nvals; i++) {
        for (int j = 0; j < mt_ntemps; j++) {
            fvals[j] = mt_data[j*nvals + i];
        }
        const double *fv = fvals;
        mmjco_tscale ts(mt_ntemps, tvals, fv);
        data[i] = ts.value(t);
    }
    return (data);
}


// Load a fit table file.  The file consists of concatenated fit tables
// for different temperatures.  The "filename" of each fit set is printed
// before fit data, and is parsed to obtain parameters.  The first line
// of the file contains the keyword "tsweep" followed by the number of
// fit sets (and temperatures).
//
bool
mmjco_mtdb::load(FILE *fp)
{
    const char *s;
    char buf[256];
    char tbf[16];
    int ntemps = 0;
    int nterms = 0;
    double temp = 0.0;
    double *data = 0;
    double *dp = 0;
    int ix = 0;

    while ((s = fgets(buf, 256, fp)) != 0) {
        while (isspace(*s))
            s++;
        if (!*s)
            continue;
        if (!strncmp(s, "tsweep", 6)) {
            s += 7;
            ntemps = atoi(s);
            continue;
        }
        if (!strncmp(s, "tca", 3)) {
            s += 3;
            char *t = tbf;
            *t++ = *s++;
            *t++ = *s++;
            *t++ = *s++;
            *t++ = *s++;
            *t++ = *s++;
            *t = 0;
            temp = atof(tbf)*1e-3;
#ifdef DEBUG
            printf("tm = %g\n", temp);
#endif
            t = tbf;
            *t++ = *s++;
            *t++ = *s++;
            *t++ = *s++;
            *t++ = *s++;
            *t++ = *s++;
            *t = 0;
#ifdef DEBUG
            double d1 = atof(tbf)*1e-4;
            printf("d1 = %g\n", d1);
#endif
            t = tbf;
            *t++ = *s++;
            *t++ = *s++;
            *t++ = *s++;
            *t++ = *s++;
            *t++ = *s++;
            *t = 0;
#ifdef DEBUG
            double d2 = atof(tbf)*1e-4;
            printf("d2 = %g\n", d2);
#endif
            t = tbf;
            *t++ = *s++;
            *t++ = *s++;
            *t = 0;
            if (ix == 0)
                mt_sm = atof(tbf)*1e-3;
#ifdef DEBUG
            printf("sm = %g\n", mt_sm);
#endif
            t = tbf;
            *t++ = *s++;
            *t++ = *s++;
            *t++ = *s++;
            *t++ = *s++;
            *t = 0;
            if (ix == 0)
                mt_xp = atoi(tbf);
#ifdef DEBUG
            printf("xp = %d\n", mt_xp);
#endif
            s++;  // -
            t = tbf;
            *t++ = *s++;
            *t++ = *s++;
            *t = 0;
            if (ix == 0) {
                nterms = atoi(tbf);
                if (!setup(ntemps, nterms))
                    return (0);
                data = new double[nterms*6];
            }
#ifdef DEBUG
            printf("nt = %d\n", nterms);
#endif
            t = tbf;
            *t++ = *s++;
            *t++ = *s++;
            *t++ = *s++;
            *t = 0;
            if (ix == 0)
                mt_thr = atof(tbf)*1e-3;
#ifdef DEBUG
            printf("th = %g\n", mt_thr);
#endif
            dp = data;
            ix++;
            continue;
        }
        if (!dp) {
            fprintf(stderr, "Error: file is corrupt.\n");
            return (0);
        }
        if (sscanf(buf, "%lf, %lf, %lf, %lf, %lf, %lf,", dp, dp+1, dp+2, dp+3,
                dp+4, dp+5) != 6) {
            fprintf(stderr, "Error: read failed in sweep file.\n");
            return (0);
        }
        dp += 6;
        if ((dp - data)/6 == nterms) {
            if (!add_table(buf, temp, ix-1, data)) {
                fprintf(stderr, "Error: failed to add fit set.\n");
                return (0);
            }
        }
    }
    delete [] data;
    return (1);
}


#ifdef STANDALONE

int main(int argc, char **argv)
{
    FILE *fp = fopen("sweepfile", "r");
    if (!fp)
        return (1);
    mmjco_mtdb mt;
    if (!mt.load(fp))
        return (1);

    char buf[80];
    for (;;) {
        fgets(buf, 80, stdin);
        double t = atof(buf);

        double *data = mt.new_tab(t);

        double *dp = data;
        for (int i = 0; i < mt.num_terms(); i++) {
            fprintf(stdout, "%10.6f,%10.6f,%10.6f,%10.6f,%10.6f,%10.6f\n",
                dp[0], dp[1], dp[2], dp[3], dp[4], dp[5]);
            dp += 6;
        }
    }
    fclose(fp);
}

#endif

