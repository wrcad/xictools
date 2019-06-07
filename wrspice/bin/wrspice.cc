
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
 * WRspice Circuit Simulation and Analysis Tool                           *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1992 Stephen R. Whiteley
****************************************************************************/

//
// The main function for WRspice
//


#include "config.h"
#ifdef HAVE_SECURE
#include "secure.h"
#endif
#include "spglobal.h"
#include "cshell.h"
#include "commands.h"
#include "variable.h"
#include "simulator.h"
#include "input.h"
#include "device.h"
#include "graph.h"
#include "output.h"
#include "keywords.h"
#include "toolbar.h"
#include "kluif.h"
#include "csdffile.h"
#include "psffile.h"
#include "kwords_fte.h"
#include "datavec.h"
#include "device.h"
#include "acdefs.h"
#include "distdefs.h"
#include "dcodefs.h"
#include "optdefs.h"
#include "statdefs.h"
#include "noisdefs.h"
#include "pzdefs.h"
#include "sensdefs.h"
#include "tfdefs.h"
#include "trandefs.h"
#include "dctdefs.h"
#include "spnumber/hash.h"
#include "miscutil/lstring.h"
#include "miscutil/miscutil.h"
#include "miscutil/random.h"
#include "miscutil/pathlist.h"
#include "miscutil/filestat.h"
#include "miscutil/childproc.h"

#ifdef HAVE_LOCAL_ALLOCATOR
#include "malloc/local_malloc.h"
#endif

#ifdef HAVE_PWD_H
#include <pwd.h>
#endif

#ifdef HAVE_SIGNAL
#include <sys/types.h>
#include <signal.h>
#endif

#include <time.h>
#ifndef HAVE_GETRUSAGE
#ifdef HAVE_FTIME
#include <sys/types.h>
#include <sys/timeb.h>
#endif
#endif

#ifdef HAVE_FENV_H
#include <fenv.h>
#endif

#ifdef WIN32
#include "miscutil/msw.h"
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#endif
#include <sys/time.h>
#include <new>
#ifdef __linux
#include <fpu_control.h>
#include <limits.h>
#endif

// defines WRS_RELEASE_TAG
#include "reltag.h"

// These are set as compiler -D defines
#ifndef PREFIX
#define PREFIX "/usr/local"
#endif
#ifndef OSNAME
#define OSNAME "unknown"
#endif
#ifndef ARCH
#define ARCH ""
#endif
#ifndef DIST_SUFFIX
#define DIST_SUFFIX ""
#endif
#ifndef TOOLS_ROOT
#define TOOLS_ROOT "xictools"
#endif
#ifndef APP_ROOT
#define APP_ROOT "wrspice"
#endif
#ifndef SPICE_VERSION
#define SPICE_VERSION "unknown"
#endif
#ifndef DEVLIB_VERSION
#define DEVLIB_VERSION ""
#endif
#ifndef SPICE_BUILD_DATE
#define SPICE_BUILD_DATE "Tue Aug 29 15:59:20 PDT 2017"
#endif
#ifndef SPICE_PROG
#define SPICE_PROG "wrspice"
#endif
#ifndef GFX_PROG
#define GFX_PROG ""
#endif
#ifndef BUG_ADDR
#define BUG_ADDR "wrspice@wrcad.com"
#endif
#ifndef SPICE_NEWS
#define SPICE_NEWS "news"
#endif
#ifndef SPICE_EDITOR
#define SPICE_EDITOR "xeditor"
#endif
#ifndef SPICE_OPTCHAR
#define SPICE_OPTCHAR "-"
#endif
#ifndef SPICE_ASCIIRAWFILE
#define SPICE_ASCIIRAWFILE "1"
#endif
#ifndef SPICE_HOST
#define SPICE_HOST ""
#endif
#ifndef SPICE_DAEMONLOG
#define SPICE_DAEMONLOG "/tmp/spiced.log"
#endif
#ifndef SPICE_NOTICE
#define SPICE_NOTICE ""
#endif


// WRspice creates and listens on a named pipe.  Text written to the
// pipe is sourced as WRspice input.  This is the default pipe name.
//
#define WRSPICE_FIFO "wrsfifo"

sGlobal Global;

// Random number generator.
sRnd Rnd;

namespace {
    void start_timer();
    void init_sigs();
    void process_args(int*, char**);
    void shift(int, int*, char**);
    void succumb(int, bool);
    void panic_to_gdb();

    const char *usage = 
    "Usage: %s [-] [-b] [-c caseflags] [-i] [-j] [-s] [-n] [-x] [-o outfile]\n"
        "\t[-r rawfile] [-t term] [file ...]\n";
}

#ifdef HAVE_LOCAL_ALLOCATOR

// This handles errors from the memory allocator if it is linked.
namespace {
    void memory_error_log(const char*, void*, long*, int);

    struct WRSinit
    {
        WRSinit()
            {
                Memory()->register_error_log(memory_error_log);
            }
    };
    WRSinit _wrsinit_;
}

#else

// A recursive malloc call is bad news!
namespace {
    bool InMalloc;
    bool GotSigInMalloc;
}


void *
operator new(size_t size)
{
    InMalloc = true;
    void *v = malloc(size);
    InMalloc = false;
    if (GotSigInMalloc) {
        GotSigInMalloc = false;
        CP.ResetControl();
        throw 127;
    }
    if (!v) {
        GRpkgIf()->ErrPrintf(ET_WARN, "virtual memory limit exceeded.\n");
        if (Sp.GetFlag(FT_SIMFLAG)) {
            if (Sp.CurCircuit())
                Sp.CurCircuit()->set_inprogress(false);
            if (OP.curPlot())
                OP.curPlot()->set_active(false);
            Sp.SetFlag(FT_SIMFLAG, false);
        }
        CP.ResetControl();
        throw 127;
    }
    return (v);
}
#endif

// This app's main classes
IFsimulator Sp;
SPinput IP;
IFoutput OP;
SPgraphics GP;
CshPar CP;

// Set up the ginterf package, include all drivers except Versatec.
//
#define GR_CONFIG (GR_ALL_PKGS | GR_ALL_DRIVERS)
#include "ginterf/gr_pkg_setup.h"

// Declare the graphics package.
namespace { SpGrPkg _sp; }
GRpkg& GR = _sp;

#ifdef HAVE_LOCAL_ALLOCATOR
namespace {
    void memory_error_log(const char *what, void *chunk, long *stack, int stsz)
    {
        if (!chunk) {
            // out of memory
            fputs("FATAL ERROR: ", stderr);
            fputs(what, stderr);
            fputs("\n", stderr);
            if (Sp.GetFlag(FT_SIMFLAG)) {
                if (Sp.CurCircuit())
                    Sp.CurCircuit()->set_inprogress(false);
                if (OP.curPlot())
                    OP.curPlot()->set_active(false);
                Sp.SetFlag(FT_SIMFLAG, false);
            }
            CP.ResetControl();
            throw 127;
        }

        int fd = open("wrspice_mem_errors.log", O_CREAT|O_WRONLY|O_APPEND,
            0644);
        if (fd >= 0) {
            char buf[256];
            sprintf(buf, "%s %s\n", Global.Version(), datestring());
            write(fd, buf, strlen(buf));
            sprintf(buf, "%s 0x%lx\n", what, (long)chunk);
            write(fd, buf, strlen(buf));
            for (int i = 0; i < stsz; i++) {
                sprintf(buf, "#%d 0x%lx\n", i, stack[i]);
                write(fd, buf, strlen(buf));
            }
            close(fd);
        }
        // Flag used in fatal() function.
        Global.SetMemError(true);
        fputs("*** memory fault detected, logfile updated.\n", stderr);
    }
}
#endif


// Callback struct for the graphics.
//
struct wrs_if : GRappCallStubs
{
    const char *GetPrintCmd();
    char *ExpandHelpInput(char*);
    bool ApplyHelpInput(const char*);
    pix_list *ListPixels();
    int BackgroundPixel();
};


// Returns the current print command, or makes one up.  Also called by
// the help widget for printing.
//
const char *
wrs_if::GetPrintCmd()
{
    variable *v = Sp.GetRawVar(kw_hcopycommand);
    if (v && v->type() == VTYP_STRING)
        return (lstring::copy(v->string()));
    char *prname = getenv("PRINTER");
    if (!prname || !*prname)
        return (lstring::copy("lpr -h"));
    char buf[128];
    sprintf(buf, "lpr -P%s -h", prname);
    return (lstring::copy(buf));
}


namespace {
    // Measure the token length, terminated by non-slphanum.  Assume
    // that the first character is accounted for ('$').
    //
    int toklen(const char *str)
    {
        int cnt = 1;
        str++;  // skip '$'
        while (isalnum(*str)) {
            cnt++;
            str++;
        }
        return (cnt);
    }


    // Return true if kw prefixes str, followed by non-alphsnum.
    //
    bool match(const char *str, const char *kw)
    {
        while (*str && *kw) {
            if (*str != *kw)
                return (false);
        }
        if (*kw)
            return (false);
        if (isalnum(*str))
            return (false);
        return (true);
    }


