
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
 * Xic Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "config.h"  // for HAVE_LOCAL_ALLOCATOR
#include "main.h"
#include "drcif.h"
#include "si_parsenode.h"
#include "si_parser.h"
#include "si_interp.h"
#include "si_daemon.h"
#include "miscutil/childproc.h"

#include <signal.h>
#include <errno.h>
#ifdef WIN32
#include <conio.h>
#include "miscutil/msw.h"
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#endif

#ifdef __linux
#ifndef __USE_GNU
#define __USE_GNU
#endif
#endif
#ifdef __APPLE__
#define _XOPEN_SOURCE
#endif
#ifndef WIN32
#include <ucontext.h>
#endif
#ifdef HAVE_LOCAL_ALLOCATOR
#include "malloc/local_malloc.h"
#endif


namespace {
#ifdef WIN32
    void sig_hdlr(int);
#else
    void sig_hdlr(int, siginfo_t*, void*);
#endif

    bool NoDieOnIntr;

    // start gdb on panic
    const bool PanicToGDB = true;
}

#ifdef __linux
#define sig_t sighandler_t
#endif
#ifdef WIN32
typedef void(*sig_t)(int);
#endif

#define SIG_HDLR (sig_t)&sig_hdlr


// Static function.
// ^C handler exported to graphics.
//
void
cMain::InterruptHandler()
{
    SI()->SetInterrupt();
    DSP()->SetInterrupt(DSPinterUser);
}


// Initialize the signal handling.  Call with false at program startup,
// true when setup is complete.
//
void
cMain::InitSignals(bool no_die)
{
    if (no_die)
        // On startup, allow ^C to stop program
        NoDieOnIntr = true;
    DSP()->SetInterrupt(DSPinterNone);

#ifdef WIN32
    signal(SIGSEGV, SIG_HDLR);
    signal(SIGILL, SIG_HDLR);

    signal(SIGINT, SIG_HDLR);
    signal(SIGTERM, SIG_HDLR);
    signal(SIGFPE, SIG_HDLR);
#ifdef SIGHUP
    signal(SIGHUP, SIG_HDLR);
#endif
#ifdef SIGQUIT
    signal(SIGQUIT, SIG_HDLR);
#endif
#ifdef SIGSYS
    signal(SIGSYS, SIG_HDLR);
#endif
#ifdef SIGTRAP
    signal(SIGTRAP, SIG_HDLR);
#endif
#ifdef SIGABRT
    signal(SIGABRT, SIG_HDLR);
#endif
#ifdef SIGEMT
    signal(SIGEMT, SIG_HDLR);
#endif
#ifdef SIGURG
    signal(SIGURG, SIG_HDLR);
#endif
#ifdef SIGPIPE
    signal(SIGPIPE, SIG_HDLR);
#endif
#ifdef SIGCHLD
    signal(SIGCHLD, SIG_HDLR);
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
#ifdef SIGUSR1
    signal(SIGUSR1, SIG_HDLR);
#endif
#ifdef SIGUSR2
    signal(SIGUSR2, SIG_HDLR);
#endif
#ifdef SIGIO
    signal(SIGIO, SIG_IGN);
#endif
#else
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO | SA_RESTART;
    sa.sa_sigaction = sig_hdlr;
    sigaction(SIGSEGV, &sa, 0);
    sigaction(SIGBUS, &sa, 0);
    sigaction(SIGILL, &sa, 0);

    sigaction(SIGINT, &sa, 0);
    sigaction(SIGTERM, &sa, 0);
    sigaction(SIGFPE, &sa, 0);
#ifdef SIGHUP
    sigaction(SIGHUP, &sa, 0);
#endif
#ifdef SIGQUIT
    sigaction(SIGQUIT, &sa, 0);
#endif
#ifdef SIGSYS
    sigaction(SIGSYS, &sa, 0);
#endif
#ifdef SIGTRAP
    sigaction(SIGTRAP, &sa, 0);
#endif
#ifdef SIGABRT
    sigaction(SIGABRT, &sa, 0);
#endif
#ifdef SIGEMT
    sigaction(SIGEMT, &sa, 0);
#endif
#ifdef SIGURG
    sigaction(SIGURG, &sa, 0);
#endif
#ifdef SIGPIPE
    sigaction(SIGPIPE, &sa, 0);
#endif
#ifdef SIGCHLD
    sigaction(SIGCHLD, &sa, 0);
#endif
#ifdef SIGXCPU
    sigaction(SIGXCPU, &sa, 0);
#endif
#ifdef SIGXFSZ
    sigaction(SIGXFSZ, &sa, 0);
#endif
#ifdef SIGVTALRM
    sigaction(SIGVTALRM, &sa, 0);
#endif
#ifdef SIGUSR1
    sigaction(SIGUSR1, &sa, 0);
#endif
#ifdef SIGUSR2
    sigaction(SIGUSR2, &sa, 0);
#endif
#ifdef SIGIO
    sigaction(SIGIO, &sa, 0);
#endif
#endif
}


// Address of faulting line, for debugging
void *DeathAddr;

