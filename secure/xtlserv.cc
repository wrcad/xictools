
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
 * License server and authentication, and related utilities               *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef VERSION
#define VERSION "0.0.0"
#endif
#ifndef OSNAME
#define OSNAME "unknown"
#endif
#ifndef ARCH
#define ARCH "unknown"
#endif

//#define DEBUG

#include "config.h"
#include "secure.h"
#include "secure_prv.h"
#include "key.h"
#include "miscutil/encode.h"
#include "miscutil/miscutil.h"
#include "miscutil/lstring.h"
#include "miscutil/randval.h"
#include "miscutil/tvals.h"
#include "miscutil/pathlist.h"
#include "miscutil/services.h"

#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#ifdef HAVE_GETPWUID
#include <pwd.h>
#endif

#ifdef WIN32
#include "miscutil/msw.h"
#include <mapi.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#endif

// These are set as compiler -D defines
#ifndef PREFIX   
#define PREFIX "/usr/local"
#endif
#ifndef TOOLS_ROOT
#define TOOLS_ROOT "xictools"
#endif
#ifndef APP_ROOT
#define APP_ROOT "license"
#endif

#ifdef WIN32
#define CLOSESOCKET(x) shutdown(x, SD_SEND), closesocket(x)
#else
#define CLOSESOCKET(x) close(x)
#endif

#ifdef WIN32
// Tell the msw interface that we're Generation 4.
//const char *msw::MSWpkgSuffix = "-4";
#endif

// Timeout for read/write.
#define IO_TIME_MS 5000

// Seconds between purges of stale entries in jobs list.
#define CHECK_INTERVAL 300

// Seconds above which a job is considered stale.  This is time since
// last XTV_CHECK request from the application.  This should be
// somewhat more than AC_CHECK_TIME_MSEC, in seconds.  Allow some slop
// in case connection is delayed by gdb, resource contention, etc. 
// Here, we'll use the original value for backwards compatibility.  If
// serving only xic-4.2.6 or later and wrspice-4.2.2 or later, the
// value could be smaller.
// 
#define CHECK_DELTA 900

#ifndef HAVE_STRERROR
#ifndef SYS_ERRLIST_DEF
char *sys_errlist[];
#endif
#endif


namespace xtlserv {
    // Random number generator;
    randval rnd;

    // List of currently active processes.
    struct job
    {
        job() { request = 0; lastack = 0; next = 0; }

        sJobReq *request;
        unsigned long lastack;
        job *next;
    };
    job *JobList;

#ifdef WIN32
    PROCESS_INFORMATION *msw_NewProcess(const char*, unsigned int, bool,
        void* = 0, void* = 0, void* = 0);
#endif
    void get_default_dir();
    void process_opts(int, char**);
    void usage(int);
    void process_request(int);
    void ack_send(int, sJobReq*, bool, int);
    bool fill_req(sJobReq*, const char*, const char*, const char*, int, int);
    int test_me(const sJobReq*);
    int validate(sJobReq*, bool, const sJobReq*);
    dblk *find_valid(dblk*, sJobReq*);
    bool check_time(dblk*);
    bool check_users(dblk*, dblk*, sJobReq*, const sJobReq*);
    bool read_block(int, void*, size_t);
    bool write_block(int, void*, size_t);
    char *datestring();
    void errlog(const char*, ...);
    void perrlog(const char*, bool);
    void jobs_dump(int);
    void decode(const unsigned char*, char*);
    void alter_key(bool);
    void frand(unsigned char*, size_t, unsigned char*, int);
    char *hoststr(char*);
    bool set_nonblock(int, int);
    void check_log();
    void clean_list();
    void sig_hdlr(int);
    void start_timer(void);
#ifdef WIN32
    void CALLBACK alarm_hdlr(HWND, UINT, uintptr_t, DWORD);
#else
    void alarm_hdlr(int);
#endif
#ifdef MAIL_ADDR
#ifdef WIN32
    void mapi_send(const char*, const char*, const char*);
#endif
#endif
}


// The messages are encoded for security
// 0   "open request, host %s, pid %ld",
// 1   "close request, host %s, pid %ld",
// 2   "affirmed, host %s, pid %ld",
// 3   "Level %d: %s@%s.",
// 4   "purging dead job, host %s, pid %ld",
// 5   "bad code, host %s, code %d, pid %ld"
// 6   "denied host %s pid %ld",
// 7   "can't open LICENSE"
// 8   "memory allocation error"
// 9   "LICENSE read error"
// 10  "decode error"
// 11  "not licensed"
// 12  "license expired"
// 13  "user limit reached"
// 14  "unknown error"
// 15  "any_host_access"
// 16  "any_host_access_b"

