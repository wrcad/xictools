
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2005 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL STEPHEN R. WHITELEY BE LIABLE       *
 *   FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION      *
 *   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN           *
 *   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN         *
 *   THE SOFTWARE.                                                        *
 *                                                                        *
 * This library is available for non-commercial use under the terms of    *
 * the GNU Library General Public License as published by the Free        *
 * Software Foundation; either version 2 of the License, or (at your      *
 * option) any later version.                                             *
 *                                                                        *
 * A commercial license is available to those wishing to use this         *
 * library or a derivative work for commercial purposes.  Contact         *
 * Whiteley Research Inc., 456 Flora Vista Ave., Sunnyvale, CA 94086.     *
 * This provision will be enforced to the fullest extent possible under   *
 * law.                                                                   *
 *                                                                        *
 * This library is distributed in the hope that it will be useful,        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 * Library General Public License for more details.                       *
 *                                                                        *
 * You should have received a copy of the GNU Library General Public      *
 * License along with this library; if not, write to the Free             *
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.     *
 *========================================================================*
 *                                                                        *
 * WRspice Accessories: wrspiced WRspice server daemon                    *
 *                                                                        *
 *========================================================================*
 $Id: wrspiced.cc,v 2.79 2015/09/16 00:13:23 stevew Exp $
 *========================================================================*/

/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1985 Wayne A. Christopher, U. C. Berkeley CAD Group
**********/

//
// Daemon for servicing requests for remote WRspice runs.
//
// Do remote spice jobs.  The first line transmitted will be of the
// format:  "user host program".  The program field may be left out,
// in which case it defaults to SPICE_PATH.  The program is then
// executed, with server-specific arguments.  The remote host should
// wait for a response from wrspiced before sending any data -- so far
// the only responses sent are "ok" and "toomany".
//
// There is an extended protocol for use with Xic IPC, see note below.
//

#include "config.h"
#include "lstring.h"
#include "pathlist.h"
#include "miscutil.h"
#include "services.h"
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>

#ifdef HAVE_SOCKET
#ifdef WIN32
#include "msw.h"
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#endif

#ifdef HAVE_GETRUSAGE
#include <sys/time.h>
#include <time.h>
#include <sys/resource.h>
#else
#include <time.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#include <sys/file.h>
#include <fcntl.h>
#endif

#include <signal.h>
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif


// Maximum number of running jobs, additional jobs are refused.
#define MAXJOBS 5

// The protocol version, returned to the client for compatibility. 
// This is new for wrspice-3.2.9, earlier servers do not have this
// feature.
//
#define PROTOCOL_VERSION "2.0"

// Initialize Spice-specific global variables.
// 
// directory for executables
// environment: SPICE_EXEC_DIR
// char *Spice_Exec_Dir               directory for executables
//
// path for aspice
// environment: SPICE_PATH
// char *Spice_Exec_Prog              full path to program
// 
// daemon log file
// environment: SPICE_DAEMONLOG
// char *Spiced_Log                   path to daemon log file
//

// These are set as compiler -D defines
#ifndef PREFIX
#define PREFIX "/usr/local"
#endif
#ifndef TOOLS_ROOT
#define TOOLS_ROOT "xictools"
#endif
#ifndef SPICE_PROG
#define SPICE_PROG "wrspice"
#endif
#ifndef SPICE_DAEMONLOG
#define SPICE_DAEMONLOG "/tmp/wrspiced.log"
#endif

extern int errno;
extern char **environ;
#ifndef HAVE_STRERROR
#ifndef SYS_ERRLIST_DEF
extern char *sys_errlist[];
#endif
#endif

#ifdef WIN32
// Tell the msw interface that we're Generation 4.
const char *msw::MSWpkgSuffix = "-4";

#define CLOSESOCKET(x) shutdown(x, SD_SEND), closesocket(x)
#else
#define CLOSESOCKET(x) close(x)
#endif

