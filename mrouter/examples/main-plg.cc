
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA 2016, http://wrcad.com       *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL STEPHEN R. WHITELEY OR WHITELEY     *
 *   RESEARCH INC. BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,   *
 *   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,   *
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        *
 *   DEALINGS IN THE SOFTWARE.                                            *
 *                                                                        *
 *   Licensed under the Apache License, Version 2.0 (the "License");      *
 *   you may not use this file except in compliance with the License.     *
 *   You may obtain a copy of the License at                              *
 *                                                                        *
 *        http://www.apache.org/licenses/LICENSE-2.0                      *
 *                                                                        *
 *   Unless required by applicable law or agreed to in writing, software  *
 *   distributed under the License is distributed on an "AS IS" BASIS,    *
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or      *
 *   implied. See the License for the specific language governing         *
 *   permissions and limitations under the License.                       *
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
 $Id: main-plg.cc,v 1.10 2017/02/10 05:08:35 stevew Exp $
 *========================================================================*/

#include <getopt.h>
#include <unistd.h>
#include "mrouter.h"


//
// mrplg:  Version of 'mrouter' that loads the plug-in rather than
//         compile-time linking to libmrouter.
//
// This provides an example of the application code used to
// load mrouter capability at run-time.
//
// To build, you must first build and install MRouter.  Then, make
// using the Makefile suplied in the examples.  The functionality is
// exactly the same as 'mrouter'.

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
            "usage:  mrouter [-switches] [-q design_name]\n\n"
            "switches:\n"
            "\t-c <file>\t\tConfiguration file name if not route.cfg.\n"
            "\t-v <level>\t\tVerbose output level.\n"
            "\t-i <file>\t\tPrint route names and pitches and exit.\n"
            "\t-p <name>\t\tSpecify global power bus name.\n"
            "\t-g <name>\t\tSpecify global ground bus name.\n"
            "\t-r <int>\t\tSpecify output resolution.\n"
            "\t-h \t\t\tPrint this message.\n"
            "\t-f \t\t\tForce routable.\n"
            "\t-k <int>\t\tSet number of tries..\n"
            "\t-q <design_name>\tImpersonate qrouter.\n"
            "\n";
        printf(msg, MR_VERSION);
    }
}


//*****************************************************************************
// Begin MRouter plug-in interface

// ****  EXAMPLE CODE
// Whiteley Research Inc.
// No copyright, public domain, no restrictions.
//
// This is an example of code that can be added to an application to
// load the MRouter plug-in.  It is adapted from the actual code in
// Xic, and can serve as a starting point for integratio of MRouter
// into a third-party application as a plug-in.


// You will need to include the main mrouter include file.
// #include "/usr/local/xictools/mrouter/include/mrouter.h"
#ifdef WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif
#include <string.h>
#include <stdio.h>


#ifdef notdef
// This feature is not used in the present example.

//
// This is optional, but one can implement an I/O object for the
// router, for example to redirect message output to log files.  By
// default, such messages go to standard output or standard error
// channels.
//

class cMRio : public cLDio
{
public:
    cMRio();
    virtual ~cMRio();

    void emitErrMesg(const char*);
    void flushErrMesg();
    void emitMesg(const char*);
    void flushMesg();
    void destroy();

private:
    FILE *log_fp;
    FILE *err_fp;
    int  io_num;
    static int io_cnt;
};

int cMRio::io_cnt = 0;

cMRio::cMRio()
{
    log_fp = 0;
    err_fp = 0;
    io_num = io_cnt++;
}


cMRio::~cMRio()
{
    if (log_fp && log_fp != stdout)
        fclose(log_fp);
    if (err_fp && err_fp != stderr)
        fclose(err_fp);
}


#define LOGBASE         "mrouter"

void
cMRio::emitErrMesg(const char *msg)
{
    if (!msg || !*msg)
        return;
    if (!err_fp) {
        char buf[32];
        strcpy(buf, LOGBASE);
        if (io_num > 0)
            sprintf(buf + strlen(buf), "-%d", io_num);
        strcat(buf, ".errs");
        // Open the log file by some means.
        err_fp = OpenLog(buf, "w");  // You need to implement this.
        if (!err_fp)
            err_fp = stderr;
    }
    fputs(msg, err_fp);
}

void
cMRio::flushErrMesg()
{
    if (err_fp)
        fflush(err_fp);
}

void
cMRio::emitMesg(const char *msg)
{
    if (!msg || !*msg)
        return;
    if (!log_fp) {
        char buf[32];
        strcpy(buf, LOGBASE);
        if (io_num > 0)
            sprintf(buf + strlen(buf), "-%d", io_num);
        strcat(buf, ".log");
        // Open the log file by some means.
        log_fp = OpenLog(buf, "w");  // You need to implement this.
        if (!log_fp)
            log_fp = stdout;
    }
    fputs(msg, log_fp);
}

