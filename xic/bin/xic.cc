
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2007 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  WHITELEY RESEARCH INCORPORATED PROPRIETARY SOFTWARE                   *
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
 * XIC Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id: xic.cc,v 5.247 2017/04/16 20:27:55 stevew Exp $
 *========================================================================*/

#include "config.h"
#include "secure.h"
#include "main.h"
#include "oa_if.h"
#include "python_if.h"
#include "tcltk_if.h"
#include "mrouter_if.h"
#include "fio.h"
#include "cd_variable.h"
#include "edit.h"
#include "sced.h"
#include "sced_spiceipc.h"
#include "drc.h"
#include "ext.h"
#include "cd_lgen.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "dsp_layer.h"
#include "si_parsenode.h"
#include "si_parser.h"
#include "si_interp.h"
#include "si_lisp.h"
#include "pcell_params.h"
#include "tech.h"
#include "menu.h"
#include "cvrt.h"
#include "errorlog.h"
#include "events.h"
#include "promptline.h"
#include "select.h"
#include "layertab.h"
#include "filetool.h"
#include "ghost.h"
#include "timer.h"
#include "reltag.h" // defines CVS_RELEASE_TAG
#include "pathlist.h"
#include "filestat.h"
#include "tvals.h"
#include "grfont.h"
#include "timedbg.h"
#include "update_itf.h"
#include "crypt.h"

#include "file_menu.h"
#include "cell_menu.h"
#include "edit_menu.h"
#include "view_menu.h"
#include "attr_menu.h"
#include "cvrt_menu.h"
#include "drc_menu.h"
#include "ext_menu.h"
#include "user_menu.h"
#include "help_menu.h"
#include "pbtn_menu.h"
#include "ebtn_menu.h"

#include <errno.h>
#include <sys/time.h>

#ifdef HAVE_FENV_H
#include <fenv.h>
#endif

#ifdef WIN32
#include <conio.h>
#include "msw.h"
#include "miscutil.h"
#include "stackdump.h"
#endif

// Set up the ginterf package, include all drivers.
//
#define GR_CONFIG (GR_ALL_PKGS | GR_ALL_DRIVERS)
#include "gr_pkg_setup.h"

// FreeBSD malloc resets memory allocated/freed to 0x0a.
// Need to bypass local malloc to use this.
// const char *_malloc_options = "J";

// These are set as compiler -D defines
#ifndef PREFIX
#define PREFIX "/usr/local"
#endif
#ifndef TOOLS_ROOT
#define TOOLS_ROOT "xictools"
#endif
#ifndef APP_ROOT
#define APP_ROOT "xic"
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
#ifndef VERSION_STR
#define VERSION_STR "unknown"
#endif
#ifndef SPICE_PROG
#define SPICE_PROG ""
#endif
#ifndef BUG_ADDR
#define BUG_ADDR "xic@wrcad.com"
#endif
#ifndef BUILD_DATE
#define BUILD_DATE "Mon Mar  4 15:42:16 PST 2013"
#endif

namespace {
    // Default encoded password for script decryption.  Positions 7-20 is
    // the encoded default password, the first 7 characters are magic and
    // holy, see cryptmain.cc
    // The default password is "qwerty"
    const char DefaultPassword[] = "`$Y%Z~@mBD0JFon2uVsw@@@@@@@@@@@";
}

#ifdef WIN32

// Dump a stack backtrace if unhandled exception.
namespace {
    cStackDump _stackdump_(GDB_OFILE);
}

// If the application has this name, it will behave as a filetool
// program.
#define FILETOOL "filetool.exe"

// Tell the msw interface that we're Generation 4.
const char *msw::MSWpkgSuffix = "-4";

// in debug.c
extern "C" { void SetupStackDump(TCHAR*); }

namespace {
    bool pause = false;
    const char *pausemsg = "Press any key to exit...\n";
}
#define PAUSE() if (pause) { printf(pausemsg); getch(); }

#else

// If the application has this name, it will behave as a filetool
// program.
#define FILETOOL "filetool"

#define PAUSE()
#endif


// Class for command line parsing.
//
class sCmdLine
{
public:
    sCmdLine()
        {
            NumArgs = 0;
            Args = 0;
            CmapSaver = 0;
            CmdFile = 0;
            ServerPort = -1;
            ImaFileTool = false;
        }

    const char *next_arg()
        {
            if (NumArgs > 1) {
                Args++;
                NumArgs--;
                return (*Args);
            }
            return (0);
        }

    void shift(int i, int argc, char **argv) const
        {
            while (i < argc) {
                argv[i] = argv[i+1];
                i++;
            }
        }

    void process_args(int, char**, bool);

    int  NumArgs;
    char **Args;
    int CmapSaver;       // if 1 no dual plane, if 2 no private colors
    char *CmdFile;       // optional startup commands
    int ServerPort;      // port number, server mode
    bool ImaFileTool;    // application filetool polymorph
};
namespace {sCmdLine cmdLine; }

namespace {
    // Under Windows, create detached process if set
    bool WinBg;

    struct updif : public updif_t
    {
        const char *HomeDir() const
            {
                return (XM()->HomeDir());
            }

        const char *Product() const
            {
                return (XM()->Product());
            }

        const char *VersionString() const
            {
                return (XM()->VersionString());
            }

        const char *OSname() const
            {
                return (XM()->OSname());
            }

        const char *Arch() const
            {
                return (XM()->Arch());
            }

        const char *DistSuffix() const
            {
                return (XM()->DistSuffix());
            }

        const char *Prefix() const
            {
                return (XM()->Prefix());
            }
    };
    updif main_itf;


    void
    check_for_update()
    {
        if (!XM()->UpdateIf())
            return;
        UpdIf udif(*XM()->UpdateIf());
        // Update check requires a password;
        if (!udif.username() || !udif.password())
            return;
        release_t my_rel = udif.my_version();
        if (my_rel == release_t(0))
            return;

        fprintf(stdout, "Checking for update...");
        fflush(stdout);

        release_t new_rel = udif.distrib_version();
        if (new_rel == release_t(0)) {
            const char *msg =
    "Unable to access the repository to check for updates.  Your password\n"
    "may have expired.  To stop this message from appearing you can\n"
    "purchase a maintenance extension, or set NoCheckUpdates, or delete\n"
    "your .wrpasswd file.";
            DSPmainWbag(PopUpMessage(msg, false))
        }
        else if (my_rel < new_rel) {
            char *my_rel_str = my_rel.string();
            char *new_rel_str = new_rel.string();
            char buf[256];
            sprintf(buf, "This release is %s.\n"
                "An update, %s, is available.\n"
                "Use \'!update\' to download/install.",
                my_rel_str, new_rel_str);
            delete [] my_rel_str;
            delete [] new_rel_str;

            DSPmainWbag(PopUpMessage(buf, false))
        }
        fprintf(stdout, " Done\n");
    }
}

// Export
const updif_t *cMain::xm_updif = &main_itf;

namespace {
    // Instantiate the components.
    cMain       _xm_;               // application class
    cTimeDbg    _tdb_;              // exec time measurement package
    cOAif       *_oa_;              // OpenAccess interface
    cPyIf       *_py_;              // Python interface
    cTclIf      *_tk_;              // Tcl/Tk interface
    cMRcmdIf    *_mr_;              // Router interface