namespace {
    int Nrunning = 0;
    int MaxJobs = MAXJOBS;
    int pids[MAXJOBS];
    const char *Spice_Exec_Dir;
    const char *Spice_Exec_Prog;
    const char *Spiced_Log;

    // In Win32, the wrspiced program is called again with -winbg
    // given on the command line.  The new process has no console. 
    // This variable suppresses printing to the console, and more in
    // Win32.  It is never set if not in Win32
    //
    static bool WinBg;

    // Exit signal handler, update log and kill running processes.
    //
    void kill_spawn(int sig)
    {
        FILE *fp = fopen(Spiced_Log, "a");
        if (fp)
            fprintf(fp, "\n-- received signal %d\n", sig);
#ifndef WIN32
// In Win32 there is no clean way to kill a process with no message
// queue.
        for (int i = 0; i < MAXJOBS; i++) {
            if (pids[i] > 0) {
                kill(pids[i], SIGTERM);
                if (fp)
                    fprintf(fp, "-- killed active process, pid = %d\n",
                        pids[i]);
            }
        }
#endif
        if (fp) {
            fprintf(fp, "-- daemon exited, pid = %d, date = %s\n\n",
                (int)getpid(), miscutil::dateString());
            fclose(fp);
        }
        exit(0);
    }

    void init_vars();
#ifndef WIN32
    void sigchild(int);
#endif
    void log_perror(const char*);
    void log_printf(const char*, ...);
}


#ifdef WIN32

struct thdata
{
    thdata(HANDLE p, HANDLE i, HANDLE o, char *ifn, char *ofn, int s)
        {
            process = p;
            infile = i;
            outfile = o;
            infile_name = ifn;
            outfile_name = ofn;
            skt = s;
        }

    HANDLE process;
    HANDLE infile;
    HANDLE outfile;
    char *infile_name;
    char *outfile_name;
    int skt;
};


// Wait for the process to complete, and send the data in the output file
// back.  This executes in its own thread.
//
void
th_local_hdlr(void *arg)
{
    thdata *t = (thdata*)arg;
    WaitForSingleObject(t->process, INFINITE);
    CloseHandle(t->infile);
    CloseHandle(t->outfile);
    FILE *fp = fopen(t->outfile_name, "r");
    if (fp) {
        int c;
        while ((c = getc(fp)) != EOF) {
            char cc = c;
            send(t->skt, &cc, 1, 0);
        }
        fclose(fp);
    }
    unlink(t->outfile_name);
    unlink(t->infile_name);
    delete [] t->infile_name;
    delete [] t->outfile_name;
    CLOSESOCKET(t->skt);
    delete t;
}


// Create temp file, Win32 only.
//
char *
smktemp(const char *id)
{
    static int num;
    if (num == 0)
        num = getpid();
    else
        num++;

    if (!id)
        id = "wspd";
    
    const char *path = getenv("SPICE_TMP_DIR");
    if (!path)
        path = getenv("TMPDIR");
    if (!path) {
        path = "/tmp";
        mkdir(path);
    }

    char buf[512];
    if (!access(path, W_OK))
        sprintf(buf, "%s/%s%d", path, id, num);
    else
        sprintf(buf, "%s%d", id, num);
    return (lstring::copy(buf));
}

#endif


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


