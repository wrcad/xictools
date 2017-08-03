
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA, http://wrcad.com            *
 *  Copyright (C) 2017 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
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
 * Misc. Utilities Library                                                *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "config.h"
#include "childproc.h"

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#include <stdio.h>
#include <stdlib.h>


// This centralizes the SIGCHLD handling.  Since there are many places
// in the application where child processes are created and monitored,
// have to be careful that they don't clobber one another.  The
// application should not set handlers for SIGCHLD, but instead use
// the registration mechanism below

sProc *sProc::p_ptr = 0;

// Instantiated here only.
namespace {sProc _proc_; }


sProc::sProc()
{
    if (p_ptr) {
        fprintf(stderr, "Singleton struct sProc already instantiated.\n");
        exit(1);
    }
    p_ptr = this;

    p_siglist = 0;
}


// Call this in lieu of signal(SIGCHLD, hdlr), the *data is any value
// to be passed along.
//
void
sProc::RegisterChildHandler(int pid, SigcHandlerFunc hdlr, void *data)
{
    p_siglist = new sSigC(pid, hdlr, data, p_siglist);
}


// Remove a handler from the list.  It is unlikely that this will be
// needed, as list elements are removed when a child terminates.
//
void
sProc::RemoveChildHandler(int pid, SigcHandlerFunc hdlr)
{
    sSigC *sp = 0;
    for (sSigC *s = p_siglist; s; s = s->next) {
        if (s->pid == pid && s->handler == hdlr) {
            if (!sp)
                p_siglist = s->next;
            else
                sp->next = s->next;
            delete s;
            return;
        }
        sp = s;
    }
}


// The handler, call the user's handler and remove the entry from the
// list.  This should be called from the application's signal handler.
//
void
sProc::SigchldHandler()
{
#ifdef HAVE_SYS_WAIT_H
    if (!p_siglist)
        return;

    // We get here after wakeup from ^Z, wait() hangs indefinitely.
    int status;
    int pid = (int)waitpid(-1, &status, WNOHANG);
    if (!pid)
        return;

    sSigC *sp = 0, *sn;
    for (sSigC *s = p_siglist; s; s = sn) {
        sn = s->next;
        if (s->pid == pid) {
            if (s->handler)
                (*s->handler)(pid, status, s->data);
            if (WIFEXITED(status) || WIFSIGNALED(status)) {
                if (!sp)
                    p_siglist = sn;
                else
                    sp->next = sn;
                delete s;
                continue;
            }
        }
        sp = s;
    }
#endif
}

