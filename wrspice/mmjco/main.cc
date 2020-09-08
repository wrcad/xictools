//==============================================
// Main function for mmjco program.
// S. R. Whiteley, wrcad.com,  Synopsys, Inc
//==============================================
//
// License:  GNU General Public License Version 3m 29 June 2007.

#include "mmjco.h"
#include <string.h>


namespace {
    void gslhdlr(const char*, const char*, int, int)
    {
    }
}


class mmjco_cmds
{
public:
    mmjco_cmds()
        {
            mmc_pair_data   = 0;
            mmc_qp_data     = 0;
            mmc_xpts        = 0;
            mmc_numxpts     = 0;
            mmc_temp        = 0.0;
            mmc_d1          = 0.0;
            mmc_d2          = 0.0;
            mmc_sf          = 0.0;
            mmc_datafile    = 0;

            mmc_nterms      = 0;
            mmc_thr         = 0.0;
            mmc_fitfile     = 0;

            mmc_pair_model  = 0;
            mmc_qp_model    = 0;

            gsl_set_error_handler(&gslhdlr);
        }

    ~mmjco_cmds()
        {
            delete [] mmc_pair_data;
            delete [] mmc_qp_data;
            delete [] mmc_xpts;
            delete [] mmc_datafile;
            delete [] mmc_fitfile;
            delete [] mmc_pair_model;
            delete [] mmc_qp_model;
        }

    int mm_create_data(int, char**);
    int mm_create_fit(int, char**);
    int mm_create_model(int, char**);
    int mm_load_data(int, char**);
    int mm_load_fit(int, char**);

private:
    // create_data context
    complex<double> *mmc_pair_data;
    complex<double> *mmc_qp_data;
    double *mmc_xpts;
    int mmc_numxpts;
    double mmc_temp;
    double mmc_d1;
    double mmc_d2;
    double mmc_sf;
    const char *mmc_datafile;

    // create_fit context
    int mmc_nterms;
    double mmc_thr;
    const char *mmc_fitfile;
    mmjco_fit mmc_mf;

    // create_model context
    complex<double> *mmc_pair_model;
    complex<double> *mmc_qp_model;
};