    // Replace href with a new version where toklen chars starting at
    // pos are replaced with newtext.
    //
    void repl(char **href, int pos, int toklen, const char *newtext)
    {
        char *t = new char[strlen(*href) - toklen + strlen(newtext) + 1];
        strncpy(t, *href, pos);
        char *e = lstring::stpcpy(t + pos, newtext);
        strcpy(e, *href + pos + toklen);
        delete [] *href;
        *href = t;
    }
}


// This is the callback that processes the '$' expansion for keywords
// passed to the help system.  The argument is either returned or
// freed.
//
char *
wrs_if::ExpandHelpInput(char *href)
{
    const char *s = strchr(href, '$');
    int maxrepl = 100;
    while (s) {
        // Make sure that we don't get caught in a loop.
        maxrepl--;
        if (!maxrepl)
            break;

        bool found = true;
        int tlen = toklen(s);
        int pos = s - href;
        if (match(s, "$PROGROOT"))
            repl(&href, pos, tlen, Global.ProgramRoot());
        else if (match(s+1, "EXAMPLES")) {
            sLstr lstr;
            lstr.add(Global.ProgramRoot());
            lstr.add("/examples");
            repl(&href, pos, tlen, lstr.string());
        }
        else if (match(s+1, "HELP")) {
            sLstr lstr;
            lstr.add(Global.ProgramRoot());
            lstr.add("/help");
            repl(&href, pos, tlen, lstr.string());
        }
        else if (match(s+1, "DOCS")) {
            sLstr lstr;
            lstr.add(Global.ProgramRoot());
            lstr.add("/docs");
            repl(&href, pos, tlen, lstr.string());
        }
        else if (match(s+1, "SCRIPTS")) {
            sLstr lstr;
            lstr.add(Global.ProgramRoot());
            lstr.add("/scripts");
            repl(&href, pos, tlen, lstr.string());
        }
        else {
            found = false;
            // Check for matches to Xic and environment variables.
            //
            char *nm = new char[tlen];
            strncpy(nm, s+1, tlen-1);
            nm[tlen-1] = 0;
            if (nm[0]) {
                VTvalue vv;
                if (Sp.GetVar(nm, VTYP_STRING, &vv)) {
                    found = true;
                    repl(&href, pos, tlen, vv.get_string());
                }
                else {
                    const char *ev = getenv(nm);
                    if (ev) {
                        found = true;
                        repl(&href, pos, tlen, ev);
                    }
                }
            }
            delete [] nm;
        }
        if (found)
            s = strchr(href + pos, '$');
        else
            s = strchr(s+1, '$');
    }
    return (href);
}


// Help window application anchor text processing.
//
bool
wrs_if::ApplyHelpInput(const char *fname)
{
    const char *t = strrchr(fname, '.');
    if (!t || t == fname)
        return (false);

    if (lstring::cieq(t+1, "cir")) {
        // If the URL has a ".cir" extension, source it.
        wordlist wl;
        wl.wl_word = (char*)fname;
        CommandTab::com_source(&wl);
        return (true);
    }
    if (lstring::cieq(t+1, "raw")) {
        // If the URL has a ".raw" extension, load it.
        wordlist wl;
        wl.wl_word = (char*)fname;
        CommandTab::com_load(&wl);
        return (true);
    }
    if (cCSDFout::is_csdf_ext(fname)) {
        // If the URL has a CSDF extension, load it.
        wordlist wl;
        wl.wl_word = (char*)fname;
        CommandTab::com_load(&wl);
        return (true);
    }
    return (false);
}


pix_list *
wrs_if::ListPixels()
{
    sColor *colors = SpGrPkg::DefColors;
    pix_list *p0 = 0;
    for (int i = 0; i < GR.CurDev()->numcolors; i++) {
        pix_list *p = new pix_list;
        p->pixel = colors[i].pixel;
        p->r = colors[i].red;
        p->g = colors[i].green;
        p->b = colors[i].blue;
        p->next = p0;
        p0 = p;
    }
    return (p0);
}


int
wrs_if::BackgroundPixel()
{
    if (GP.Cur())
        return (GP.Cur()->color(0).pixel);
    return (SpGrPkg::DefColors[0].pixel);
}
// End of wrs_if functions.


struct sCmdOpts
{
    bool nogdata;
    bool batchmode;
    bool servermode;
    bool interactive;
    bool portmon;
    bool nocmdcomplete;
    bool noreadinit;
    bool notoolbar;
    const char *term;
    const char *display;
    const char *rawfile;
    const char *output;
    const char *caseflags;
    wordlist *modules;
};

namespace {
    wrs_if wrsif;
    sCmdOpts CmdLineOpts;

#ifdef HAVE_ATEXIT
    void exit_proc()
        {
            if (Global.FifoName())
                unlink(Global.FifoName());
            CP.SetupTty(fileno(stdin), true);
        }
#endif

#ifdef HAVE_SECURE
    // Authentication
    inline bool authmode()
    {
        char *s = getenv("XT_AUTH_MODE");
#ifdef WIN32
        return (!s || (*s != 's' && *s != 'S'));
#else
        return (s && (*s == 'l' || *s == 'L'));
#endif
    }

    sAuthChk Auth(authmode());
#endif
}

typedef void (*SigType)();


// Error message handler called by GR.
//
void
err_callback(Etype type, bool to_stdout, const char *errmsg)
{
    if (Sp.GetFlag(FT_SILENT) && type != ET_INTERR && type != ET_INTERRS)
        return;
    ToolBar()->PopUpSpiceErr(to_stdout, errmsg);
}


#ifdef WIN32

namespace {
    // Thread procedure to listen for interrupts.
    //
    void intr_proc(void *arg)
    {
        HANDLE hs = (HANDLE)arg;
        for (;;) {
            WaitForSingleObject(hs, INFINITE);
            raise(SIGINT);
        }
    }


    // Thread procedure to listen for SIGTERM signals.
    //
    void term_proc(void *arg)
    {
        HANDLE hs = (HANDLE)arg;
        for (;;) {
            WaitForSingleObject(hs, INFINITE);
            raise(SIGTERM);
        }
    }


    // Thread procedure to listen on the named pipe.
    //
    void pipe_thread_proc(void *arg)
    {
        HANDLE hpipe = (HANDLE)arg;
        for (;;) {
            if (ConnectNamedPipe(hpipe, 0)) {

                char *tempfile = filestat::make_temp("sp");
                FILE *fp = fopen(tempfile, "w");
                if (fp) {
                    unsigned int total = 0;
                    char buf[2048];
                    for (;;) {
                        DWORD bytes_read;
                        bool ok = ReadFile(hpipe, buf, 2048, &bytes_read, 0);
                        if (!ok)
                            break;
                        if (bytes_read > 0) {
                            fwrite(buf, 1, bytes_read, fp);
                            total += bytes_read;
                        }
                    }
                    fclose(fp);
                    if (total > 0)
                        CP.AddPendingSource(tempfile);
                    filestat::queue_deletion(tempfile);
                }
                else
                    perror(tempfile);
                delete [] tempfile;
                DisconnectNamedPipe(hpipe);
            }
        }
    }
}

#endif

namespace {
    void setup_fifo()
    {
        // Create a named pipe for input.  Text written to the
        // pipe will be sourced.  The name can be given with the
        // WRSPICE_FIFO environment variable, or defaults to the
        // name provided here.  In Unix/Linux, this can appear
        // anywhere in the file system, all components but the
        // "filename" must exist.  If the "filename" conflicts
        // with an existing entity, we add an integer suffix.  In
        // Windows, only the "filename" part is used.  The path
        // prefix is always inplicit, and is "\\.\pipe\".
        //
        // Thus, you can use your favorite text editor outside of
        // WRspice to edit an input file, then "save" it to the
        // fifo to source it into WRspice.  Note that you need to
        // also save it to a disk file if you want to keep it.

        const char *ff = getenv("WRSPICE_FIFO");
        if (!ff || !*ff)
            ff = WRSPICE_FIFO;
#ifdef WIN32
        char buf[256];
        ff = lstring::strip_path(ff);
        snprintf(buf, BSIZE_SP - 4, "\\\\.\\pipe\\%s", ff);
        char *t = buf + strlen(buf);
        int cnt = 1;
        for (;;) {
            if (access(buf, F_OK) < 0)
                break;
            sprintf(t, "%d", cnt);
        }
        SECURITY_DESCRIPTOR sd;
        InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
        SECURITY_ATTRIBUTES sa;
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.lpSecurityDescriptor = &sd;
        sa.bInheritHandle = false;

        HANDLE hpipe = CreateNamedPipe(buf,
            PIPE_ACCESS_INBOUND,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES,
            2048,
            2048,
            NMPWAIT_USE_DEFAULT_WAIT,
            &sa);

        if (hpipe != INVALID_HANDLE_VALUE) {
            fprintf(stdout, "Listening for input on pipe named %s.\n",
                buf);
            Global.SetFifoName(lstring::copy(buf));
            _beginthread(pipe_thread_proc, 0, hpipe);
        }
#else
        char *fpath = 0;
        if (lstring::is_rooted(ff))
            fpath = lstring::copy(ff);
        else {
            passwd *pw = getpwuid(getuid());
            if (pw == 0) {
                GRpkgIf()->Perror("getpwuid");
                char *cwd = getcwd(0, 0);
                fpath = pathlist::mk_path(cwd, ff);
                delete [] cwd;
            }
            else
                fpath = pathlist::mk_path(pw->pw_dir, ff);
        }

        char *t = 0;
        int cnt = 1;
        for (;;) {
            if (access(fpath, F_OK) < 0)
                break;
            if (cnt == 1) {
                char *tt = new char[strlen(fpath) + 6];
                t = lstring::stpcpy(tt, fpath);
                delete [] fpath;
                fpath = tt;
            }
            sprintf(t, "%d", cnt);
            cnt++;
        }
        int ret = mkfifo(fpath, 0666);
        if (ret == 0) {
            fprintf(stdout, "Listening for input on pipe named %s.\n", fpath);
            Global.SetFifoName(fpath);
            fpath = 0;
        }
        delete [] fpath;
#endif
    }
}


