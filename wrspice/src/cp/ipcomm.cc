
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

#include <sys/types.h>
#include <sys/time.h>
#ifdef WIN32
#include "miscutil/msw.h"
#else
#include <sys/wait.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/file.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#endif
#include <errno.h>
#include <signal.h>
#include "config.h"
#include "graph.h"
#include "spglobal.h"
#include "cshell.h"
#include "kwords_fte.h"
#include "commands.h"
#include "simulator.h"
#include "output.h"
#include "toolbar.h"
#include "miscutil/childproc.h"


#ifdef WIN32
#define CLOSESOCKET(x) shutdown(x, SD_SEND), closesocket(x)
#else
#define CLOSESOCKET(x) close(x)
#endif

namespace {
    bool netdbg()
    {
        static bool checked;
        static bool set;

        if (!checked) {
            checked = true;
            set = (getenv("XTNETDEBUG") != 0);
        }
        return (set);
    }
}


// Set up a socket and start listening.  Set the port number,
// and the socket descriptor.  Both are >= 0 on success.
//
bool
CshPar::InitIPC()
{
    if (cp_acct_sock < 0) {
        int sd = socket(AF_INET, SOCK_STREAM, 0);
        if (sd < 0) {
            GRpkgIf()->Perror("socket");
            return (true);
        }
        struct sockaddr_in skt;
        skt.sin_family = AF_INET;
        skt.sin_addr.s_addr = INADDR_ANY;
        skt.sin_port = 0;
        if (bind(sd, (struct sockaddr*)&skt, sizeof(skt))) {
            GRpkgIf()->Perror("bind");
            return (true);
        }
#if 1
        socklen_t len = sizeof(skt);
#else
        int len = sizeof(skt);
#endif
        if (getsockname(sd, (struct sockaddr*)&skt, &len)) {
            GRpkgIf()->Perror("getsockname");
            return (true);
        }
        listen(sd, 1);
        cp_acct_sock = sd;
        cp_port = ntohs(skt.sin_port);
    }
    return (false);
}


bool
CshPar::InitIPCmessage()
{
    int msg = accept(cp_acct_sock, 0, 0);
    if (msg < 0) {
        GRpkgIf()->Perror("accept");
        return (true);
    }
#ifdef F_SETOWN
    // set socket pid for oob data, oob is used to pass interrupts
    // to remote runs
    fcntl(msg, F_SETOWN, getpid());
#endif
    cp_mesg_sock = msg;
    return (false);
}


namespace {
    bool write_msg(const char *str)
    {
        if (!str)
            return (true);
        if (netdbg())
            TTY.err_printf("wrspice: sending \"%s\"\n", str);

        // Note that the trailing null byte is sent as termination.
        int len = strlen(str) + 1;

        int fd = CP.MesgSocket();
        int bytes = 0;
        for (;;) {
            int i = send(fd, str, len - bytes, 0);
            if (i == -1) {
                GRpkgIf()->Perror("send");
                return (false);
            }
            bytes += i;
            if (bytes == len)
                break;
            str += i;
        }
        if (netdbg())
            TTY.err_printf("wrspice: done\n");
        return (true);
    }


    bool write_data(const char *buf, unsigned int nbytes)
    {
        if (!buf || !nbytes)
            return (true);
        if (netdbg())
            TTY.err_printf("wrspice: sending data, %d bytes.\n", nbytes);
        int len = nbytes;

        int fd = CP.MesgSocket();
        int bytes = 0;
        for (;;) {
            int i = send(fd, buf, len - bytes, 0);
            if (i == -1) {
                GRpkgIf()->Perror("send");
                return (false);
            }
            bytes += i;
            if (bytes == len)
                break;
            buf += i;
        }
        if (netdbg())
            TTY.err_printf("wrspice: done\n");
        return (true);
    }


    // Reverse the byte order if the MSB's are at the top address, i.e.,
    // switch to/from "network byte order".  This will reverse bytes on
    // Intel x86, but is a no-op on Sun SPARC (for example).
    //
    // IEEE floating point is assumed here!
    //
    double net_byte_reorder(double d)
    {
        static double d1p0 = 1.0;

        if ( ((unsigned char*)&d1p0)[7] ) {
            // This means MSB's are at top address, reverse bytes.
            double dr;
            unsigned char *t = (unsigned char*)&dr;
            unsigned char *s = (unsigned char*)&d;
            t[0] = s[7];
            t[1] = s[6];
            t[2] = s[5];
            t[3] = s[4];
            t[4] = s[3];
            t[5] = s[2];
            t[6] = s[1];
            t[7] = s[0];
            return (dr);
        }
        else
            return (d);
    }
}