namespace xtlserv {
    const unsigned char strs0[] =
    {0x7d, 0x7c, 0x73, 0x7a, 0x2c, 0x7e, 0x73, 0x7f, 0x83, 0x73, 0x81, 0x80,
     0x38, 0x2c, 0x74, 0x7d, 0x81, 0x80, 0x2c, 0x33, 0x81, 0x38, 0x2c, 0x7c,
     0x77, 0x70, 0x2c, 0x33, 0x78, 0x70, 0x0};
    const unsigned char strs1[] =
    {0x71, 0x78, 0x7d, 0x81, 0x73, 0x2c, 0x7e, 0x73, 0x7f, 0x83, 0x73, 0x81,
     0x80, 0x38, 0x2c, 0x74, 0x7d, 0x81, 0x80, 0x2c, 0x33, 0x81, 0x38, 0x2c,
     0x7c, 0x77, 0x70, 0x2c, 0x33, 0x78, 0x70, 0x0};
    const unsigned char strs2[] =
    {0x6f, 0x72, 0x72, 0x77, 0x7e, 0x7b, 0x73, 0x70, 0x38, 0x2c, 0x74, 0x7d,
     0x81, 0x80, 0x2c, 0x33, 0x81, 0x38, 0x2c, 0x7c, 0x77, 0x70, 0x2c, 0x33,
     0x78, 0x70, 0x0};
    const unsigned char strs3[] =
    {0x58, 0x73, 0x82, 0x73, 0x78, 0x2c, 0x33, 0x70, 0x46, 0x2c, 0x33, 0x81,
     0x4c, 0x33, 0x81, 0x3a, 0x16, 0x0};
    const unsigned char strs4[] =
    {0x7c, 0x83, 0x7e, 0x75, 0x77, 0x7a, 0x75, 0x2c, 0x70, 0x73, 0x6f, 0x70,
     0x2c, 0x76, 0x7d, 0x6e, 0x38, 0x2c, 0x74, 0x7d, 0x81, 0x80, 0x2c, 0x33,
     0x81, 0x38, 0x2c, 0x7c, 0x77, 0x70, 0x2c, 0x33, 0x78, 0x70, 0x0};
    const unsigned char strs5[] =
    {0x6e, 0x6f, 0x70, 0x2c, 0x71, 0x7d, 0x70, 0x73, 0x38, 0x2c, 0x74, 0x7d,
     0x81, 0x80, 0x2c, 0x33, 0x81, 0x38, 0x2c, 0x71, 0x7d, 0x70, 0x73, 0x2c,
     0x33, 0x70, 0x38, 0x2c, 0x7c, 0x77, 0x70, 0x2c, 0x33, 0x78, 0x70, 0x0};
    const unsigned char strs6[] =
    {0x70, 0x73, 0x7a, 0x77, 0x73, 0x70, 0x2c, 0x74, 0x7d, 0x81, 0x80, 0x2c,
     0x33, 0x81, 0x2c, 0x7c, 0x77, 0x70, 0x2c, 0x33, 0x78, 0x70, 0x0};
    const unsigned char strs7[] =
    {0x71, 0x6f, 0x7a, 0x35, 0x80, 0x2c, 0x7d, 0x7c, 0x73, 0x7a, 0x2c, 0x58,
     0x57, 0x51, 0x53, 0x5a, 0x61, 0x53, 0x0};
    const unsigned char strs8[] =
    {0x7b, 0x73, 0x7b, 0x7d, 0x7e, 0x87, 0x2c, 0x6f, 0x78, 0x78, 0x7d, 0x71,
     0x6f, 0x80, 0x77, 0x7d, 0x7a, 0x2c, 0x73, 0x7e, 0x7e, 0x7d, 0x7e, 0x0};
    const unsigned char strs9[] =
    {0x58, 0x57, 0x51, 0x53, 0x5a, 0x61, 0x53, 0x2c, 0x7e, 0x73, 0x6f, 0x70,
     0x2c, 0x73, 0x7e, 0x7e, 0x7d, 0x7e, 0x0};
    const unsigned char strs10[] =
    {0x70, 0x73, 0x71, 0x7d, 0x70, 0x73, 0x2c, 0x73, 0x7e, 0x7e, 0x7d, 0x7e,
     0x0};
    const unsigned char strs11[] =
    {0x7a, 0x7d, 0x80, 0x2c, 0x78, 0x77, 0x71, 0x73, 0x7a, 0x81, 0x73, 0x70,
     0x0};
    const unsigned char strs12[] =
    {0x78, 0x77, 0x71, 0x73, 0x7a, 0x81, 0x73, 0x2c, 0x73, 0x84, 0x7c, 0x77,
     0x7e, 0x73, 0x70, 0x0};
    const unsigned char strs13[] =
    {0x83, 0x81, 0x73, 0x7e, 0x2c, 0x78, 0x77, 0x7b, 0x77, 0x80, 0x2c, 0x7e,
     0x73, 0x6f, 0x71, 0x74, 0x73, 0x70, 0x0};
    const unsigned char strs14[] =
    {0x83, 0x7a, 0x79, 0x7a, 0x7d, 0x85, 0x7a, 0x2c, 0x73, 0x7e, 0x7e, 0x7d,
     0x7e, 0x0};
    const unsigned char strs15[] =
    {0x6f, 0x7a, 0x87, 0x6d, 0x74, 0x7d, 0x81, 0x80, 0x6d, 0x6f, 0x71, 0x71,
     0x73, 0x81, 0x81, 0x0};
    const unsigned char strs16[] =
    {0x6f, 0x7a, 0x87, 0x6d, 0x74, 0x7d, 0x81, 0x80, 0x6d, 0x6f, 0x71, 0x71,
     0x73, 0x81, 0x81, 0x6d, 0x6e, 0x0};

    char *default_dir;
    char *LogFileName, *LicenseDir;

    int GivenPort;

    // A simple class to handle text output.  Sometimes we want to print,
    // sometimes not.
    //
    struct sOut
    {
        void cat(const char*, ...);
        void dump(int);
        void set_silent(bool s) { Silent = s; }

    private:
        char *OutBuf;   // output buffer
        bool Silent;    // suppress terminal output
    };
    sOut out;
}


int
main(int argc, char **argv)
{
    if (argc == 2 && !strcmp(argv[1], "--v")) {
        printf("%s %s %s\n", VERSION, OSNAME, ARCH);
        exit(0);
    }

    using namespace xtlserv;
#ifdef WIN32
    FreeConsole();
    bool winbg = false;
#endif
    get_default_dir();

    // get the special args, needed before process_opts()
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-s") || !strcmp(argv[i], "-S"))
            out.set_silent(true);
#ifdef WIN32
        else if (!strcmp(argv[i], "-winbg")) {
            out.set_silent(true);
            winbg = true;
        }
#endif
    }
    out.cat(
        "XicTools license server daemon %s (C) Whiteley Research Inc. 2015\n",
        VERSION);
    if (sizeof(long) != 4)
        out.cat("Encoding for %d-byte integers\n", sizeof(long));

    // take care of command line
    process_opts(argc, argv);

    rnd.rand_seed(time(0) + getpid());
    alter_key(false);

    switch (validate(0, true, 0)) {
    case ERR_NOLIC:
        out.cat("Error: can't find any license files in %s\n", LicenseDir);
        out.dump(true);
        exit(1);
    case ERR_OK:
        break;
    default:
        out.cat("Errors found in license files, please correct.\n");
        out.dump(true);
        exit(1);
    }

    out.cat("License directory: %s\n", LicenseDir);
    out.cat("Log file: %s\n", LogFileName);

#ifdef WIN32
    // initialize winsock
    { WSADATA wsadata;
      if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0) {
        out.cat(
    "Error: WSA initialization failed, no interprocess communication.\n");
        out.dump(true);
        exit(1);
      }
    }
#endif

    servent *sp = getservbyname(XTLSERV_SERVICE, "tcp");

    short nport;
    if (GivenPort)
        nport = GivenPort;
    else if (sp)
        nport = ntohs(sp->s_port);
    else
        nport = XTLSERV_PORT;

    protoent *pp = getprotobyname("tcp");
    if (pp == 0) {
        out.cat("Error: tcp: unknown protocol.  Network setup error?\n");
        out.dump(true);
        exit(1);
    }

    // Create the socket
    int s = socket(AF_INET, SOCK_STREAM, pp->p_proto);
    if (s < 0) {
        out.cat("Failed to create socket, network setup error?\n");
        out.dump(true);
        exit(1);
    }

    // This avoids system delay in rebinding
    int on = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on));

    sockaddr_in sin;
    memset(&sin, 0, sizeof(sockaddr_in));
    sin.sin_port = htons(nport);
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    if (bind(s, (sockaddr*)&sin, sizeof(sockaddr_in)) < 0) {
        out.cat("Failed to bind socket, already in use?\n");
        out.dump(true);
        exit(1);
    }

    out.cat("Starting %s/tcp: service on port %d.\n", XTLSERV_SERVICE, nport);

    FILE *fp = fopen(LogFileName, "a");
    if (!fp) {
        out.cat("Can't open log file, server exiting.\n");
        out.dump(true);
        CLOSESOCKET(s);
        exit(1);
    }
    fclose(fp);

    // Disconnect from the controlling terminal
#ifdef WIN32
    if (!winbg) {
        char cmdline[256];
        GetModuleFileName(0, cmdline, 256);
        char *cmdstr = GetCommandLine();
        while (*cmdstr && !isspace(*cmdstr))
            cmdstr++;
        sprintf(cmdline + strlen(cmdline), "%s -winbg", cmdstr);

        // This has to be done before calling CreateProcess(), since deleting
        // the message box kills the new process.  Stupid!
        out.cat("Running...\n");
        out.dump(false);

        PROCESS_INFORMATION *info = msw_NewProcess(cmdline,
            DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP, false);
        if (!info) {
            out.cat("Couldn't execute background process, exiting.\n");
            out.dump(true);
            exit (1);
        }
        CloseHandle(info->hProcess);
        delete info;
        exit (0);
    }