#ifdef WIN32
// The c++ exceptions don't seem to work in Windows, so back to
// setjmp/longjmp.
jmp_buf msw_jbf[4];
int msw_jbf_sp;
#endif


int
main(int argc, char **argv)
{
    // Test for bad longjmp (MFB used to do this).
    static bool started;
    if (started) {
        fprintf(stderr, "main: Internal Error: jump to zero\n");
        succumb(EXIT_BAD, true);
    }
    started = true;

    // Something for Cadence PSF writer.
    cPSFout::vo_init(&argc, argv);

#ifdef HAVE_FENV_H
    // This sets the x87 precision mode.  The default under Linux is
    // to use 80-bit mode, which produces subtle differences from
    // FreeBSD and other systems, eg, (int)(1000*atof("0.3")) is 300
    // in 64-bit mode, 299 in 80-bit mode.  For this reason, we used
    // to use 64-bit mode.

#if defined(__FreeBSD__) || defined(__APPLE__)
#define __control_word __control
#endif

    // THIS IS x87-SPECIFIC!!!
    // Actually, the old x87 stack seems not to be used anymore,
    // instead the MMX registers and SSEx are used.  SSE3/4 don't have
    // the intermediate over-precision problem, as it uses
    // "right-sized" registers.  Thus, the code below is probably of
    // historical interest only, and is inactive unless the
    // environment vars are set.

    if (getenv("WRSPICE64")) {
        fenv_t env;
        fegetenv(&env);
        env.__control_word &= ~0x300;
        env.__control_word |= 0x200;  // 64-bit precision
        printf("Using 64-bit x87 precision.\n");
        fesetenv(&env);
    }
    else if (getenv("WRSPICE80")) {
        fenv_t env;
        fegetenv(&env);
        env.__control_word |= 0x300;  // 80-bit precision
        printf("Using 80-bit precision.\n");
        fesetenv(&env);
    }
#endif

    // Set default FPE handling.
    Sp.SetFPEmode(FPEdefault);

    // Initialize the internal keyword database, should doi this
    // before calling SP.SetVar.
    KW.initDatabase();

    // Set our own error handling.
    GR.RegisterErrorCallback(err_callback);

    // Configure temp filename generator.
    filestat::make_temp_conf("sp", "SPICE_TMP_DIR");

    CP.SetProgram(Sp.Simulator());
    Global.initialize(argv[0]);

    for (int i = 1; i < argc; i++) {
        // If we find one of these special arguments, print some info
        // and exit.

        if (lstring::eq(argv[i], "--v")) {
            printf("%s %s %s\n", Global.Version(), Global.OSname(),
                Global.Arch());
            return (0);
        }
        if (lstring::eq(argv[i], "--vv")) {
            printf("%s\n", WRS_RELEASE_TAG);
            exit (0);
        }
        if (lstring::eq(argv[i], "--vb")) {
            printf("%s\n", Global.BuildDate());
            exit (0);
        }
    }

    int year = 0;
    {
        const char *s = Global.BuildDate();
        char *tok;
        while ((tok = lstring::gettok(&s)) != 0) {
            if (strlen(tok) == 4 && tok[0] == '2' && isdigit(tok[1]) &&
                    isdigit(tok[2]) && isdigit(tok[3])) {
                year = atoi(tok);
                delete [] tok;
                break;
            }
            delete [] tok;
        }
        if (!year)
            year = 2017;
    }
#ifdef BDCODE
    printf("WRspice circuit simulation system, release %s, build %s\n"
        "Copyright (C) Whiteley Research Inc, Sunnyvale CA  %d\n"
        "All Rights Reserved\n\n", Global.Version(), BDCODE, year);
#else
    printf("WRspice circuit simulation system, release %s\n"
        "Copyright (C) Whiteley Research Inc, Sunnyvale CA  %d\n"
        "All Rights Reserved\n\n", Global.Version(), year);
#endif

#ifdef WIN32
    WSADATA wsadata;
    if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0)
        fprintf(stderr,
    "Warning: WSA initialization failed, no interprocess communication.\n");

    // The inno installer is sensitive to this, prevents install when
    // application is active.
    CreateMutex(0, false, "WRspiceMutex");
#endif

    {
        bool xstart = false, noxstart = false;

        // Copy the arg list.
        char **av = new char*[argc+1];
        if (!av) {
            fprintf(stderr, "Memory allocation error\n");
            exit(1);
        }
        int i, j = 0;
        for (i = 0; i < argc; i++) {
            if (lstring::cieq(argv[i], "-x"))
                xstart = true;
            else {
                if (lstring::cieq(argv[i], "-b") ||
                        lstring::cieq(argv[i], "-s") ||
                        lstring::cieq(argv[i], "-p"))
                    noxstart = true;
                av[j++] = argv[i];
            }
        }
        argc -= (i - j);
        av[j] = 0;
        argv = av;
        if (xstart && !noxstart) {
            // Start in our own terminal window.
            sLstr lstr;
            for (i = 0; argv[i]; i++) {
                if (i)
                    lstr.add_c(' ');
                lstr.add(argv[i]);
            }
            miscutil::fork_terminal(lstr.string());
            exit (0);
        }
    }

    Sp.CheckDevlib();
    process_args(&argc, argv);
    Sp.SetFlag(FT_BATCHMODE, CmdLineOpts.batchmode);
    Sp.SetFlag(FT_SERVERMODE, CmdLineOpts.servermode);
    if (CmdLineOpts.batchmode || CmdLineOpts.servermode || CmdLineOpts.portmon)
        CP.SetFlag(CP_NOTTYIO, true);
    if (CmdLineOpts.display) {
        if (lstring::cieq(CmdLineOpts.display, "none"))
            CP.SetDisplay(0);
        else
#ifdef WIN32
            CP.SetDisplay(":0");
#else
            CP.SetDisplay(CmdLineOpts.display);
#endif
    }
    else {
#ifdef WIN32
        CP.SetDisplay(":0");
#else
        CP.SetDisplay(getenv("DISPLAY"));
#endif
    }
    if (CmdLineOpts.term)
        Sp.SetVar("term", CmdLineOpts.term);
    if (CmdLineOpts.rawfile) {
        OP.getOutDesc()->set_outFile(CmdLineOpts.rawfile);
        Sp.SetVar("rawfile", OP.getOutDesc()->outFile());
    }
    if (CmdLineOpts.output) {
        if (!(freopen(CmdLineOpts.output, "w", stdout))) {
            perror(CmdLineOpts.output);
            exit(EXIT_BAD);
        }
    }

    bool istty = isatty(fileno(stdin));
    // Note:  we start in batch mode by default if stdin is not a
    // terminal.

    // Cgywin warning!  The mintty and other terminals presently
    // (04/2015) don't work, at all.  isatty returns false.  Giving
    // the -i option might sort-of work for some things.  You really
    // must use a DOS box running a bash console, which is no longer
    // the default in Cygwin.

    if ((!CmdLineOpts.interactive && !istty) || Sp.GetFlag(FT_SERVERMODE))
        Sp.SetFlag(FT_BATCHMODE, true);
    if ((CmdLineOpts.interactive && !istty) || CmdLineOpts.nocmdcomplete)
        CP.SetFlag(CP_NOCC, true);
    if (!istty || Sp.GetFlag(FT_BATCHMODE))
        TTY.setmore(false);
    if (Sp.GetFlag(FT_SERVERMODE) && !CmdLineOpts.rawfile)
        OP.getOutDesc()->set_outFile(0);

    CP.SetFlag(CP_INTERACTIVE, false);
    char buf[BSIZE_SP];

    snprintf(buf, BSIZE_SP,  ". %s",
        Global.StartupDir() ? Global.StartupDir() : "");
    
    {
#define WRS_TIMEFILE "wrs-procid"
        const char *path = getenv("SPICE_TMP_DIR");
        if (!path)  
            path = getenv("TMPDIR");
        if (!path)
            path = "/tmp";
        char *tbf = pathlist::mk_path(path, WRS_TIMEFILE);
#ifdef HAVE_SECURE
        if (!Auth.validate(WRSPICE_CODE, buf, tbf))
            return (EXIT_BAD);
#endif
        delete [] tbf;
    }
    init_sigs();

    // Init graphics, so we can setrdb in init file.
    TTY.init_more();
    if (!Sp.GetFlag(FT_BATCHMODE)) {
        if (CP.Display()) {
            if (GR.InitPkg(GR_CONFIG, &argc, argv))
                // failed to open X
                // Display() is a "use X" flag
                CP.SetDisplay(0);
            else
                GR.InitColormap(0, 0, false);
        }
        if (!CP.Display()) {
            GR.SetNullGraphics();
            if (!lstring::cieq(CmdLineOpts.display, "none"))
                GR.ErrPrintf(ET_WARN, "no graphics package available.\n");
        }
    }

    // Now check for unrecognized options.
    for (int i = 1; i < argc; i++) {
        if (*argv[i] == '-') {
            fprintf(stderr, "Error: bad option %s\n", argv[i]);
            fprintf(stderr, usage, CP.Program());
            return (EXIT_BAD);
        }
    }

    Sp.PreInit();

    bool read_err;