    namespace xic_main {
        int start_proc(void*);
        int read_cell_proc(void*);
        void onexit();
        void do_batch_commands();
        void exec_batch_script(const char*);
    }
}
cSelections Selections;             // selections package

#if defined(HAVE_GTK1) && !defined(DYNAMIC_LIBS)
namespace { void setup_gtk() }
#endif

//-----------------------------------------------------------------------------
// Main function

int
main(int argc, char **argv)
{
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

    if (getenv("XIC64")) {
        fenv_t env;
        fegetenv(&env);
        env.__control_word &= ~0x300;
        env.__control_word |= 0x200;  // 64-bit precision
        printf("Using 64-bit x87 precision.\n");
        fesetenv(&env);
    }
    else if (getenv("XIC80")) {
        fenv_t env;
        fegetenv(&env);
        env.__control_word |= 0x300;  // 80-bit precision
        printf("Using 80-bit precision.\n");
        fesetenv(&env);
    }
#endif

    // Configure temp filename generator.
    filestat::make_temp_conf("xic", "XIC_TMP_DIR");

    // Initial pass to set RunMode.
    cmdLine.process_args(argc, argv, true);

#ifndef WIN32
    // If XIC_LIBRARY_PATH is found in the environment, prepend it to
    // LD_LIBRARY_PATH.  This will be used when loading plug-ins.
    // we actually have to start running again, as LD_LIBRARY_PATH
    // can't be changed in the current executable.
    //
    if (getenv("XIC_LIBRARY_PATH")) {
#ifdef __APPLE__
        extern char **environ;
#endif
        const char *xp = getenv("XIC_LIBRARY_PATH");
        if (*xp) {
            const char *lp = getenv("LD_LIBRARY_PATH");
            sLstr lstr;
            lstr.add(xp);
            if (lp && *lp) {
                lstr.add_c(':');
                lstr.add(lp);
            }
            setenv("LD_LIBRARY_PATH", lstr.string(), true);
            unsetenv("XIC_LIBRARY_PATH");
            execve(argv[0], &argv[0], environ);
        }
    }
#endif

#ifdef WIN32
    if (!getenv("XTNETDEBUG")) {
        if (!WinBg && XM()->RunMode() == ModeServer) {
            // This is the parent process.  Since we don't have
            // fork(), create a new process with WinBg set,
            // disconnected from the controlling terminal.

            char cmdline[256];
            GetModuleFileName(0, cmdline, 256);
            char *cmdstr = GetCommandLine();
            while (*cmdstr && !isspace(*cmdstr))
                cmdstr++;
            sprintf(cmdline + strlen(cmdline), "%s --WinBg", cmdstr);

            const char *logdir  = getenv("XIC_LOG_DIR");
            if (!logdir || !*logdir)
                logdir = getenv("XIC_TMP_DIR");
            if (!logdir || !*logdir)
                logdir = getenv("TMPDIR");
            if (!logdir || !*logdir)
                logdir = "/tmp";
            char *fname = new char[strlen(logdir) + 30];
            mkdir(logdir);
            sprintf(fname, "%s/%s", logdir, "daemon_out.log");
            HANDLE hlog = CreateFile(fname, GENERIC_WRITE, 0, 0, CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL, 0);
            sprintf(fname, "%s/%s", logdir, "daemon_err.log");
            HANDLE herr = CreateFile(fname, GENERIC_WRITE, 0, 0, CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL, 0);
            delete [] fname;

            PROCESS_INFORMATION *info = msw::NewProcess(cmdline,
                DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP,
                true, 0, hlog, herr);
            if (!info) {
                fprintf(stderr,
                    "Couldn't execute background process, exiting.\n");
                exit (1);
            }
            CloseHandle(info->hProcess);
            delete info;
            printf("Running...\n");
            exit (0);
        }

        HANDLE hcons = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO sinfo;
        GetConsoleScreenBufferInfo(hcons, &sinfo);
        if (sinfo.dwCursorPosition.X == 0 && sinfo.dwCursorPosition.Y == 0) {
            // This means that we have a new console, so the app was
            // started from an icon.  If the cwd is "c:\", which is
            // the default for the Start Menu shortcuts, try to cd to
            // somewhere more reasonable.

            pause = true;
            char *cwd = getcwd(0, 0);
            if (cwd && lstring::cieq(cwd, "c:\\")) {
                char *home = pathlist::get_home("XIC_START_DIR");
                if (home) {
                    chdir(home);
                    delete [] home;
                }
            }
            free(cwd);
        }
    }
#endif

    if (XM()->RunMode() == ModeNormal) {
#ifdef BDCODE
        printf("%s integrated circuit %s release %s, build %s\n"
            "Copyright (C) Whiteley Research Inc., Sunnyvale, CA  %4d\n"
            "All Rights Reserved\n\n", XM()->Product(), XM()->Description(),
            XM()->VersionString(), BDCODE, XM()->BuildYear());
#else
        printf("%s integrated circuit %s release %s\n"
            "Copyright (C) Whiteley Research Inc., Sunnyvale, CA  %4d\n"
            "All Rights Reserved\n\n", XM()->Product(), XM()->Description(),
            XM()->VersionString(), XM()->BuildYear());
#endif
    }

    // The "Program" is the rooted path to the running executable file.
    XM()->SetProgram(pathlist::get_bin_path(argv[0]));

    // copy the arg list
    char **av = new char*[argc+1];
    int i;
    for (i = 0; i < argc; i++)
        av[i] = argv[i];
    av[i] = 0;
    argv = av;

#ifdef WIN32
    // Initialize winsock, before validate.
    bool winsock_failed = false;
    WSADATA wsadata;
    if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0) {
        Log()->WarningLog(mh::Initialization,
            "Windows Socket Architecture initialization failed,\n"
            "no interprocess communication.\n");
        winsock_failed = true;
    }

    // The inno installer is sensitive to this, prevents install when
    // application is active.
    CreateMutex(0, false, "XicMutex");