#else
    out.dump(false);
    if (fork()) {
        CLOSESOCKET(s);
        _exit(0);
    }
    for (int i = 0; i < 10; i++)
        if (i != s)
            close(i);
    open("/", O_RDONLY);
    dup2(0, 1);
    dup2(0, 2);
    setsid();
#endif

    fp = fopen(LogFileName, "a");
    if (!fp) {
        out.cat("Error: can't open log file.\n");
        out.dump(true);
        exit(1);
    }
    fprintf(fp, "New license server, pid = %d, port = %d, date = %s\n",
        (int)getpid(), nport, datestring());
    fclose(fp);

    // check the job queue periodically
    start_timer();

    // handle SIGHUP (purge dead jobs)
#ifdef SIGHUP
    signal(SIGHUP, alarm_hdlr);
#endif

    // handle other signals (avoid core dumps)
    signal(SIGINT, sig_hdlr);
#ifdef SIGQUIT
    signal(SIGQUIT, sig_hdlr);
#endif
    signal(SIGILL, sig_hdlr);
#ifdef SIGTRAP
    signal(SIGTRAP, sig_hdlr);
#endif
    signal(SIGABRT, sig_hdlr);
#ifdef SIGEMT
    signal(SIGEMT, sig_hdlr);
#endif
    signal(SIGFPE, sig_hdlr);
#ifdef SIGBUS
    signal(SIGBUS, sig_hdlr);
#endif
    signal(SIGSEGV, sig_hdlr);
#ifdef SIGSYS
    signal(SIGSYS, sig_hdlr);
#endif
#ifdef SIGPIPE
    signal(SIGPIPE, sig_hdlr);
#endif
    signal(SIGTERM, sig_hdlr);

    // Start listening for requests
    listen(s, 5);

    for (;;) {
        socklen_t len = sizeof(sockaddr_in);
        sockaddr_in from;
        int g = accept(s, (sockaddr*)&from, &len);
        if (g < 0) {
            if (errno != EINTR) {
                perrlog("accept", true);
                CLOSESOCKET(s);
                exit(1);
            }
            continue;
        }
        process_request(g);
        CLOSESOCKET(g);
    }
    return (0);
}


void
xtlserv::sOut::cat(const char *fmt, ...)
{
    if (Silent)
        return;
    va_list args;
    char buf[256];
    va_start(args, fmt);
    vsnprintf(buf, 256, fmt, args);
    va_end(args);
    if (!OutBuf) {
        OutBuf = new char[strlen(buf) + 1];
        strcpy(OutBuf, buf);
        return;
    }
    char *s = new char[strlen(OutBuf) + strlen(buf) + 1];
    strcpy(s, OutBuf);
    strcat(s, buf);
    delete [] OutBuf;
    OutBuf = s;
}


void
xtlserv::sOut::dump(int error)
{
    if (Silent)
        return;
    if (OutBuf) {
#ifdef WIN32
        MessageBox(0, OutBuf, error ? "xtlserv - ERROR" : "xtlserv",
            error ? MB_ICONSTOP : MB_ICONINFORMATION);
#else
        fputs(OutBuf, error? stderr : stdout);
#endif
        delete [] OutBuf;
        OutBuf = 0;
    }
}
// End of sOut functions.


#ifdef WIN32

// Exec cmdline as a new sub-process.
//
PROCESS_INFORMATION *
xtlserv::msw_NewProcess(const char *cmdline, unsigned int flags,
    bool inherit_hdls, void *hin, void *hout, void *herr)
{
    STARTUPINFO startup;
    memset(&startup, 0, sizeof(STARTUPINFO));
    startup.cb = sizeof(STARTUPINFO);
    if (hin || hout || herr) {
        startup.dwFlags = STARTF_USESTDHANDLES;
        startup.hStdInput = hin ? hin : GetStdHandle(STD_INPUT_HANDLE);
        startup.hStdOutput = hout ? hout : GetStdHandle(STD_OUTPUT_HANDLE);
        startup.hStdError = herr ? herr : GetStdHandle(STD_ERROR_HANDLE);
    }
    PROCESS_INFORMATION *info = new PROCESS_INFORMATION;
    if (!CreateProcess(0, (char*)cmdline, 0, 0, inherit_hdls, flags, 0, 0,
            &startup, info)) {
        delete info;
        return (0);
    }
    CloseHandle(info->hThread);
    return (info);
}

#endif


void
xtlserv::get_default_dir()
{
    const char *prefix = getenv("XT_PREFIX");
    if (!prefix || !lstring::is_rooted(prefix)) {
        prefix = PREFIX;
#ifdef WIN32
        char *string = msw::GetProgramRoot("XtLserv");
        if (string) {   
            char *sp = strstr(string, TOOLS_ROOT);
            if (sp) {
                sp[-1] = 0;
                prefix = string;
            }
        }
#endif
    }
    char buf[256];
    sprintf(buf, "%s/%s/%s", prefix, TOOLS_ROOT, APP_ROOT);
    default_dir = new char[strlen(buf) + 1];
    strcpy(default_dir, buf);
}


namespace {
    // Return true if s points to a string of digits terminated by white   
    // space or null.
    //
    bool check_digits(const char *s)
    {
        if (!isdigit(*s))
            return (false);
        while (*++s) {
            if (isspace(*s))
                break;
            if (!isdigit(*s))
                return (false);
        }
        return (true);
    }
}


// Set up the file paths from the command line.
//
void
xtlserv::process_opts(int argc, char **argv)
{
    char *licdir = 0;
    char *logdir = 0;
#ifdef WIN32
    char *direc = msw::GetInstallDir("xtlserv");
#else
    char *direc = 0;
#endif

    // Process command line
    for (int i = 1; i < argc; i++) {
        if (*argv[i] == '-') {
            switch (*(argv[i]+1)) {
            case 'l':
            case 'L':
                // license file
                i++;
                if (i < argc)
                    licdir = argv[i];
                else
                    usage(1);
                break;
            case 'o':
            case 'O':
                // log file
                i++;
                if (i < argc)
                    logdir = argv[i];
                else
                    usage(1);
                break;
            case 'd':
            case 'D':
                // directory
                i++;
                if (i < argc)
                    direc = argv[i];
                else
                    usage(1);
                break;
            case 'p':
            case 'P':
                // port number
                i++;
                if (i < argc && check_digits(argv[i]))
                    GivenPort = atoi(argv[i]);
                else
                    usage(1);
                break;
            case 'h':
            case 'H':
                usage(0);
            case 's':
            case 'S':
            case 'w':
                break;
            default:
                out.cat("Unknown option -%c, aborted.\n", *(argv[i]+1));
                usage(1);
            }
        }
        else if (*argv[i] == '?')
            usage(0);
    }
    if (licdir)
        LicenseDir = lstring::copy(licdir);
    else if (direc)
        LicenseDir = lstring::copy(direc);
    else
        LicenseDir = lstring::copy(default_dir);
    if (logdir) {
        LogFileName = new char[strlen(logdir) + strlen(LOG_FILE) + 2];
        sprintf(LogFileName, "%s/%s", logdir, LOG_FILE);
    }
    else if (direc) {
        LogFileName = new char[strlen(direc) + strlen(LOG_FILE) + 2];
        sprintf(LogFileName, "%s/%s", direc, LOG_FILE);
    }
    else {
        LogFileName = new char[strlen(default_dir) + strlen(LOG_FILE) + 2];
        sprintf(LogFileName, "%s/%s", default_dir, LOG_FILE);
    }
}