void
cMRio::flushMesg()
{
    if (log_fp)
        fflush(log_fp);
}

void
cMRio::destroy()
{
    delete this;
}
// End of cMRio functions.
#endif  // notdef


namespace {
    // Variables used by the plug-in interface.  You will probably
    // want to put these, and the functions below, into a class.

    cLDDBif *if_l;                      // Pointer to a LDDB object.
    cMRif   *if_r;                      // Pointer to a MRouter object.
    cLDDBif*  (*if_new_db)();           // Constructor function.
    cMRif*  (*if_new_router)(cLDDBif*); // Constructor function.
    bool    if_error;                   // Error flag.
    const char *if_version = MR_VERSION; // Version string from mrouter.h.

    // We have two version strings each consisting of three integers
    // separated by periods, as <major>.<minor>.<release>.  The major
    // and minor fields must match (this represents the interface
    // level).  We allow any release number, but may want to warn if
    // there is a mismatch.  In this case there may be feature
    // differences, but nothing should crash and burn.
    //
    // Return value:
    // 0    interface mismatch or syntax error
    // 1    all components match
    // 2    release values differ, others match
    //
    int check_version(const char *v1, const char *v2)
    {
        if (!v1 || !v2)
            return (0);

        unsigned int major1, minor1, release1;
        if (sscanf(v1, "%u.%u.%u", &major1, &minor1, &release1) != 3)
            return (0); 

        unsigned int major2, minor2, release2;
        if (sscanf(v2, "%u.%u.%u", &major2, &minor2, &release2) != 3)
            return (0);

        if (major1 != major2 || minor1 != minor2)
            return (0);
        return (release1 == release2 ? 1 : 2);
    }
}


namespace {
    const char *so_sfx()
    {
#ifdef __APPLE__
        return (".dylib");
#else
#ifdef WIN32
        return (".dll");
#else
        return (".so");
#endif
#endif
    }
}


#ifdef WIN32
#define MROUTER_HOME    "\\usr\\local\\xictools\\mrouter"
#else
#define MROUTER_HOME    "/usr/local/xictools/mrouter"
#endif
#define LIBMROUTER      "libmrouter"