#endif

    // Must be done before using LibPath below!
    XM()->InitializeVariables();

    int code =
        XM()->Auth()->validate(XIC_CODE, CDvdb()->getVariable(VA_LibPath));
    if (!code) {
        PAUSE();
        return (1);
    }

    // To detect feature level:
    // if (!EditIf()->hasEdit())        // Xiv level   (no editing)
    // else if (!ExtIf()->hasExtract()) // XicII level (no drc/extract/sced)
    // else                             // Xic level   (all features)

    if (code != XIC_DAEMON_CODE) {
        // We can force a lower level by setting these in the environment.
        if (getenv("FORCE_XIV"))
            code = XIV_CODE;
        else if (getenv("FORCE_XICII") && code != XIV_CODE)
            code = XICII_CODE;
    }

    if (code == XIC_DAEMON_CODE) {
        // This is like XIC_CODE but only ModeServer is allowed.
        if (XM()->RunMode() != ModeServer) {
            PAUSE();
            return (1);
        }
        new cEdit();                    // physical editing
        new cSced();                    // schematic editing
        new cDRC();                     // DRC package
        new cExt();                     // extraction package
    }
    else if (code == XIV_CODE) {
        new cEditIfStubs();             // no physical editing
        new cScedIfStubs();             // no schematic editing
        new cDrcIfStubs();              // no DRC package
        new cExtIfStubs();              // no extraction package
    }
    else if (code == XICII_CODE) {
        new cEdit();                    // physical editing
        new cScedIfStubs();             // no schematic editing
        new cDrcIfStubs();              // no DRC package
        new cExtIfStubs();              // no extraction package
    }
    else if (code == XIC_CODE) {
        new cEdit();                    // physical editing
        new cSced();                    // schematic editing
        new cDRC();                     // DRC package
        new cExt();                     // extraction package
    }
    else {
        // wtf?
        PAUSE();
        return (1);
    }

    unsigned int gmode;
    if (XM()->RunMode() != ModeNormal)
        gmode = GR_ALL_DRIVERS;
    else {
        gmode = (GR_ALL_PKGS | GR_ALL_DRIVERS);
#if defined(HAVE_GTK1) && !defined(DYNAMIC_LIBS)
        setup_gtk();
#endif
    }

    if (GRpkgIf()->InitPkg(gmode, &argc, argv)) {
        PAUSE();
        return (1);
    }

    cmdLine.process_args(argc, argv, false);

    if (GRpkgIf()->InitColormap(48,
            (cmdLine.CmapSaver == 2 ? -1 : 64), !cmdLine.CmapSaver)) {
        PAUSE();
        return (1);
    }
    XM()->InitializeMenus();

    if (EditIf()->hasEdit() && ExtIf()->hasExtract()) {
#ifdef WIN32
        if (winsock_failed)
            SCD()->spif()->SetParentSpPort(0);
#endif
        if (SCD()->spif()->ParentSpPort() > 0)
            SCD()->spif()->InitSpice();
    }

    XM()->InitSignals(false);
    GRpkgIf()->RegisterSigintHdlr(&cMain::InterruptHandler);

    GRwbag *gx = GRpkgIf()->NewWbag("Xic", dspPkgIf()->NewGX());
    if (!gx) {
        XM()->Auth()->closeValidation();
        PAUSE();
        return (1);
    }

    // Create the OpenAccess interface.  If this succeeds, the menus
    // created in Initialize below will contain the OpenAccess entries.
    // Really don't need the pointer, but it shuts up compiler warning.
    _oa_ = new cOAif;

    // Start Python interface.
    _py_ = new cPyIf;

    // Start Tcl/Tk interface.
    _tk_ = new cTclIf;

    if (dspPkgIf()->Initialize(gx)) {
        XM()->Auth()->closeValidation();
        PAUSE();
        return (1);
    }

    if (!Log()->OpenLogDir(lstring::strip_path(XM()->Program()))) {
        XM()->Auth()->closeValidation();
        PAUSE();
        return (1);
    }

    if (cmdLine.ImaFileTool) {
        XM()->ExecStartupScript(".filetoolrc");

        cFileTool ft(XM()->VersionString());
        bool ret = ft.run(cmdLine.NumArgs, cmdLine.Args);

        XM()->SetExitCleanupDone(true);
        XM()->Auth()->closeValidation();
        fflush(stdout);
        if (ret) {
            Log()->CloseLogDir();
            exit(0);
        }
        stringlist *sl = Log()->ListLogDir();
        if (sl) {
            fprintf(stderr, "Operation failed, logfiles in %s retained.\n",
                Log()->LogDirectory());
            stringlist::destroy(sl);
        }
        else
            fprintf(stderr, "Operation failed.\n");
        exit(127);
    }

    if (XM()->RunMode() == ModeNormal) {
        dspPkgIf()->RegisterIdleProc(&xic_main::start_proc, 0);
        dspPkgIf()->AppLoop();
        return (0);
    }

    xic_main::start_proc(0);
    return (0);
}


//-----------------------------------------------------------------------------
// Callbacks used in main function

// The application startup procedure, called after the GUI has been
// initialized in graphics mode, or called directly otherwise.
//
int
xic_main::start_proc(void*)
{
    XM()->AppInit();

    // Start Router interface, after reading tech file and init scripts.
    if (EditIf()->hasEdit())
        _mr_ = new cMRcmdIf;

    FILE *fp = pathlist::open_path_file(XM()->LogoFontFileName(),
        CDvdb()->getVariable(VA_LibPath), "r", 0, true);
    if (fp) {
        ED()->logoFont()->parseChars(fp);
        fclose(fp);
    }

    char *cwd = getcwd(0, 0);
    fprintf(stderr, "Current directory: %s\n", cwd);
    free(cwd);

    if (EditIf()->hasEdit() && ExtIf()->hasExtract()) {
        if (XM()->RunMode() == ModeBatch) {
            XM()->EditCell(0, true);
            xic_main::do_batch_commands();
            XM()->SetExitCleanupDone(true);
            SCD()->spif()->CloseSpice();
            XM()->Auth()->closeValidation();
            fflush(stdout);
            Log()->CloseLogDir();
            exit(0);
        }
        if (XM()->RunMode() == ModeServer) {
            int ret = XM()->Daemon(cmdLine.ServerPort);
            XM()->SetExitCleanupDone(true);
            SCD()->spif()->CloseSpice();
            XM()->Auth()->closeValidation();
            Log()->CloseLogDir();
            exit (ret);
        }
    }
    Timer()->start(getenv("XIC_NOTIMER") ? 0 : 200);
    dspPkgIf()->RegisterIdleProc(xic_main::read_cell_proc, 0);

    if (!CDvdb()->getVariable(VA_NoCheckUpdate)) {
        check_for_update();
        fprintf(stdout, "Checking for message...");
        fflush(stdout);
        char *message = UpdIf::message(APP_ROOT);
        fprintf(stdout, " Done\n");
        if (message) {
            DSPmainWbag(PopUpInfo(MODE_ON, message))
            delete [] message;
        }
    }
    if (UpdIf::new_release(APP_ROOT, VERSION_STR))
        Menu()->MenuButtonPress("help", MenuNOTES);

#ifdef HAVE_ATEXIT
    atexit(xic_main::onexit);
#endif
    return (0);
}


// Idle procedure to load initial cell into editor.  No execution
// until the first expose event arrives.
//
int
xic_main::read_cell_proc(void*)
{
    if (!XM()->IsAppReady())
        return (true);
    XM()->EditCell(0, true);
    if (!XM()->IsAppReady()) {
        // graphics halted by some signal
        XM()->Auth()->closeValidation();
        exit(1);
    }
    return (false);
}


void
xic_main::onexit()
{
    if (!XM()->IsExitCleanupDone())
        XM()->Exit(ExitPanic);
}


//-----------------------------------------------------------------------------
// Batch mode support