namespace {
    // Exit function for signal handlers.
    //
    void
    die_now(const char *msg, bool panic)
    {
#ifdef HAVE_LOCAL_ALLOCATOR
        // Avoid malloc recursion message when handling signals while in
        // allocator.  This can happen when the operating system kills the
        // application with a TERM signal due to excessive memory
        // consumption.
        Memory()->force_not_busy();
#endif
        if (XM()->RunMode() == ModeBackground) {
            // Running background DRC job.  The pid is unknown to the
            // license server.  All cleanup is done by parent.
            DrcIf()->fileMessage(msg);
#ifdef SIGKILL
            raise(SIGKILL);
#endif
            exit(1);
        }

        fputs("\n", stderr);
        if (msg) {
            fputs(msg, stderr);
            const char *s = msg + strlen(msg) - 1;
            if (*s != '\n')
                fputs("\n", stderr);
        }
        if (panic) {
            if (PanicToGDB)
                XM()->Exit(ExitDebugger);
            else
                XM()->Exit(ExitPanic);
        }
        else
            XM()->Exit(ExitNormal);
    }
}


namespace {
    // The signal handler
    //
    void
#ifdef WIN32
    sig_hdlr(int sig)
    {
        signal(sig, SIG_HDLR);  // reset for SysV
#else
    sig_hdlr(int sig, siginfo_t*, void *uc)
    {
        // This should put the address of a faulting line in DeathAddr.
        //
        if (sig == SIGSEGV || sig == SIGBUS || sig == SIGILL) {
            static bool didit;
            if (!didit) {
                didit = true;
#ifdef __sparc
                if (uc)
                    DeathAddr =
                        (void*)((ucontext_t*)uc)->uc_mcontext.gregs[REG_PC];
#else
#ifdef __linux
                // in sys/ucontext.h
#if defined(__x86_64) || defined(__x86_64__)
                // opteron
                if (uc)
                    DeathAddr =
                        (void*)((ucontext_t*)uc)->uc_mcontext.gregs[REG_RIP];
#else
                // x86
#ifndef REG_EIP
#define REG_EIP EIP
#endif
                if (uc)
                    DeathAddr =
                        (void*)((ucontext_t*)uc)->uc_mcontext.gregs[REG_EIP];
#endif // defined(__x86_64 || defined(__x86_64__)
#else
#ifdef __APPLE__
                // OS-X
                if (uc)
#ifdef __ppc__
                    DeathAddr = (void*)((ucontext_t*)uc)->uc_mcontext->ss.srr0;
#else
#ifdef __x86_64__
                    DeathAddr =
                        (void*)((ucontext_t*)uc)->uc_mcontext->__ss.__rip;
#else
#ifdef __arm64__
                    DeathAddr =
                        (void*)((ucontext_t*)uc)->uc_mcontext->__ss.__pc;
#else
                    DeathAddr = (void*)((ucontext_t*)uc)->uc_mcontext->ss.eip;
#endif // __arc64__
#endif // __x86_64__
#endif // __ppc__
#else
#ifdef __FreeBSD__
                if (uc)
                    DeathAddr = (void*)((ucontext_t*)uc)->uc_mcontext.mc_eip;
#endif // __FreeBSD__
#endif // __APPLE__
#endif // __linux
#endif // __sparc
            }
        }
#endif // WIN32

        if (sig == SIGINT) {
            fprintf(stderr, "interrupt\n");
            if (NoDieOnIntr)
                cMain::InterruptHandler();
            else {
                fprintf(stderr, "\nReceived interrupt, exiting.\n");
                XM()->Exit(ExitPanic);
            }
        }
        else if (sig == SIGTERM)
            die_now("Termination signal received.\n", false);

#ifdef SIGURG
        else if (sig == SIGURG) {
            // This is received when oob data arrives, which indicates
            // interrupt sent from client in server mode
            if (siDaemon::server_skt() > 0) {
                int c;
                for (;;) {
                    int i = recv(siDaemon::server_skt(), (char*)&c, 1, MSG_OOB);
                    if (i < 0 && errno == EINTR)
                        continue;
                    break;
                }
            }
            raise(SIGINT);
            return;
        }
#endif

#ifdef SIGPIPE
        else if (sig == SIGPIPE) {
            return;
        }
#endif
#ifdef SIGCHLD
        else if (sig == SIGCHLD)
            Proc()->SigchldHandler();
#endif
        else if (sig == SIGSEGV)
            die_now("Fatal internal error: segmentation violation.\n", true);
#ifdef SIGBUS
        else if (sig == SIGBUS)
            die_now("Fatal internal error: bus error.\n", true);
#endif
        else if (sig == SIGILL)
            die_now("Fatal internal error: illegal instruction.\n", true);
        else if (sig == SIGFPE) {
            static int fpecnt = 1;
            const char *s = "Warning: floating point exception.\n";
            if (XM()->RunMode() == ModeBackground)
                DrcIf()->fileMessage(s);
            else
                fprintf(stderr, "\n%s", s);
            fpecnt++;
            if (fpecnt == 5)
                die_now(
                "Fatal internal error: too many floating point exceptions.\n",
                    true);
        }
#ifdef SIGIO
        else if (sig == SIGIO)
            return;
#endif
        else {
            if (XM()->RunMode() == ModeBackground) {
                char buf[64];
                snprintf(buf, sizeof(buf),
                    "Received signal %d, (ignored)\n", sig);
                DrcIf()->fileMessage(buf);
            }
            else {
                fprintf(stderr, "\nReceived signal %d, exiting.\n", sig);
                XM()->Exit(ExitPanic);
            }
        }
    }
}