// Open the router plug-in and connect, if possible.  On success, the
// constructor functions are set non-null and this function returns
// true.  On failure, the error flag is set, and false is returned.
//
// The application should call this once when starting up.
//
bool openRouter()
{
    if_r = 0;
    if_l = 0;
    if_new_db = 0;
    if_new_router = 0;
    if_error = false;

    // We are silent unless a debugging flag is set in the environment.
    bool verbose = (getenv("MR_PLUGIN_DBG") != 0);
#ifdef WIN32
    HMODULE handle = 0;
#else
    void *handle = 0;
#endif

    char buf[512];
    // The user can provide a path to the plug-in through the
    // environment.
    const char *path = getenv("MROUTER_PATH");
    if (!path || !*path) {
        // Otherwise use the MRouter installation location.

        const char *mrhome = getenv("MROUTER_HOME");
        if (!mrhome)
            mrhome = MROUTER_HOME;
#ifdef WIN32
        sprintf(buf, "%s\\lib\\%s%s", mrhome, LIBMROUTER, so_sfx());
#else
        sprintf(buf, "%s/lib/%s%s", mrhome, LIBMROUTER, so_sfx());
#endif
        path = buf;
    }
#ifdef WIN32
    handle = LoadLibrary(path);
    if (!handle) {
        if (verbose) {
            int code = GetLastError();
            printf("Failed to load %s,\ncode %d.\n", path, code);
        }
        if_error = true;
        return (false);
    }
#else
    handle = dlopen(path, RTLD_LAZY | RTLD_GLOBAL);
    if (!handle) {
        if (verbose)
            printf("Failed to load %s,\n%s.\n", path, dlerror());
        if_error = true;
        return (false);;
    }
#endif

#ifdef WIN32
    // Check that the plug-in version matches our code.  The
    // ld_version_string function in the library returns this.

    const char *(*vrs)() = (const char*(*)())GetProcAddress(handle,
        "ld_version_string");
    if (!vrs) {
        if (verbose) {
            int code = GetLastError();
            printf("Failed to find ld_version_string, code %d.\n",
                code);
        }
        if_error = true;
        return (false);
    }
    int vc = check_version((*vrs)(), if_version);
    if (vc == 0) {
        if (verbose) {
            printf("fatal version mismatch: Xic=%s mrouter=%s\n",
                if_version, (*vrs)());
        }
        if_error = true;
        return (false);
    }
    if (vc == 2) {
        if (verbose) {
            printf("acceptable version mismatch: Xic=%s mrouter=%s\n",
                if_version, (*vrs)());
        }
    }

    // Now get the pointers to the constructor functions.

    cLDDBif*(*dptr)() = (cLDDBif*(*)())GetProcAddress(handle, "new_lddb");
    if (!dptr) {
        if (verbose) {
            int code = GetLastError();
            printf("Failed to find new_lddb, error code %d.\n", code);
        }
        if_error = true;
        return (false);
    }
    cMRif*(*rptr)(cLDDBif*) = (cMRif*(*)(cLDDBif*))GetProcAddress(handle,
        "new_router");
    if (!rptr) {
        if (verbose) {
            int code = GetLastError();
            printf("Failed to find new_router, error code %d.\n", code);
        }
        if_error = true;
        return (false);
    }
#else
    // Check that the plug-in version matches our code.  The
    // ld_version_string function in the library returns this.

    const char *(*vrs)() = (const char*(*)())dlsym(handle, "ld_version_string");
    if (!vrs) {
        if (verbose)
            printf("Failed to find ld_version_string: %s\n", dlerror());
        if_error = true;
        return (false);
    }
    int vc = check_version((*vrs)(), if_version);
    if (vc == 0) {
        if (verbose) {
            printf("fatal version mismatch: Xic=%s mrouter=%s\n",
                if_version, (*vrs)());
        }
        if_error = true;
        return (false);
    }
    if (vc == 2) {
        if (verbose) {
            printf("acceptable version mismatch: Xic=%s mrouter=%s\n",
                if_version, (*vrs)());
        }
    }

    // Now get the pointers to the constructor functions.

    cLDDBif*(*dptr)() = (cLDDBif*(*)())dlsym(handle, "new_lddb");
    if (!dptr) {
        if (verbose)
            printf("Failed to find new_lddb: %s\n", dlerror());
        if_error = true;
        return (false);
    }
    cMRif*(*rptr)(cLDDBif*) = (cMRif*(*)(cLDDBif*))dlsym(handle, "new_router");
    if (!rptr) {
        if (verbose)
            printf("Failed to find new_router: %s\n", dlerror());
        if_error = true;
        return (false);
    }
#endif

    // If we get here, all is well, and the application can assume use
    // of the plug-in.

    if_new_db = dptr;
    if_new_router = rptr;
    return (true);
}


// This function calls the constructor functions in the plug-in, and
// sets the pointers.  Here we assume that the application needs only
// one database and one router object, which is most likely, but not
// required by the plug-in.  Once the caller has access to these
// objects (through the if_l and if_r pointers), the entire database
// and router interface is available to the application.
//
// This can be called just after openRouter, or in response to a user
// request for router functionality.  The objects can be destroyed and
// recreated if necessary (use the standard C++ delete operator to
// destroy).
//
bool createRouter()
{
    if (if_error) {
        printf(
            "createRouter:  error flag set, can't create new router.");
        return (false);
    }
    if (!if_new_db) {
        printf(
            "createRouter:  null access function, can't create new LDDB.");
        return (false);
    }
    if (!if_new_router) {
        printf(
            "createRouter:  null access function, can't create new router.");
        return (false);
    }
    delete if_l;
    if_l = (*if_new_db)();
//    if_l->setIOhandler(new cMRio);
    delete if_r;
    if_r = (*if_new_router)(if_l);
    printf("Loading MRouter\n");
    return (true);
}

// End of MRouter plug-in interface
//*****************************************************************************


int
main(int argc, char *argv[])
{
    // This should be the only place where this function differs from
    // the standard mrouter main.
    //
    // cLDDBif *db = cLDDBif::newLDDB();
    // cMRif *mr = cMRif::newMR(db);
    if (!openRouter()) {
        fprintf(stderr, "Failed to open plug-in.\n");
        return (1);
    }
    if (!createRouter()) {
        fprintf(stderr, "Failed to load plug-in.\n");
        return (1);
    }
    cLDDBif *db = if_l;
    cMRif *mr = if_r;

    char *configfile = 0;
    char *infofile = 0;
    char *design_name = 0;
    int iscale = 1;

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
                configfile = new char[strlen(arg)+1];
                strcpy(configfile, arg);
                break;
            case 'v':
                db->setVerbose(atoi(arg));
                break;
            case 'i':
                infofile = new char[strlen(arg)+1];
                strcpy(infofile, arg);
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
                design_name = new char[strlen(arg)+1];
                strcpy(design_name, arg);
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