// Print usage message and exit with code.
//
void
xtlserv::usage(int code)
{
    out.cat(
"xtlserv [-s][-p port][-l license_path][-o log_path][-d directory_path]\n");
    out.cat("-s:  Don't print anything to screen.\n");
    out.cat(
        "  port: Use given port number instead of default %d.\n",
        XTLSERV_PORT);
    out.cat(
        "  license_path: Path to directory containing %s (overrules -d).\n",
        AUTH_FILE);
    out.cat(
        "  log_path: Path to directory containing %s (overrules -d).\n",
        LOG_FILE);
    out.cat(
        "  directory_path: Path to directory containing %s and %s.\n",
        AUTH_FILE, LOG_FILE);
    out.cat(
        "  directory_path default: %s.\n", default_dir);
    out.dump(code);
    exit(code);
}


namespace {
    // Strip the :crap off and compare the host names, return true if
    // match case-insensitive.
    //
    bool host_comp(const char *h1, const char *h2)
    {
        while (*h1 && *h1 != ':' && *h2 && *h2 != ':') {
            char c1 = isupper(*h1) ? tolower(*h1) : *h1;
            char c2 = isupper(*h2) ? tolower(*h2) : *h2;
            if (c1 != c2)
                return (false);
            h1++;
            h2++;
        }
        return ((*h1 == 0 || *h1 == ':') && (*h2 == 0 || *h2 == ':'));
    }
}


// Dispatch code for service request.
//
void
xtlserv::process_request(int s)
{
    char buf[128];
    sJobReq c;
    if (!read_block(s, &c, sizeof(sJobReq))) {
        perrlog("read", false);
        return;
    }
    if (ntohl(c.reqtype) == XTV_OPEN) {
        decode(strs0, buf);
        errlog(buf, hoststr(c.host), ntohl(c.pid));
        int ret;
        if (ntohl(c.code) == SERVER_CODE)
            // reserved for server, someone playing games?
            ret = ERR_SRVREQ;
        else {
            ret = validate(&c, false, 0);
            if (ret == ERR_NOTLIC)
                ret = test_me(&c);
        }
        ack_send(s, &c, ret ? false : true, ret);
    }
    else if (ntohl(c.reqtype) == XTV_CHECK) {
#ifdef DEBUG
        errlog("check request, pid %ld", ntohl(c.pid));
#endif
        for (job *j = JobList; j; j = j->next) {
            if (j->request->pid == c.pid &&
                    host_comp(j->request->host, c.host)) {
                // found it
                j->request->date = time(0);
                ack_send(s, &c, true, 0);
                return;
            }
        }
        // no acknowledgement if not in list
    }
    else if (ntohl(c.reqtype) == XTV_CLOSE) {
        decode(strs1, buf);
        errlog(buf, hoststr(c.host), ntohl(c.pid));
        // remove from list
        job *jp = 0;
        for (job *j = JobList; j; j = j->next) {
            if (j->request->pid == c.pid &&
                    host_comp(j->request->host, c.host)) {
                if (jp)
                    jp->next = j->next;
                else
                    JobList = j->next;
                delete j->request;
                delete j;
                return;
            }
            jp = j;
        }
        // no acknowledgement sent
    }
    else if (ntohl(c.reqtype) == XTV_DUMP) {
        errlog("DUMP command received");
        jobs_dump(s);
    }
    else if (ntohl(c.reqtype) == XTV_KILL) {
        errlog("KILL command received (exiting)");
        CLOSESOCKET(s);
        exit(0);
    }
    else {
        // bad code, something smells
        decode(strs5, buf);
        errlog(buf, hoststr(c.host), ntohl(c.code), ntohl(c.pid));
#ifdef MAIL_ADDR
        // send mail - this could be a security concern
        char fbuf[128];
        decode(strs3, lstring::stpcpy(fbuf, "xtlserv: "));
        sprintf(buf, fbuf, ERR_BADREQ, c.user, c.host);
        miscutil::send_mail(MAIL_ADDR, "SecurityReport:ProcReq", buf);
#endif
    }
}


// Send back the encoded struct, indicating pass/fail.  This mangles the
// sJobReq struct.
//
void
xtlserv::ack_send(int s, sJobReq *c, bool ok, int retval)
{
    char buf[64];
    unsigned char sum[16], nsum[16];
    MD5cx ctx;
    ctx.update((const unsigned char*)c, sizeof(sJobReq));
    ctx.final(sum);

    if (ok) {
        if (ntohl(c->reqtype) == XTV_OPEN) {
            decode(strs2, buf);
            errlog(buf, hoststr(c->host), ntohl(c->pid));
        }
        for (int i = 0; i < 16; i++) {
            if (sum[i] < 127)
                nsum[15-i] = sum[i]+1;
            else if (sum[i] == 127)
                nsum[15-i] = 0;
            else
                nsum[15-i] = sum[i];
        }
    }
    else {
        decode(strs6, buf);
        char *t = buf + strlen(buf);
        *t++ = ',';
        *t++ = ' ';
        switch (retval) {
        case ERR_NOLIC:
            decode(strs7, t);
            break;
        case ERR_NOMEM:
            decode(strs8, t);
            break;
        case ERR_RDLIC:
            decode(strs9, t);
            break;
        case ERR_CKSUM:
            decode(strs10, t);
            break;
        case ERR_NOTLIC:
            decode(strs11, t);
            break;
        case ERR_TIMEXP:
            decode(strs12, t);
            break;
        case ERR_USRLIM:
            decode(strs13, t);
            break;
        default:
            retval = ERR_UNKNO;
            // fallthrough
        case ERR_UNKNO:
            decode(strs14, t);
            break;
        }
        errlog(buf, hoststr(c->host), ntohl(c->pid));

        for (int i = 0; i < 16; i++)
            nsum[i] = sum[i];
    }
    frand((unsigned char*)c, sizeof(sJobReq), nsum, retval);
    if (!write_block(s, c, sizeof(sJobReq)))
        perrlog("write", false);
}


// Fill the job request struct.
//
bool
xtlserv::fill_req(sJobReq *c, const char *host, const char *addr,
    const char *alt, int progcode, int mode)
{
    c->clear();

    char buf[256];
    strcpy(buf, host);
    STRIPHOST(buf);

    if (alt) {
        char *t = buf + strlen(buf);
        *t++ = ':';
        strcpy(t, alt);
#ifndef WIN32
        // Under Windows, this is the ID code, otherwise it is a HW
        // address that needs to be lower case.
        while (*t) {
            if (isupper(*t))
                *t = tolower(*t);
            t++;
        }
#endif
        strncpy(c->host, buf, 64);
        c->addr[0] = 192;
        c->addr[1] = 168;
        c->addr[2] = 0;
        c->addr[3] = 1;
    }
    else if (addr) {
        strncpy(c->host, buf, 64);
        int a0, a1, a2, a3;
        if (sscanf(addr, "%d.%d.%d.%d", &a0, &a1, &a2, &a3) != 4)
            return (false);

        c->addr[0] = a0;
        c->addr[1] = a1;
        c->addr[2] = a2;
        c->addr[3] = a3;
    }
    else {
        strncpy(c->host, buf, 64);
        c->addr[0] = 192;
        c->addr[1] = 168;
        c->addr[2] = 0;
        c->addr[3] = 1;
    }

#ifdef HAVE_GETPWUID
    {
        passwd *pw;
        pw = getpwuid(getuid());
        if (pw)
            strncpy(c->user, pw->pw_name, 64);
        else
            strncpy(c->user, "Unknown User", 64);
    }
#else
#ifdef WIN32
    {
        DWORD len = 256;
        char tbuf[256];
        if (!GetUserName(tbuf, &len))
            strcpy(tbuf, "Unknown User");
        strncpy(c->user, tbuf, 64);
    }
#else
    strncpy(c->user, "Unknown User", 64);
#endif
#endif

    c->date = htonl(time(0));
    c->reqtype = htonl(mode);
    c->pid = htonl(getpid());
    c->code = htonl(progcode);
    return (true);
}


