//==============================================
// Main function for mmjco program.
// S. R. Whiteley, wrcad.com,  Synopsys, Inc
//==============================================
//
// License:  GNU General Public License Version 3m 29 June 2007.

#include "mmjco.h"
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#ifdef WRSPICE
#include "../../xt_base/include/miscutil/pathlist.h"
// This brings in lstring.h.
#endif


namespace {
    void gslhdlr(const char*, const char*, int, int)
    {
    }
}

enum DFTYPE { DFDATA, DFRAWCPLX, DFRAWREAL };

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
            mmc_sm          = 0.0;
            mmc_datafile    = 0;
            mmc_tcadir      = 0;

            mmc_nterms      = 0;
            mmc_thr         = 0.0;
            mmc_fitfile     = 0;

            mmc_pair_model  = 0;
            mmc_qp_model    = 0;

            gsl_set_error_handler(&gslhdlr);

#ifdef WRSPICE
            char *home = pathlist::get_home();
            if (home) {
                sLstr lstr;
                lstr.add(home);
                delete [] home;
                lstr.add("/.mmjco");
                char *av[2];
                av[1] = lstr.string_trim();
                mm_set_dir(2, av);
                delete [] av[1];
            }
#endif
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

    int mm_set_dir(int, char**);
    int mm_create_data(int, char**);
    int mm_create_fit(int, char**);
    int mm_create_model(int, char**);
    int mm_load_data(int, char**);
    int mm_load_fit(int, char**);

private:
    void save_data(const char*, FILE*, DFTYPE, const double*,
        const complex<double>*, const complex<double>*, int);

    // create_data context
    complex<double> *mmc_pair_data;
    complex<double> *mmc_qp_data;
    double *mmc_xpts;
    int mmc_numxpts;
    double mmc_temp;
    double mmc_d1;
    double mmc_d2;
    double mmc_sm;
    const char *mmc_datafile;
    const char *mmc_tcadir;

    // create_fit context
    int mmc_nterms;
    double mmc_thr;
    const char *mmc_fitfile;
    mmjco_fit mmc_mf;

    // create_model context
    complex<double> *mmc_pair_model;
    complex<double> *mmc_qp_model;
};


// Set the directory path for default input/output.
int
mmjco_cmds::mm_set_dir(int argc, char **argv)
{
    if (argc < 2) {
        printf("Error: missing directory path.\n");
        return (1);
    }
    const char *dir = argv[1];
    struct stat st;
    if (stat(dir, &st) < 0) {
        mkdir(dir, 0777);
        if (stat(dir, &st) < 0) {
            printf("Error: cannot stat path.\n");
            return (1);
        }
    }
    if (!(st.st_mode & S_IFDIR)) {
        printf("Error: path is not a directory.\n");
        return (1);
    }
    delete [] mmc_tcadir;
    char *bf = new char[strlen(dir) + 1];
    strcpy(bf, dir);
    mmc_tcadir = bf;
    return (0);
}