namespace {
// The batch commands can be followed by a parameter block that begins
// with an '@' character.  There can be no white space between the
// command name and the '@'.  The tokens that follow can contain white
// space, if the entire thing is appropriately quoted in the command
// line.
//
// Each parameter has a leading single-character identifier, followed
// in non-boolean cases by additional tokens.  The parameter clause is
// terminated with a '@' character, which is optional except after
// strings.  The equal signs are optional.  The identifiers are
//
//  w=l,b,r,t@
//    Define a window, args are floating-point numbers in microns.
//  n=name@
//    Define s cell name.
//  o=outname@
//    Define a name for the output file.
//  l=+l1,l2,...@
//  l=-l1,l2,...@
//    Define a layer list for only (+), or skip(-).
//  s=scale@
//    Define a scale factor, scale is a floating-point number .001-1000.
//  f@
//    Set the flatten flag.
//  e[N]@
//    N is an optional integer 0-3, this will set the empty cell
//    filtering level.  'n' is equiv, to 'n3', 'n0' is a no-op.
//  c@
//    Set the clip-to-window flag.
//  d@
//    Set a flag to delete the archive file of the current cell when
//    the batch operation completes.
//  m=maxerrs@
//    Set the DRC max batch errors parameter.
//  r=method@
//    Set the DRC recording method (0-2).
//
// The constructor will parse an argument block.
//
struct Bargs
{
    Bargs(const char*);
    ~Bargs()
        {
            delete [] ba_name;
            delete [] ba_outname;
            stringlist::destroy(ba_layers);
        }

    BBox ba_BB;
    double ba_scale;
    char *ba_name;
    char *ba_outname;
    stringlist *ba_layers;
    int ba_maxerrs;
    int ba_method;
    bool ba_skiplayers;
    bool ba_usewin;
    bool ba_flatten;
    bool ba_clip;
    unsigned char ba_ecf_level;
    bool ba_delete_file;
    bool ba_use_scale;
    bool ba_nogo;
};

Bargs::Bargs(const char *s)
{
    ba_scale = 1.0;
    ba_name = 0;
    ba_outname = 0;
    ba_layers = 0;
    ba_maxerrs = -1;
    ba_method = -1;
    ba_skiplayers = false;
    ba_usewin = false;
    ba_flatten = false;
    ba_clip = false;
    ba_ecf_level = ECFnone;
    ba_delete_file = false;
    ba_use_scale = false;
    ba_nogo = false;

    const char *errm = "Batch argument error: %s.\n";
    if (!s || !*s)
        return;
    for ( ; *s; s++) {
        if (isspace(*s) || *s == '@')
            continue;
        char c = isupper(*s) ? tolower(*s) : *s;
        if (c == 'w') {
            // window  w=l,b,r,t;
            if (ba_usewin) {
                fprintf(stderr, errm, "multiple window definitions");
                ba_nogo = true;
                return;
            }
            s++;
            while (isspace(*s) || *s == '=')
                s++;
            int n;
            double l, b, r, t;
            if (sscanf(s, "%lf,%lf,%lf,%lf%n", &l, &b, &r, &t, &n) >= 4) {
                s += n-1;
                ba_BB.left = INTERNAL_UNITS(l);
                ba_BB.bottom = INTERNAL_UNITS(b);
                ba_BB.right = INTERNAL_UNITS(r);
                ba_BB.top = INTERNAL_UNITS(t);
                ba_BB.fix();
                ba_usewin = true;
            }
            else {
                fprintf(stderr, errm, "bad window definition syntax");
                ba_nogo = true;
                return;
            }
        }
        else if (c == 's') {
            // scale  s=scale;
            if (ba_use_scale) {
                fprintf(stderr, errm, "multiple scale definitions");
                ba_nogo = true;
                return;
            }
            s++;
            while (isspace(*s) || *s == '=')
                s++;
            int n;
            double d;
            if (sscanf(s, "%lf%n", &d, &n) >= 1) {
                s += n-1;
                if (d < .001 || d > 1000.0) {
                    fprintf(stderr, errm, "scale factor out of range");
                    ba_nogo = true;
                    return;
                }
                ba_scale = d;
                ba_use_scale = true;
            }
            else {
                fprintf(stderr, errm, "bad scale definition syntax");
                ba_nogo = true;
                return;
            }
        }
        else if (c == 'n') {
            // cellname  n=cellname;
            if (ba_name) {
                fprintf(stderr, errm, "multiple cell name definitions");
                ba_nogo = true;
                return;
            }
            s++;
            while (isspace(*s) || *s == '=')
                s++;
            const char *t = s;
            while (*t && !isspace(*t) && *t != '@')
                t++;
            int n = t - s;
            if (n > 0) {
                ba_name = new char[n + 1];
                memcpy(ba_name, s, n);
                ba_name[n] = 0;
                s = t;
            }
            else {
                fprintf(stderr, errm, "bad cell name definition syntax");
                ba_nogo = true;
                return;
            }
        }
        else if (c == 'o') {
            // outfile  o=outname;
            if (ba_outname) {
                fprintf(stderr, errm, "multiple file name definitions");
                ba_nogo = true;
                return;
            }
            s++;
            while (isspace(*s) || *s == '=')
                s++;
            const char *t = s;
            while (*t && !isspace(*t) && *t != '@')
                t++;
            int n = t - s;
            if (n > 0) {
                ba_outname = new char[n + 1];
                memcpy(ba_outname, s, n);
                ba_outname[n] = 0;
                s = t;
            }
            else {
                fprintf(stderr, errm, "bad file name definition syntax");
                ba_nogo = true;
            }
        }
        else if (c == 'l') {
            // layer list  l=+/-name,name,...;
            // + use, - skip
            if (ba_layers) {
                fprintf(stderr, errm, "multiple layer list definitions");
                ba_nogo = true;
                return;
            }
            s++;
            while (isspace(*s) || *s == '=')
                s++;
            if (*s == '+')
                s++;
            else if (*s == '-') {
                ba_skiplayers = true;
                s++;
            }
            else {
                fprintf(stderr, errm, "missing +/- in layer list definition");
                ba_nogo = true;
                return;
            }
            stringlist *e = 0;
            for (;;) {
                while (isspace(*s) || *s == ',')
                    s++;
                const char *t = s;
                while (*t && !isspace(*t) && *t != ',' && *t != '@')
                    t++;
                int n = t - s;
                if (n > 0) {
                    char *ln = new char[n+1];
                    memcpy(ln, s, n);
                    ln[n] = 0;
                    if (!ba_layers)
                        ba_layers = e = new stringlist(ln, 0);
                    else {
                        e->next = new stringlist(ln, 0);
                        e = e->next;
                    }
                    s = t;
                }
                if (!*s || *s == '@')
                    break;
            }
            if (!ba_layers) {
                fprintf(stderr, errm, "no layers in layer list definition");
                ba_nogo = true;
                return;
            }
        }
        else if (c == 'f')  // flatten  f
            ba_flatten = true;
        else if (c == 'e') {
            // empty cell filtering level e[N]
            if (isdigit(s[1])) {
                if (s[1] == '1')
                    ba_ecf_level = ECFall;
                else if (s[1] == '2')
                    ba_ecf_level = ECFpre;
                else if (s[1] == '3')
                    ba_ecf_level = ECFpost;
            }
            else
                ba_ecf_level = ECFall;
        }
        else if (c == 'c')  // clip  c;
            ba_clip = true;
        else if (c == 'd')  // delete archive file  d;
            ba_delete_file = true;
        else if (c == 'm') {
            // DRC max errors, m=maxerrs
            if (ba_maxerrs != -1) {
                fprintf(stderr, errm, "multiple maxerrs definitions");
                ba_nogo = true;
                return;
            }
            s++;
            while (isspace(*s) || *s == '=')
                s++;
            int n;
            int d;
            if (sscanf(s, "%d%n", &d, &n) >= 1) {
                s += n-1;
                if (d < 0 || d > 100000) {
                    fprintf(stderr, errm, "maxerrs out of range");
                    ba_nogo = true;
                    return;
                }
                ba_maxerrs = d;
            }
            else {
                fprintf(stderr, errm, "bad maxerrs syntax");
                ba_nogo = true;
                return;
            }
        }
        else if (c == 'r') {
            // DRC record method, r=method
            if (ba_method != -1) {
                fprintf(stderr, errm, "multiple record method definitions");
                ba_nogo = true;
                return;
            }
            s++;
            while (isspace(*s) || *s == '=')
                s++;
            int n;
            int d;
            if (sscanf(s, "%d%n", &d, &n) >= 1) {
                s += n-1;
                if (d < 0 || d > 2) {
                    fprintf(stderr, errm, "record method out of range");
                    ba_nogo = true;
                    return;
                }
                ba_method = d;
            }
            else {
                fprintf(stderr, errm, "bad record method syntax");
                ba_nogo = true;
                return;
            }
        }
    }
}
} // End anon. namespace.