// Routine to process messages received through the interprocess
// communication port.  Read the message and respond.  Return false on EOF.
// Arg fc is first character of message.
//
bool
CshPar::MessageHandler(int fc)
{
    // clear interrupt flags
    cp_flags[CP_INTRPT] = false;
    Sp.SetFlag(FT_INTERRUPT, false);

    char buf[BSIZE_SP];
    if (fc == '\n' || !fc) {
        write_msg("ok");
        return (true);
    }
    char *s = buf;
    if (fc > ' ')
        *s++ = fc;
    for (;;) {
        char c;
        int i = recv(cp_mesg_sock, &c, 1, 0);
        if (i <= 0) {
            // EOF or error
            if (i < 0) {
                if (errno == EINTR)
                    continue;
                GRpkgIf()->Perror("recv");
            }
            return (false);
        }

        // Messages from Xic are 0-terminated, also regognize network
        // termination.
        if (c == 0)
            break;
        if (c == '\n' && s > buf && *(s-1) == '\r') {
            *(s-1) = 0;
            break;
        }
        *s++ = c;
    }
    *s-- = 0;
    while (s >= buf && (*s == '\n' || *s == '\r'))
        *s-- = 0;

    if (netdbg())
        TTY.err_printf("wrspice: received \"%s\"\n", buf);
    if (!buf[0]) {
        write_msg("ok");
        return (true);
    }

    s = buf;
    cp_flags[CP_MESSAGE] = true;
    // some "secret" interface functions
    if (lstring::eq(buf, "inprogress")) {
        // Return whether or not a simulation is in progress.
        if (Sp.InProgress())
            write_msg("y");
        else
            write_msg("n");
    }
    else if (lstring::eq(buf, "decksource")) {
        // Source a deck.
        bool ok = Sp.SpSource(0, false, false, 0);
        cp_flags[CP_CWAIT] = true;  // have to reset this
        Sp.Periodic();
        write_msg(ok ? "ok" : "er");
    }
    else if (lstring::eq(buf, "curplot")) {
        // Return the name of the current plot.
        const char *c = 0;
        if (OP.curPlot())
            c = OP.curPlot()->type_name();
        if (c && *c)
            write_msg(c);
        else
            write_msg("none");
    }
    else if (lstring::eq(buf, "curckt")) {
        // Return the name of the current circuit.
        const char *c = 0;
        if (Sp.CurCircuit())
            c = Sp.CurCircuit()->name();
        if (c && *c)
            write_msg(c);
        else
            write_msg("none");
    }
    else if (lstring::match("freeplot", buf)) {
        // Destroy the named plot, or the current plot if no name. 
        // This removes all plots with the same circuit name, taking
        // care of analyses that produce multiple plots.

        lstring::advtok(&s);
        char *name = lstring::gettok(&s);
        if (name) {
            if (!lstring::eq(name, "constants"))
                OP.removePlot(name, true);
            delete [] name;
        }
        else if (OP.curPlot()) {
            const char *c = OP.curPlot()->type_name();
            if (c && *c && !lstring::eq(c, "constants"))
                OP.removePlot(c, true);
        }
        write_msg("ok");
    }
    else if (lstring::match("freecir", buf)) {
        // Destroy the named circuit, or the current circuit if no name.
        lstring::advtok(&s);
        char *name = lstring::gettok(&s);
        if (name) {
            Sp.FreeCircuit(name);
            delete [] name;
        }
        else if (Sp.CurCircuit()) {
            const char *c = Sp.CurCircuit()->name();
            if (c && *c)
                Sp.FreeCircuit(c);
        }
        write_msg("ok");
    }
    else if (lstring::match("close", buf)) {
        // no response to close
        lstring::advtok(&s);
        pid_t pid = isdigit(*s) ? atoi(s) : 0;
        if (pid > 0) {
            // Spice is parent of graphical editor.
            CLOSESOCKET(cp_mesg_sock);
            cp_mesg_sock = -1;
        }
        else if (pid == 0) {
            // Spice is child of graphical editor, exit
            fatal(false);
        }
    }
    else if (lstring::match("winid", buf)) {
        // So the plots don't disappear below Xic window.
        lstring::advtok(&s);
        int wid = atoi(s);
        if (wid)
            ToolBar()->RegisterBigForeignWindow(wid);
        write_msg("ok");
    }
    else if (lstring::match("vecget", buf)) {
        // Print and return scalar (0'th component) vector value.
        lstring::advtok(&s);
        bool wrote = false;
        if (*s) {
            sCKT *ckt = Sp.CurCircuit() ? Sp.CurCircuit()->runckt() : 0;
            sDataVec *dv = OP.vecGet(s, ckt);
            if (dv) {
                if (dv->isreal())
                    sprintf(buf, "ok%15e", dv->realval(0));
                else 
                    sprintf(buf, "ok%15e,%15e", dv->realval(0),
                        dv->imagval(0));
                write_msg(buf);
                wrote = true;
            }
        }
        if (!wrote)
            write_msg(buf);
    }
    else if (lstring::match("eval", buf)) {
        // Evaluate an expression, return result as binary data.
        lstring::advtok(&s);
        bool wrote = false;
        while (*s) {
            // while "loop", so we can use break

            pnlist *pl0 = Sp.GetPtree(s, true);
            if (pl0 == 0)
                break;
            sDvList *dl0 = Sp.DvList(pl0);
            if (dl0 == 0)
                break;

            sDataVec *dv = dl0->dl_dvec;
            if (dv) {
                int len = dv->length();
                int datalen = len*sizeof(double);
                if (dv->iscomplex())
                    datalen *= 2;
                // Header: "okdr" or "okdc", 4B len in network byte order.
                datalen += 8;

                sprintf(buf, "data %d", datalen);
                write_msg(buf);

                char *dbuf = new char[datalen];
                unsigned int *uip = (unsigned int*)dbuf;
                double *dp = (double*)(dbuf + 8);
                dbuf[0] = 'o';
                dbuf[1] = 'k';
                dbuf[2] = 'd';  // double data, extend to other types
                dbuf[3] = dv->iscomplex() ? 'c' : 'r';
                uip[1] = htonl(len);
                for (int i = 0; i < len; i++) {
                    if (dv->iscomplex()) {
                        *dp++ = net_byte_reorder(dv->realval(i));
                        *dp++ = net_byte_reorder(dv->imagval(i));
                    }
                    else
                        *dp++ = net_byte_reorder(dv->realval(i));
                }
                write_data(dbuf, datalen);
                delete [] dbuf;
                wrote = true;
            }
            sDvList::destroy(dl0);
            break;
        }
        if (!wrote)
            write_msg(buf);
    }
    else if (lstring::match("ping", buf)) {
        // New in 3.2.15, the ping command has an argument string
        // which allows (at present) the subcircuit catchar and name
        // mapping mode to set from Xic.  The new interface is
        // designed to be backwards compatible so that any Xic and
        // WRspice releases can coexist.
        //
        // WRspice:
        //   Pre-3.2.5, subc catchar fixed as ':', SPICE3 mode, ping
        //     returns "ok".
        //   3.2.5 and later, subc catchar set in WRspice, uses SPICE3
        //     mode, ping returns "ok<catchar>".
        //   3.2.15 and later, use this interface, i.e., catchar and
        //     mode set by Xic, ping returns "ok<catchar><mode>"
        //
        // Xic:
        //   Pre-3.2.6, did not send ping, assumes ':' and SPICE3 mode.
        //   3.2.6 and later, sends ping, looks for subc catchar in ping
        //     return, assumes SPICE3 mode.
        //   3.2.23 and later, uses this interface.
        //
        // The logic here is:
        //   No ping:  catchar is ':', SPICE3 mode.
        //   Ping with no args:  catchar is default, returned, use
        //     SPICE3 mode, mode not returned.
        //   Ping with args: set and return catchar and mode.
        //
        // When WRspice is started in "port monitor" mode, the subc
        // catchar is set to ":" and mode to SPICE3.

        // The argument(s) to ping have the following syntax:
        //   [-sc<catchar>][ ][-sm<mode>]
        // where <mode> is a digit character ('0' plus the enum value).

        lstring::advtok(&s);
        bool modeset = false;
        for (;;) {
            if (isspace(*s)) {
                s++;
                continue;
            }
            if (*s != '-')
                break;
            s++;
            if (s[0] == 's' && s[1] == 'c') {
                s += 2;
                if (*s && !isspace(*s))
                    Sp.SetSubcCatchar(*s);
                s++;
                continue;
            }
            if (s[0] == 's' && s[1] == 'm') {
                s += 2;
                if (s[0] == '0')
                    Sp.SetSubcCatmode(SUBC_CATMODE_WR);
                else if (s[0] == '1')
                    Sp.SetSubcCatmode(SUBC_CATMODE_SPICE3);
                s++;
                modeset = true;
                continue;
            }
        }
        if (modeset)
            sprintf(buf, "ok%c%c", Sp.SubcCatchar(), '0' + Sp.SubcCatmode());
        else
            sprintf(buf, "ok%c", Sp.SubcCatchar());
        write_msg(buf);
    }
    else {
        // Run as WRspice built-in command.
        bool intr = cp_flags[CP_INTERACTIVE];
        cp_flags[CP_INTERACTIVE] = false;
#ifdef WIN32
        extern jmp_buf msw_jbf[4];
        extern int msw_jbf_sp;

        {
            volatile bool dopop = false;
            if (msw_jbf_sp < 3) {
                msw_jbf_sp++;
                dopop = true;
            }
            if (setjmp(msw_jbf[msw_jbf_sp]) == 0) {
                EvLoop(buf);
            }
            if (dopop)
                msw_jbf_sp--;
        }
#else
        try { EvLoop(buf); }
        catch (int) { }
#endif
        cp_flags[CP_CWAIT] = true;  // have to reset this
        cp_flags[CP_INTERACTIVE] = intr;
        Sp.Periodic();
        write_msg("ok");
    }
    cp_flags[CP_MESSAGE] = false;
    return (true);
}


