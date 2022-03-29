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
#include <math.h>

#define DEBUG


// Create the data for a new fit table for temperature mt_res_temp by
// interpolating from the presently stored tables.
//
void
mmjco_mtdb::new_tab()
{
    const double *tvals = mt_temps;
    double *fvals = new double[mt_ntemps];

    delete [] mt_res_data;
    mt_res_data = new double[mt_nterms*6];

    int nvals = mt_nterms*6;
    for (int i = 0; i < nvals; i++) {
        for (int j = 0; j < mt_ntemps; j++) {
            fvals[j] = mt_data[j*nvals + i];
        }
        const double *fv = fvals;
        mmjco_tscale ts(mt_ntemps, tvals, fv);
        mt_res_data[i] = ts.value(mt_res_temp);
    }
    delete [] fvals;
}


// Create the gap parameters for a new fit table for temperature t by
// interpolating from the presently stored deltas.
//
void
mmjco_mtdb::new_dels()
{
    const double *tvals = mt_temps;
    double *fvals = new double[mt_ntemps*2];

    delete [] mt_res_dels;
    mt_res_dels = new double[2];

    int nvals = 2;
    for (int i = 0; i < nvals; i++) {
        for (int j = 0; j < mt_ntemps; j++) {
            fvals[j] = mt_dels[j*nvals + i];
        }
        const double *fv = fvals;
        mmjco_tscale ts(mt_ntemps, tvals, fv);
        mt_res_dels[i] = ts.value(mt_res_temp);
    }
    delete [] fvals;
}


// Load a fit table or sweep file.  The file consists of concatenated
// fit tables for different temperatures.  The data of each fit set is
// printed before fit data, and is parsed to obtain parameters.  For
// sweep files, the first line of the file contains the keyword
// "tsweep" followed by the number of fit sets, the minimum
// temperature, delta temperature, smoothing factor, number of TCA
// points, dimension of fit parameter set, and fit threshold.  A point
// table contains the keyword "tpoint" followed by the number of fit
// sets, the first temperature, the last temperature, and additional
// parameters as for sweep.
//
// The file pointer is already pointing to the first record to read,
// and the number of records to read is passed.
//
bool
mmjco_mtdb::load(FILE *fp, int ntemps, int nterms)
{
    const char *s;
    char buf[256];
    double temp = 0.0;
    double *data = 0;
    double *dp = 0;
    double d1 = 0.0, d2 = 0.0;
    int ix = 0;

    while ((s = fgets(buf, 256, fp)) != 0) {
        while (isspace(*s))
            s++;
        if (!*s)
            continue;

        // New format: print values, easy to parse.
        if (!strncmp(s, "tcafit", 6)) {
            if (sscanf(s+7, "%lf %lf %lf", &temp, &d1, &d2) != 3) {
                fprintf(stderr, "Error: wrong record separator token count.\n");
                return (false);
            }
            if (ix == 0) {
                if (!setup(ntemps, nterms))
                    return (false);
                data = new double[nterms*6];
            }

            dp = data;
            ix++;
            continue;
        }

        if (!dp) {
            fprintf(stderr, "Error: file is corrupt.\n");
            return (false);
        }
        if (sscanf(buf, "%lf, %lf, %lf, %lf, %lf, %lf,", dp, dp+1, dp+2, dp+3,
                dp+4, dp+5) != 6) {
            fprintf(stderr, "Error: read failed in sweep file.\n");
            return (false);
        }
        dp += 6;
        if ((dp - data)/6 == nterms) {
            if (!add_table(buf, temp, d1, d2, ix-1, data)) {
                fprintf(stderr, "Error: failed to add fit set.\n");
                return (false);
            }
            if (ix == ntemps)
                break;
        }
    }
    delete [] data;
    return (true);
}


// Write the parameters to a ".fit" file.
//
bool
mmjco_mtdb::dump_file(const char *fname)
{
    if (!fname) {
        fprintf(stderr, "Error: null filename pointer.\n");
        return (false);
    }
    if (!mt_res_data || !mt_res_dels) {
        fprintf(stderr, "Error: null data pointer.\n");
        return (false);
    }

    FILE *fp = fopen(fname, "w");
    if (fp) {
        fprintf(fp, "tcafit %11.4e %11.4e %11.4e %.3f %-4d %-2d %.3f\n",
            mt_res_temp, mt_res_dels[0], mt_res_dels[1], mt_sm, mt_xp,
            mt_nterms, mt_thr);
        const double *dp = mt_res_data;
        for (int i = 0; i < mt_nterms; i++) {
            fprintf(fp, "%12.5e,%12.5e,%12.5e,%12.5e,%12.5e,%12.5e\n",
                dp[0], dp[1], dp[2], dp[3], dp[4], dp[5]);
            dp += 6;
        }
        fclose(fp);
    }
    else {
        fprintf(stderr, "Warning: could not write %s.\n", fname);
        return (false);
    }
    return (true);
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