int
main(int, char **av)
{
    init_vars();
    int wrspiced_port = 0;

    const char *usage =
        "wrspiced [-l logfile][-p program][-m maxjobs][-t port]";
    bool in_fg = false;
    av++;
    if (*av) {
        char *lf = 0, *sp = 0;
        int mj = 0;
        while (*av) {
            if (!strcmp(*av, "-l")) {
                // Path to log file.
                av++;
                if (*av) {
                    lf = lstring::copy(*av);
                    av++;
                    continue;
                }
            }
            else if (!strcmp(*av, "-p")) {
                // Program path.
                av++;
                if (*av) {
                    sp = lstring::copy(*av);
                    av++;
                    continue;
                }
            }
            else if (!strcmp(*av, "-m")) {
                // Max concurrent jobs.
                av++;
                if (*av) {
                    mj = atoi(*av);
                    if (mj <= 0) {
                        fprintf(stderr, "bad \"maxjobs\"\n");
                        return (1);
                    }
                    av++;
                    continue;
                }
            }
            else if (!strcmp(*av, "-t")) {
                // Port number.
                av++;
                if (*av) {
                    if (!check_digits(*av)) {
                        fprintf(stderr, "bad port number\n");
                        return (1);
                    }
                    wrspiced_port = atoi(*av);
                    av++;
                    continue;
                }
            }
            else if (!strcmp(*av, "-fg")) {
                // Stay in foreground.
                av++;
                in_fg = true;
            }
#ifdef WIN32
            else if (!strcmp(*av, "-winbg")) {
                av++;
                WinBg = true;
            }
#endif
            else {
                fprintf(stderr,
                    "usage: %s\ndefaults:\n  logfile\t%s\n  program\t%s\n"
                    "  maxjobs\t%d\n  port   \tspice/tcp or %d\n",
                    usage, Spiced_Log, Spice_Exec_Prog, MaxJobs,
                    WRSPICE_PORT);
                return (1);
            }
        }
        if (lf)
            Spiced_Log = lf;
        if (sp)
            Spice_Exec_Prog = sp;
        if (mj)
            MaxJobs = mj;
    }
    Spice_Exec_Prog = pathlist::expand_path(Spice_Exec_Prog, false, true);
    Spiced_Log = pathlist::expand_path(Spiced_Log, false, true);
    fprintf(stderr, "wrspiced:\n  target\t%s\n  jobs\t\t%d\n  logfile\t%s\n",
        Spice_Exec_Prog, MaxJobs, Spiced_Log);
    FILE *fp = fopen(Spiced_Log, "a");
    if (!fp) {
        if (!WinBg)
            fprintf(stderr,
            "Warning: *** can't write to log file, logging disabled. ***\n");
    }
    else
        fclose(fp);

#ifdef WIN32
    // initialize winsock
    WSADATA wsadata;
    if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0) {
        fprintf(stderr,
        "Error: WSA initialization failed, no interprocess communication.\n");
        return (1);
    }
#endif

    if (wrspiced_port <= 0) {
        servent *sp = getservbyname(WRSPICE_SERVICE, "tcp");
        if (sp)
            wrspiced_port = ntohs(sp->s_port);
        else
            wrspiced_port = WRSPICE_PORT;
    }
    protoent *pp = getprotobyname("tcp");
    if (pp == 0) {
        fprintf(stderr, "Error: tcp: unknown protocol\n");
        return (1);
    }
    if (!WinBg)
        fprintf(stderr, "  port   \t%d\n", wrspiced_port);

    // Create the socket
    int skt = socket(AF_INET, SOCK_STREAM, pp->p_proto);
    if (skt < 0) {
        perror("socket");
        return (1);
    }
    sockaddr_in sin;
    sin.sin_port = htons(wrspiced_port);
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    if (bind(skt, (sockaddr*)&sin, sizeof(sockaddr_in)) < 0) {
        perror("bind");
        return (1);
    }

    if (!in_fg) {
        // Disconnect from the controlling terminal.
#ifdef WIN32
        if (!WinBg) {
            // This is the parent process.  Since we don't have
            // fork(), create a new process with WinBg set,
            // disconnected from the controlling terminal.  To Bill,
            // let me simply say, "fork you"

            char cmdline[256];
            GetModuleFileName(0, cmdline, 256);
            char *cmdstr = GetCommandLine();
            while (*cmdstr && !isspace(*cmdstr))
                cmdstr++;
            sprintf(cmdline + strlen(cmdline), "%s -winbg", cmdstr);

            PROCESS_INFORMATION *info = msw::NewProcess(cmdline,
                DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP, false);
            if (!info) {
                fprintf(stderr,
                    "Couldn't execute background process, exiting.\n");
                return (1);
            }
            CloseHandle(info->hProcess);
            delete info;
            printf("Running...\n");
            return (0);
        }
#else
        if (fork())
            return (0);
        for (int i = 0; i < 10; i++) {
            if (i != skt)
                close(i);
        }
        open("/", O_RDONLY);
        dup2(0, 1);
        dup2(0, 2);
        setsid();
#endif
    }

