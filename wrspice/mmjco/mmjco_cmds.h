//==============================================
// Command structure definitions for mmjco.
// S. R. Whiteley, wrcad.com,  Synopsys, Inc
//==============================================
//
// License:  GNU General Public License Version 3m 29 June 2007.

#ifndef MMJCO_CMDS_H
#define MMJCO_CMDS_H

#include "mmjco.h"


#define MAX_ARGC 16
enum DFTYPE { DFDATA, DFRAWCPLX, DFRAWREAL };

class mmjco_cmds
{
public:
    mmjco_cmds();
    ~mmjco_cmds();
    int mm_set_dir(int, char**);
    int mm_create_data(int, char**);
    int mm_create_fit(int, char**);
    int mm_create_model(int, char**);
    int mm_load_data(int, char**);
    int mm_load_fit(int, char**);

    static void get_av(char**, int*, char*);

private:
    void save_data(const char*, FILE*, DFTYPE, const double*,
        const complex<double>*, const complex<double>*, int);

    // create_data context
    complex<double> *mmc_pair_data;
    complex<double> *mmc_qp_data;
    double *mmc_xpts;
    int mmc_numxpts;
    DFTYPE mmc_ftype;
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

#endif