// Create a TCA data set and write this to a file.  The data set is
// saved in an internal data register, replacing any existing data.
//
// cd[ata] [-t temp] [-d|-d1|-d2 delta] [-s smooth] [-x nx] -f [filename] \
//  [-r | -rr | -rd]
//
int
mmjco_cmds::mm_create_data(int argc, char **argv)
{
    double temp = 4.2;
    double d1 = 1.4;
    double d2 = 1.4;
    double sm = 0.008;
    int nx  = 500;
    char *datafile = 0;
#ifdef WRSPICE
    // Use real-valued rawfile for WaveView.
    DFTYPE dtype = DFRAWREAL;
#else
    DFTYPE dtype = DFDATA;
#endif

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
                    sm = (a >= 0.001 ? a : 0.0);
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
            if (argv[i][1] == 'r') {
                if (argv[i][2] == 'r')
                    dtype = DFRAWREAL;
                else if (argv[i][2] == 'd')
                    dtype = DFDATA;
                else
                    dtype = DFRAWCPLX;
                continue;
            }
        }
    }

    // Setup TCA computation.
    mmjco m(temp, d1, d2, sm);

    delete [] mmc_pair_data;
    delete [] mmc_qp_data;
    delete [] mmc_xpts;
    mmc_temp = temp;
    mmc_d1 = d1;
    mmc_d2 = d2;
    mmc_sm = sm;
    mmc_numxpts = nx;
    mmc_pair_data = new complex<double>[mmc_numxpts];
    mmc_qp_data = new complex<double>[mmc_numxpts];
    mmc_xpts = new double[mmc_numxpts];

    // Compute TCA data.
    double x0 = 0.001;
    double dx = (2.0 - x0)/(mmc_numxpts-1);
    for (int i = 0; i < mmc_numxpts; i++) {
        if (sm > 0.0) {
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

    const char *filename = datafile;
    if (!datafile) {
        char *dp;
        if (mmc_tcadir) {
            datafile = new char[strlen(mmc_tcadir) + 32];
            sprintf(datafile, "%s/", mmc_tcadir);
            dp = datafile + strlen(datafile);
        }
        else {
            datafile = new char[32];
            dp = datafile;
        }
        filename = dp;
        sprintf(dp, "tca%03d%03d%03d%02d%04d",
            (int)(mmc_temp*100), (int)(mmc_d1*100), (int)(mmc_d2*100),
            (int)(mmc_sm*1000), mmc_numxpts);
        if (dtype == DFDATA)
            strcat(datafile, ".data");
        else
            strcat(datafile, ".raw");
    }
    FILE *fp = fopen(datafile, "w");
    if (!fp) {
        printf("Error: unable to open file %s.\n", filename);
        return (1);
    }
    save_data(filename, fp, dtype, mmc_xpts, mmc_pair_data, mmc_qp_data,
        mmc_numxpts);
    fclose(fp);
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
        char *tbuf;
        if (mmc_datafile) {
            tbuf = new char[strlen(mmc_datafile) + 12];
            strcpy(tbuf, mmc_datafile);
            char *t = strrchr(tbuf, '.');
            if (t && t > tbuf)
                *t = 0;
        }
        else {
            if (mmc_tcadir) {
                tbuf = new char[strlen(mmc_tcadir) + 20];
                sprintf(tbuf, "%s/", mmc_tcadir);
            }
            else {
                tbuf = new char[20];
                *tbuf = 0;
            }
            strcat(tbuf, "tca_data");
        }
        
        sprintf(tbuf+strlen(tbuf), "-%02d%03d.fit", mmc_nterms,
            (int)(mmc_thr*1000));
        fitfile = tbuf;
    }
    mmc_mf.save_fit_parameters(fitfile);
    delete [] mmc_fitfile;
    mmc_fitfile = fitfile;
    return (0);
}


// cm [-h thr] [-f [filename]] [-r | -rr | -rd]
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
#ifdef WRSPICE
    DFTYPE dtype = DFRAWCPLX;
#else
    DFTYPE dtype = DFDATA;
#endif

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
            if (argv[i][1] == 'r') {
                if (argv[i][2] == 'r')
                    dtype = DFRAWREAL;
                else if (argv[i][2] == 'd')
                    dtype = DFDATA;
                else
                    dtype = DFRAWCPLX;
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
        FILE *fp = fopen(modfile, "w");
        if (!fp) {
            printf("Error: unable to open file %s.\n", modfile);
            return (1);
        }
        save_data(modfile, fp, dtype, mmc_xpts, mmc_pair_model, mmc_qp_model,
            mmc_numxpts);
        fclose(fp);
        delete [] modfile;
    }
    return (0);
}


namespace {
    // Return true if the string represents an absolute path.
    //
    bool
    is_rooted(const char *string)
    {
        if (!string || !*string)
            return (false);
        if (*string == '/')
            return (true);
        if (*string == '~')
            return (true);
#ifdef WIN32
        if (*string == '\\')
            return (true);
        if (strlen(string) >= 3 && isalpha(string[0]) && string[1] == ':' &&
                (string[2] == '/' || string[2] == '\\'))
            return (true);
#endif
        return (false);
    }
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

    FILE *fp;
    char *path = 0;
    if (is_rooted(argv[1]) || !mmc_tcadir) {
        fp = fopen(argv[1], "r");
        if (!fp) {
            printf("Error: unable to open file %s.\n", argv[1]);
            return (1);
        }
    }
    else {
        path = new char[strlen(mmc_tcadir) + strlen(argv[1]) + 2];
        sprintf(path, "%s/%s", mmc_tcadir, argv[1]);
        fp = fopen(path, "r");
        if (!fp) {
            delete [] path;
            printf("Error: unable to open file %s.\n", argv[1]);
            return (1);
        }
    }
    char buf[256];

