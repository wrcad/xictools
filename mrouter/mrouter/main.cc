
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
 *                                                                        *
 * LEF/DEF Database and Maze Router.                                      *
 *                                                                        *
 * Stephen R. Whiteley (stevew@wrcad.com)                                 *
 * Whiteley Research Inc. (wrcad.com)                                     *
 *                                                                        *
 * Portions adapted from Qrouter by Tim Edwards,                          *
 * (www.opencircuitdesign.com) which used code by Steve Beccue.           *
 * See original headers where applicable.                                 *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include <unistd.h>
#include "mrouter.h"

#ifndef VERSION_STR
#define VERSION_STR "unknown"
#endif
#ifndef OSNAME_STR
#define OSNAME_STR "unknown"
#endif
#ifndef ARCH_STR
#define ARCH_STR "unknown"
#endif


// 
// mrouter: Stand-alone router program.
// 
// The mrouter program by default reads commands from stdin and
// executes them interactively.  In this mode, you probably don't
// want/need program arguments, since equivalent commands can be given
// instead.  The available arguments are mostly inherited from the
// qrouter program.  If "-q design_name" is given, the program behaves
// like qrouter, i.e., it reads a config file and expects to be given
// a DEF file basename (the "design_name").  The DEF file is
// processed, and a new def file containing the physical routes is
// produced, and mrouter exits.  Without "-q design_name", no files
// are read or written, the user must explicitly give read/write
// commands.  Exit with "quit" or "exit".  The user can also supply a
// script file redirected to the standard input.  In this case the
// prompting is suppressed, and the program exits on error or end of
// file.
//

namespace {
    // help_message
    //
    // Tell user how to use the program.
    //
    void
    help_message()
    {
        const char *msg =
            "MRouter-%s  maze router program.\n\n"
            "usage:  mrouter [switches...]\n\n"
            "switches:\n"
            "\t-c <file>\t\tConfiguration file name if not route.cfg.\n"
            "\t-v <level>\t\tVerbose output level.\n"
            "\t-i <file>\t\tPrint route names and pitches and exit.\n"
            "\t-p <name>\t\tSpecify global power bus name.\n"
            "\t-g <name>\t\tSpecify global ground bus name.\n"
            "\t-r <int>\t\tSpecify output resolution.\n"
            "\t-h \t\t\tPrint this message.\n"
            "\t-f \t\t\tForce routable.\n"
            "\t-k <int>\t\tSet number of tries.\n"
            "\t-q <design_name>\tImpersonate qrouter.\n"
            "\t--v \t\t\tPrint version osname arch and exit.\n"
            "\n";
        printf(msg, MR_VERSION);
    }
}