// If the -B option is given, do the processing after the first cell
// has been read in.  The -B option is given as
//  -Bscriptfile
//  -B-command
//
void
xic_main::do_batch_commands()
{
    if (!cmdLine.CmdFile)
        return;
    if (*cmdLine.CmdFile == '-') {
        if (!DSP()->CurCellName()) {
            fprintf(stderr, "Aborted, no current cell!\n");
            return;
        }

        char *s = lstring::copy(cmdLine.CmdFile + 1);
        GCarray<char*> gc_s(s);

        const char *c = s;
        while (isalpha(*s))
            s++;
        const char *argblk = 0;
        if (*s == '@') {
            *s++ = 0;
            argblk = s;
        }

        Bargs args(argblk);
        if (args.ba_nogo)
            return;

        if (!args.ba_name)
            args.ba_name = lstring::copy(DSP()->CurCellName()->string());

        if (!strcmp(c, "drc")) {
            if (strcmp(DSP()->CurCellName()->string(), args.ba_name))
                DSP()->SetCurCellName(CD()->CellNameTableAdd(args.ba_name));
            CDcbin cbin(DSP()->CurCellName());
            if (!cbin.phys()) {
                fprintf(stderr, "DRC aborted, no current cell!\n");
                return;
            }
            if (!args.ba_usewin)
                args.ba_BB = *cbin.phys()->BB();

            if (args.ba_maxerrs >= 0)
                DRC()->setMaxErrors(args.ba_maxerrs);
            if (args.ba_method >= 0)
                DRC()->setErrorLevel((DRClevelType)args.ba_method);

            char buf[256];
            sprintf(buf, "drcerror.log.%s.%d", args.ba_name, (int)getpid());
            FILE *fp = fopen(buf, "w");
            if (!fp) {
                fprintf(stderr, "Could not open file %s.\n", buf);
                return;
            }
            if (!DRC()->printFileHeader(fp, DSP()->CurCellName()->string(),
                    &args.ba_BB)) {
                fprintf(stderr, "Error: %s\n", Errs()->get_error());
                return;
            }

            DRC()->batchTest(&args.ba_BB, fp, 0, 0);

            DRC()->printFileEnd(fp, DRC()->getErrCount(), true);
            fclose(fp);
            printf("DRC done, error count %d.\n", DRC()->getErrCount());

            if (args.ba_delete_file) {
                if (FIO()->IsSupportedArchiveFormat(cbin.fileType())) {
                    const char *fn = cbin.phys()->fileName();
                    if (fn)
                        unlink(fn);
                }
            }
            return;
        }

        FIOcvtPrms prms;
        if (args.ba_use_scale)
            prms.set_scale(args.ba_scale);

        prms.set_allow_layer_mapping(true);
        if (args.ba_layers) {
            CDvdb()->setVariable(VA_SkipInvisible, "p");
            if (args.ba_skiplayers) {
                for (stringlist *sl = args.ba_layers; sl; sl = sl->next) {
                    CDl *ld = CDldb()->findLayer(sl->string, Physical);
                    if (ld)
                        ld->setInvisible(true);
                }
            }
            else {
                CDlgen gen(Physical);
                CDl *ld;
                while ((ld = gen.next()) != 0)
                    ld->setInvisible(true);
                for (stringlist *sl = args.ba_layers; sl; sl = sl->next) {
                    ld = CDldb()->findLayer(sl->string, Physical);
                    if (ld)
                        ld->setInvisible(false);
                }
            }
        }

        // These don't completely work.  Windowing only works when
        // flattening.
        if (args.ba_usewin) {
            prms.set_window(&args.ba_BB);
            prms.set_use_window(true);
        }
        prms.set_flatten(args.ba_flatten);
        prms.set_clip(args.ba_clip);
        prms.set_ecf_level((ECFlevel)args.ba_ecf_level);

        if (!strcmp(c, "tocgx")) {
            if (!args.ba_outname) {
                args.ba_outname = new char[strlen(args.ba_name) + 5];
                char *t = lstring::stpcpy(args.ba_outname, args.ba_name);
                strcpy(t, ".cgx");
            }
            prms.set_destination(args.ba_outname, Fcgx);
            stringlist *namelist =
                new stringlist(lstring::copy(args.ba_name), 0);
            FIO()->ConvertToCgx(namelist, &prms);
            stringlist::destroy(namelist);
        }
        else if (!strcmp(c, "togds")) {
            if (!args.ba_outname) {
                args.ba_outname = new char[strlen(args.ba_name) + 5];
                char *t = lstring::stpcpy(args.ba_outname, args.ba_name);
                strcpy(t, ".gds");
            }
            prms.set_destination(args.ba_outname, Fgds);
            stringlist *namelist =
                new stringlist(lstring::copy(args.ba_name), 0);
            FIO()->ConvertToGds(namelist, &prms);
            stringlist::destroy(namelist);
        }
        else if (!strcmp(c, "tooas")) {
            if (!args.ba_outname) {
                args.ba_outname = new char[strlen(args.ba_name) + 5];
                char *t = lstring::stpcpy(args.ba_outname, args.ba_name);
                strcpy(t, ".oas");
            }
            prms.set_destination(args.ba_outname, Foas);
            stringlist *namelist =
                new stringlist(lstring::copy(args.ba_name), 0);
            FIO()->ConvertToOas(namelist, &prms);
            stringlist::destroy(namelist);
        }
        else if (!strcmp(c, "tocif")) {
            if (!args.ba_outname) {
                args.ba_outname = new char[strlen(args.ba_name) + 5];
                char *t = lstring::stpcpy(args.ba_outname, args.ba_name);
                strcpy(t, ".cif");
            }
            prms.set_destination(args.ba_outname, Fcif);
            stringlist *namelist =
                new stringlist(lstring::copy(args.ba_name), 0);
            FIO()->ConvertToCif(namelist, &prms);
            stringlist::destroy(namelist);
        }
        else if (!strcmp(c, "toxic")) {
            prms.set_destination(args.ba_outname, Fnative);
            stringlist *namelist =
                new stringlist(lstring::copy(args.ba_name), 0);
            FIO()->ConvertToNative(namelist, &prms);
            stringlist::destroy(namelist);
        }
        else
            fprintf(stderr,
                "Unknown batch command %s.\n", cmdLine.CmdFile + 1);
        return;
    }
    xic_main::exec_batch_script(cmdLine.CmdFile);
}