#ifdef WIN32
    if (setjmp(msw_jbf[msw_jbf_sp]) == 0) {
#else
    try {
#endif
        read_err = false;

        // Now source the system startup file.
        FILE *fp;
        char sbuf[64];
        if (Global.StartupDir() && *Global.StartupDir()) {
            SYS_STARTUP(sbuf);
            char *fn = pathlist::mk_path(Global.StartupDir(), sbuf);
            char *ftmp = pathlist::expand_path(fn, false, true);
            delete [] fn;
            fn = ftmp;
            if ((fp = fopen(fn, "r")) != 0) {
                Sp.SpSource(fp, true, true, fn);
                fclose(fp);
            }
            else
                GR.ErrPrintf(ET_WARN, "can't find system init file.\n");
            delete [] fn;
        }
        else
            GR.ErrPrintf(ET_WARN,
                "startup path not set, can't find init file.\n");

        // Now read the user's startup file(s).
        if (!Sp.GetFlag(FT_SERVERMODE) && !CmdLineOpts.noreadinit) {
            USR_STARTUP(sbuf);

            // First, the home startup.
            char *home = pathlist::get_home(0);
            if (home) {
                char *fn = pathlist::mk_path(home, sbuf);
                delete [] home;
                if ((fp = fopen(fn, "r")) != 0) {
                    Sp.SpSource(fp, true, true, fn);
                    fclose(fp);
                }
                delete [] fn;
            }

            // Next, the curdir startup.
            if ((fp = fopen(sbuf, "r")) != 0) {
                Sp.SpSource(fp, true, true, sbuf);
                fclose(fp);
            }
        }
    }
#ifdef WIN32
    else {
#else
    catch (int) {
#endif
        read_err = true;
    }

    // The command-line case flags override those set in the init files.
    if (CmdLineOpts.caseflags)
        sHtab::parse_ciflags(CmdLineOpts.caseflags);
    Sp.PostInit();

    if (read_err)
        fprintf(stderr, "Warning: error executing init file.\n");

    // Load KLU and device model modules.
    klu_if.find_klu();
    if (!Sp.GetVar(kw_nomodload, VTYP_BOOL, 0))
        Sp.LoadModules(0);
    else {
        for (wordlist *wl = CmdLineOpts.modules; wl; wl = wl->wl_next)
            Sp.LoadModules(wl->wl_word);
    }
    wordlist::destroy(CmdLineOpts.modules);
    CmdLineOpts.modules = 0;

    if (!CmdLineOpts.notoolbar && !Sp.GetFlag(FT_BATCHMODE))
        ToolBar()->Toolbar();

    wordlist *wl0 = 0, *wle = 0;
    if (!Sp.GetFlag(FT_SERVERMODE)) {
        for (int j = 1; j < argc; j++) {
            if (!wl0)
                wl0 = wle = new wordlist(argv[j], 0);
            else {
                wle->wl_next = new wordlist(argv[j], wle);
                wle = wle->wl_next;
            }
        }
    }

    wordlist *tmpfiles;
    file_elt *f0 = file_elt::wl_to_fl(wl0, &tmpfiles);
    if (!f0 && !wl0 && Sp.GetFlag(FT_BATCHMODE)) {
        char *tempfile = filestat::make_temp("sp");
        FILE *fp = fopen(tempfile, "w");
        if (!fp) {
            perror(tempfile);
            succumb(EXIT_BAD, true);
        }
        wordlist *wt = new wordlist(tempfile, 0);
        wt->wl_next = tmpfiles;
        if (wt->wl_next)
            wt->wl_next->wl_prev = wt;
        tmpfiles = wt;
        int c;
        while ((c = getchar()) != EOF)
            putc(c, fp);
        fclose(fp);

        f0 = new file_elt(tempfile, 1, 0);
        delete [] tempfile;
    }
    wordlist::destroy(wl0);

    start_timer();

    if (!Sp.GetFlag(FT_BATCHMODE)) {
#ifdef WIN32
        // Set up a thread and a semaphore to enable passing SIGINT.
        char tbuf[64];
        sprintf(tbuf, "wrspice.sigint.%d", getpid());
        HANDLE hintr = CreateSemaphore(0, 0, 1, tbuf);
        _beginthread(intr_proc, 0, hintr);
        // Set up a thread and a semaphore to enable passing SIGTERM.
        sprintf(tbuf, "wrspice.sigterm.%d", getpid());
        HANDLE hterm = CreateSemaphore(0, 0, 1, tbuf);
        _beginthread(term_proc, 0, hterm);
#endif
        if (!CmdLineOpts.portmon) {
            CP.SetupTty(fileno(stdin), false);
            init_sigs();  // #!@&$!% ncurses!
#ifdef HAVE_ATEXIT
            atexit(exit_proc);
#endif
            if (miscutil::new_release(APP_ROOT, SPICE_VERSION))
                ToolBar()->PopUpNotes();

            // Create a named pipe that listens for input.
            setup_fifo();

            if (Global.NewsFile() && *Global.NewsFile()) {
                char *fn = pathlist::expand_path(Global.NewsFile(), false,
                    true);
                FILE *fp = fopen(fn, "r");
                if (fp) {
                    while (fgets(buf, BSIZE_SP, fp))
                        fputs(buf, stdout);
                    fclose(fp);
                }
                delete [] fn;
            }
        }
    }

    // Load the files given on the command line.
    for (file_elt *f = f0; f; f = f->next()) {

        FILE *fp = fopen(f->filename(), "r");
        if (!fp) {
            perror(f->filename());
            for (wordlist *w = tmpfiles; w; w = w->wl_next)
                unlink(w->wl_word);
            succumb(EXIT_BAD, true);
        }

        Sp.SpSource(fp, false, false, f->filename());
        fclose(fp);

        if (Sp.GetFlag(FT_BATCHMODE)) {
            CP.SetFlag(CP_INTERACTIVE, false);
            if (!Sp.CurCircuit()) {
                VTvalue vv;
                if (Sp.GetVar(kw_mplot_cur, VTYP_STRING, &vv))
                    // margin analysis performed
                    Sp.RemVar(kw_mplot_cur);
                else
                    fprintf(stderr,
                        "Warning: no circuit loaded from file %s.\n",
                        lstring::strip_path(f->filename()));
                continue;
            }
            else if (!Sp.CurCircuit()->runonce()) {
                if (Sp.GetFlag(FT_SERVERMODE))
                    Sp.Run(OP.getOutDesc()->outFile());
                else
                    Sp.RunBatch();
                Sp.CurCircuit()->set_runonce(false);
            }
            Sp.SetCurCircuit(0);
        }
    }
    for (wordlist *w = tmpfiles; w; w = w->wl_next)
        unlink(w->wl_word);
    wordlist::destroy(tmpfiles);
    file_elt::destroy(f0);

    if (!Sp.GetFlag(FT_BATCHMODE)) {
        if (CmdLineOpts.portmon) {
            CP.SetFlag(CP_INTERACTIVE, false);
            if (CP.InitIPC())
                succumb(EXIT_BAD, false);
            fprintf(stdout, "port %d\n", CP.Port());
            fflush(stdout);
            if (CP.InitIPCmessage())
                succumb(EXIT_BAD, false);

            // Set the interface to support legacy clients by default.
            // These may be updated by the protocol.
            Sp.SetSubcCatchar(':');
            Sp.SetSubcCatmode(SUBC_CATMODE_SPICE3);

            // Now divert stdout to stderr channel, so that
            // output appears on screen when stdout is redirected.
            //
            setvbuf(stdout, 0, _IONBF, 0);
            // Call this for stderr, too.  It is probably redundant for
            // Unix/Linux, but it seems to be critical for Windows.
            // In Windows, stderr appears to disappear otherwise.
            //
            setvbuf(stderr, 0, _IONBF, 0);
            dup2(fileno(stderr), fileno(stdout));

            for (;;) {
#ifdef WIN32
                if (setjmp(msw_jbf[msw_jbf_sp]) == 1)
                    break;
#endif
                CP.Reset(false);
                try { while (CP.EvLoop(0) == 1) ; }
                catch (int) { }
            }
        }
        else {
            for (;;) {
                CP.SetupTty(fileno(stdin), false);
#ifdef WIN32
                setjmp(msw_jbf[msw_jbf_sp]);
#endif
                CP.Reset(true);
                try { while (CP.EvLoop(0) == 1) ; }
                catch (int) { continue; }
                break;
            }
            CP.SetupTty(fileno(stdin), true);
        }
    }
    succumb(EXIT_NORMAL, false);
    return (0);
}


//
// Timer - this is used for the licensing system periodic test, UNIX
// only.
//

namespace {
    unsigned long elapsed_time;

#ifdef WIN32

    void CALLBACK timer_thread_cb(HWND, UINT, UINT, DWORD)
    { 
        elapsed_time = millisec(); 
    } 

#else

    void alarm_hdlr(int)
    {
#ifdef HAVE_SIGACTION
#else
        signal(SIGALRM, alarm_hdlr);
#endif

        static unsigned long die_when;
        elapsed_time = millisec();
        if (die_when && elapsed_time > die_when) {
            fatal(true);
#ifdef SIGKILL
            raise(SIGKILL);
#endif
            exit(1);
        }
#ifdef HAVE_SECURE
        char *s = Auth.periodicTest(elapsed_time);
        if (s) {
            delete [] s;
            die_when = elapsed_time + AC_LIFETIME_MINUTES*60*1000;
        }
#endif
    }

    typedef void(*sighdlr)(int, siginfo_t*, void*);
#endif


    // Start an asynchronous timer which increments the elapsed_time
    // parameter.  This is used for the security periodic test, Unix
    // only.
    //
    void start_timer()
    {
#ifdef WIN32
        // Not used
        // SetTimer(0, 1000, 60000, timer_thread_cb);
        (void)timer_thread_cb;
#else
        itimerval it;
        it.it_value.tv_sec = 60;
        it.it_value.tv_usec = 0;
        it.it_interval.tv_sec = 60;
        it.it_interval.tv_usec = 0;
        setitimer(ITIMER_REAL, &it, 0);

#ifdef HAVE_SIGACTION
        struct sigaction sa; 
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sa.sa_handler = alarm_hdlr;
        sigaction(SIGALRM, &sa, 0);
#else
        signal(SIGALRM, alarm_hdlr);
#endif

#endif
    }

#ifdef HAVE_SIGACTION
#else
#define SIG_HDLR IFsimulator::SigHdlr
#endif

    // When in interactive mode and the rdline interface is active,
    // the terminal can be left in a fouled up state if the program
    // terminates unexpectedly.  Make sure all interrupts exit
    // cleanly.
    //
    void init_sigs()
    {
#ifdef HAVE_SIGACTION
        struct sigaction sa; 
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sa.sa_sigaction = (sighdlr)IFsimulator::SigHdlr;

        sigaction(SIGINT, &sa, 0);
#ifdef SIGTSTP
        if (!Sp.GetFlag(FT_BATCHMODE)) {
            sigaction(SIGTSTP, &sa, 0);
            sigaction(SIGCONT, &sa, 0);
        }
#endif
#ifdef SIGHUP
        sigaction(SIGHUP,   &sa, 0);
#endif
#ifdef SIGQUIT
        sigaction(SIGQUIT,  &sa, 0);
#endif
#ifdef SIGTRAP
        sigaction(SIGTRAP,  &sa, 0);
#endif
#ifdef SIGABRT
        sigaction(SIGABRT,  &sa, 0);
#endif
#ifdef SIGEMT
        sigaction(SIGEMT,   &sa, 0);
#endif
#ifdef SIGPIPE
        sigaction(SIGPIPE,  &sa, 0);
#endif
#ifdef SIGTERM
        sigaction(SIGTERM,  &sa, 0);
#endif
#ifdef SIGXCPU
        sigaction(SIGXCPU,  &sa, 0);
#endif
#ifdef SIGXFSZ
        sigaction(SIGXFSZ,  &sa, 0);
#endif
#ifdef SIGVTALRM
        sigaction(SIGVTALRM, &sa, 0);
#endif
#ifdef SIGPROF
        sigaction(SIGPROF,  &sa, 0);
#endif
#ifdef SIGWINCH
        sigaction(SIGWINCH, &sa, 0);
#endif
#ifdef SIGUSR1
        sigaction(SIGUSR1,  &sa, 0);
#endif
#ifdef SIGUSR2
        sigaction(SIGUSR2,  &sa, 0);
#endif
#ifdef SIGURG
        sigaction(SIGURG,   &sa, 0);
#endif
#ifdef SIGCHLD
        sigaction(SIGCHLD,  &sa, 0);
#endif
#ifdef SIGSYS
        sigaction(SIGSYS,   &sa, 0);
#endif

        sa.sa_flags = SA_SIGINFO;
        sa.sa_sigaction = (sighdlr)IFsimulator::SigHdlr;

#ifdef SIGSEGV
        sigaction(SIGSEGV,  &sa, 0);
#endif
#ifdef SIGBUS
        sigaction(SIGBUS,   &sa, 0);
#endif
#ifdef SIGILL
        sigaction(SIGILL,   &sa, 0);
#endif
#ifdef SIGFPE
        sigaction(SIGFPE,   &sa, 0);
#endif
#ifdef SIGIO
        signal(SIGIO,  SIG_IGN);
#endif

#else
#ifdef HAVE_SIGNAL
        // Set up signal handling
        signal(SIGINT, SIG_HDLR);

#ifdef SIGTSTP
        if (!Sp.GetFlag(FT_BATCHMODE)) {
            signal(SIGTSTP, SIG_HDLR);
            signal(SIGCONT, SIG_HDLR);
        }
#endif
#ifdef SIGHUP
        signal(SIGHUP,  SIG_HDLR);
#endif
#ifdef SIGQUIT
        signal(SIGQUIT, SIG_HDLR);
#endif
#ifdef SIGTRAP
        signal(SIGTRAP, SIG_HDLR);
#endif
#ifdef SIGABRT
        signal(SIGABRT, SIG_HDLR);
#endif
#ifdef SIGEMT
        signal(SIGEMT,  SIG_HDLR);
#endif
#ifdef SIGPIPE
        signal(SIGPIPE, SIG_HDLR);
#endif
#ifdef SIGTERM
        signal(SIGTERM, SIG_HDLR);
#endif
#ifdef SIGXCPU
        signal(SIGXCPU, SIG_HDLR);
#endif
#ifdef SIGXFSZ
        signal(SIGXFSZ, SIG_HDLR);
#endif
#ifdef SIGVTALRM
        signal(SIGVTALRM, SIG_HDLR);
#endif
#ifdef SIGPROF
        signal(SIGPROF, SIG_HDLR);
#endif
#ifdef SIGWINCH
        signal(SIGWINCH, SIG_HDLR);
#endif
#ifdef SIGUSR1
        signal(SIGUSR1, SIG_HDLR);
#endif
#ifdef SIGUSR2
        signal(SIGUSR2, SIG_HDLR);
#endif
#ifdef SIGURG
        signal(SIGURG,  SIG_HDLR);
#endif
#ifdef SIGCHLD
        signal(SIGCHLD, SIG_HDLR);
#endif
#ifdef SIGSYS
        signal(SIGSYS,  SIG_HDLR);
#endif
#ifdef SIGSEGV
        signal(SIGSEGV, SIG_HDLR);
#endif
#ifdef SIGILL
        signal(SIGILL,  SIG_HDLR);
#endif
#ifdef SIGFPE
        signal(SIGFPE,  SIG_HDLR);
#endif
#ifdef SIGIO
        signal(SIGIO,  SIG_IGN);
#endif

#endif
#endif
    }
}


#ifdef WIN32
#define CLOSESOCKET(x) shutdown(x, SD_SEND), closesocket(x)
#else
#define CLOSESOCKET(x) close(x)
#endif

namespace {
    void process_args(int *acp, char **av)
    {
        wordlist *modend = 0;
        int ac = *acp;
        for (int j = 1; j < ac; ) {
            if (*av[j] != Global.OptChar()) {   // Option argument
                j++;
                continue;
            }
            switch (av[j][1]) {

            case '\0':  // No raw file
                CmdLineOpts.nogdata = true;
                shift(j, &ac, av);
                break;

            case 'b':   // Batch mode
            case 'B':
                if (av[j][2] && !isspace(av[j][2])) {
                    fprintf(stderr, usage, CP.Program());
                    exit(EXIT_BAD);
                }
                CmdLineOpts.batchmode = true;
                shift(j, &ac, av);
                break;

            case 'c':   // Case flags string
            case 'C':
                if (av[j][2] && !isspace(av[j][2])) {
                    fprintf(stderr, usage, CP.Program());
                    exit(EXIT_BAD);
                }
                if (ac > j) {
                    shift(j, &ac, av);
                    CmdLineOpts.caseflags = lstring::copy(av[j]);
                }
                else {
                    fprintf(stderr, usage, CP.Program());
                    exit(EXIT_BAD);
                }
                shift(j, &ac, av);
                break;

            case 's':   // Server mode
            case 'S':
                if (av[j][2] && !isspace(av[j][2])) {
                    fprintf(stderr, usage, CP.Program());
                    exit(EXIT_BAD);
                }
                CmdLineOpts.servermode = true;
                shift(j, &ac, av);
                break;

            case 'i':   // Interactive mode, also if given after -p or -j
            case 'I':   // turns on toolbar
                if (av[j][2] && !isspace(av[j][2])) {
                    fprintf(stderr, usage, CP.Program());
                    exit(EXIT_BAD);
                }
                CmdLineOpts.interactive = true;
                CmdLineOpts.notoolbar = false;
                shift(j, &ac, av);
                break;

            case 'l':   // License host[:port]
            case 'L':
                if (ac > j) {
                    if (av[j][2] && !isspace(av[j][2])) {
                        fprintf(stderr, usage, CP.Program());
                        exit(EXIT_BAD);
                    }
                    shift(j, &ac, av);
#ifdef HAVE_SECURE
                    Auth.set_validation_host(av[j]);
#endif
                }
                else {
                    fprintf(stderr, usage, CP.Program());
                    exit(EXIT_BAD);
                }
                shift(j, &ac, av);
                break;

            case 'p':   // Port monitor mode
            case 'P':
                if (av[j][2] && !isspace(av[j][2])) {
                    fprintf(stderr, usage, CP.Program());
                    exit(EXIT_BAD);
                }
                CmdLineOpts.portmon = true;
                CmdLineOpts.interactive = true;
                CmdLineOpts.notoolbar = true;
                shift(j, &ac, av);
                break;

            case 'q':   // No command completion
            case 'Q':
                if (av[j][2] && !isspace(av[j][2])) {
                    fprintf(stderr, usage, CP.Program());
                    exit(EXIT_BAD);
                }
                CmdLineOpts.nocmdcomplete = true;
                shift(j, &ac, av);
                break;

            case 'j':   // Jspice3 compatibility (no toolbar, msgs to stderr)
            case 'J':
                if (av[j][2] && !isspace(av[j][2])) {
                    fprintf(stderr, usage, CP.Program());
                    exit(EXIT_BAD);
                }
                CmdLineOpts.notoolbar = true;
                Sp.SetFlag(FT_JS3EMU, true);
                // This sets FT_NOERRWIN.
                Sp.SetVar(kw_noerrwin);
                // These set the jspice3 subcircuit expansion method.
                Sp.SetVar(kw_subc_catmode, "spice3");
                Sp.SetVar(kw_subc_catchar, ":");
                shift(j, &ac, av);
                break;

            case 'm':
            case 'M':
                if (!lstring::cieq(av[j] + 2, "none")) {
                    if (av[j][2] && !isspace(av[j][2])) {
                        fprintf(stderr, usage, CP.Program());
                        exit(EXIT_BAD);
                    }
                    if (ac > j) {
                        shift(j, &ac, av);
                        // Save for later.
                        if (!modend) {
                            CmdLineOpts.modules = modend =
                                new wordlist(lstring::copy(av[j]), 0);
                        }
                        else {
                            modend->wl_next =
                                new wordlist(lstring::copy(av[j]), 0);
                            modend = modend->wl_next;
                        }
                    }
                    else {
                        fprintf(stderr, usage, CP.Program());
                        exit(EXIT_BAD);
                    }
                }
                Sp.SetVar(kw_nomodload);
                shift(j, &ac, av);
                break;

            case 'n':   // Don't read .spiceinit
            case 'N':
                if (av[j][2] && !isspace(av[j][2])) {
                    fprintf(stderr, usage, CP.Program());
                    exit(EXIT_BAD);
                }
                CmdLineOpts.noreadinit = true;
                shift(j, &ac, av);
                break;

            case 't':   // Terminal type
            case 'T':
                if (ac > j) {
                    if (av[j][2] && !isspace(av[j][2])) {
                        fprintf(stderr, usage, CP.Program());
                        exit(EXIT_BAD);
                    }
                    shift(j, &ac, av);
                    CmdLineOpts.term = lstring::copy(av[j]);
                }
                else {
                    fprintf(stderr, usage, CP.Program());
                    exit(EXIT_BAD);
                }
                shift(j, &ac, av);
                break;

            case 'd':   // X display
            case 'D':
                if (lstring::cieq(av[j] + 2, "none")) {
                    CmdLineOpts.display = lstring::copy("none");
                    shift(j, &ac, av);
                }
                else {
                    // leave this in for X
                    if (av[j][2] && !isspace(av[j][2])) {
                        fprintf(stderr, usage, CP.Program());
                        exit(EXIT_BAD);
                    }
                    av[j++] = lstring::copy("-display");
                    if (ac > j)
                        CmdLineOpts.display = lstring::copy(av[j++]);
                    else {
                        fprintf(stderr, usage, CP.Program());
                        exit(EXIT_BAD);
                    }
                }
                break;

            case 'r':   // The rawfile
            case 'R':
                if (ac > j) {
                    if (av[j][2] && !isspace(av[j][2])) {
                        fprintf(stderr, usage, CP.Program());
                        exit(EXIT_BAD);
                    }
                    shift(j, &ac, av);
                    CmdLineOpts.rawfile = lstring::copy(av[j]);
                }
                else {
                    fprintf(stderr, usage, CP.Program());
                    exit(EXIT_BAD);
                }
                Sp.SetFlag(FT_RAWFGIVEN, true);
                shift(j, &ac, av);
                break;

            case 'o':   // Output file
            case 'O':
                if (ac > j) {
                    if (av[j][2] && !isspace(av[j][2])) {
                        fprintf(stderr, usage, CP.Program());
                        exit(EXIT_BAD);
                    }
                    shift(j, &ac, av);
                    CmdLineOpts.output = lstring::copy(av[j]);
                }
                else {
                    fprintf(stderr, usage, CP.Program());
                    exit(EXIT_BAD);
                }
                shift(j, &ac, av);
                break;

            default:
                // Keep for now, may be a toolkit option, which will
                // be swallowed by GR.InitPkg.

                j++;
            }
        }
        *acp = ac;
    }


    void shift(int i, int *acp, char **av)
    {
        (*acp)--;
        while (i < *acp) {
            av[i] = av[i+1];
            i++;
        }
    }


    bool expiring;
}


void
fatal(int error)
{
    static bool in_fatal;

    if (in_fatal)
        exit(EXIT_BAD);
    in_fatal = true;
    if (Global.MemError()) {
        fprintf(stderr,
            "\n**********\n"
        "A memory fault was detected!  Please email ./wrspice_mem_errors.log\n"
            "to stevew@wrcad.com\n"
            "**********\n\n");
    }
    if (!Sp.GetFlag(FT_BATCHMODE)) {
        GP.HaltFullScreenGraphics();
        CP.SetupTty(fileno(stdin), true);
        if (CP.MesgSocket() >= 0)
            CLOSESOCKET(CP.MesgSocket());
        if (!error) {
            if (CP.GetFlag(CP_INTERACTIVE)) {
                char *cmd = getenv("SPICE_FUN_CMD");
                if (!cmd) {
                    printf("%s-%s done\n", Sp.Simulator(), Sp.Version());
                    printf("Thanks for using WRspice!\n");
                }
                else
                    system(cmd);
            }
        }
    }
#ifdef HAVE_SECURE
    Auth.closeValidation();
#endif
    if (error)
        panic_to_gdb();
    exit(EXIT_NORMAL);
}


namespace {
    void succumb(int how, bool immed)
    {
#ifdef HAVE_SECURE
        Auth.closeValidation();
#endif
        if (immed) {
            if (Global.FifoName())
                unlink(Global.FifoName());
            _exit(how);
        }
        exit(how);
    }


    // Address of faulting line, for debugging.
    void* DeathAddr;

    void panic_to_gdb()
    {
        expiring = true;
        char header[256];
        sprintf(header, "%s-%s %s (%s)", Global.Product(),
            Global.Version(), Global.OSname(), Global.TagString());

        miscutil::dump_backtrace(CP.Program(), header, 0, DeathAddr);
        if (access(GDB_OFILE, F_OK) == 0) {
            if (!getenv("SPICENOMAIL") && !getenv("XTNOMAIL")) {
                bool ret = miscutil::send_mail(Global.BugAddr(),
                    "WRspice crash report", "", GDB_OFILE);
                if (ret)
                    fprintf(stderr, "File %s emailed to %s.\n", GDB_OFILE,
                        Global.BugAddr());
                else
                    fprintf(stderr, "Failed to email %s file to %s, please"
                        " send manually.\n", GDB_OFILE,
                        Global.BugAddr());
            }
            else
                fprintf(stderr, "Please email %s file to %s.\n", GDB_OFILE,
                    Global.BugAddr());
        }
        exit(EXIT_BAD);
    }
}


#if defined(HAVE_GTK1) || defined(HAVE_GTK2)
namespace {
    //
    // Some setup code for the GTK library.
    //
    void setup_gtk()
    {
#if defined(HAVE_GTK2) && defined(HAVE_LOCAL_ALLOCATOR)
        // Recent gtk2 releases use a "g_slice" memory manager which seg
        // faults when our internal memory manager is used.  This seems to
        // fix the problem.
        putenv(lstring::copy("G_SLICE=always-malloc"));
#endif

#if defined(HAVE_GTK1) && !defined(DYNAMIC_LIBS)
        // There is a portability problem with the statically-linked
        // gtk when theme engines are imported.  In particular, libpng
        // has very restrictive verson matching, and if an engine
        // (such as eazel) tries to read a png image, and the shared
        // code is compiled against a different libpng release than
        // the one we supply, the application will exit.  Thus, we (by
        // default) turn off themes generally, and supply our own,
        // known to work.

        if (getenv("XT_USE_GTK_THEMES"))
            return;
        char *rcp = pathlist::mk_path(
            Global.StartupDir(), "default_theme/gtkrc");
        if (!access(rcp, R_OK)) {

            // We're going to check for gtkrc in default_theme and its
            // parent dir, parent dir last so it has precedence.  If the
            // user modifies gtkrc, it should be copied to the parent dir
            // so it won't get clobbered by a software update.

            char *v1 = new char[2*strlen(Global.StartupDir()) + 42];
            sprintf(v1, "%s=%s/%s:%s/%s", "GTK_RC_FILES", Global.StartupDir(),
                "default_theme/gtkrc", Global.StartupDir(), "gtkrc");
            putenv(v1);

            char *v2 = new char[strlen(Global.StartupDir()) + 32];
            sprintf(v2, "%s=%s/%s", "GTK_EXE_PREFIX", Global.StartupDir(),
                "default_theme");
            putenv(v2);
        }
        else
            // Don't read any gtkrc files, so no themes
            putenv(lstring::copy("GTK_RC_FILES="));
        delete [] rcp;
#endif
    }
}
#endif


namespace {
    // If str contains white space or ':', return true.
    //
    inline bool needs_quote(const char *str)
    {
        for (const char *s = str; *s; s++) {
            if (isspace(*s) || *s == ':')
                return (true);
        }
        return (false);
    }
}


void
sGlobal::initialize(const char *argv_0)
{
    g_product = "WRspice";
    g_version = SPICE_VERSION;
    g_devlib_version = DEVLIB_VERSION;
    g_tag_string = WRS_RELEASE_TAG;
    g_notice = SPICE_NOTICE;
    g_build_date = SPICE_BUILD_DATE;
    g_os_name = OSNAME;
    g_arch = ARCH;
    g_dist_suffix = DIST_SUFFIX;
    g_gfx_prog_name = GFX_PROG;
    g_fifo_name = 0;

    g_tools_root = TOOLS_ROOT;
    g_app_root = APP_ROOT;

    const char *string = getenv("WRSPICE_HOME");
    if (string && *string) {
        g_program_root = lstring::copy(string);

        // The g_prefix is used by the software installer, which puts
        // all tools under an "xictools" directory.  Update the
        // g_prefix if WRSPICE_HOME follows this convention.

        char *pfx = lstring::copy(string);
        char *tp = pathlist::mk_path(TOOLS_ROOT, APP_ROOT);
        char *sp = strstr(pfx, tp);
        delete [] tp;
        if (sp) {     
            sp[-1] = 0;
            g_prefix = pfx;
        }
        else
            delete [] pfx;
    }
    if (!g_program_root) {
        const char *pfx = getenv("XT_PREFIX");
        if (pfx && *pfx) {
            char *p1 = pathlist::mk_path("/", pfx);
            char *p2 = pathlist::mk_path(p1, TOOLS_ROOT);
            delete [] p1;
            g_program_root = pathlist::mk_path(p2, APP_ROOT);
            delete [] p2;
            g_prefix = lstring::copy(pfx);
        }
    }
#ifdef WIN32
    if (!g_program_root)
        g_program_root = msw::GetProgramRoot(Product());
#endif
    if (!g_program_root) {
        char *p1 = pathlist::mk_path("/", PREFIX);
        char *p2 = pathlist::mk_path(p1, TOOLS_ROOT);
        delete [] p1;
        g_program_root = pathlist::mk_path(p2, APP_ROOT);
        delete [] p2;
    }
    if (!g_prefix)
        g_prefix = lstring::copy(PREFIX);

    // directory for startup files
    string = getenv("SPICE_LIB_DIR");
    if (string && *string)
        g_startup_dir = lstring::copy(string);
    else {
        char *pth = pathlist::mk_path(ProgramRoot(), "startup");
        g_startup_dir = pathlist::expand_path(pth, true, true);
        delete [] pth;
    }

    // search path for scripts, circuits
    string = getenv("SPICE_INP_PATH");
    if (string && *string)
        g_input_path = lstring::copy(string);
    else {
        const char *root = ProgramRoot();
        bool nq = needs_quote(root);
        sLstr lstr;
        lstr.add("( . ");
        if (nq)
            lstr.add_c('"');
        char *pth = pathlist::mk_path(root, "scripts");
        lstr.add(pth);
        delete [] pth;
        if (nq)
            lstr.add_c('"');
        lstr.add(" )");
        g_input_path = lstr.string_trim();
    }

    // search path for help database
    string = getenv("SPICE_HLP_PATH");
    if (string && *string)
        g_help_path = lstring::copy(string);
    else {
        const char *root = ProgramRoot();
        bool nq = needs_quote(root);
        sLstr lstr;
        lstr.add("( ");
        if (nq)
            lstr.add_c('"');
        char *pth = pathlist::mk_path(root, "help");
        lstr.add(pth);
        delete [] pth;
        if (nq)
            lstr.add_c('"');
        lstr.add(" )");
        g_help_path = lstr.string_trim();
    }

    // path to release notes
    string = getenv("SPICE_DOCS_DIR");
    if (string && *string)
        g_docs_dir = lstring::copy(string);
    else {
        char *pth = pathlist::mk_path(ProgramRoot(), "docs");
        g_docs_dir = pathlist::expand_path(pth, true, true);
        delete [] pth;
    }

    // executables directory
    string = getenv("SPICE_EXEC_DIR");
    if (string && *string)
        g_exec_dir = lstring::copy(string);
    else {
        const char *root = ProgramRoot();
        char *p = pathlist::expand_path(root, true, true);
        char *e = lstring::strrdirsep(p);
        if (e)
            *e = 0;
        g_exec_dir = pathlist::mk_path(p, "bin");
        delete [] p;
    }

    char *s;

    // Path to program binary, for aspice and for panic-to-gdb.
    g_exec_prog = pathlist::get_bin_path(argv_0);

    // news file
    string = getenv("SPICE_NEWS_FILE");
    if (string == 0)
        s = pathlist::mk_path(StartupDir(), SPICE_NEWS);
    else if (lstring::is_rooted(string))
        s = lstring::copy(string);
    else
        s = pathlist::mk_path(StartupDir(), string);
    g_news_file = s;

    // text editor
    string = getenv("SPICE_EDITOR");
    if (string == 0)
        string = getenv("EDITOR");
    if (string == 0 || *string == 0 || lstring::cieq(string, "default"))
        string = SPICE_EDITOR;
    g_editor = lstring::copy(string);

    // address for bug reports
    string = getenv("SPICE_BUGADDR");
    if (string == 0)
        string = BUG_ADDR;
    g_bug_addr = lstring::copy(string);

    // host for rspice
    string = getenv("SPICE_HOST");
    if (string == 0)
        string = SPICE_HOST;
    g_host = lstring::copy(string);

    // log file for daemon
    string = getenv("SPICE_DAEMONLOG");
    if (string == 0)
        string = SPICE_DAEMONLOG;
    g_daemon_log = lstring::copy(string);

    // command line option prefix
    string = getenv("SPICE_OPTCHAR");
    if (string == 0)
        string = SPICE_OPTCHAR;
    g_opt_char = *string;

    // ASCII format output
    string = getenv("SPICE_ASCIIRAWFILE");
    if (string == 0)
        string = SPICE_ASCIIRAWFILE;
    if (*string == '0' || *string == 'f' || *string == 'F' ||
            *string == 'n' || *string == 'N')
        g_ascii_out = false;
    else
        g_ascii_out = true;

    g_mem_error = false;
#ifndef WIN32
    setup_gtk();
#endif
}


// These are always available.
#ifndef AN_op
#define ANop
#endif
#ifndef AN_dc
#define AN_dc
#endif

IFanalysis *IFanalysis::analyses[] = {
#ifdef AN_op
    &DCOinfo,
#endif
#ifdef AN_dc
    &DCTinfo,
#endif
#ifdef AN_tran
    &TRANinfo,
#endif
#ifdef AN_ac
    &ACinfo,
#endif
#ifdef AN_pz
    &PZinfo,
#endif
#ifdef AN_sense
    &SENSinfo,
#endif
#ifdef AN_tf
    &TFinfo,
#endif
#ifdef AN_disto
    &DISTOinfo,
#endif
#ifdef AN_noise
    &NOISEinfo,
#endif
    0  // important
};

int IFanalysis::num_analyses =
    (int)(sizeof(IFanalysis::analyses)/sizeof(IFanalysis*));

IFparm sCKTnode::params[] = {
    IP("nodeset", PARM_NS, IF_REAL, "suggested initial voltage"),
    IP("ic",      PARM_IC, IF_REAL, "initial voltage"),
    IP("type",    PARM_NODETYPE, IF_INTEGER, "output type of equation")
};
int sCKTnode::num_params = (int)(sizeof(sCKTnode::params)/sizeof(IFparm));


IFsimulator::IFsimulator()
{
    ft_simulator = SPICE_PROG;
    ft_description = "circuit simulation program";
    ft_version = SPICE_VERSION;

    DEV.init();

    // Set up the hash table for 'set' variables.
    // This is case insensitive, so that setting shell variables itl1, ITL1
    // are recognized properly as options when the circuit is run.
    ft_options = new sHtab(true);

    // Character used to separate generated name fields in subcircuit
    //
    ft_subc_catchar = DEF_SUBC_CATCHAR;

    // This can follow a number, separating a units string from the number.
    //
    ft_units_catchar = DEF_UNITS_CATCHAR;

    // Character used to separate numerator units from denominator
    // units in units string.
    //
    ft_units_sepchar = DEF_UNITS_SEPCHAR;

    // Character that separates plot name from vector name when referencing
    // vectors.
    //
    ft_plot_catchar = DEF_PLOT_CATCHAR;

    // Character that indicates a "special" vector.
    //
    ft_spec_catchar = DEF_SPEC_CATCHAR;

    ft_subc_catmode = SUBC_CATMODE_WR;
};


// Static function.
// Return in p the path to the startup file.  Return true if the file
// exists and is accessible.
//
bool
IFsimulator::StartupFileName(char **p)
{
    char sbuf[64];
    USR_STARTUP(sbuf);
    char *path = 0;
    char *home = pathlist::get_home(0);
    if (home) {
        path = pathlist::mk_path(home, sbuf);
        delete [] home;
    }
    else
        path = lstring::copy(sbuf);
    bool ret = !access(path, F_OK);
    if (p)
        *p = path;
    else
        delete [] path;
    return (ret);
}


#ifdef HAVE_SIGNAL

namespace {
    int idle_id;

    int ft_restart(void*)
    {
        idle_id = 0;
        CP.ResetControl();
        if (!CP.GetFlag(CP_NOTTYIO))
            TTY.out_printf("\n");
#ifdef WIN32
        longjmp(msw_jbf[msw_jbf_sp], 1);
#else
        throw SIGINT;
#endif
    }
}


// Static function.
// The signal handler.
//
#ifdef HAVE_SIGACTION
void
IFsimulator::SigHdlr(int sig, void *si, void*)
#else
void
IFsimulator::SigHdlr(int sig)
#endif
{
#ifdef HAVE_SIGACTION
    struct sigaction sa; 
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_sigaction = (sighdlr)IFsimulator::SigHdlr;

    // This should put the address of a faulting line in DeathAddr.
    //
    if (sig == SIGSEGV || sig == SIGBUS || sig == SIGILL || sig == SIGFPE) {
        if (!DeathAddr)
            DeathAddr = ((siginfo_t*)si)->si_addr;
    }
#else
    signal(sig, SIG_HDLR);  // for SysV
#endif

    if (sig == SIGINT) {
        if (Sp.GetFlag(FT_BATCHMODE))
            succumb(EXIT_BAD, true);

        Sp.SetFlag(FT_INTERRUPT, true);
        CP.SetFlag(CP_INTRPT, true);
        if (Sp.GetFlag(FT_SIMFLAG) || CmdLineOpts.portmon)
            return;

#ifdef WIN32
        if (InMalloc) {
            GotSigInMalloc = true;
            return;
        }
#endif

        // Must use an idle function to throw the exception, as
        // throwing an exception in a signal handler produces
        // undefined results.  Without the graphics event loop
        // (CP.Display() set) idle procedure calls are not available. 
        // The exception causes the command loop to restart, aborting
        // the present operation and getting a new prompt.  Without
        // the exception, pausing the analysis still works fine.

        if (CP.Display()) {
            if (idle_id)
                ToolBar()->RemoveIdleProc(idle_id);
            idle_id = ToolBar()->RegisterIdleProc(ft_restart, 0);
        }
    }

#ifdef SIGCHLD
    else if (sig == SIGCHLD)
        Proc()->SigchldHandler();
#endif

#ifdef SIGURG
    else if (sig == SIGURG) {
        // this is received when oob data arrives, which indicates
        // interrupt for remote run
        if (CP.MesgSocket() > 0) {
            int c;
            recv(CP.MesgSocket(), (char*)&c, 1, MSG_OOB);
        }
        raise(SIGINT);
        return;
    }
#endif
#ifdef SIGPIPE
    else if (sig == SIGPIPE) {
        fprintf(stderr, "broken pipe\n");
        succumb(EXIT_BAD, true);
    }
#endif
#ifdef SIGTSTP
    else if (sig == SIGTSTP) {
        if (!Sp.GetFlag(FT_BATCHMODE)) {
            CP.SetupTty(fileno(stdin), true);
            signal(SIGTSTP, SIG_DFL);
            raise(SIGTSTP);
        }
    }
    else if (sig == SIGCONT) {
        if (!Sp.GetFlag(FT_BATCHMODE)) {
            if (expiring)
                return;
            if (TTY.getpgrp() != getpid()) {
                // we're in the background
                return;
            }
            CP.SetupTty(fileno(stdin), false);
#ifdef HAVE_SIGACTION
            sigaction(SIGTSTP, &sa, 0);
#else
            signal(SIGTSTP, SIG_HDLR);
#endif
#ifdef __linux
            // Throwing an exception from a signal handler produces
            // undefined results, but seems to work ok in linux.
            // OS X will terminate.
            if (CP.GetFlag(CP_CWAIT))
                throw SIGCONT;
#endif
        }
    }
#endif
    else if (sig == SIGFPE) {
#ifdef WIN32
        // We don't have a second arg to the signal handler, but
        // the status flags are still good.
        //
        const char *msg = "unknown floating point error";
        if (fetestexcept(FE_DIVBYZERO))
            msg = "floating point divide by zero";
        else if (fetestexcept(FE_OVERFLOW))
            msg = "floating point overflow";
        else if (fetestexcept(FE_INVALID))
            msg = "invalid floating point operation";
        feclearexcept(FE_ALL_EXCEPT);
        GRpkgIf()->ErrPrintf(ET_ERROR, "math error, %s.\n", msg);
#else
        siginfo_t *info = (siginfo_t*)si;
        Sp.FPexception(info->si_code);
#endif
        // We can't return, as this causes an infinite loop.
        fprintf(stderr, "\nUnhandled floating point exception, exiting.\n");
        fatal(false);  // No gdb/email in this case.
    }
    else if (sig == SIGILL) {
        fprintf(stderr, "\ninternal error -- illegal instruction\n");
        fatal(true);
    }
#ifdef SIGBUS
    else if (sig == SIGBUS) {
        fprintf(stderr, "\ninternal error -- bus error\n");
        fatal(true);
    }
#endif
#ifdef SIGSEGV
    else if (sig == SIGSEGV) {
        fprintf(stderr, "\ninternal error -- segmentation violation\n");
        fatal(true);
    }
#endif
#ifdef SIGSYS
    else if (sig == SIGSYS) {
        fprintf(stderr, "\ninternal error -- bad argument to system call\n");
        fatal(true);
    }
#endif
#ifdef SIGABRT
    else if (sig == SIGABRT) {
        // For FreeBSD, MALLOC_OPTIONS=A, we can't call anything
        // that calls malloc(), etc., including
        // aAuthChk::closeValidation(), or we get a loop.

        fprintf(stderr, "\nReceived signal %d, exiting.\n", sig);
        if (!Sp.GetFlag(FT_BATCHMODE)) {
            GP.HaltFullScreenGraphics();
            CP.SetupTty(fileno(stdin), true);
            if (CP.MesgSocket() >= 0)
                CLOSESOCKET(CP.MesgSocket());
            ToolBar()->CloseGraphicsConnection();
        }
        if (Global.FifoName())
            unlink(Global.FifoName());
        _exit(EXIT_BAD);  // avoid global destructors
    }
#endif
#ifdef SIGWINCH
    else if (sig == SIGWINCH) {
        TTY.init_more();
    }
#endif
    else {
        // Under FreeBSD, sending HUP generates a SIGCONT after the
        // fprintf below, so application continues after announcing
        // termination.
#ifdef SIGCONT
        signal(SIGCONT, SIG_IGN);
#endif
        fprintf(stderr, "\nReceived signal %d, exiting.\n", sig);
        succumb(EXIT_BAD, true);
    }
}

#endif