#ifdef SIGCHLD
    signal(SIGCHLD, sigchild);
#else
#ifdef SIGCLD
    signal(SIGCLD, sigchild);
#endif
#endif
    fp = fopen(Spiced_Log, "a");
    if (fp) {
        fprintf(fp, "\n-- new daemon, pid = %d, date = %s\n\n",
            (int)getpid(), miscutil::dateString());
        fclose(fp);
    }

    char *user = 0;
    char *hst_or_dsp = 0;
    char *program = 0;

    signal(SIGTERM, kill_spawn);
    signal(SIGINT, kill_spawn);
#ifdef SIGHUP
    signal(SIGHUP, kill_spawn);
#endif

    // Start listening for requests
    listen(skt, 5);
    sockaddr_in from;
    socklen_t len = sizeof(sockaddr_in);
    for (;;) {
        int ioskt = accept(skt, (sockaddr*)&from, &len);
        if (ioskt < 0) {
            if (errno != EINTR) {
                log_perror(">>> accept");
                return (1);
            }
            continue;
        }
        // Should do some sort of verification
        char buf[BUFSIZ];

again:
        int i = recv(ioskt, buf, BUFSIZ, 0);
        if (i < 0) {
            log_perror(">>> recv");
            CLOSESOCKET(ioskt);
            continue;
        }
        if (i == BUFSIZ) {
            log_printf(">>> init line too long\n", buf);
            CLOSESOCKET(ioskt);
            continue;
        }
        buf[i] = 0;

        // The init message consists of at least two tokens:
        //   user host_or_display [program]
        //
        // The user token consists of the user name on the client
        // machine, possibly in the form "user@host".  This is not
        // used except in the log file.  The user name can be followed
        // by colon-separated options:
        //
        //   :+t  Use Xic IPC ("portmon" mode), show the WRspice toolbar.
        //   :-t  Use Xic IPC ("portmon" mode"), don't show the WRspice
        //        toolbar.
        //
        // There are also a couple of special user tokens for backward
        // compatibility:
        //   xic_user     Anonymous Xic user, use Xic IPC, show toolbar.
        //   xic_user_nt  Anonymous Xic user, use Xic IPC, don't show
        //                toolbar.
        //
        // If there is no directive to use Xic IPC, the standard Spice3
        // server mode will be used.
        //
        // The host_or_display token is either the client host name,
        // or the client display name including the host name.  If a
        // display name (the string contains a colon), then X graphics
        // will be used in Xic IPC.  If not, then WRspice will not use
        // graphics, and of course there will be no toolbar shown,
        // when using Xic IPC.  If not useing Xic IPC, a bare hostname
        // should be passed, as no graphics will be used.
        //
        // The optional third value provides the name or path to the
        // WRspice executable on the server machine.

        delete [] user;
        user = 0;
        delete [] hst_or_dsp;
        hst_or_dsp = 0;
        delete [] program;
        program = 0;

        char *s = buf;
        user = lstring::gettok(&s);
        hst_or_dsp = lstring::gettok(&s);
        program = lstring::getqtok(&s);
        if (!hst_or_dsp) {

            // The caller can send a single word "version" and get the
            // protocol version in response.  Servers earlier than
            // wrspice-3.2.9 will not handle this, and will close the
            // connection.

            if (user && !strcmp(user, "version")) {
                send(ioskt, PROTOCOL_VERSION, strlen(PROTOCOL_VERSION)+1, 0);
                goto again;
            }
            log_printf(">>> bad init line: %s\n", buf);
            CLOSESOCKET(ioskt);
            continue;
        }
        if (!program)
            program = lstring::copy(Spice_Exec_Prog);
        else if (!lstring::is_rooted(program)) {
            const char *pgname = lstring::strrdirsep(Spice_Exec_Prog);
            if (pgname && !strcmp(pgname+1, program)) {
                delete [] program;
                program = lstring::copy(Spice_Exec_Prog);
            }
            else {
                // Add the path.
                char *t =
                    new char[strlen(program) + strlen(Spice_Exec_Dir) + 2];
                sprintf(t, "%s/%s", Spice_Exec_Dir, program);
                delete [] program;
                program = t;
            }
        }

        bool xic_ipc = false;
        bool show_toolbar = true;

        // Parse and strip the user token flags.
        char *t = strrchr(user, ':');
        if (t) {
            *t++ = 0;
            if (t[1] == 't' || t[1] == 'T') {
                if (t[0] == '+') {
                    xic_ipc = true;
                    show_toolbar = true;
                }
                else if (t[0] == '-') {
                    xic_ipc = true;
                    show_toolbar = false;
                }
            }
        }

        // Handle special cases.
        if (!strcmp(user, "xic_user")) {
            xic_ipc = true;
            show_toolbar = true;
        }
        else if (!strcmp(user, "xic_user_nt")) {
            xic_ipc = true;
            show_toolbar = false;
        }

        bool use_x = true;
        if (!strchr(hst_or_dsp, ':'))
            use_x = false;
        else if (!xic_ipc) {
            // Display passed but not using Xic IPC, strip display
            // suffix.
            *strchr(hst_or_dsp, ':') = 0;
        }

        if (!use_x)
            show_toolbar = false;

        if (Nrunning >= MaxJobs) {
            // Too many jobs
            send(ioskt, "toomany", 8, 0);
            log_printf("%s: %s@%s: rejected (maxjobs) - %d jobs now\n",
                miscutil::dateString(), user, hst_or_dsp, Nrunning);
            CLOSESOCKET (ioskt);
            continue;
        }
        send(ioskt, "ok", 3, 0);

#ifdef WIN32
        sLstr lstr;
        lstr.add(program);
        if (xic_ipc) {
            if (use_x) {
                if (show_toolbar) {
                    // Called remotely by Xic, start program in
                    // portmon mode, with toolbar visible.
                    lstr.add(" -P -I -D ");
                    lstr.add(hst_or_dsp);
                }
                else {
                    // Called remotely by Xic, start program in
                    // portmon mode, with toolbar invisible.
                    lstr.add(" -P -D ");
                    lstr.add(hst_or_dsp);
                }
            }
            else {
                // Called remotely by Xic, start program in
                // portmon mode, with no graphics.
                lstr.add(" -P -Dnone ");
            }
        }
        else
            lstr.add(" -S");

        char *infile = smktemp("in");
        FILE *ifp = fopen(infile, "w");
        if (ifp) {
            bool noerr = true;
            while (noerr) {
                int cnt = 0;
                for (;;) {
                    if (recv(ioskt, buf + cnt, 1, 0) > 0)
                        cnt++;
                    else {
                        buf[cnt] = 0;
                        noerr = false;
                        break;
                    }
                    if (buf[cnt-1] == '\n') {
                        buf[cnt] = 0;
                        break;
                    }
                }
                if (*buf == '@')
                    break;
                fputs(buf, ifp);
            }
            fclose(ifp);
        }

        SECURITY_ATTRIBUTES sa;
        memset(&sa, 0, sizeof(SECURITY_ATTRIBUTES));
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.bInheritHandle = true;
        char *outfile = smktemp("out");
        HANDLE hin = CreateFile(infile, GENERIC_READ, 0, &sa,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        HANDLE hout = CreateFile(outfile, GENERIC_WRITE,
            FILE_SHARE_READ, &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

        PROCESS_INFORMATION *info = msw::NewProcess(lstr.string(),
            DETACHED_PROCESS|CREATE_NEW_PROCESS_GROUP, true, hin, hout, hout);
        if (!info) {
            log_printf("%s: %s@%s, \"%s\", pid %d\n", miscutil::dateString(),
                user, hst_or_dsp, program, -1);
            CLOSESOCKET(ioskt);
            continue;
        }
        Nrunning++;
        for (int j = 0; j < MAXJOBS; j++) {
            if (pids[j] == 0) {
                pids[j] = info->dwProcessId;
                break;
            }
        }
        log_printf("%s: %s@%s, \"%s\", pid %d\n", miscutil::dateString(),
            user, hst_or_dsp, program, info->dwProcessId);

        thdata *th = new thdata(info->hProcess, hin, hout, infile, outfile,
            ioskt);
        delete info;
        _beginthread(th_local_hdlr, 0, th);

#else
        if ((i = fork()) == 0) {
            CLOSESOCKET(skt);
#ifdef SIGCHLD
            signal(SIGCHLD, SIG_IGN);
#else
#ifdef SIGCLD
            signal(SIGCLD, SIG_IGN);
#endif
#endif
            dup2(ioskt, 0);
            dup2(ioskt, 1);
            dup2(ioskt, 2);
            char *argv[6];
            argv[0] = program;
            if (xic_ipc) {
                if (use_x) {
                    if (show_toolbar) {
                        // Called remotely by Xic, start program in
                        // portmon mode, with toolbar visible.
                        argv[1] = (char*)"-P";
                        argv[2] = (char*)"-I";  // turn on toolbar
                        argv[3] = (char*)"-D";
                        argv[4] = hst_or_dsp;
                        argv[5] = 0;
                    }
                    else {
                        // Called remotely by Xic, start program in
                        // portmon mode, with toolbar invisible.
                        argv[1] = (char*)"-P";
                        argv[2] = (char*)"-D";
                        argv[3] = hst_or_dsp;
                        argv[4] = 0;
                    }
                }
                else {
                    // Called remotely by Xic, start program in
                    // portmon mode, with no graphics.
                    argv[1] = (char*)"-P";
                    argv[2] = (char*)"-Dnone";
                    argv[3] = 0;
                }
            }
            else {
                argv[1] = (char*)"-S";
                argv[2] = 0;
            }
            execve(program, argv, environ);
            log_perror(program);
            return (1);
        }
        else if (i > 0) {
            Nrunning++;
            for (int j = 0; j < MAXJOBS; j++) {
                if (pids[j] == 0) {
                    pids[j] = i;
                    break;
                }
            }
            log_printf("%s: %s@%s, \"%s\", pid %d\n", miscutil::dateString(),
                user, hst_or_dsp, program, i);
        }
        CLOSESOCKET(ioskt);
#endif
    }
}


namespace {
    // Initialize internal variables.
    //
    void
    init_vars()
    {
        const char *prefix = getenv("XT_PREFIX");
        if (!prefix || !lstring::is_rooted(prefix))
            prefix = PREFIX;

        // executables directory
        const char *string = getenv("SPICE_EXEC_DIR");
#ifdef WIN32
        if (string == 0) {
            string = msw::GetProgramRoot("WRspice");
            if (string) {
                char *p = pathlist::expand_path(string, true, true);
                delete [] string;
                string = p;
                p = (char*)lstring::strrdirsep(string);
                if (p)
                    strcpy(p+1, "bin");
                // <Prefix>/TOOLS_ROOT/bin
            }
        }
#endif
        if (string == 0) {
            // Path is xictools/bin in Windows. xictoiols/wrspice/bin
            // otherwise.
#ifdef WIN32
            char *s = new char[strlen(prefix) + strlen(TOOLS_ROOT) + 10];
            sprintf(s, "%s/%s/bin", prefix, TOOLS_ROOT);
            string = s;
#else
            char *s = new char[strlen(prefix) + strlen(TOOLS_ROOT) +
                strlen(SPICE_PROG) + 10];
            sprintf(s, "%s/%s/%s/bin", prefix, TOOLS_ROOT, SPICE_PROG);
            string = s;
#endif
        }
        Spice_Exec_Dir = string;

        // path to spice program
        string = getenv("SPICE_PATH");
        if (string == 0) {
            char *s =
                new char[strlen(Spice_Exec_Dir) + strlen(SPICE_PROG) + 2];
            strcpy(s, Spice_Exec_Dir);
            strcat(s, "/");
            strcat(s, SPICE_PROG);
            string = s;
        }
        else {
            if (lstring::is_rooted(string)) {
                char *s = new char[strlen(string) + 1];
                strcpy(s, string);
                string = s;
            }
            else {
                char *s =
                    new char[strlen(Spice_Exec_Dir) + strlen(string) + 2];
                strcpy(s, Spice_Exec_Dir);
                strcat(s, "/");
                strcat(s, string);
                string = s;
            }
        }
        Spice_Exec_Prog = string;

        // log file for daemon
        string = getenv("SPICE_DAEMONLOG");
        if (string == 0)
            string = SPICE_DAEMONLOG;
        char *s = new char[strlen(string) + 1];
        strcpy(s, string);
        Spiced_Log = s;
    }


#ifndef WIN32

    // Record what happens to child.
    //
    void
    sigchild(int)
    {
#ifdef HAVE_GETRUSAGE
        // Assume BSDRUSAGE -> wait3()
        int stats;
        rusage ru;
        int pid = wait3(&stats, 0, &ru);
        if (pid == -1) {
            log_perror(">>> wait3");
            return;
        }
        Nrunning--;
        for (int j = 0; j < MAXJOBS; j++) {
            if (pids[j] == pid) {
                pids[j] = 0;
                break;
            }
        }
        int rc = 0;
#else
#ifdef HAVE_SYS_WAIT_H
        int stats;
        int pid = wait(&stats);
        if (pid == -1) {
            log_perror(">>> wait");
            return;
        }
        Nrunning--;
        for (int j = 0; j < MAXJOBS; j++) {
            if (pids[j] == pid) {
                pids[j] = 0;
                break;
            }
        }
        int rc = -1;
        if (WIFEXITED(stats))
            rc = WEXITSTATUS(stats);
        else if (WIFSIGNALED(stats))
            rc = 127;
#endif
#endif

        // Write a log entry
        char buf[BUFSIZ];
#ifdef HAVE_GETRUSAGE
        sprintf(buf, "%d:%d.%06d", (int)ru.ru_utime.tv_sec / 60,
                (int)ru.ru_utime.tv_sec % 60, (int)ru.ru_utime.tv_usec);
        char *t;
        for (t = buf; *t; t++) ;
        for (t--; *t == '0'; *t-- = '\0') ;
#else
        strcpy(buf, "unknown");
#endif
        log_printf("%s: pid %d, exit %d, time %s\n", miscutil::dateString(),
            pid, rc, buf);
    }

#endif


    // Replacement for perror() which supports diversion to log file.
    //
    void
    log_perror(const char *str)
    {
#ifdef HAVE_STRERROR
        if (str && *str)
            log_printf("%s: %s\n", str, strerror(errno));
        else
            log_printf("%s\n", strerror(errno));
#else
        if (str && *str)
            log_printf("%s: %s\n", str, sys_errlist[errno]);
        else
            log_printf("%s\n", sys_errlist[errno]);
#endif
    }


    // Print a formatted string to the log file.
    //
    void
    log_printf(const char *fmt, ...)
    {
        va_list args;
        char buf[BUFSIZ];
        va_start(args, fmt);
        vsnprintf(buf, BUFSIZ, fmt, args);
        va_end(args);

        FILE *fp = fopen(Spiced_Log, "a");
        if (fp) {
            fputs(buf, fp);
            fclose(fp);
        }
    }
}


#else // not HAVE_SOCKET

int
main()
{
    fprintf(stderr, "No socket support, can't execute!\n");
    return (1);
}
#endif