// Handle execution of the batch-mode script, including extracting the
// parameters from the given string.  The parameters appear in the
// form:
//    scriptfile,parm1=val1,...
// i.e., the character ',' separates the fields, and the parameters.
//
void
xic_main::exec_batch_script(const char *scr)
{
    char *script = lstring::copy(scr);
    char *args = strchr(script, ',');
    while (args) {
        // let '\?' hide the terminator
        if (args == script || *(args-1) == '\\')
            args = strchr(args+1, ',');
        else
            break;
    }
    if (args)
        *args++ = 0;

    sif_err err;
    SIfile *sfp = SIfile::create(script, 0, &err);
    if (!sfp) {
        if (err == sif_open)
            fprintf(stderr, "Could not open file %s.\n", script);
        else if (err == sif_crypt)
            fprintf(stderr, "Could not decrypt file %s.\n", script);
        return;
    }
    PCellParam *pars;
    if (PCellParam::parseParams(args, &pars)) {
        SI()->Interpret(sfp, 0, 0, 0, false, pars);
        pars->free();
        EditIf()->ulCommitChanges();
        XM()->CommitCell();
    }
    else {
        Errs()->add_error("Parameter parse failed.");
        fprintf(stderr, "%s\n", Errs()->get_error());
    }
    delete sfp;
}


//-----------------------------------------------------------------------------
// cMain constructor

// Called from constructor.
void
cMain::InitializeStrings()
{
    //  s/S[erver] or l/L[ocal]
    const char *s = getenv("XT_AUTH_MODE");
#ifdef WIN32
    xm_auth = new sAuthChk(!s || (*s != 's' && *s != 'S'));
#else
    xm_auth = new sAuthChk(s && (*s == 'l' || *s == 'L'));
#endif
    xm_product_code = XIC_PRODUCT_CODE;

    xm_product = "Xic";
    xm_description = "design system";
    xm_program = 0;
    xm_tools_root = TOOLS_ROOT;
    xm_app_root = APP_ROOT;
    xm_homedir = pathlist::get_home("XIC_START_DIR");

    const char *string = getenv("XIC_HOME");
    if (string && *string) {
        xm_program_root = lstring::copy(string);

        // The xm_prefix is used by the software installer, which puts
        // all tools under an "xictools" directory.  Update the
        // xm_prefix if XIC_HOME follows this convention.

        char *pfx = lstring::copy(string);
        char *tp = pathlist::mk_path(TOOLS_ROOT, APP_ROOT);
        char *sp = strstr(pfx, tp);
        delete [] tp;
        if (sp) {
            sp[-1] = 0;
            xm_prefix = pfx;
        }
        else
            delete [] pfx;
    }
    if (!xm_program_root) {
        const char *pfx = getenv("XT_PREFIX");
        if (pfx && *pfx) {
            char *p1 = pathlist::mk_path("/", pfx);
            char *p2 = pathlist::mk_path(p1, TOOLS_ROOT);
            delete [] p1;
            xm_program_root = pathlist::mk_path(p2, APP_ROOT);
            delete [] p2;
            xm_prefix = lstring::copy(pfx);
        }
    }
#ifdef WIN32
    if (!xm_program_root)
        xm_program_root = msw::GetProgramRoot(Product());
#endif
    if (!xm_program_root) {
        char *p1 = pathlist::mk_path("/", PREFIX);
        char *p2 = pathlist::mk_path(p1, TOOLS_ROOT);
        delete [] p1;
        xm_program_root = pathlist::mk_path(p2, APP_ROOT);
        delete [] p2;
    }
    if (!xm_prefix)
        xm_prefix = lstring::copy(PREFIX);

    xm_version_string = VERSION_STR;
    xm_tag_string = CVS_RELEASE_TAG;
    xm_about_file = "xic_mesg";

    xm_os_name = OSNAME;
    xm_arch = ARCH;
    xm_dist_suffix = DIST_SUFFIX;
    xm_geometry = lstring::copy(getenv("XIC_GEOMETRY"));

    xm_build_date = BUILD_DATE;
    s = xm_build_date;
    char *tok;
    while ((tok = lstring::gettok(&s)) != 0) {
        if (strlen(tok) == 4 && tok[0] == '2' && isdigit(tok[1]) &&
                isdigit(tok[2]) && isdigit(tok[3])) {
            xm_build_year = atoi(tok);
            delete [] tok;
            break;
        }
        delete [] tok;
    }
    if (!xm_build_year)
        xm_build_year = 2015;

    xm_tech_file_base = "xic_tech";
    xm_default_edit_name = "noname";
    xm_device_lib_name = "device.lib";
    xm_model_lib_name = "model.lib";
    xm_model_subdir_name = "models";

    xm_init_script = ".xicinit";
    xm_startup_script = ".xicstart";
    xm_macro_file_name = ".xicmacros";
    xm_exec_directory = 0;

    const char *spice_exec_name = getenv("SPICE_EXEC_NAME");
    if (!spice_exec_name || !*spice_exec_name)
        spice_exec_name = SPICE_PROG;
    xm_spice_exec_name = lstring::copy(spice_exec_name);
    xm_font_file_name = "xic_font";
    xm_logo_font_file_name = "xic_logofont";

    xm_log_name = "xic_run.log";

    string = getenv("SPICE_EXEC_DIR");
    if (string && *string)
        xm_exec_directory = lstring::copy(string);
    else {
#ifdef WIN32
        char *dp = msw::GetProgramRoot("WRspice");
        if (dp) {
            char *e = lstring::strrdirsep(dp);
            if (e)
                *e = 0;
            xm_exec_directory = pathlist::mk_path(dp, "bin");
            delete [] dp;
        }
#endif
        if (!xm_exec_directory) {
            const char *prefix = getenv("XT_PREFIX");
            if (!prefix || !lstring::is_rooted(prefix))
                prefix = PREFIX;
            char *p1 = pathlist::mk_path(prefix, TOOLS_ROOT);
#ifdef WIN32
            // Path is xictools/bin in Windows, xictools/wrspice/bin
            // otherwise.
#else
            const char *sppg = SPICE_PROG;
            if (!sppg || !*sppg)
                sppg = "wrspice";
            char *p2 = pathlist::mk_path(p1, sppg);
            delete [] p1;
            p1 = p2;
#endif
            xm_exec_directory = pathlist::mk_path(p1, "bin");
            delete [] p1;
        }
    }
}


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