//
// Process termination handlers for Xic
//
namespace {
    bool XicIsAlive;

#ifdef WIN32

    void thread_proc(void *arg)
    {
        PROCESS_INFORMATION *info = (PROCESS_INFORMATION*)arg;
        if (WaitForSingleObject(info->hProcess, INFINITE) == WAIT_OBJECT_0) {
            DWORD status;
            GetExitCodeProcess(info->hProcess, &status);
            if (status)
                GRpkgIf()->ErrPrintf(ET_WARN,
                    "process %ld exited with status %ld.\n",
                    info->dwProcessId, status);
            if (netdbg()) {
                TTY.err_printf("Background job %ld done.\n",
                    info->dwProcessId);
            }
            CloseHandle(info->hProcess);
            delete info;
            XicIsAlive = false;
        }
    }

#else
            
    void sigchild(int pid, int status, void*)
    {
        if (WIFEXITED(status) && WEXITSTATUS(status))
            GRpkgIf()->ErrPrintf(ET_WARN,
                "process %d exited with status %d.\n",
                pid, WEXITSTATUS(status));
        else if (WIFSIGNALED(status))
            GRpkgIf()->ErrPrintf(ET_WARN,
                "process %d terminated by signal %d.\n",
                pid, WIFSIGNALED(status));
        XicIsAlive = false;
        if (netdbg())
            TTY.err_printf("Background job %d done.\n", pid);
    }

#endif
}