// Create a TCA data set and write this to a file.  The data set is
// saved in an internal data register, replacing any existing data.
//
// cd[ata] [-t temp] [-d|-d1|-d2 delta] [-s smooth] [-x nx] -f [filename]
//
int
mmjco_cmds::mm_create_data(int argc, char **argv)
{
    double temp = 4.2;
    double d1 = 1.4;
    double d2 = 1.4;
    double sf = 0.008;
    int nx  = 500;
    char *datafile = 0;

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            double a;
            if (argv[i][1] == 't') {
                if (++i == argc) {
                    printf("Error: missing temperature, exiting.\n");
                    return (1);
                }
                if (sscanf(argv[i], "%lf", &a) == 1 && a >= 0.0 && a < 300.0)
                    temp = a;
                else {
                    printf("Error: bad -t (temperature), exiting.\n");
                    return (1);
                }
                continue;
            }
            if (argv[i][1] == 'd') {
                if (argv[i][2] == '1') {
                    if (++i == argc) {
                        printf("Error: missing delta, exiting.\n");
                        return (1);
                    }
                    if (sscanf(argv[i], "%lf", &a) == 1 && a > 0.0 && a < 5.0) {
                        d1 = a;
                    }
                    else {
                        printf("Error: bad -d1 (delta), exiting.\n");
                        return (1);
                    }
                    continue;
                }
                if (argv[i][2] == '2') {
                    if (++i == argc) {
                        printf("Error: missing delta, exiting.\n");
                        return (1);
                    }
                    if (sscanf(argv[i], "%lf", &a) == 1 && a > 0.0 && a < 5.0) {
                        d2 = a;
                    }
                    else {
                        printf("Error: bad -d2 (delta), exiting.\n");
                        return (1);
                    }
                    continue;
                }
                if (++i == argc) {
                    printf("Error: missing delta, exiting.\n");
                    return (1);
                }
                if (sscanf(argv[i], "%lf", &a) == 1 && a > 0.0 && a < 5.0) {
                    d1 = a;
                    d2 = a;
                }
                else {
                    printf("Error: bad -d (delta), exiting.\n");
                    return (1);
                }
                continue;
            }
            if (argv[i][1] == 's') {
                if (++i == argc) {
                    printf("Error: missing smoothing factor, exiting.\n");
                    return (1);
                }
                if (sscanf(argv[i], "%lf", &a) == 1 && a >= 0.0 && a < 0.099)
                    sf = (a >= 0.001 ? a : 0.0);
                else {
                    printf("Error: bad -s (smoothing factor), exiting.\n");
                    return (1);
                }
                continue;
            }
            if (argv[i][1] == 'x') {
                if (++i == argc) {
                    printf("Error: missing datapoints, exiting.\n");
                    return (1);
                }
                int d;
                if (sscanf(argv[i], "%d", &d) == 1 && d >= 10 && d <= 10000)
                    nx = d;
                else {
                    printf("Error: bad -x (number of data points), exiting.\n");
                    return (1);
                }
                continue;
            }
            if (argv[i][1] == 'f') {
                if (++i == argc) {
                    printf("Error: missing filename, exiting.\n");
                    return (1);
                }
                char f[256];
                if (sscanf(argv[i], "%s", &f) == 1)
                    datafile = strdup(f);
                else {
                    printf("Error: bad -f (data filename), exiting.\n");
                    return (1);
                }
                continue;
            }
        }
    }

    // Setup TCA computation.
    mmjco m(temp, d1, d2, sf);

    delete [] mmc_pair_data;
    delete [] mmc_qp_data;
    delete [] mmc_xpts;
    mmc_temp = temp;
    mmc_d1 = d1;
    mmc_d2 = d2;
    mmc_sf = sf;
    mmc_numxpts = nx;
    mmc_pair_data = new complex<double>[mmc_numxpts];
    mmc_qp_data = new complex<double>[mmc_numxpts];
    mmc_xpts = new double[mmc_numxpts];

    // Compute TCA data.
    double x0 = 0.001;
    double dx = (2.0 - x0)/(mmc_numxpts-1);
    for (int i = 0; i < mmc_numxpts; i++) {
        if (sf > 0.0) {
            mmc_pair_data[i] = m.Jpair_smooth(x0);
            mmc_qp_data[i] = m.Jqp_smooth(x0);
        }
        else {
            mmc_pair_data[i] = m.Jpair(x0);
            mmc_qp_data[i] = m.Jqp(x0);
        }
        mmc_xpts[i] = x0;
        x0 += dx;
    }

    if (!datafile) {
        datafile = new char[32];
        sprintf(datafile, "tca%03d%03d%03d%02d%04d.data",
            (int)(mmc_temp*100), (int)(mmc_d1*100), (int)(mmc_d2*100),
            (int)(mmc_sf*1000), mmc_numxpts);
    }
    m.save_data(datafile, mmc_xpts, mmc_pair_data, mmc_qp_data, mmc_numxpts);
    delete [] mmc_datafile;
    mmc_datafile = datafile;
    return (0);
}


// Create a fitting table for the existing internal TCA data and write
// this to a file.  The fitting paramers are stored internally.
//
// cf[it] [-n terms] [-h thr] [-f filename]
//
int
mmjco_cmds::mm_create_fit(int argc, char **argv)
{
    if (!mmc_numxpts) {
        printf("Error: no TCA data in memory, use \"cd\" or \"ld\".\n");
        return (1);
    }
    int nterms = 8;
    double thr = 0.2;
    char *fitfile = 0;

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            double a;
            if (argv[i][1] == 'n') {
                if (++i == argc) {
                    printf("Error: missing term count, exiting.\n");
                    return (1);
                }
                int d;
                if (sscanf(argv[i], "%d", &d) == 1 && d > 3 && d < 21 && !(d&1))
                    nterms = d;
                else {
                    printf(
                    "Error: bad -n (number of terms, 4-20 even), exiting.\n");
                    return (1);
                }
                continue;
            }
            if (argv[i][1] == 'h') {
                if (++i == argc) {
                    printf("Error: missing threshold, exiting.\n");
                    return (1);
                }
                if (sscanf(argv[i], "%lf", &a) == 1 && a >= 0.05 && a < 0.5)
                    thr = a;
                else {
                    printf("Error: bad -h (threshold), exiting.\n");
                    return (1);
                }
                continue;
            }
            if (argv[i][1] == 'f') {
                if (++i == argc) {
                    printf("Error: missing filename, exiting.\n");
                    return (1);
                }
                char f[256];
                if (sscanf(argv[i], "%s", &f) == 1)
                    fitfile = strdup(f);
                else {
                    printf("Error: bad -f (fit filename), exiting.\n");
                    return (1);
                }
                continue;
            }
        }
    }

    mmc_nterms = nterms;
    mmc_thr = thr;

    // Compute fitting parameters.
    mmc_mf.new_fit_parameters(mmc_xpts, mmc_pair_data, mmc_qp_data, mmc_numxpts,
        mmc_nterms, mmc_thr);

    if (!fitfile) {
        char buf[80];
        if (mmc_datafile) {
            strcpy(buf, mmc_datafile);
            char *t = strrchr(buf, '.');
            if (t && t > buf)
                *t = 0;
        }
        else
            strcpy(buf, "tca_data");
        
        sprintf(buf+strlen(buf), "-%02d%03d.fit", mmc_nterms,
            (int)(mmc_thr*1000));
        fitfile = new char[strlen(buf)+1];
        strcpy(fitfile, buf);
    }
    mmc_mf.save_fit_parameters(fitfile);
    delete [] mmc_fitfile;
    mmc_fitfile = fitfile;
    return (0);
}