// See if the local host is licensed for the requested job, with a
// user limit.  If so, validation will succeed if the present request
// is within the limit, from any host.
//
int
xtlserv::test_me(const sJobReq *cref)
{
    int mode = htonl(cref->reqtype);
    if (mode == XTV_DUMP || mode == XTV_KILL)
        return (ERR_UNKNO);
    int progcode = htonl(cref->code);

    sJobReq c;
    static char *working_ip;

    bool onepass = false;
    char hostname[256];
    if (gethostname(hostname, 256) < 0)
        return (ERR_UNKNO);

    switch (mode) {
    case XTV_OPEN:
    case XTV_CHECK:
    case XTV_CLOSE:
        break;
    default:
        return (ERR_UNKNO);
    }

#ifdef WIN32
    // It seems that Windows DHCP clients don't handle the hostname
    // setting via server feature.  This is good, as there is no
    // trouble with dhcp/non-dhcp breaking the licensing.  It might be
    // safer to use the function below to get the host name anyway,
    // this is the "computer name" not overridden by a "cluster name".
    //
    // GetComputerNameEx(ComputerNamePhysicalNetBIOS, hostname, 256);

    char winid[128];
    if (!msw::GetProductID(winid, 0))
        return (false);
    if (!fill_req(&c, hostname, 0, winid, progcode, mode))
        return (ERR_UNKNO);
    // We can also license with IP/HW so keep searching if initial
    // failure.
    // onepass = true;

#else
    static char *working_alt;
#ifdef __APPLE__
    // In Apple, the host name can change with DHCP, therefor we
    // currently ignore it.  The machine ID is sufficient.

    if (!working_ip && !working_alt) {
        char *sn = getMacSerialNumber();
        if (sn) {
            bool ret = fill_req(&c, sn, 0, 0, progcode, mode);
            delete [] sn;
            if (!ret)
                return (ERR_UNKNO);
        }
    }
    else {
        if (!fill_req(&c, hostname, working_ip, working_alt,
                progcode, mode))
            return (ERR_UNKNO);
    }
#else
    if (!working_ip && !working_alt) {
        hostent *he = gethostbyname(hostname);
        if (!he)
            return (false);
        unsigned char *a = (unsigned char*)he->h_addr_list[0];
        working_ip = new char[24];
        sprintf(working_ip, "%d.%d.%d.%d", a[0], a[1], a[2], a[3]);
    }
    if (!fill_req(&c, hostname, working_ip, working_alt, progcode, mode))
        return (ERR_UNKNO);
#endif
#endif

    int rval;
#ifdef __APPLE__
    if (working_ip || working_alt)
        rval = validate(&c, false, cref);
    else {
        char *sn = getMacSerialNumber();
        rval = validate(&c, false, cref);
        delete [] sn;
    }
#else
    rval = validate(&c, false, cref);
#endif
    if (rval == ERR_OK)
        return (ERR_OK);

    if (!onepass && mode == XTV_OPEN && rval == ERR_NOTLIC) {

#ifdef __APPLE__
        char *sn = getMacSerialNumber();
        if (sn) {
            char tbf[32];
            strncpy(tbf, sn, 32);
            delete [] sn;

            // New-style Apple licenses use the ID string as the
            // hostname, with the magic IP.  The actual host name is a
            // wild card, since it can change with DHCP.  We've
            // already checkd this on the first pass, and it failed. 
            // Here we check the old style Apple license, that uses
            // the host name and the ID as the key.

            if (!fill_req(&c, hostname, 0, tbf, progcode, mode))
                return (ERR_UNKNO);
            rval = validate(&c, false, cref);
            if (rval == ERR_OK)
                return (ERR_OK);
        }
#endif

        // First access failed, try the other interface addresses.
        miscutil::ifc_t *ifc = miscutil::net_if_list();
        for (miscutil::ifc_t *i = ifc; i; i = i->next()) {

            if (i->ip() &&
                    (!working_ip || strcmp(i->ip(), working_ip))) {
                if (!fill_req(&c, hostname, i->ip(), 0, progcode, mode))
                    return (ERR_UNKNO);
                rval = validate(&c, false, cref);
                if (rval == ERR_OK)
                    return (ERR_OK);
            }
            if (i->hw()) {
                if (!fill_req(&c, hostname, 0, i->hw(), progcode, mode))
                    return (ERR_UNKNO);
                rval = validate(&c, false, cref);
                if (rval == ERR_OK)
                    return (ERR_OK);
            }
        }
    }
    return (rval);
}