// Fork the graphical editor, and set up connections for interaction.
//
void
CommandTab::com_sced(wordlist *wl)
{
    extern char **environ;
    if (!CP.Display()) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "no X display available.\n");
        return;
    }
    if (XicIsAlive) {
        TTY.printf("Graphical editor is already active.\n");
        return;
    }
    if (CP.MesgSocket() >= 0) {
        CLOSESOCKET(CP.MesgSocket());
        CP.SetMesgSocket(-1);
    }

    // Find the path to Xic.  In Windows, this is .../xictools/bin,
    // otherwise it is .../xictools/xic/bin, for current releases.

    sLstr xpstr;
    VTvalue vv;
    if (Sp.GetVar(kw_xicpath, VTYP_STRING, &vv))
        xpstr.add(vv.get_string());
    else {
        const char *progname = Global.GfxProgName();
        if (!progname || !*progname)
            progname = "xic";
#ifdef WIN32
        char *dp = msw::GetProgramRoot("Xic");
        if (dp) {
            char *e = lstring::strrdirsep(dp);
            if (e)
                *(e+1) = 0;
            xpstr.add(dp);
            xpstr.add("bin/");
            delete [] dp;
            xpstr.add(progname);
        }
#endif
        if (!xpstr.string()) {
            xpstr.add(Global.Prefix());
            xpstr.add_c('/');
            xpstr.add(Global.ToolsRoot());
            xpstr.add("/bin/");
            xpstr.add(progname);
        }
    }