// Initialize the path variables.
//
void
cMain::InitializeVariables()
{
    CDvdb()->setVariable(VA_ProgramRoot, ProgramRoot());

    const char *string = getenv("XIC_SYM_PATH");
    if (string && *string)
        CDvdb()->setVariable(VA_Path, string);
    else
        CDvdb()->setVariable(VA_Path, "( . )");

    string = getenv("XIC_LIB_PATH");
    if (string && *string)
        CDvdb()->setVariable(VA_LibPath, string);
    else {
        const char *root = ProgramRoot();
        bool nq = needs_quote(root);
        sLstr lstr;
        lstr.add("( . ");
        if (nq)
            lstr.add_c('"');
        char *pth = pathlist::mk_path(root, "startup");
        lstr.add(pth);
        delete [] pth;
        if (nq)
            lstr.add_c('"');
        lstr.add(" )");
        CDvdb()->setVariable(VA_LibPath, lstr.string());
    }

    string = getenv("XIC_HLP_PATH");
    if (string && *string)
        CDvdb()->setVariable(VA_HelpPath, string);
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
        CDvdb()->setVariable(VA_HelpPath, lstr.string());
    }

    string = getenv("XIC_SCR_PATH");
    if (string && *string)
        CDvdb()->setVariable(VA_ScriptPath, string);
    else {
        const char *root = ProgramRoot();
        bool nq = needs_quote(root);
        sLstr lstr;
        lstr.add("( ");
        if (nq)
            lstr.add_c('"');
        char *pth = pathlist::mk_path(root, "scripts");
        lstr.add(pth);
        delete [] pth;
        if (nq)
            lstr.add_c('"');
        lstr.add(" )");
        CDvdb()->setVariable(VA_ScriptPath, lstr.string());
    }

    string = getenv("XIC_DOCS_DIR");
    if (string && *string)
        CDvdb()->setVariable(VA_DocsDir, string);
    else {
        char *pth = pathlist::mk_path(ProgramRoot(), "docs");
        char *tbf = pathlist::expand_path(pth, true, true);
        delete [] pth;
        CDvdb()->setVariable(VA_DocsDir, tbf);
        delete [] tbf;
    }
}


// Register the application's menus, and other misc. setup.
//
void
cMain::InitializeMenus()
{
    // Initialize error logging
    //
    Log()->SetMsgLogName("xic_error.log");
    Log()->SetMemErrLogName("xic_mem_errors.log");
    Log()->SetMailAddress(BUG_ADDR);

    // Setup Menus
    Menu()->RegisterMenu(createFileMenu());
    Menu()->RegisterMenu(createCellMenu());
    if (EditIf()->hasEdit()) {
        Menu()->RegisterMenu(ED()->createEditMenu());
        Menu()->RegisterMenu(ED()->createModfMenu());
    }
    Menu()->RegisterMenu(createViewMenu());
    Menu()->RegisterMenu(createAttrMenu());
    Menu()->RegisterMenu(createCvrtMenu());
    if (ExtIf()->hasExtract()) {
        Menu()->RegisterMenu(DRC()->createMenu());
        Menu()->RegisterMenu(EX()->createMenu());
    }
    if (EditIf()->hasEdit()) {
        Menu()->RegisterMenu(createUserMenu());
    }
    Menu()->RegisterMenu(createHelpMenu());
    Menu()->RegisterMiscMenu(createMiscMenu());
    if (EditIf()->hasEdit()) {
        Menu()->RegisterButtonMenu(ED()->createPbtnMenu(), Physical);
    }
    if (ExtIf()->hasExtract()) {
        Menu()->RegisterButtonMenu(SCD()->createEbtnMenu(), Electrical);
    }
    Menu()->RegisterSubwMenu(createSubwViewMenu());
    Menu()->RegisterSubwMenu(createSubwAttrMenu());
    Menu()->RegisterSubwMenu(createSubwHelpMenu());
}


const char *
cMain::NextArg()
{
    return (cmdLine.next_arg());
}


// For the -Rprefix option, must be called before InitializeVariables. 
// The given prefix will override the environment.
//
void
cMain::SetPrefix(const char *prefix)
{
    if (prefix && *prefix) {
        delete [] xm_prefix;
        xm_prefix = lstring::copy(prefix);

        delete [] xm_program_root;
        char *p1 = pathlist::mk_path(prefix, TOOLS_ROOT);
        xm_program_root = pathlist::mk_path(p1, APP_ROOT);
        delete [] p1;
    }
}


#define RELNOTE_BASE "xic"

char *
cMain::ReleaseNotePath()
{
    char buf[256];
    const char *docspath = CDvdb()->getVariable(VA_DocsDir);
    if (!docspath) {
        sprintf(buf, "No path to docs (DocsDir not set).");
        DSPmainWbag(PopUpMessage(buf, true))
        return (0);
    }

    // Remove quotes.
    char *path = pathlist::expand_path(docspath, false, true);
    sprintf(buf, "%s%s", RELNOTE_BASE, XM()->VersionString());

    // The version string is in the form generation.major.minor, strip
    // off the minor part.
    char *t = strrchr(buf, '.');
    if (t)
        *t = 0;

    t = new char[strlen(path) + strlen(buf) + 2];
    sprintf(t, "%s/%s", path, buf);
    delete [] path;
    return (t);
}
// End of cMain functions.


//-----------------------------------------------------------------------------
// Command line argument processing

