
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2010 Whiteley Research Inc, all rights reserved.        *
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
 * Misc. Utilities Library                                                *
 *                                                                        *
 *========================================================================*
 $Id: childproc.cc,v 1.3 2017/05/03 17:34:48 stevew Exp $
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