// Main function for validation testing.  Returns 0 if ok, an error
// code otherwise.  If chkfiles is true, just check all license files
// for viability.
//
int
xtlserv::validate(sJobReq *c, bool chkfiles, const sJobReq *cref)
{
    DIR *dir = opendir(LicenseDir);
    if (!dir)
        return (ERR_NOLIC);
    dblk *blocks = new dblk[NUMBLKS+1];
    if (!blocks) {
        closedir(dir);
        return (ERR_NOMEM);
    }
    dirent *de;
    const int nn = strlen(AUTH_FILE);
    int error = ERR_NOLIC;
    bool cmsg = false;
    int errcnt = 0;
    while ((de = readdir(dir)) != 0) {
        if (lstring::prefix(AUTH_FILE, de->d_name) &&
                (!de->d_name[nn] || de->d_name[nn] == '.')) {
            char *path = pathlist::mk_path(LicenseDir, de->d_name);
            error = ERR_NOTLIC;
            if (chkfiles) {
                struct stat st;
                if (stat(path, &st) < 0) {
                    out.cat("Error: can't stat %s file, %s.\n", de->d_name,
                        strerror(errno));
                    out.dump(true);
                    errcnt++;
                    delete [] path;
                    continue;
                }
                if (S_ISDIR(st.st_mode)) {
                    out.cat("Error: %s is a directory.\n", de->d_name);
                    out.dump(true);
                    errcnt++;
                    delete [] path;
                    continue;
                }
                if (st.st_size != 1536) {
                    out.cat(
                "Error: %s file size is not 1536, file must be corrupt.\n",
                        de->d_name);
                    if (!cmsg) {
                        out.cat(
                "You must use the binary mode of ftp and similar programs "
                "when transporting\nthe LICENSE[.xxx] file(s).\n");
                        out.dump(true);
                        cmsg = true;
                    }
                    errcnt++;
                    delete [] path;
                    continue;
                }
            }
            FILE *fp = fopen(path, "rb");
            delete [] path;
            if (!fp) {
                if (chkfiles) {
                    out.cat("Error: can't open %s file, %s.\n", de->d_name,
                        strerror(errno));
                    errcnt++;
                }
                else
                    perrlog("open", false);
                continue;
            }
            int size = (NUMBLKS+1)*sizeof(dblk);
            int msz = fread(blocks, size, 1, fp);
            fclose(fp);
            if (msz != 1) {
                if (chkfiles) {
                    out.cat("Error: premature EOF reading file %s.\n",
                        de->d_name);
                    errcnt++;
                }
                else
                    errlog("premature EOF reading %s", de->d_name);
                continue;
            }
            // copy checksum and zero before test
            unsigned char *sump = blocks[NUMBLKS].sum;
            unsigned char sum[16];
            for (int j = 0; j < 16; j++) {
                sum[j] = sump[j];
                sump[j] = 0;
            }
            MD5cx ctx;
            alter_key(true);
            ctx.update((const unsigned char*)blocks, size);
            ctx.update((const unsigned char*)key, sizeof(key));
            alter_key(false);
            unsigned char final[16];
            ctx.final(final);
            if (memcmp(sum, final, 16)) {
                if (chkfiles) {
                    out.cat("Error: bad checksum for file file %s.\n",
                        de->d_name);
                    errcnt++;
                }
                else {
                    errlog("checksum error reading %s", de->d_name);
#ifdef MAIL_ADDR
                    // send mail - this could be a security concern
                    char buf[512], fbuf[128];
                    decode(strs3, lstring::stpcpy(fbuf, "xtlserv: "));
                    sprintf(buf, fbuf, error, c->user, c->host);
                    miscutil::send_mail(MAIL_ADDR, "SecurityReport:Validate",
                        buf);
#endif
                }
                continue;
            }
            if (chkfiles)
                continue;

            dblk *myblock = find_valid(blocks, c);
            if (myblock) {
                // Found a match!
                if (!check_time(myblock))
                    error = ERR_TIMEXP;
                else if (!check_users(myblock, blocks, c, cref))
                    error = ERR_USRLIM;
                else
                    error = ERR_OK;
                break;
            }
        }
    }
    for (int i = 0; i <= NUMBLKS; i++)
        blocks[i].clear();
    delete [] blocks;
    closedir(dir);
    if (chkfiles && error == ERR_NOTLIC && errcnt == 0)
        error = ERR_OK;
    return (error);
}


// Find the block with matching checksum.
//
dblk*
xtlserv::find_valid(dblk *blocks, sJobReq *c)
{
    block myblk;
    int i;
    for (i = 0; c->host[i] && i < HOSTNAMLEN; i++) {
        char x = c->host[i];
        // Gen3 block hostname always lower case.
        myblk.hostname[i] = isupper(x) ? tolower(x) : x;
    }
    alter_key(true);
    unsigned char *s = (unsigned char*)key;
    for ( ; i < HOSTNAMLEN; i++)
        myblk.hostname[i] = *s++;
    myblk.addr[0] = c->addr[0];
    myblk.addr[1] = c->addr[1];
    myblk.addr[2] = c->addr[2];
    myblk.addr[3] = c->addr[3];
    myblk.code[0] = ntohl(c->code);
    myblk.code[1] = key[0];
    myblk.code[2] = key[16];
    myblk.code[3] = key[32];

    MD5cx ctx;
    ctx.update((const unsigned char*)&myblk, sizeof(block));
    ctx.update((const unsigned char*)key, sizeof(key));
    alter_key(false);
    unsigned char final[16];
    ctx.final(final);
    for (i = 0; i < NUMBLKS; i++)
        if (!memcmp(final, blocks[i].sum, 16))
            break;
    if (i < NUMBLKS)
        return (blocks + i);

    // If the host name contained an upper-case character, try again.
    // The Gen2 licenses do not have hostnames lower-cased.
    bool ucfound = false;
    for (i = 0; c->host[i] && i < HOSTNAMLEN; i++) {
        char x = c->host[i];
        if (isupper(x))
           ucfound = true;
        myblk.hostname[i] = x;
    }
    if (ucfound) {
        alter_key(true);
        s = (unsigned char*)key;
        for ( ; i < HOSTNAMLEN; i++)
            myblk.hostname[i] = *s++;
        myblk.addr[0] = c->addr[0];
        myblk.addr[1] = c->addr[1];
        myblk.addr[2] = c->addr[2];
        myblk.addr[3] = c->addr[3];
        myblk.code[0] = ntohl(c->code);
        myblk.code[1] = key[0];
        myblk.code[2] = key[16];
        myblk.code[3] = key[32];

        ctx.reinit();
        ctx.update((const unsigned char*)&myblk, sizeof(block));
        ctx.update((const unsigned char*)key, sizeof(key));
        alter_key(false);
        ctx.final(final);
        for (i = 0; i < NUMBLKS; i++)
            if (!memcmp(final, blocks[i].sum, 16))
                break;
        if (i < NUMBLKS)
            return (blocks + i);
    }

    if (!strchr(c->host, ':')) {
        // No entry for host.  Look for a site license.
        // Hardware address keys are ineligible.
        //
        // Class C site
        char buf[64];
        decode(strs15, buf);
        for (i = 0; buf[i] && i < HOSTNAMLEN; i++)
            myblk.hostname[i] = buf[i];
        alter_key(true);
        s = (unsigned char*)key;
        for ( ; i < HOSTNAMLEN; i++)
            myblk.hostname[i] = *s++;
        myblk.addr[0] = c->addr[0];
        myblk.addr[1] = c->addr[1];
        myblk.addr[2] = c->addr[2];
        myblk.addr[3] = 0;
        myblk.code[0] = ntohl(c->code);
        myblk.code[1] = key[0];
        myblk.code[2] = key[16];
        myblk.code[3] = key[32];

        ctx.reinit();
        ctx.update((const unsigned char*)&myblk, sizeof(block));
        ctx.update((const unsigned char*)key, sizeof(key));
        alter_key(false);
        ctx.final(final);
        for (i = 0; i < NUMBLKS; i++)
            if (!memcmp(final, blocks[i].sum, 16))
                break;
        if (i < NUMBLKS)
            return (blocks + i);

        // Class B site
        decode(strs16, buf);
        for (i = 0; buf[i] && i < HOSTNAMLEN; i++)
            myblk.hostname[i] = buf[i];
        alter_key(true);
        s = (unsigned char*)key;
        for ( ; i < HOSTNAMLEN; i++)
            myblk.hostname[i] = *s++;
        myblk.addr[0] = c->addr[0];
        myblk.addr[1] = c->addr[1];
        myblk.addr[2] = 0;
        myblk.addr[3] = 0;
        myblk.code[0] = ntohl(c->code);
        myblk.code[1] = key[0];
        myblk.code[2] = key[16];
        myblk.code[3] = key[32];

        ctx.reinit();
        ctx.update((const unsigned char*)&myblk, sizeof(block));
        ctx.update((const unsigned char*)key, sizeof(key));
        alter_key(false);
        ctx.final(final);
        for (i = 0; i < NUMBLKS; i++)
            if (!memcmp(final, blocks[i].sum, 16))
                break;
        if (i < NUMBLKS)
            return (blocks + i);
    }
    return (0);
}