// Process command line args that aren't swallowed by X.  Args are:
// -B*        batch mode (no graphics)
// -C         no private colors
// -C1        no dual plane colors
// -E         start in electrical mode rather than physical
// -Kpasswd   password for encoded scripts
// -Pport:pid establish spice connection through port (not documented)
// -Rprefix   use prefix (equiv to /usr/local) to find startup files
// -S         server mode (no graphics)
// -Text      specify the extension of technology file to use
// Anything left over above the zeroth arg is taken as a file to
// edit.
//
void
sCmdLine::process_args(int argc, char **argv, bool prep)
{
    if (prep) {
        // Initial pass, need to detect certain options here.  The "real"
        // option processing is done on a second call.

        if (!strcmp(lstring::strip_path(argv[0]), FILETOOL)) {
            ImaFileTool = true;
            XM()->SetRunMode(ModeBatch);
            return;
        }
        if (argc > 1 && argv[1][0] == '-' && argv[1][1] == 'F') {
            ImaFileTool = true;
            XM()->SetRunMode(ModeBatch);
            return;
        }
        for (int i = 1; i < argc; i++) {
            if (!strcmp(argv[1], "--v")) {
                fprintf(stdout, "%s %s\n", XM()->VersionString(),
                    XM()->OSname());
                exit (0);
            }
            if (!strcmp(argv[1], "--vv")) {
                fprintf(stdout, "%s\n", XM()->TagString());
                exit (0);
            }
            if (!strcmp(argv[1], "--vb")) {
                fprintf(stdout, "%s\n", XM()->BuildDate());
                exit (0);
            }
            if (argv[i][0] == '-' && argv[i][1] == 'B') {
                XM()->SetRunMode(ModeBatch);
                continue;
            }
            if (argv[i][0] == '-' && argv[i][1] == 'S') {
                XM()->SetRunMode(ModeServer);
                continue;
            }
            if (argv[i][0] == '-' && argv[i][1] == 'R') {
                // -Rprefix
                // This must be done before InitializeVariables.
                XM()->SetPrefix(argv[i + 2]);
                continue;
            }
            if (argv[i][0] == '-' && argv[i][1] == 'L') {
                // -Llichost[:port]
                // This must be done before licence validation.
                const char *host = argv[i] + 2;
                if (*host)
                    XM()->Auth()->set_validation_host(host);
                continue;
            }
            if (!strcmp(argv[i], "--WinBg")) {
                WinBg = true;
                continue;
            }
        }
        return;
    }
    if (ImaFileTool) {
        if (argc > 1 && argv[1][0] == '-' && argv[1][1] == 'F') {
            argc--;
            shift(1, argc, argv);
        }
        NumArgs = argc;
        Args = argv;
        return;
    }

    XM()->SetInitialMode(Physical);  // default initial mode

    if (ExtIf()->hasExtract()) {
        // Set default script decoding password
        SI()->SetKey(DefaultPassword+7);
        SCD()->spif()->SetParentSpPort(0);  // no default port
        SCD()->spif()->SetParentSpPID(0);
    }

    int ac = 1;
    while (ac < argc) {
        if (ExtIf()->hasExtract()) {
            if (argv[ac][0] == '-' && argv[ac][1] == 'B') {
                // batch mode
                XM()->SetRunMode(ModeBatch);
                if (argv[ac][2])
                    CmdFile = lstring::copy(argv[ac] + 2);
                argc--;
                shift(ac, argc, argv);
                continue;
            }
        }
        if (argv[ac][0] == '-' && argv[ac][1] == 'C') {
            // color amp usage option
            if (argv[ac][2] == '1')
                CmapSaver = 1;
            else
                CmapSaver = 2;
            argc--;
            shift(ac, argc, argv);
            continue;
        }
        if (ExtIf()->hasExtract()) {
            if (argv[ac][0] == '-' && argv[ac][1] == 'E') {
                // start in electrical mode
                XM()->SetInitialMode(Electrical);
                argc--;
                shift(ac, argc, argv);
                continue;
            }
        }
        if (argv[ac][0] == '-' && argv[ac][1] == 'G') {
            XM()->SetGeometry(lstring::copy(argv[ac] + 2));
            argc--;
            shift(ac, argc, argv);
            continue;
        }
        if (argv[ac][0] == '-' && argv[ac][1] == 'H') {
            // switch to directory
            if (chdir(argv[ac] + 2) < 0) {
                fprintf(stderr, "Warning: directory change failed.\n");
                if (!argv[ac][2])
                    fprintf(stderr,
                        "-H option usage: -Hdirectory_path (no space).\n");
                else
                    fprintf(stderr, "%s\n  %s\n", argv[ac]+2, strerror(errno));
            }
            argc--;
            shift(ac, argc, argv);
            continue;
        }
        if (ExtIf()->hasExtract()) {
            if (argv[ac][0] == '-' && argv[ac][1] == 'K') {
                // -Kpassword
                const char *pw = argv[ac] + 2;
                sCrypt cr;
                if (!cr.getkey(pw)) {
                    char k[13];
                    cr.readkey(k);
                    SI()->SetKey(k);
                }
                else
                    fprintf(stderr, "-K option: failed to set key.\n");
                argc--;
                shift(ac, argc, argv);
                continue;
            }
        }
        if (argv[ac][0] == '-' && argv[ac][1] == 'L') {
            // -Llichost[:port]
            // We read this on the first pass.
            argc--;
            shift(ac, argc, argv);
            continue;
        }
        if (ExtIf()->hasExtract()) {
            if (argv[ac][0] == '-' && argv[ac][1] == 'P') {
                // -Pport:pid
                if (isdigit(argv[ac][2])) {
                    int sp_port, sp_pid;
                    int j = sscanf(argv[ac]+2, "%d:%d", &sp_port, &sp_pid);
                    if (j != 2 || sp_port <= 0 || sp_pid <= 0) {
                        fprintf(stderr, "Bad arg to -P option, ignored.\n");
                        SCD()->spif()->SetParentSpPort(0);
                        SCD()->spif()->SetParentSpPID(0);
                    }
                    else {
                        SCD()->spif()->SetParentSpPort(sp_port);
                        SCD()->spif()->SetParentSpPID(sp_pid);
                    }
                    argc--;
                    shift(ac, argc, argv);
                    continue;
                }
                continue;
            }
        }
        if (argv[ac][0] == '-' && argv[ac][1] == 'R') {
            // -Rprefix
            // We read this on the first pass.
            argc--;
            shift(ac, argc, argv);
            continue;
        }
        if (ExtIf()->hasExtract()) {
            if (argv[ac][0] == '-' && argv[ac][1] == 'S') {
                XM()->SetRunMode(ModeServer);
                if (isdigit(argv[ac][2]))
                    ServerPort = atoi(argv[ac] + 2);
                argc--;
                shift(ac, argc, argv);
                continue;
            }
        }
        if (argv[ac][0] == '-' && argv[ac][1] == 'T') {
            // set technology extension
            // this will be null if no -T[xxx] given, blank if bare -T given
            Tech()->SetTechExtension(argv[ac] + 2);
            argc--;
            shift(ac, argc, argv);
            continue;
        }
        if (!strcmp(argv[ac], "--WinBg")) {
            // Windows "to background" flag, ignore
            argc--;
            shift(ac, argc, argv);
            continue;
        }
        ac++;
    }
    NumArgs = argc;
    Args = argv;
}


#if defined(HAVE_GTK1) && !defined(DYNAMIC_LIBS)
namespace {
    // Some setup code for the GTK library (GTK-1 only!).
    //
    void setup_gtk()
    {
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
        const char *libpath = CDvdb()->getVariable(VA_LibPath);
        if (!libpath)
            return;
        char *path;
        FILE *fp = pathlist::open_path_file("default_theme/gtkrc", libpath,
            "r", &path, true);
        if (fp) {
            fclose(fp);

            // We're going to check for gtkrc in default_theme and its
            // parent dir, parent dir last so it has precedence.  If the
            // user modifies gtkrc, it should be copied to the parent dir
            // so it won't get clobbered by a software update.

            char *rcp = new char[2*strlen(path) + 2];
            char *e = lstring::stpcpy(rcp, path);
            char *ee = e;
            *e++ = ':';
            strcpy(e, path);
            e = lstring::strrdirsep(ee);
            if (e) {
                char *fn = e + 1;
                *e = 0;
                e = lstring::strrdirsep(ee);
                if (e) {
                    e++;
                    while ((*e = *fn++) != 0)
                        e++;
                }
                else
                    *ee = 0;
            }
            else
                *ee = 0;

            char *v1 = new char[strlen(rcp) + 16];
            sprintf(v1, "%s=%s", "GTK_RC_FILES", rcp);
            putenv(v1);
            delete [] rcp;

            e = lstring::strrdirsep(path);
            if (e)
                *e = 0;
            else
                *path = 0;
            char *v2 = new char[strlen(path) + 18];
            sprintf(v2, "%s=%s", "GTK_EXE_PREFIX", path);
            putenv(v2);
            delete [] path;
        }
        else
            // Don't read any gtkrc files, so no themes
            putenv(lstring::copy("GTK_RC_FILES="));
        delete [] libpath;
    }
}
#endif