// cm [-h thr] [-f [filename]]
int
mmjco_cmds::mm_create_model(int argc, char **argv)
{
    if (!mmc_numxpts) {
        printf("Error: no TCA data in memory, use \"cd\" or \"ld\".\n");
        return (1);
    }
    if (mmc_nterms <= 0) {
        printf("Error: no fit data in memory, use \"cf\" or \"lf\".\n");
        return (1);
    }

    double loc_thr = (mmc_thr > 0.0 ? mmc_thr : 0.2);
    bool got_f = false;
    char *modfile = 0;
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (argv[i][1] == 'h') {
                if (++i == argc) {
                    printf("Error: missing threshold, exiting.\n");
                    return (1);
                }
                double a;
                if (sscanf(argv[i], "%lf", &a) == 1 && a >= 0.05 && a < 0.5)
                    loc_thr = a;
                else {
                    printf("Error: bad -h (threshold), exiting.\n");
                    return (1);
                }
                continue;
            }
            if (argv[i][1] == 'f') {
                got_f = true;
                if (++i < argc) {
                    char f[256];
                    if (sscanf(argv[i], "%s", &f) == 1)
                        modfile = strdup(f);
                    else {
                        printf("Error: bad -f (model filename), exiting.\n");
                        return (1);
                    }
                }
                continue;
            }
        }
    }

    delete [] mmc_pair_model;
    delete [] mmc_qp_model;
    mmc_mf.tca_fit(mmc_xpts, mmc_numxpts, &mmc_pair_model, &mmc_qp_model);

    double res = mmc_mf.residual(mmc_pair_model, mmc_qp_model,
        mmc_pair_data, mmc_qp_data, mmc_numxpts, loc_thr);
    printf("Model created, residual = %g\n", res);

    if (got_f) {
        if (!modfile) {
            char buf[64];
            if (mmc_fitfile) {
                strcpy(buf, mmc_fitfile);
                char *t = strrchr(buf, '.');
                if (t && t > buf)
                    *t = 0;
            }
            else
                strcpy(buf, "tca_data");
            strcat(buf, ".model");
            modfile = new char[strlen(buf) + 1];
            strcpy(modfile, buf);
        }
        mmjco::save_data(modfile, mmc_xpts, mmc_pair_model, mmc_qp_model,
            mmc_numxpts);
        delete [] modfile;
    }
    return (0);
}


// Load the internal data registers from a data file, as produced with
// the "cd" command.  This overwrites any existing TCA data.
//
// ld filename
int
mmjco_cmds::mm_load_data(int argc, char **argv)
{
    if (argc < 2) {
        printf("Error: no filename given.\n");
        return (1);
    }
    FILE *fp = fopen(argv[1], "r");
    if (!fp) {
        printf("Error: unable to open file %s.\n", argv[1]);
        return (1);
    }
    char buf[256];
    int cnt = 0;
    while (fgets(buf, 256, fp) != 0) {
        int i;
        double x, pr, pi, qr, qi;
        if (sscanf(buf, "%d %lf %lf %lf %lf %lf",
                &i, &x, &pr, &pi, &qr, &qi) == 6)
            cnt++;
    }
    if (cnt > 2) {
        printf("Error: no valid data in file.\n");
        return (1);
    }

    delete [] mmc_pair_data;
    delete [] mmc_qp_data;
    delete [] mmc_xpts;

    rewind(fp);
    cnt = 0;
    while (fgets(buf, 256, fp) != 0) {
        int i;
        double x, pr, pi, qr, qi;
        if (sscanf(buf, "%d %lf %lf %lf %lf %lf",
                &i, &x, &pr, &pi, &qr, &qi) == 6) {
            mmc_xpts[cnt] = x;
            mmc_pair_data[cnt].real(pr);
            mmc_pair_data[cnt].imag(pi);
            mmc_qp_data[cnt].real(qr);
            mmc_qp_data[cnt].imag(qi);
            cnt++;
        }
    }
    mmc_numxpts = cnt;
    // XXX maybe want to save these in the data file?
    mmc_temp = 0.0;
    mmc_d1 = 0.0;
    mmc_d2 = 0.0;
    mmc_sf = 0.0;
    delete [] mmc_datafile;
    char *datafile = new char[strlen(argv[1]) + 1];
    strcpy(datafile, argv[1]);
    delete [] mmc_datafile;
    mmc_datafile = datafile;
    return (0);
}