int
main(int argc, char *argv[])
{
    cLDDBif *db = cLDDBif::newLDDB();
    cMRif *mr = cMRif::newMR(db);

    const char *configfile = 0;
    const char *infofile = 0;
    const char *design_name = 0;
    int iscale = 1;

    if (argc == 2 && !strcmp(argv[1], "--v")) {
        printf("%s %s %s\n", VERSION_STR, OSNAME_STR, ARCH_STR);
        exit (0);
    }

    for (int i = 1; i < argc; i++) {
        if (*argv[i] == '-') {
            int opt = argv[i][1];
            const char *arg = 0;

            // First grab the argument for options that have one.
            switch (opt) {
            case 'c':
            case 'i':
            case 'k':
            case 'v':
            case 'p':
            case 'g':
            case 'r':
            case 'q':
            case 'd':  // Debugging flags, undocumented
                if (argv[i][2] == '\0') {
                    i++;
                    if (i < argc)
                        arg = argv[i];
                    if (i >= argc || (arg && *arg == '-')) {
                        fprintf(stderr,
                            "Option -%c needs an argument, ignored.\n", opt);
                        continue;
                    }
                }
                else
                    arg = argv[i] + 2;
            }

            switch (opt) {
            case 'c':
                configfile = lstring::copy(arg);
                break;
            case 'v':
                db->setVerbose(atoi(arg));
                break;
            case 'i':
                infofile = lstring::copy(arg);
                break;
            case 'p':
                db->addGlobal(arg);
                break;
            case 'g':
                db->addGlobal(arg);
                break;
            case 'r':
                // This is not used presently.
                if (sscanf(arg, "%d", &iscale) != 1 || iscale < 1) {
                    fprintf(stderr, "Bad resolution scalefactor \"%s\", "
                        "integer expected.\n", arg);
                    iscale = 1;
                }
                break;
            case 'h':
                help_message();
                return (0);
            case 'f':
                mr->setForceRoutable(true);
                break;
            case 'k':
                mr->setKeepTrying(atoi(arg));
                break;
            case 'q':
                design_name = lstring::copy(arg);
                break;
            case 'd':
                db->setDebug(strtol(arg, 0, 0));
                break;
            default:
                fprintf(stderr, "Unknown option %c, ignored.\n", opt);
            }
        }
        else
            fprintf(stderr, "Unknown token %s, ignored.\n", argv[i]);
    }

    if (design_name) {
        // Act like the "qrouter" program (without TCL) from Qrouter.

        // Set internal ordering to match Qrouter.
        // User should be aware that the routing may be different when
        // not using Qrouter emulation.
        db->setDebug(db->debug() | 0x1);

        FILE *infoFILEptr = 0;
        if (infofile) {
            infoFILEptr = fopen(infofile, "w" );
            if (!infoFILEptr) {
                fprintf(stderr, "Can't open %s: ", infofile);
                perror(0);
                return (1);
            }
            delete [] infofile;
        }

        mr->readConfig(configfile, (infoFILEptr != 0));
        delete [] configfile;
     
        if (infoFILEptr) {
            db->printInfo(infoFILEptr);
            fclose(infoFILEptr);
            return (0);
        }

        char *in_def_file = new char[strlen(design_name) + 5];
        strcpy(in_def_file, design_name);
        delete [] design_name;
        char *dotptr = strrchr(in_def_file, '.');
        if (!dotptr || strcmp(dotptr, ".def"))
            strcat(in_def_file, ".def");

        char *out_def_file = new char[strlen(in_def_file) + 12];
        strcpy(out_def_file, in_def_file);
        dotptr = strrchr(out_def_file, '.');
        if (dotptr && !strcmp(dotptr, ".def"))
            strcpy(dotptr, "_route.def");
        else
            strcat(out_def_file, "_route.def");

        // Dump a lef file for fun.
        db->lefWrite("lef_out_test.lef");

        db->defRead(in_def_file);
        mr->initRouter();
        mr->setMaskVal(MASK_AUTO);
        mr->doFirstStage(false, -1);
        mr->setMaskVal(MASK_NONE);
        int result = mr->doSecondStage(false, false);
        if (result > 0 && result < 5)
            mr->doSecondStage(false, false);

        // Need to call this before writing DEF.
        mr->setupRoutePaths();

        // In Qrouter, the source DEF file is updated to add the
        // physical routes.  We do the same here.
        db->writeDefRoutes(in_def_file, out_def_file);
        
        // Alternatively, we can regenerate the DEF file instead.  The
        // result should be logically the same.
        // db->defWrite(out_def_file);

        mr->printClearFailed();
        delete [] in_def_file;
        delete [] out_def_file;
        return (0);
    }

    // If interactive, read commands from stdin until user enters
    // "quit".  If stdin is a file, break on error or EOF.

    printf("MRouter-%s\n", MR_VERSION);
    bool ret = LD_OK;
    for (;;) {
        ret = mr->readScript(stdin);
        if ((ret == LD_BAD) && isatty(fileno(stdin)))
            continue;
        break;
    }

    printf("MRouter done.\n");
    return ((ret == LD_OK) ? 0 : 1);
}