// Check if trying to run after the expiration date,
// if set.  Return true if ok to proceed.
//
bool
xtlserv::check_time(dblk *blk)
{
    if (blk->timelim == 0)
        return (true);
    time_t loc;
    time(&loc);
    if ((unsigned)loc < ntohl(blk->timelim))
        return (true);
    return (false);
}


// See if there are more than the maximum number of copies
// running.  If not, return true.  Add a new XTV_OPEN request to the
// job list.
//
bool
xtlserv::check_users(dblk *blk, dblk *blbase, sJobReq *c, const sJobReq *cref)
{
    // In the job list, the reqtype field is used as a flag to indicate
    // that a user limit exists, and identifies the relevant block

    if (blk->limits[1] != 0) {
        int count = 1;
        for (job *j = JobList; j; j = j->next) {
            if (ntohl(j->request->code) == blk->limits[0] &&
                    j->request->reqtype == blk->limits[3] + 1)
                count++;
        }
#ifdef DEBUG
        errlog("count %d base %d", count, blbase[blk->limits[3]].limits[2]);
#endif
        if (count > blbase[blk->limits[3]].limits[2])
            return (false);
    }

    if (ntohl(c->reqtype) == XTV_OPEN) {
        job *j = new job;
        j->request = new sJobReq;
        memcpy(j->request, cref ? cref : c, sizeof(sJobReq));
        j->request->date = time(0);  // use server's date
        j->request->reqtype = blk->limits[1] ? blk->limits[3] + 1 : 0;
        j->next = JobList;
        JobList = j;
    }
    return (true);
}


// Read a block of data from fd.  Returns false if error, including time
// out after IO_TIME_MS milliseconds.
//
bool
xtlserv::read_block(int fd, void *buf, size_t size)
{
    fd_set readfds;
    size_t len = 0;
    int ms = IO_TIME_MS;
    timeval to;
    to.tv_sec = ms/1000;
    to.tv_usec = (ms % 1000)*1000;;

    unsigned int t0 = Tvals::millisec();
    for (;;) {
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);
        int i = select(fd+1, &readfds, 0, 0, &to);
        if (i == -1) {
            if (errno == EINTR) {
                unsigned int t = Tvals::millisec();
                ms -= (t - t0);
                t0 = t;
                if (ms <= 0)
                    i = 0;
                else {
                    to.tv_sec = ms/1000;
                    to.tv_usec = (ms % 1000)*1000;
                    continue;
                }
            }
            else
                return (false);
        }
        if (i == 0)
            return (false);
        break;
    }
    for (;;) {
        int i = recv(fd, (char*)buf + len, size - len, 0);
        if (i <= 0) {
            if (i < 0 && errno == EINTR)
                continue;
            // error or EOF
            return (false);
        }
        len += i;
        if (len == size)
            break;
    }
    return (true);
}


// Write a block of data to fd.  Returns false if error, including time
// out after IO_TIME_MS milliseconds.
//
bool
xtlserv::write_block(int fd, void *buf, size_t size)
{
    fd_set wfds;
    size_t len = 0;
    int ms = IO_TIME_MS;
    timeval to;
    to.tv_sec = ms/1000;
    to.tv_usec = (ms % 1000)*1000;;

    unsigned int t0 = Tvals::millisec();
    for (;;) {
        FD_ZERO(&wfds);
        FD_SET(fd, &wfds);
        int i = select(fd+1, 0, &wfds, 0, &to);
        if (i == -1) {
            if (errno == EINTR) {
                unsigned int t = Tvals::millisec();
                ms -= (t - t0);
                t0 = t;
                if (ms <= 0)
                    i = 0;
                else {
                    to.tv_sec = ms/1000;
                    to.tv_usec = (ms % 1000)*1000;
                    continue;
                }
            }
            else
                return (false);
        }
        if (i == 0)
            return (false);
        break;
    }
    for (;;) {
        int i = send(fd, (char*)buf + len, size - len, 0);
        if (i <= 0)
            return (false);
        len += i;
        if (len == size)
            break;
    }
    return (true);
}


// Return the date. Return value is static data.
//
char *
xtlserv::datestring()
{
#ifdef HAVE_GETTIMEOFDAY
    timeval tv;
    struct timezone tz;
    gettimeofday(&tv, &tz);
    tm *tp = localtime((time_t *) &tv.tv_sec);
    char *ap = asctime(tp);
#else
    time_t tloc;
    time(&tloc);
    tm *tp = localtime(&tloc);
    char *ap = asctime(tp);
#endif

    static char tbuf[40];
    strcpy(tbuf,ap);
    int i = strlen(tbuf);
    tbuf[i - 1] = '\0';
    return (tbuf);
}


// Print string in the log file.
//
void
xtlserv::errlog(const char *fmt, ...)
{
    va_list args;
    char buf[256];
    va_start(args, fmt);
    sprintf(buf, "%s: ", datestring());
    vsnprintf(buf + strlen(buf), 256, fmt, args);
    va_end(args);
    strcat(buf, "\n");
    FILE *fp = fopen(LogFileName, "a");
    if (fp) {
        fputs(buf, fp);
        fclose(fp);
    }
}


// Error message from syscalls, print to log file.
//
void
xtlserv::perrlog(const char *which, bool die)
{
    char buf[256];
#ifdef HAVE_STRERROR
    sprintf(buf, "%s: %s %s", which, strerror(errno),
        die ? "(exiting)" : "");
#else
    sprintf(buf, "%s: %s %s", which, sys_errlist[errno],
        die ? "(exiting)" : "");
#endif
    errlog(buf);
}


// Dump the job queue to fd.
//
void
xtlserv::jobs_dump(int fd)
{
    clean_list();  // Purge dead ones.
    char buf[512];
    int cnt = 0;
    for (job *j = JobList; j; j = j->next, cnt++) {
        sJobReq *r = j->request;
        const char *what = 0;
        int code = ntohl(r->code);
        switch (code) {
        case SERVER_CODE:
            what = "xtlserv";
            break;
        case OA_CODE:
            what = "oa";
            break;
        case XIV_CODE:
            what = "xiv";
            break;
        case XICII_CODE:
            what = "xicii";
            break;
        case XIC_CODE:
            what = "xic";
            break;
        case XIC_DAEMON_CODE:
            what = "xic daemon";
            break;
        case WRSPICE_CODE:
            what = "wrspice";
            break;
        }
        sprintf(buf, "Process %d: %s\n", cnt, what);
        if (send(fd, buf, strlen(buf) + 1, 0) <= 0) return;
        if (recv(fd, buf, 256, 0) <= 0) return;

        sprintf(buf, "Pid :\t%d\n", (int)ntohl(r->pid));
        if (send(fd, buf, strlen(buf) + 1, 0) <= 0) return;
        if (recv(fd, buf, 256, 0) <= 0) return;

        time_t t = r->date;
        tm *tp = localtime(&t);
        char *ap = asctime(tp);
        sprintf(buf, "Date:\t%s", ap);
        if (send(fd, buf, strlen(buf) + 1, 0) <= 0) return;
        if (recv(fd, buf, 256, 0) <= 0) return;

        sprintf(buf, "Host:\t%s\n", r->host);
        if (send(fd, buf, strlen(buf) + 1, 0) <= 0) return;
        if (recv(fd, buf, 256, 0) <= 0) return;

        sprintf(buf, "Addr:\t%d.%d.%d.%d\n", r->addr[0], r->addr[1],
            r->addr[2], r->addr[3]);
        if (send(fd, buf, strlen(buf) + 1, 0) <= 0) return;
        if (recv(fd, buf, 256, 0) <= 0) return;

        sprintf(buf, "User:\t%s\n", r->user);
        if (send(fd, buf, strlen(buf) + 1, 0) <= 0) return;
        if (recv(fd, buf, 256, 0) <= 0) return;

        if (j->next) {
            if (send(fd, "\n", 2, 0) <= 0) return;
            if (recv(fd, buf, 256, 0) <= 0) return;
        }
    }
    if (!JobList) {
        sprintf(buf, "No active processes.\n");
        if (send(fd, buf, strlen(buf) + 1, 0) <= 0) return;
        if (recv(fd, buf, 256, 0) <= 0) return;
    }
    if (send(fd, "@", 1, 0) <= 0) return;
    if (recv(fd, buf, 256, 0) <= 0) return;
}