#ifdef WIN32
    // Under Windows, Xic is almost always run through the batch file
    // wrapper.  Unless the user has explicitly given ".exe", add a
    // ".bat" extension if not already present.

    const char *e = strrchr(xpstr.string(), '.');
    if (!e || (!lstring::cieq(e+1, "exe") && !lstring::cieq(e+1, "bat"))) {
        xpstr.add(".bat");
    }
    // Change to DOS-style path.
    xpstr.to_dosdirsep();
#endif
    if (access(xpstr.string(), X_OK)) {
        TTY.printf("The graphical editor executable can not be found.\n");
        TTY.printf("XicPath: %s\n", xpstr.string());
        return;
    }
    if (CP.InitIPC())
        return;
    pid_t pid = getpid();

#ifdef WIN32
    char buf[256];
    sprintf(buf, " -E -P%d:%d", CP.Port(), (int)pid);
    xpstr.add(buf);
    for ( ; wl; wl = wl->wl_next)
        xpstr.append(" ", wl->wl_word);

//XXX bat file prob won't work
    PROCESS_INFORMATION *info = msw::NewProcess(0, xpstr.string(), 
        DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP, true);
    if (!info) {
        TTY.printf("The graphical editor can not be executed.\n");
        delete info;
        return;
    }
    _beginthread(thread_proc, 0, info);
    WaitForInputIdle(info->hProcess, 5000);
#else
    int i = fork();
    if (i < 0) {
        GRpkgIf()->Perror("fork");
        CLOSESOCKET(CP.AcctSocket());
        CP.SetAcctSocket(-1);
        return;
    }
    if (i == 0) {
        CLOSESOCKET(CP.AcctSocket());
        ToolBar()->CloseGraphicsConnection();
        close(fileno(stdin));
        setvbuf(stdout, 0, _IONBF, 0);
        dup2(fileno(stderr), fileno(stdout));

        char portarg[32];
        sprintf(portarg, "-P%d:%d", CP.Port(), (int)pid);
        int ac = wordlist::length(wl) + 3;
        char **av = new char*[ac+1];
        av[0] = xpstr.string_trim();
        av[1] = lstring::copy("-E");  // start in electrical mode
        av[2] = lstring::copy(portarg);
        for (i = 3; wl; i++, wl = wl->wl_next)
            av[i] = lstring::copy(wl->wl_word);
        av[i] = 0;
        execve(av[0], av, environ);
        _exit(EXIT_BAD);
    }
    Proc()->RegisterChildHandler(i, sigchild, 0);
#endif
    for (;;) {
        fd_set ready;
        FD_ZERO(&ready);
        FD_SET(CP.AcctSocket(), &ready);
        timeval to;
        to.tv_sec = 120;
        to.tv_usec = 0;
#ifdef SELECT_TAKES_INTP
        // this stupidity from HPUX
        if (select(CP.AcctSocket()+1, (int*)&ready, 0, 0, &to) == -1) {
#else
        if (select(CP.AcctSocket()+1, &ready, 0, 0, &to) == -1) {
#endif
            if (errno == EINTR)
                continue;
            CLOSESOCKET(CP.AcctSocket());
            CP.SetAcctSocket(-1);
            GRpkgIf()->ErrPrintf(ET_ERROR, "Connection broken.\n");
            break;
        }
        if (FD_ISSET(CP.AcctSocket(), &ready)) {
            int msg = accept(CP.AcctSocket(), 0, 0);
            if (msg >= 0)
                CP.SetMesgSocket(msg);
            else {
                GRpkgIf()->Perror("accept");
                CLOSESOCKET(CP.AcctSocket());
                CP.SetAcctSocket(-1);
            }
            break;
        }
        CLOSESOCKET(CP.AcctSocket());
        CP.SetAcctSocket(-1);
        GRpkgIf()->ErrPrintf(ET_ERROR, "Connection timed out.\n");
        break;
    }
    XicIsAlive = true;
}