// Load the internal fitting parameter register from a file as
// produced with the "cf" command, or mitmojco.
//
// lf filename
int
mmjco_cmds::mm_load_fit(int argc, char **argv)
{
    if (argc < 2) {
        printf("Error: no filename given.\n");
        return (1);
    }

    mmc_nterms = 0;
    mmc_thr = 0.0;
    delete [] mmc_fitfile;
    char *fitfile = new char[strlen(argv[1]) + 1];
    strcpy(fitfile, argv[1]);
    mmc_fitfile = fitfile;

    mmc_mf.load_fit_parameters(mmc_fitfile);
    mmc_nterms = mmc_mf.numterms();
    return (0);
}


namespace {
#define MAX_ARGC 16

    // Fill the av array with tokens from commandLine.  No allocation,
    // spaces are replaced with null characters.
    //
    void get_av(char **av, int *ac, char *commandLine)
    {
        *ac = 0;
        av[0] = 0;

        char *p2 = strtok(commandLine, " ");
        while (p2 && *ac < MAX_ARGC-1) {
            av[(*ac)++] = p2;
            p2 = strtok(0, " ");
        }
        av[*ac] = 0;
        int l = strlen(av[*ac-1]);
        av[*ac-1][l-1] = 0;
    }
}


// mmjco what args_for_what
// The first token "what" can be:
//   cd[ata]
//   cf[it]
//   cm[odel]
//   ld[ata]
//   lf[it]
//   h[elp]
//   q[uit] | e[xit]
// Remainder of the line contains arguments as expected by the functions
// above.
//
int main(int argc, char **argv)
{
    mmjco_cmds mmc;
    char buf[256];
    char *av[MAX_ARGC];
    int ac;
    for (;;) {
        printf("mmjco> ");
        fflush(stdout);
        char *s = fgets(buf, 256, stdin);
        get_av(av, &ac, s);
        if (ac < 1)
            continue;
        if (av[0][0] == 'c' && av[0][1] == 'd')
            mmc.mm_create_data(ac, av);
        else if (av[0][0] == 'c' && av[0][1] == 'f')
            mmc.mm_create_fit(ac, av);
        else if (av[0][0] == 'c' && av[0][1] == 'm')
            mmc.mm_create_model(ac, av);
        else if (av[0][0] == 'l' && av[0][1] == 'd')
            mmc.mm_load_data(ac, av);
        else if (av[0][0] == 'l' && av[0][1] == 'f')
            mmc.mm_load_fit(ac, av);
        else if (av[0][0] == 'q' || av[0][0] == 'e')
            break;
        else if (av[0][0] == 'h' || av[0][0] == 'v' || av[0][0] == '?') {
            printf("mmjco version %s\n", mmjco::version());
            printf(
"cd[ata]  [-t temp] [-d|-d1|-d2 delta] [-s smooth] [-x nx] [-f filename]\n"
"    Create TCA data, save internally and to file.\n"
"cf[it]  [-n terms] [-h thr] [-f filename]\n"
"    Create fit parameters for TCA data, save internally and to file.\n"
"cm[odel]  [-h thr] [-f [filename]]\n"
"    Create model for TCA data using fitting parameters, compute fit\n"
"    measure, optionally save to file.\n"
"ld[ata] filename\n"
"    Load internal data register from TCA data file.\n"
"lf[it] filename\n"
"    Load internal register from fit parameter file.\n"
"h[elp] | v[ersion] | ?\n"
"    Print this help.\n"
"q[uit] | e[xit]\n"
"    Exit mmjco.\n");

        }
        else
            printf("huh? unknown command.\n");
    }
    return (0);
}