// Decoder for the static strings.
//
void
xtlserv::decode(const unsigned char *str, char *buf)
{
    for (int j = 0; ; j++) {
        buf[j] = (str[j] ? (str[j]^1) - 13 : 0);
        if (!str[j])
            break;
    }
}


// Encode the key while not in use.
//
void
xtlserv::alter_key(bool revert)
{
    unsigned int k = 2;
    if (!revert) {
        while (k < sizeof(key) - 5) {
            if (k & 1)
                key[k] = key[k] ^ 0xf;
            else
                key[k] = key[k] ^ 0x3c;
            k++;
        }
    }
    else {
        while (k < sizeof(key) - 5) {
            if (k & 1)
                key[k] = key[k] ^ 0xf;
            else
                key[k] = key[k] ^ 0x3c;
            k++;
        }
    }
}


// Create a return token in buf.
//
void
xtlserv::frand(unsigned char *buf, size_t size, unsigned char *sum, int retval)
{
    // fill buf with crap
    for (unsigned int i = 0; i < size; i++)
        buf[i] = rnd.rand_value() % 256;

    // use first 8 bytes to generate index to sum
    int s = 0;
    for (int i = 0; i < 8; i++)
       s += buf[i];
    s %= (size - 26);  // size - (16 + 8 + 2)
    int i = s + 8;

    // copy sum to location
    for (int j = 0; j < 16; i++, j++)
        buf[i] = sum[j];

    // add retval, second to last byte
    i = size - 2;
        buf[i] = retval & 0xff;
}


// Strip off the ':' and following junk from the host name.  If there is
// a trailing ':', add a '+' to the name.
//
char *
xtlserv::hoststr(char *str)
{
    static char buf[64];
    strncpy(buf, str, 64);
    char *t = strchr(buf, ':');
    if (t) {
        *t++ = '+';
        *t++ = 0;
        while (t - buf < 64)
            *t++ = 0;
    }
    return (buf);
}


// Set the socket to/from non-blocking mode, return true on error.
//
bool
xtlserv::set_nonblock(int sfd, int nonblock)
{
    if (nonblock) {
#ifdef WIN32
        unsigned long ul = 1;
        return (ioctlsocket(sfd, FIONBIO, &ul));
#else
        int flags = fcntl(sfd, F_GETFL, 0);
        if (fcntl(sfd, F_SETFL, flags | O_NONBLOCK) == -1)
            return (true);
        return (false);
#endif
    }
    else {
#ifdef WIN32
        return (ioctlsocket(sfd, FIONBIO, 0));
#else
        int flags = fcntl(sfd, F_GETFL, 0);
        if (fcntl(sfd, F_SETFL, flags&(~O_NONBLOCK)) == -1)
            return (true);
        return (false);
#endif
    }
}


// Keep the log file smaller that ~100Kb.  If the size is exceeded,
// move the log to a backup and clear the existing log.
//
void
xtlserv::check_log()
{
    static int hours;
    // check file every 6 hours
    hours++;
    if (hours % 6)
        return;

    struct stat st;
    if (stat(LogFileName, &st) < 0) {
        if (errno != ENOENT)
            perrlog("stat", false);
        return;
    }
    if (st.st_size > 100000) {
        char buf[256];
        strcpy(buf, LogFileName);
        strcat(buf, ".last");
        unlink(buf);
#ifdef WIN32
        rename(LogFileName, buf);
#else
        link(LogFileName, buf);
        unlink(LogFileName);
#endif
    }
}


void
xtlserv::clean_list()
{
    char buf[128];
    time_t now = time(0);
    job *jp = 0, *jn;
    for (job *j = JobList; j; j = jn) {
        jn = j->next;
        // If the last check time is more than CHECK_DELTA seconds
        // old, delete the entry.  The process must have crashed.

        if (now - j->request->date > CHECK_DELTA) {
            if (jp)
                jp->next = jn;
            else
                JobList = jn;
            decode(strs4, buf);
            errlog(buf, hoststr(j->request->host), ntohl(j->request->pid));
            delete j->request;
            delete j;
            continue;
        }
        jp = j;
    }
}


// The signal handler.
//
void
xtlserv::sig_hdlr(int sig)
{
    switch (sig) {
    case SIGINT:
        errlog("SIGINT received (ignored)");
        break;
#ifdef SIGQUIT
    case SIGQUIT:
        errlog("SIGQUIT received (exiting)");
        exit(1);
        break;
#endif
#ifdef SIGPIPE
    case SIGPIPE:
        // This happens when we try to write to a closed stream.
        errlog("SIGPIPE received (ignored)");
        break;
#endif
    case SIGTERM:
        errlog("SIGTERM received (exiting)");
        exit(1);
        break;
    default:
        errlog("signal %d received (exiting)", sig);
        exit(1);
        break;
    }
}


// Initiate periodic alarm, interval is CHECK_INTERVAL seconds.
//
void
xtlserv::start_timer()
{
#ifdef WIN32
    SetTimer(0, 1000, CHECK_INTERVAL*1000, alarm_hdlr);
#else
    itimerval it;
    it.it_value.tv_sec = CHECK_INTERVAL;
    it.it_value.tv_usec = 0;
    it.it_interval.tv_sec = CHECK_INTERVAL;
    it.it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL, &it, 0);
    signal(SIGALRM, alarm_hdlr);
#endif
}


#ifdef WIN32

void CALLBACK
xtlserv::alarm_hdlr(HWND, UINT, uintptr_t, DWORD)
{
    clean_list();
    check_log();
}

#else

void
xtlserv::alarm_hdlr(int)
{
    signal(SIGALRM, alarm_hdlr);
    clean_list();
    check_log();
}

#endif


#ifdef __APPLE__

char *
secure::getMacSerialNumber()
{
    const char *cmd =
        "/usr/sbin/ioreg -l | "
        "/usr/bin/awk '/IOPlatformSerialNumber/ { print $4; }'";
    FILE *fp = popen(cmd, "r");
    if (!fp)
        return (0);

    char buf[256];
    char *s = fgets(buf, 256, fp);
    pclose(fp);
    if (s) {
        if (*s == '"') {
            s++;
            char *e = strchr(s, '"');
            if (e)
                *e = 0;
        }
        return (strdup(s));
    }
    return (0);
}

#endif