    // Read first line, determines file type.
    if (fgets(buf, 256, fp) == 0) {
        printf("Error: file is empty or unreadable.\n");
        fclose(fp);
        return (1);
    }

    if (!strcmp(buf, "Title: mmjco")) {
        // Rawfile format for TCA data.

        double T, d1, d2, sm = 0.0;
        while (fgets(buf, 256, fp) != 0) {
            if (sscanf(buf, "Plotname: T=%lf d1=%lf d2=%lf sm=%lf", &T,
                    &d1, &d2, &sm) == 4)
                break;
        }
        if (sm == 0.0) {
            printf("Error: bad or missing parameters.\n");
            fclose(fp);
            return (1);
        }

        bool iscplx = true;
        while (fgets(buf, 256, fp) != 0) {
            int d;
            if (sscanf(buf, "Flags: %c", &d) == 1)
                break;
            if (d == 'r' || d == 'R')
                iscplx = false;
        }

        int npts = 0;
        while (fgets(buf, 256, fp) != 0) {
            if (sscanf(buf, "No. Points: %d", &npts) == 1)
                break;
        }
        if (npts < 2) {
            printf("Error: too few points.\n");
            fclose(fp);
            return (1);
        }

        bool fvals = false;
        while (fgets(buf, 256, fp) != 0) {
            if (!strcmp(buf, "Values:")) {
                fvals = true;
                break;
            }
        }
        if (!fvals) {
            printf("Error: no valid data in file.\n");
            fclose(fp);
            return (1);
        }

        delete [] mmc_pair_data;
        delete [] mmc_qp_data;
        delete [] mmc_xpts;

        int ix = 0;
        int state = 0;
        if (iscplx) {
            while (fgets(buf, 256, fp) != 0) {
                if (state == 0) {
                    if (sscanf(buf, "%d %lf", &ix, mmc_xpts + ix) != 2)
                        break;
                    state++;
                    continue;
                }
                if (state == 1) {
                    double pr, pi;
                    if (sscanf(buf, "%lf %lf", &pr, &pi) != 2)
                        break;
                    mmc_pair_data[ix].real(pr);
                    mmc_pair_data[ix].imag(pi);
                    state++;
                    continue;
                }
                if (state == 1) {
                    double pr, pi;
                    if (sscanf(buf, "%lf %lf", &pr, &pi) != 2)
                        break;
                    mmc_qp_data[ix].real(pr);
                    mmc_qp_data[ix].imag(pi);
                    state = 0;
                    continue;
                }
            }
        }
        else {
            while (fgets(buf, 256, fp) != 0) {
                if (state == 0) {
                    if (sscanf(buf, "%d %lf", &ix, mmc_xpts + ix) != 2)
                        break;
                    state++;
                    continue;
                }
                if (state == 1) {
                    double d;
                    if (sscanf(buf, "%lf", &d) != 1)
                        break;
                    mmc_pair_data[ix].real(d);
                    state++;
                    continue;
                }
                if (state == 2) {
                    double d;
                    if (sscanf(buf, "%lf", &d) != 1)
                        break;
                    mmc_pair_data[ix].imag(d);
                    state++;
                    continue;
                }
                if (state == 3) {
                    double d;
                    if (sscanf(buf, "%lf", &d) != 1)
                        break;
                    mmc_qp_data[ix].real(d);
                    state++;
                    continue;
                }
                if (state == 4) {
                    double d;
                    if (sscanf(buf, "%lf", &d) != 1)
                        break;
                    mmc_qp_data[ix].imag(d);
                    state = 0;
                    continue;
                }
            }
        }
        if (ix != npts-1) {
            printf("Error: problem reading data.\n");
            fclose(fp);
            return (1);
        }

        mmc_numxpts = npts;

        mmc_temp = T;
        mmc_d1 = d1;
        mmc_d2 = d2;
        mmc_sm = sm;
    }
    else {

        // Generic TCA format.
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
        mmc_sm = 0.0;
    }

    if (!path) {
        path = new char[strlen(argv[1]) + 1];
        strcpy(path, argv[1]);
    }
    delete [] mmc_datafile;
    mmc_datafile = path;
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

