//==============================================
// Main function for mmjco command loop.
// S. R. Whiteley, wrcad.com,  Synopsys, Inc
//==============================================
//
// License:  GNU General Public License Version 3m 29 June 2007.

#include "mmjco_cmds.h"
#include <stdio.h>


// Generally, mmjco enters a command prompt loop, where the following
// commands are known:
//   cd[ata]
//   cf[it]
//   cm[odel]
//   d[ir]
//   ld[ata]
//   lf[it]
//   h[elp]
//   q[uit] | e[xit]
// The remainder of the line contains arguments as expected by the
// functions above.
//
// If the first argument to the mmjco executable is "cdf", a TCA file
// and corresponding fit file are created, and mmfco exits immediately
// in this case.  Arguments following "cdf" are the same arguments
// that can follow the cd and cf interactive commands.  This mode is
// used by the TJM device model in WRspice.
//
int main(int argc, char **argv)
{
    mmjco_cmds mmc;
    char buf[256];
    char *av[MAX_ARGC];
    int ac;

    if (argc > 1) {
        if (!strcmp(argv[1], "cdf")) {
            mmc.mm_create_data(argc, argv);
            mmc.mm_create_fit(argc, argv);
            return (0);
        }
    }
    for (;;) {
        printf("mmjco> ");
        fflush(stdout);
        char *s = fgets(buf, 256, stdin);
        mmjco_cmds::get_av(av, &ac, s);
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
"cd[ata]  [-t temp] [-d|-d1|-d2 delta] [-s smooth] [-x nx] [-f filename]\\\n"
"         [-r | -rr | -rd]\n"
"    Create TCA data, save internally and to file.\n"
"cf[it]  [-n terms] [-h thr] [-ff filename]\n"
"    Create fit parameters for TCA data, save internally and to file.\n"
"cm[odel]  [-h thr] [-fm [filename]] [-r | -rr | -rd]\n"
"    Create model for TCA data using fitting parameters, compute fit\n"
"    measure, optionally save to file.\n"
"d[ir] directory_path\n"
"    Use the given directory as source and destination for TCA files.\n"
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