    FILE *fp;
    char *path = 0;
    if (is_rooted(argv[1]) || !mmc_tcadir) {
        fp = fopen(argv[1], "r");
        if (!fp) {
            printf("Error: unable to open file %s.\n", argv[1]);
            return (1);
        }
    }
    else {
        path = new char[strlen(mmc_tcadir) + strlen(argv[1]) + 2];
        sprintf(path, "%s/%s", mmc_tcadir, argv[1]);
        fp = fopen(path, "r");
        if (!fp) {
            delete [] path;
            printf("Error: unable to open file %s.\n", argv[1]);
            return (1);
        }
    }

    mmc_nterms = 0;
    mmc_thr = 0.0;
    delete [] mmc_fitfile;
    mmc_fitfile = path;

    mmc_mf.load_fit_parameters(argv[1], fp);
    mmc_nterms = mmc_mf.numterms();
    return (0);
}


namespace {
    // Return the date. Return value is static data.
    //
    char *datestring()
    {
#define HAVE_GETTIMEOFDAY
#ifdef HAVE_GETTIMEOFDAY
        struct timeval tv;
        gettimeofday(&tv, 0);
        struct tm *tp = localtime((time_t *) &tv.tv_sec);
        char *ap = asctime(tp);
#else
        time_t tloc;
        time(&tloc);
        struct tm *tp = localtime(&tloc);
        char *ap = asctime(tp);
#endif

        static char tbuf[40];
        strcpy(tbuf, ap ? ap : "");
        int i = strlen(tbuf);
        tbuf[i - 1] = '\0';
        return (tbuf);
    }
}


// Save the TCA data in a file.
//
void
mmjco_cmds::save_data(const char *filename, FILE *fp, DFTYPE dtype,
    const double *x, const complex<double> *Jpair_data,
    const complex<double> *Jqp_data, int xsz)
{
    if (dtype == DFDATA) {
        // Save in generic format.
        fprintf(fp,
          "#    X            Jpair_real   Jpair_imag   Jqp_real     Jqp_imag\n");
        for (int i = 0; i < xsz; i++) {
            fprintf(fp, "%-4d %-12.5e %-12.5e %-12.5e %-12.5e %-12.5e\n",
                i, x[i],
                Jpair_data[i].real(), Jpair_data[i].imag(),
                Jqp_data[i].real(), Jqp_data[i].imag());
        }
        printf("TCA data saved to file %s.\n", filename);
    }
    else {
        // Save in rawfile format.
        char tbf[80];
        sprintf(tbf, "T=%.2f d1=%.2f d2=%.2f sm=%.3f", mmc_temp, mmc_d1, mmc_d2,
            mmc_sm);

        fprintf(fp, "Title: mmjco\n");
        fprintf(fp, "Date: %s\n", datestring());
        fprintf(fp, "Plotname: %s\n", tbf);
        if (dtype == DFRAWCPLX) {
            fprintf(fp, "Flags: complex\n");
            fprintf(fp, "No. Variables: 3\n");
            fprintf(fp, "No. Points: %d\n", xsz);
            fprintf(fp, "Variables:\n 0 none X\n 1 Jpair\n 2 Jqp\n");
            fprintf(fp, "Values:\n");
            for (int i = 0; i < xsz; i++) {
                fprintf(fp,
                  " %d\t%12.5e,%12.5e\n\t%12.5e,%12.5e\n\t%12.5e,%12.5e\n",
                    i, x[i], 0.0, Jpair_data[i].real(), Jpair_data[i].imag(),
                    Jqp_data[i].real(), Jqp_data[i].imag());
            }
            printf("TCA data saved to complex rawfile %s.\n", filename);
        }
        else {
            fprintf(fp, "Flags: real\n");
            fprintf(fp, "No. Variables: 5\n");
            fprintf(fp, "No. Points: %d\n", xsz);
            fprintf(fp, "Variables:\n 0 none X\n 1 Jpair_re\n 2 "
                "Jpair_im\n 3 Jqp_re\n 4 Jqp_im\n");
            fprintf(fp, "Values:\n");
            for (int i = 0; i < xsz; i++) {
                fprintf(fp,
                  " %d\t%12.5e\n\t%12.5e\n\t%12.5e\n\t%12.5e\n\t%12.5e\n",
                    i, x[i], Jpair_data[i].real(), Jpair_data[i].imag(),
                    Jqp_data[i].real(), Jqp_data[i].imag());
            }
            printf("TCA data saved to real rawfile %s.\n", filename);
        }
    }
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
        else if (av[0][0] == 'd')
            mmc.mm_set_dir(ac, av);
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

